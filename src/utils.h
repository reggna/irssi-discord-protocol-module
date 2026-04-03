#include <common.h>
#include <core/levels.h>
#include <fe-common/core/printtext.h>

static inline void discord_printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char *str = g_strdup_vprintf(fmt, args);
	va_end(args);

	printtext(NULL, NULL, MSGLEVEL_CLIENTNOTICE, "%s", str);
	g_free(str);
}

#define printf(...) discord_printf(__VA_ARGS__)
#define debug() printf("======%s:%d %s======", __FILE__, __LINE__, __func__)
