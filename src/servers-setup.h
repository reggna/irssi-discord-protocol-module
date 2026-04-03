#ifndef SERVERS_SETUP_GLOCK
#define SERVERS_SETUP_GLOCK
#include <common.h>
#include <core/chat-protocols.h>

void servers_setup_protocol_init(CHAT_PROTOCOL_REC *rec);
void servers_setup_signals_init(void);

#include "discord.h"

typedef struct {
	#include <core/server-setup-rec.h>
	token tok;
} DISCORD_SERVER_SETUP_REC;

#define DISCORD_SERVER_SETUP(sserver) \
	PROTO_CHECK_CAST(SERVER_SETUP(sserver), DISCORD_SERVER_SETUP_REC, \
			 chat_type, PROTOCOL_NAME)

#define IS_DISCORD_SERVER_SETUP(sserver) \
	(DISCORD_SERVER_SETUP(sserver) ? TRUE : FALSE)

#endif
