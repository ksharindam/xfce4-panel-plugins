MODULE = networkman
LIBDIR = /usr/lib/`gcc -dumpmachine`
PLUGIN_DIR = $(LIBDIR)/xfce4/panel/plugins
DESKTOP_DIR = /usr/share/xfce4/panel/plugins

SOURCES = $(wildcard ../../*.c dhcpcd-gtk/*.c libdhcpcd/*.c)

OBJS = $(SOURCES:.c=.o)
CC = gcc

INCLUDES = -I../.. -Idhcpcd-gtk -Ilibdhcpcd
CFLAGS = -Wall -fPIC `pkg-config --cflags libxfce4panel-2.0` ${INCLUDES}
LIBS = `pkg-config --libs libxfce4panel-2.0`

all: ${OBJS}
	${CC} -o lib$(MODULE).so -shared -fPIC ${OBJS} ${LIBS}

clean:
	rm -f ${OBJS}

install:
	install lib$(MODULE).so $(PLUGIN_DIR)
	install ../data/$(MODULE).desktop $(DESKTOP_DIR)
	install -D ../data/lxpanel-modal.ui $(DESKTOP_DIR)/../ui/lxpanel-modal.ui

uninstall:
	rm $(PLUGIN_DIR)/lib$(MODULE).so $(DESKTOP_DIR)/$(MODULE).desktop $(DESKTOP_DIR)/../ui/lxpanel-modal.ui
	find $(DESKTOP_DIR)/.. -type d -empty -delete
