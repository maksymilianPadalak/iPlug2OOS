include ../config/TemplateProject-web.mk

TARGET = ../../../public/plugin2/scripts/TemplateProject2-web.js

SRC += $(WEB_SRC)
CFLAGS += $(WEB_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(WEB_LDFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
