#include "discord.h"
#include "utils.h"
#include <curl/curl.h>
#include <jansson.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <core/signals.h>
#include <core/servers.h>
#include <fe-common/core/module-formats.h>

// Helper to cleanup slist
static void curl_slist_free_all_safe(struct curl_slist *list) {
	if (list) curl_slist_free_all(list);
}

static size_t string_data(void *buffer, size_t size, size_t nmemb, char **msg) {
	size_t realsize = size * nmemb;
	if (*msg == NULL) {
		*msg = g_strndup(buffer, realsize);
	} else {
		char *oldmsg = *msg;
		char *newbuf = g_strndup(buffer, realsize);
		*msg = g_strconcat(*msg, newbuf, NULL);
		g_free(oldmsg);
		g_free(newbuf);
	}
	return realsize;
}

static struct curl_slist *standard_headers(token tok, struct curl_slist *headers) {
	headers = curl_slist_append(headers, "User-Agent: irssi-discord/0.1");
	if (tok != NULL) {
		headers = curl_slist_append(headers, g_strdup_printf("Authorization: Bot %s", tok));
	}
	headers = curl_slist_append(headers, "Content-Type: application/json");
	return headers;
}

void discord_send_message(token tok, const char *target, string msg) {
	CURL *curl = curl_easy_init();
	if(curl) {
		struct curl_slist *headers = NULL;
		headers = standard_headers(tok, headers);

		json_t *root = json_object();
		json_object_set_new(root, "content", json_string(msg));
		char *json_data_str = json_dumps(root, 0);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, g_strdup_printf("%s/channels/%s/messages", BASE_API, target));
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data_str);

		CURLcode res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "curl_easy_perform() failed: %s", curl_easy_strerror(res));

		free(json_data_str);
		json_decref(root);
		curl_slist_free_all_safe(headers);
		curl_easy_cleanup(curl);
	}
}

static json_t *generic_get_request(token tok, const char *URL) {
	CURL *curl = curl_easy_init();
	json_t *root = NULL;
	if(curl) {
		struct curl_slist *headers = NULL;
		headers = standard_headers(tok, headers);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, URL);
		char *data = NULL;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_data);

		CURLcode res = curl_easy_perform(curl);
		if(res == CURLE_OK && data) {
			json_error_t error;
			root = json_loads(data, 0, &error);
			if(!root) {
				printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "JSON error: %s", error.text);
			}
		} else {
			printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
		}
		g_free(data);
		curl_slist_free_all_safe(headers);
		curl_easy_cleanup(curl);
	}
	return root;
}

json_t *discord_get_channel_info(token tok, const char *channel) {
	return generic_get_request(tok, g_strdup_printf("%s/channels/%s", BASE_API, channel));
}

json_t *discord_get_guild_info(token tok, const char *guild) {
	return generic_get_request(tok, g_strdup_printf("%s/guilds/%s", BASE_API, guild));
}

json_t *discord_get_message_history(token tok, const char *channel) {
	return generic_get_request(tok, g_strdup_printf("%s/channels/%s/messages", BASE_API, channel));
}

// Global Gateway State
typedef struct {
	token tok;
	char *server_tag;
	int heartbeat_interval;
	long last_sequence;
	GThread *thread;
	gboolean active;
	GAsyncQueue *queue;
	guint timeout_id;
} discord_gateway_t;

static discord_gateway_t *gateway = NULL;

typedef enum {
	GW_MSG_CHAT,
	GW_MSG_LOG
} gw_msg_type_t;

typedef struct {
	gw_msg_type_t type;
	char *channel_id;
	char *username;
	char *content;
} gateway_msg_t;

void discord_core_init(void) {
	curl_global_init(CURL_GLOBAL_ALL);
}

void discord_core_deinit(void) {
	if (gateway) {
		discord_gateway_disconnect(gateway->server_tag);
	}
	curl_global_cleanup();
}

static void push_log(const char *fmt, ...) {
	if (!gateway) return;
	va_list args;
	va_start(args, fmt);
	gateway_msg_t *msg = g_new0(gateway_msg_t, 1);
	msg->type = GW_MSG_LOG;
	msg->content = g_strdup_vprintf(fmt, args);
	va_end(args);
	g_async_queue_push(gateway->queue, msg);
}

static gboolean process_gateway_msgs(gpointer data) {
	if (!gateway) return FALSE;
	
	gateway_msg_t *msg;
	while ((msg = g_async_queue_try_pop(gateway->queue))) {
		if (msg->type == GW_MSG_CHAT) {
			SERVER_REC *server = server_find_tag(gateway->server_tag);
			if (server) {
				printformat_module("fe-common/core", server, msg->channel_id, MSGLEVEL_PUBLIC,
					TXT_PUBMSG, msg->username, msg->content, "");
			}
		} else if (msg->type == GW_MSG_LOG) {
			printtext(NULL, NULL, MSGLEVEL_CLIENTNOTICE, "Discord: %s", msg->content);
		}
		
		g_free(msg->channel_id);
		g_free(msg->username);
		g_free(msg->content);
		g_free(msg);
	}
	return TRUE;
}

static gpointer gateway_thread_func(gpointer data) {
	discord_gateway_t *gw = data;
	CURL *curl = curl_easy_init();
	if (!curl) return NULL;
	
	curl_easy_setopt(curl, CURLOPT_URL, "wss://gateway.discord.gg/?v=10&encoding=json");
	curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
	
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		push_log("Gateway connect failed: %s", curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		return NULL;
	}

	size_t rlen;
	const struct curl_ws_frame *meta;
	char buffer[16384];

	while (gw->active) {
		res = curl_ws_recv(curl, buffer, sizeof(buffer) - 1, &rlen, &meta);
		if (res == CURLE_OK) {
			buffer[rlen] = '\0';
			json_error_t error;
			json_t *root = json_loads(buffer, 0, &error);
			if (root) {
				int op = json_integer_value(json_object_get(root, "op"));
				if (op == 10) { // HELLO
					json_t *d = json_object_get(root, "d");
					gw->heartbeat_interval = json_integer_value(json_object_get(d, "heartbeat_interval"));
					
					// Send Identify
					if (gw->tok == NULL) {
						push_log("Gateway Identify failed: No token provided");
						break;
					}
					json_t *ident = json_object();
					json_object_set_new(ident, "op", json_integer(2));
					json_t *id_d = json_object();
					json_object_set_new(id_d, "token", json_string(gw->tok));
					json_object_set_new(id_d, "intents", json_integer(512 | 32768));
					json_t *props = json_object();
					json_object_set_new(props, "os", json_string("linux"));
					json_object_set_new(props, "browser", json_string("irssi"));
					json_object_set_new(props, "device", json_string("irssi"));
					json_object_set_new(id_d, "properties", props);
					json_object_set_new(ident, "d", id_d);
					
					char *payload = json_dumps(ident, 0);
					size_t sent;
					curl_ws_send(curl, payload, strlen(payload), &sent, 0, CURLWS_TEXT);
					free(payload);
					json_decref(ident);
				} else if (op == 0) { // Dispatch
					gw->last_sequence = json_integer_value(json_object_get(root, "s"));
					const char *t = json_string_value(json_object_get(root, "t"));
					if (g_strcmp0(t, "MESSAGE_CREATE") == 0) {
						json_t *d = json_object_get(root, "d");
						const char *chan_id = json_string_value(json_object_get(d, "channel_id"));
						const char *content = json_string_value(json_object_get(d, "content"));
						const char *username = json_string_value(json_object_get(json_object_get(d, "author"), "username"));
						
						if (chan_id && content && username) {
							gateway_msg_t *msg = g_new0(gateway_msg_t, 1);
							msg->type = GW_MSG_CHAT;
							msg->channel_id = g_strdup(chan_id);
							msg->username = g_strdup(username);
							msg->content = g_strdup(content);
							g_async_queue_push(gw->queue, msg);
						}
					}
				}
				json_decref(root);
			}
		} else if (res == CURLE_AGAIN) {
			g_usleep(100000); // 100ms
		} else {
			push_log("Gateway connection lost: %s", curl_easy_strerror(res));
			break;
		}
	}
	
	curl_easy_cleanup(curl);
	return NULL;
}

void discord_gateway_connect(token tok, const char *server_tag) {
	if (gateway) {
		if (g_strcmp0(gateway->server_tag, server_tag) == 0) {
			push_log("Gateway already connected for %s", server_tag);
			return;
		}
		discord_gateway_disconnect(gateway->server_tag);
	}

	printtext(NULL, NULL, MSGLEVEL_CLIENTNOTICE, "Discord: Starting Gateway connection for %s...", server_tag);

	gateway = g_new0(discord_gateway_t, 1);
	gateway->tok = g_strdup(tok);
	gateway->server_tag = g_strdup(server_tag);
	gateway->active = TRUE;
	gateway->queue = g_async_queue_new();

	gateway->timeout_id = g_timeout_add(100, process_gateway_msgs, NULL);
	gateway->thread = g_thread_new("discord-gateway", gateway_thread_func, gateway);
}

void discord_gateway_disconnect(const char *server_tag) {
	if (!gateway) return;
	gateway->active = FALSE;
	if (gateway->thread) g_thread_join(gateway->thread);
	if (gateway->timeout_id) g_source_remove(gateway->timeout_id);
	
	g_free(gateway->server_tag);
	g_free((char*)gateway->tok);
	
	gateway_msg_t *msg;
	while ((msg = g_async_queue_try_pop(gateway->queue))) {
		g_free(msg->channel_id);
		g_free(msg->username);
		g_free(msg->content);
		g_free(msg);
	}
	g_async_queue_unref(gateway->queue);
	
	g_free(gateway);
	gateway = NULL;
}
