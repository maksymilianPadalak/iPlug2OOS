include ../config/Plugin-web.mk

TARGET = ../../../public/goldeneffect2/scripts/Plugin-web.js

SRC += $(WEB_SRC)
CFLAGS += $(WEB_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(WEB_LDFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
