#ifndef SERVERS_GLOCK
#define SERVERS_GLOCK
#include <common.h>
#include <core/chat-protocols.h>

void servers_protocol_init(CHAT_PROTOCOL_REC *rec);

#include "discord.h"

typedef struct {
	#include <core/server-connect-rec.h>
	char *tok;
} DISCORD_SERVER_CONNECT_REC;

#define DISCORD_SERVER_CONNECT(conn) \
	PROTO_CHECK_CAST(SERVER_CONNECT(conn), DISCORD_SERVER_CONNECT_REC, \
			 chat_type, PROTOCOL_NAME)

#define IS_DISCORD_SERVER_CONNECT(conn) \
	(DISCORD_SERVER_CONNECT(conn) ? TRUE : FALSE)

typedef struct {
	#define STRUCT_SERVER_CONNECT_REC DISCORD_SERVER_CONNECT_REC
	#include <core/server-rec.h>
	char *tok;
} DISCORD_SERVER_REC;

#define DISCORD_SERVER(server) \
	PROTO_CHECK_CAST(SERVER(server), DISCORD_SERVER_REC, \
			 chat_type, PROTOCOL_NAME)

#define IS_DISCORD_SERVER(server) \
	(DISCORD_SERVER(server) ? TRUE : FALSE)

#endif
