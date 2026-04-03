#include "protocol.h"
#include "core.h"

#include <common.h>
#include <core/chat-protocols.h>

#include "utils.h"

#include "queries.h"
#include "chatnets.h"
#include "servers-setup.h"
#include "servers.h"
#include "channels.h"
#include "channels-setup.h"

void discord_protocol_init(void) {
	debug();
	CHAT_PROTOCOL_REC *rec = g_new0(CHAT_PROTOCOL_REC, 1);

	/*
	 * misc
	 */
	rec->name = PROTOCOL_NAME;
	rec->fullname = "Discord wrapper";

	queries_protocol_init(rec); // queries.h
	chatnets_protocol_init(rec); // chatnets.h
	servers_setup_protocol_init(rec); // servers-setup.h
	servers_protocol_init(rec); // servers.h
	channels_protocol_init(rec); // channels.h
	channels_setup_protocol_init(rec); // channels_setup.h

	chat_protocol_register(rec);
}
