include ../config/EffectTemplate-web.mk

TARGET = ../../../public/effect/scripts/EffectTemplate-web.js

SRC += $(WEB_SRC)
CFLAGS += $(WEB_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(WEB_LDFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
