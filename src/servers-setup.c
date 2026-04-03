#include "servers-setup.h"
#include "core.h"

#include <common.h>
#include <core/chat-protocols.h>

#include <core/servers.h>
#include <core/servers-setup.h>

#include "utils.h"

static DISCORD_SERVER_SETUP_REC *create(void) {
	debug();
	return g_new0(DISCORD_SERVER_SETUP_REC, 1);
}

void servers_setup_protocol_init(CHAT_PROTOCOL_REC *rec) {
	rec->create_server_setup = (SERVER_SETUP_REC *(*)(void)) create;
}

#include <lib-config/iconfig.h>

static void read_config(SERVER_SETUP_REC *proto, CONFIG_NODE *node) {
	/*
	 * Global signal: Ensure we're only reading configuration for Discord server
	 * setup records. Without this check, we'd try to access the Discord-specific
	 * 'tok' field on every record (including IRC), causing a segfault.
	 */
	if (!IS_DISCORD_SERVER_SETUP(proto))
		return;

	DISCORD_SERVER_SETUP_REC *dproto = DISCORD_SERVER_SETUP(proto);
	debug();
	// Support both 'password' (standard) and 'token' config keys.
	dproto->password = config_node_get_str(node, "password", NULL);
	if (dproto->password == NULL) {
		dproto->tok = config_node_get_str(node, "token", NULL);
	}
}

#include "servers.h"

void fill_conn(SERVER_CONNECT_REC *conn, SERVER_SETUP_REC *sserver) {
	/*
	 * Global signal: Only populate the Discord connection record if the
	 * connection is actually a Discord protocol connection.
	 */
	if (!IS_DISCORD_SERVER_CONNECT(conn) || !IS_DISCORD_SERVER_SETUP(sserver))
		return;

	DISCORD_SERVER_CONNECT_REC *dconn = DISCORD_SERVER_CONNECT(conn);
	DISCORD_SERVER_SETUP_REC *dsserver = DISCORD_SERVER_SETUP(sserver);
	debug();
	// Use password if provided (preferred for security), otherwise fall back to token field.
	if (dsserver->password != NULL) {
		dconn->tok = g_strdup(dsserver->password);
	} else if (dsserver->tok != NULL) {
		dconn->tok = g_strdup(dsserver->tok);
	}

	// Discord bots need a nick; default to username or "discord" if none provided.
	if (dconn->username != NULL) {
		dconn->nick = g_strdup(dconn->username);
	} else if (dconn->nick == NULL) {
		dconn->nick = g_strdup("discord");
	}
}

#include <core/signals.h>

void servers_setup_signals_init(void) {
	signal_add("server setup read", (SIGNAL_FUNC) read_config);
	signal_add("server setup fill server", (SIGNAL_FUNC) fill_conn);
}
