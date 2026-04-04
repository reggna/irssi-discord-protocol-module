#include "servers.h"
#include "core.h"

#include <common.h>
#include <core/signals.h>
#include <core/chat-protocols.h>

#include <core/servers.h>

#include "utils.h"

#include "channels.h"

static DISCORD_SERVER_CONNECT_REC *create(void) {
	debug();
	DISCORD_SERVER_CONNECT_REC *conn = g_new0(DISCORD_SERVER_CONNECT_REC, 1);
	conn->type = module_get_uniq_id("SERVER CONNECT", 0);
	return conn;
}

static gboolean manual_connect(gpointer data) {
	DISCORD_SERVER_REC *server = data;
	CHAT_PROTOCOL_REC *proto = chat_protocol_find(PROTOCOL_NAME);
	if (proto != NULL && proto->server_connect != NULL) {
		proto->server_connect((SERVER_REC *)server);
	}
	return FALSE;
}

static DISCORD_SERVER_REC *init(DISCORD_SERVER_CONNECT_REC *connrec) {
	debug();
	DISCORD_SERVER_REC *server = g_new0(DISCORD_SERVER_REC, 1);
	server->type = module_get_uniq_id("SERVER", 0);
	server->chat_type = PROTOCOL;
	server->connect_tag = -1;
	server->readtag = -1;

	connrec->chat_type = PROTOCOL;
	connrec->no_connect = 1;
	server->connrec = connrec;

	server->tok = g_strdup(connrec->tok);

	connrec->address = g_strdup("gateway.discord.gg");
	connrec->port = 443; // TODO: make this default a better way

	server_connect_ref(SERVER_CONNECT(connrec));
	server_connect_init((SERVER_REC *) server);

	g_idle_add(manual_connect, server);
	return server;
}

static void sig_connected(SERVER_REC *server) {
	/*
	 * Ensure we only process Discord servers. Since the "server connected" signal
	 * is global, it will be emitted for every server connection (e.g. IRC).
	 * Handling a non-Discord server here would lead to invalid memory access.
	 */
	if (!IS_DISCORD_SERVER(server))
		return;

	DISCORD_SERVER_REC *dserver = DISCORD_SERVER(server);
	debug();
	// Initialize our WebSocket Gateway connection in a background thread.
	discord_gateway_connect(dserver->tok, dserver->tag);
}

static void connect(DISCORD_SERVER_REC *server) {
	debug();
	if (server->tok != NULL) {
		printf("Connecting to Discord with token...");
	} else {
		printf("token is NULL!");
	}

	/*
	 * Bypass Irssi's core TCP connection logic. Discord requires a WebSocket
	 * connection, which we handle ourselves using libcurl.
	 *
	 * By calling server_connect_finished() directly, we satisfy Irssi's
	 * internal state machine without letting it try to open a socket or
	 * send IRC commands (like CAP LS) that would result in Cloudflare errors.
	 */
	server->connected = 1;
	server_connect_finished((SERVER_REC *) server);
	return;
}

static void destroy(SERVER_CONNECT_REC *conn) {
	debug();
	return;
}

static void sig_disconnected(SERVER_REC *server) {
	if (!IS_DISCORD_SERVER(server))
		return;

	debug();
	discord_gateway_disconnect(server->tag);
}

void servers_protocol_init(CHAT_PROTOCOL_REC *rec) {
	rec->create_server_connect = (SERVER_CONNECT_REC *(*)(void)) create;
	rec->server_init_connect = (SERVER_REC *(*)(SERVER_CONNECT_REC *)) init;
	rec->server_connect = (void (*)(SERVER_REC *)) connect;
	rec->destroy_server_connect = destroy;

	signal_add_first("server connected", (SIGNAL_FUNC) sig_connected);
	signal_add("server disconnected", (SIGNAL_FUNC) sig_disconnected);
}
