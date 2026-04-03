#include "signals.h"
#include <core/signals.h>
#include "utils.h"

#include "chatnets.h"
#include "servers-setup.h"
#include "queries.h"

void discord_signals_init(void) {
	chatnets_signals_init();
	servers_setup_signals_init();
	queries_signals_init();
}
