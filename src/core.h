#ifndef CORE_GLOCK
#define CORE_GLOCK

// Various types
typedef unsigned char byte;

#define MODULE discord_
// TODO: use MODULE in NAMESPACE
#define NAMESPACE(funcname) discord ##_## funcname 
// TODO: make MODULE_NAME dependent on MODULE
#define MODULE_NAME "discord"

#define PROTOCOL_NAME "Discord"
#define PROTOCOL (chat_protocol_lookup(PROTOCOL_NAME))

#define IRSSI_PACKAGE "irssi-discord"
#define DISCORD_VERSION "10"

void NAMESPACE(init)();
void NAMESPACE(deinit)();

#endif
