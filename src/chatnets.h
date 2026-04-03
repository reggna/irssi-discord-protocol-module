#include <common.h>
#include <core/chat-protocols.h>
void chatnets_protocol_init(CHAT_PROTOCOL_REC *rec);
void chatnets_signals_init(void);

#include "discord.h"

typedef struct {
	#include <core/chatnet-rec.h>
	string email;
} DISCORD_CHATNET_REC;
