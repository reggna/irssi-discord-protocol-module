NAME = discord

SRCDIR = src/
OBJDIR = obj/
LIBDIR = lib/

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
LIBS = $(LIBDIR)/lib$(NAME).so

INCDIR = include/
IRSSI_DIST = $(INCDIR)/irssi

# Dependencies
PKG_CONFIG_DEPS = libcurl jansson glib-2.0

CFLAGS = -Wall -Werror -g -fPIC \
         -DNAME=\"$(NAME)\" \
         -DUOFF_T_LONG_LONG=1 \
         -I$(INCDIR) \
         -I$(IRSSI_DIST) \
         -I$(IRSSI_DIST)/src \
         -I$(IRSSI_DIST)/src/core/ \
         -I$(IRSSI_DIST)/src/fe-common/core/ \
         $(shell pkg-config $(PKG_CONFIG_DEPS) --cflags)

LDFLAGS = -shared $(shell pkg-config $(PKG_CONFIG_DEPS) --libs)

.PHONY: all
all: $(LIBS)

$(LIBS): $(OBJS) | $(LIBDIR)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBDIR) $(OBJDIR):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(LIBDIR) $(OBJDIR)

.PHONY: test
test: all
	LIB=$(abspath $(LIBS)) irssi --home=test/irssi
