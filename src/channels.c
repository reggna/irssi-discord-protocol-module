#include "channels.h"
#include "core.h"

#include <stdbool.h>

#include <common.h>
#include <core/signals.h>
#include <core/chat-protocols.h>

#include <core/channels.h>
#include <core/servers.h>
#include <core/nicklist.h>

#include "utils.h"

#include "servers.h"
#include <jansson.h>
#include "discord.h"

enum {
	DISCORD_CHAN_GUILD_TEXT = 0,
	DISCORD_CHAN_DM = 1,
	DISCORD_CHAN_GUILD_VOICE = 2,
	DISCORD_CHAN_GROUP_DM = 3,
	DISCORD_CHAN_GUILD_CATEGORY = 4,
	DISCORD_CHAN_GUILD_NEWS = 5,
	DISCORD_CHAN_GUILD_STORE = 6,
	DISCORD_CHAN_GUILD_NEWS_THREAD = 10,
	DISCORD_CHAN_GUILD_PUBLIC_THREAD = 11,
	DISCORD_CHAN_GUILD_PRIVATE_THREAD = 12,
	DISCORD_CHAN_GUILD_STAGE_VOICE = 13,
	DISCORD_CHAN_GUILD_DIRECTORY = 14,
	DISCORD_CHAN_GUILD_FORUM = 15
};

static CHANNEL_REC * create(DISCORD_SERVER_REC *server,
                            const char *name,
                            const char *visible_name,
                            int automatic) {
	debug();
	CHANNEL_REC *rec = g_new0(CHANNEL_REC, 1);
	channel_init(rec, (SERVER_REC *) server, name, visible_name, automatic);
	return rec;
}

#include <fe-common/core/module-formats.h>
static void join(DISCORD_SERVER_REC *server, const char *name, int automatic) {
	debug();
	char *visible_name = NULL;

	if (server->tok == NULL) {
		printf("Cannot join channel %s: No token set for server", name);
		return;
	}

	json_t *root = discord_get_channel_info(server->tok, name);
	if (!root) {
		printf("Failed to get channel info for %s", name);
		return;
	}

	int chan_type = json_integer_value(json_object_get(root, "type"));

	switch(chan_type) {
		case DISCORD_CHAN_GUILD_TEXT:
		case DISCORD_CHAN_GUILD_NEWS:
		case DISCORD_CHAN_GUILD_NEWS_THREAD:
		case DISCORD_CHAN_GUILD_PUBLIC_THREAD:
		case DISCORD_CHAN_GUILD_PRIVATE_THREAD: {
			const char *channel_name = json_string_value(json_object_get(root, "name"));
			const char *guild_id = json_string_value(json_object_get(root, "guild_id"));
			
			if (guild_id) {
				json_t *guilds = discord_get_guild_info(server->tok, guild_id);
				if (guilds) {
					const char *guild_name = json_string_value(json_object_get(guilds, "name"));
					visible_name = g_strdup_printf("%s/%s", guild_name, channel_name);
					json_decref(guilds);
				}
			}
			
			if (!visible_name) {
				visible_name = g_strdup(channel_name);
			}
			break;
		}
		case DISCORD_CHAN_DM: {
			chat_protocol_find(server->tag)->query_create(server->tag, name, automatic);
			json_decref(root);
			return;
		}
		default :
			printf("Unsupported channel type: %d", chan_type);
			visible_name = g_strdup(name);
			break;
	}
	
	CHANNEL_REC *rec = create(server, name, visible_name, automatic);
	g_free(visible_name);

	// Populate nicklist
	switch(chan_type) {
		case DISCORD_CHAN_GUILD_TEXT:
		case DISCORD_CHAN_GUILD_NEWS:
		case DISCORD_CHAN_GUILD_NEWS_THREAD:
		case DISCORD_CHAN_GUILD_PUBLIC_THREAD:
		case DISCORD_CHAN_GUILD_PRIVATE_THREAD: {
			const char *guild_id = json_string_value(json_object_get(root, "guild_id"));
			if (guild_id) {
				json_t *members = discord_get_guild_members(server->tok, guild_id);
				if (members && json_is_array(members)) {
					size_t size = json_array_size(members);
					for (size_t i = 0; i < size; i++) {
						json_t *member = json_array_get(members, i);
						json_t *user = json_object_get(member, "user");
						const char *username = json_string_value(json_object_get(user, "username"));
						if (username) {
							NICK_REC *nickrec = g_new0(NICK_REC, 1);
							nickrec->nick = g_strdup(username);
							nicklist_insert(rec, nickrec);

							if (server->nick && g_ascii_strcasecmp(username, server->nick) == 0) {
								nicklist_set_own(rec, nickrec);
							}
						}
					}
					json_decref(members);
				}
			}
			break;
		}
		case DISCORD_CHAN_GROUP_DM: {
			json_t *recipients = json_object_get(root, "recipients");
			if (recipients && json_is_array(recipients)) {
				size_t size = json_array_size(recipients);
				for (size_t i = 0; i < size; i++) {
					json_t *user = json_array_get(recipients, i);
					const char *username = json_string_value(json_object_get(user, "username"));
					if (username) {
						NICK_REC *nickrec = g_new0(NICK_REC, 1);
						nickrec->nick = g_strdup(username);
						nicklist_insert(rec, nickrec);

						if (server->nick && g_ascii_strcasecmp(username, server->nick) == 0) {
							nicklist_set_own(rec, nickrec);
						}
					}
				}
			}
			break;
		}
	}

	json_t *history = discord_get_message_history(server->tok, name);
	if (history && json_is_array(history)) {
		size_t size = json_array_size(history);
		for (size_t i = 0; i < size; i++) {
			// Discord history is returned newest-first, so we iterate backwards for Irssi
			json_t *message = json_array_get(history, size - 1 - i);
			const char *text_msg = json_string_value(json_object_get(message, "content"));
			json_t *author = json_object_get(message, "author");
			const char *username = json_string_value(json_object_get(author, "username"));
			
			if (text_msg && username) {
				printformat_module("fe-common/core", server, name, MSGLEVEL_PUBLIC,
					TXT_PUBMSG, username, text_msg, "");
			}
		}
		json_decref(history);
	}
	
	json_decref(root);
}

static int is_channel(DISCORD_SERVER_REC *server, const char *data) {
	debug();
	return TRUE;
}

static void send_message(DISCORD_SERVER_REC *server, const char *target, const char *msg, int target_type) {
	debug();
	discord_send_message(server->tok, target, msg);
}

static int isnickflag(SERVER_REC *server, char flag) {
	return FALSE;
}

static const char *get_nick_flags(SERVER_REC *server) {
	return "";
}

static void sig_connected(SERVER_REC *server) {
	/*
	 * Global signal: Ensure we only initialize Discord-specific function pointers.
	 * If we didn't check the protocol type here, we'd overwrite 'channels_join'
	 * for IRC servers too, causing Irssi to crash when they try to join channels.
	 */
	if (!IS_DISCORD_SERVER(server))
		return;

	server->channels_join = (void (*)(SERVER_REC *, const char *, int)) join;
	server->isnickflag = isnickflag;
	server->ischannel = (int (*)(SERVER_REC *, const char *)) is_channel;
	server->get_nick_flags = get_nick_flags;
	server->send_message = (void (*)(SERVER_REC *, const char *, const char *, int))send_message;
}

void channels_protocol_init(CHAT_PROTOCOL_REC *rec) {
	rec->channel_create = (CHANNEL_REC *(*)(SERVER_REC *, const char *, const char *, int)) create;
	signal_add("server connected", (SIGNAL_FUNC) sig_connected);
}
