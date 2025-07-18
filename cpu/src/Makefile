MODULE = cpu
LIBDIR = /usr/lib/`gcc -dumpmachine`
PLUGIN_DIR = $(LIBDIR)/xfce4/panel/plugins
DESKTOP_DIR = /usr/share/xfce4/panel/plugins
DESTDIR =
CFLAGS = `pkg-config --cflags --libs libxfce4panel-2.0` -Wl,-rpath=$(LIBDIR) -shared -fPIC -Wall
all:
	gcc -o lib$(MODULE).so $(CFLAGS) $(MODULE).c
	
clean:
	rm -f lib$(MODULE).so

install:
	install lib$(MODULE).so $(PLUGIN_DIR)
	install ../data/$(MODULE).desktop $(DESKTOP_DIR)

uninstall:
	rm $(PLUGIN_DIR)/lib$(MODULE).so $(DESKTOP_DIR)/$(MODULE).desktop
