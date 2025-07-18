MODULE = volumepulse
LIBDIR = /usr/lib/`gcc -dumpmachine`
PLUGIN_DIR = $(LIBDIR)/xfce4/panel/plugins
DESKTOP_DIR = /usr/share/xfce4/panel/plugins

SOURCES = $(wildcard ../../*.c *.c)

OBJS = $(SOURCES:.c=.o)
CC = gcc

INCLUDES = -I../..
CFLAGS = -Wall -fPIC `pkg-config --cflags libxfce4panel-2.0` ${INCLUDES} -DLXPLUG
LIBS = `pkg-config --libs libxfce4panel-2.0`

all: ${OBJS}
	${CC} -o lib$(MODULE).so -shared -fPIC ${OBJS} ${LIBS}
	
clean:
	rm -f ${OBJS}

install:
	install lib$(MODULE).so $(PLUGIN_DIR)
	install ../data/$(MODULE).desktop $(DESKTOP_DIR)
	install -D ../data/lxplug-volumepulse.ui $(DESKTOP_DIR)/../ui/lxplug-volumepulse.ui

uninstall:
	rm $(PLUGIN_DIR)/lib$(MODULE).so $(DESKTOP_DIR)/$(MODULE).desktop $(DESKTOP_DIR)/../ui/lxplug-volumepulse.ui
	find $(DESKTOP_DIR)/.. -type d -empty -delete


