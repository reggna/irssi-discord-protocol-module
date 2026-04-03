#ifndef DISCORD_H
#define DISCORD_H

#include "core.h"
#include <jansson.h>

typedef const char *string;
typedef const char chID[20]; // Channel ID is up to 20 digits usually
typedef string token;

#define BASE_API "https://discord.com/api/v" DISCORD_VERSION

void discord_core_init(void);
void discord_core_deinit(void);

// REST functions (currently synchronous but should be async later)
void discord_send_message(token tok, const char *target, string msg);
json_t *discord_get_channel_info(token tok, const char *channel);
json_t *discord_get_guild_info(token tok, const char *guild);
json_t *discord_get_message_history(token tok, const char *channel);

// Gateway functions
void discord_gateway_connect(token tok, const char *server_tag);
void discord_gateway_disconnect(const char *server_tag);

#endif
