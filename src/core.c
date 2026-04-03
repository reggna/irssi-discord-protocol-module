#include "core.h"
#include <common.h>
#include <core/modules.h>

#include "utils.h"

#include "protocol.h"
#include "signals.h"
#include "discord.h"

#include <core/chat-protocols.h>
#include <core/chatnets.h>
#include <core/servers-setup.h>
#include <core/servers.h>

/*
 * Irssi creates "dummy" protocol records for unknown chat networks when loading
 * a config before this module is loaded. These default to IRC behavior, which
 * causes Irssi to try and perform an IRC handshake (CAP LS) on Discord servers.
 *
 * fix_protocols() iterates through Irssi's internal lists and updates any
 * "Discord" records to use our real protocol ID, preventing IRC interference.
 */
static void fix_protocols(void) {
	int discord_id = chat_protocol_lookup(PROTOCOL_NAME);
	if (discord_id == -1) return;

	GSList *tmp;
	// Update chat networks (chatnets) to use the Discord protocol ID
	for (tmp = chatnets; tmp != NULL; tmp = tmp->next) {
		CHATNET_REC *rec = tmp->data;
		if (g_ascii_strcasecmp(rec->name, PROTOCOL_NAME) == 0) {
			rec->chat_type = discord_id;
		}
	}

	// Update server setup records (from /SERVER ADD or config)
	for (tmp = setupservers; tmp != NULL; tmp = tmp->next) {
		SERVER_SETUP_REC *rec = tmp->data;
		if (rec->chatnet != NULL && g_ascii_strcasecmp(rec->chatnet, PROTOCOL_NAME) == 0) {
			rec->chat_type = discord_id;
		}
	}

	// Update any currently active server connections
	for (tmp = servers; tmp != NULL; tmp = tmp->next) {
		SERVER_REC *rec = tmp->data;
		if (rec->connrec != NULL && rec->connrec->chatnet != NULL &&
		    g_ascii_strcasecmp(rec->connrec->chatnet, PROTOCOL_NAME) == 0) {
			rec->chat_type = discord_id;
		}
	}
}

void NAMESPACE(init)() {
	discord_core_init();
	discord_protocol_init();
	fix_protocols();
	discord_signals_init();
	module_register(MODULE_NAME, "core");
}

void NAMESPACE(deinit)() {
	debug();
	discord_core_deinit();
}

#ifdef IRSSI_ABI_VERSION
void NAMESPACE(abicheck)(int *version)
{
	*version = IRSSI_ABI_VERSION;
}
#endif
