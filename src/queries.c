#include "queries.h"
#include "core.h"

#include <common.h>
#include <core/chat-protocols.h>
#include <core/queries.h>

#include "utils.h"
#include "discord.h"
#include <jansson.h>

#include <fe-common/core/module-formats.h>

static DISCORD_QUERY_REC *create(const char *server_tag,
                                 const char *nick,
                                 int automatic) {
	debug();
	DISCORD_QUERY_REC *rec = g_new0(DISCORD_QUERY_REC, 1);
	rec->chat_type = PROTOCOL;
	rec->server_tag = g_strdup(server_tag);
	rec->name = g_strdup(nick);

	query_init((QUERY_REC *) rec, automatic);

	if (rec->server) {
		json_t *history = discord_get_message_history(rec->server->tok, rec->name);
		if (history && json_is_array(history)) {
			size_t size = json_array_size(history);
			for (size_t i = 0; i < size; i++) {
				json_t *message = json_array_get(history, size - 1 - i);
				const char *username = json_string_value(json_object_get(json_object_get(message, "author"), "username"));
				const char *content = json_string_value(json_object_get(message, "content"));
				
				if (username && content) {
					printformat_module_window("fe-common/core", rec->window, MSGLEVEL_MSGS,
						TXT_MSG_PRIVATE_QUERY, username, "", content);
				}
			}
			json_decref(history);
		}
	}
	
	return rec;
}

static void sig_created(DISCORD_QUERY_REC *query, int automatic) {
	debug();
	if (query->server) {
		json_t *root = discord_get_channel_info(query->server->tok, query->name);
		if (root) {
			json_t *recipients = json_object_get(root, "recipients");
			if (json_array_size(recipients) > 0) {
				const char *user_name = json_string_value(json_object_get(json_array_get(recipients, 0), "username"));
				if (user_name) {
					query->visible_name = g_strdup(user_name);
				}
			}
			json_decref(root);
		}
	}
}

void queries_protocol_init(CHAT_PROTOCOL_REC *rec) {
	rec->query_create = (QUERY_REC *(*)(const char *, const char *, int)) create;
}

#include <core/signals.h>

void queries_signals_init(void) {
	signal_add("query created", (SIGNAL_FUNC) sig_created);
}
