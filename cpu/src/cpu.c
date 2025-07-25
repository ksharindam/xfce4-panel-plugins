/*
 * CPU usage plugin to lxpanel
 *
 * Copyright (C) 2006-2008 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *               2006-2008 Jim Huang <jserv.tw@gmail.com>
 *               2009 Marty Jack <martyj19@comcast.net>
 *               2009 Jürgen Hötzel <juergen@archlinux.org>
 *               2012 Rafał Mużyło <galtgendo@gmail.com>
 *               2012-2013 Henry Gebhardt <hsggebhardt@gmail.com>
 *               2013 Marko Rauhamaa <marko@pacujo.net>
 *               2014 Andriy Grytsenko <andrej@rep.kiev.ua>
 *               2015 Rafał Mużyło <galtgendo@gmail.com>
 *
 * This file is a part of LXPanel project.
 *
 * Copyright (C) 2004 by Alexandre Pereira da Silva <alexandre.pereira@poli.usp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
/*A little bug fixed by Mykola <mykola@2ka.mipt.ru>:) */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <sys/wait.h>// XFCE
//#include <glib/gi18n.h>

//#include "plugin.h"
#include <libxfce4panel/libxfce4panel.h>
// for xfce_spawn_command_line()
#include <libxfce4ui/libxfce4ui.h>

#define BORDER_SIZE 2
#define PLUGIN_WIDTH 44
#define FG_COLOR "rgb(0,205,0)"
#define BG_COLOR "white"
#define FONT_SIZE 12

/* #include "../../dbg.h" */

typedef unsigned long long CPUTick;		/* Value from /proc/stat */
typedef float CPUSample;			/* Saved CPU utilization value as 0.0..1.0 */

struct cpu_stat {
    CPUTick u, n, s, i;				/* User, nice, system, idle */
};

/* Private context for CPU plugin. */
typedef struct {
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkRGBA foreground_color;			/* Foreground color for drawing area */
    GdkRGBA background_color;			/* Background color for drawing area */
#else
    GdkColor foreground_color;			/* Foreground color for drawing area */
    GdkColor background_color;			/* Background color for drawing area */
#endif
    GtkWidget * da;				/* Drawing area */
    cairo_surface_t * pixmap;				/* Pixmap to be drawn on drawing area */

    guint timer;				/* Timer for periodic update */
    CPUSample * stats_cpu;			/* Ring buffer of CPU utilization values */
    unsigned int ring_cursor;			/* Cursor for ring buffer */
    guint pixmap_width;				/* Width of drawing area pixmap; also size of ring buffer; does not include border size */
    guint pixmap_height;			/* Height of drawing area pixmap; does not include border size */
    struct cpu_stat previous_cpu_stat;		/* Previous value of cpu_stat */
    gboolean show_percentage;				/* Display usage as a percentage */
    //config_setting_t *settings; XFCE
} CPUPlugin;

static void redraw_pixmap(CPUPlugin * c);
static gboolean cpu_update(CPUPlugin * c);
static gboolean draw(GtkWidget * widget, cairo_t * cr, CPUPlugin * c);

static void cpu_destructor (XfcePanelPlugin *plugin, gpointer user_data);

/* Redraw after timer callback or resize. */
static void redraw_pixmap(CPUPlugin * c)
{
    cairo_t * cr = cairo_create(c->pixmap);
    cairo_set_line_width (cr, 1.0);
    /* Erase pixmap. */
    cairo_rectangle(cr, 0, 0, c->pixmap_width, c->pixmap_height);
    cairo_set_source_rgba(cr, c->background_color.blue,  c->background_color.green, c->background_color.red, c->background_color.alpha);
    cairo_fill(cr);

    /* Recompute pixmap. */
    unsigned int i;
    unsigned int drawing_cursor = c->ring_cursor;
    cairo_set_source_rgba(cr, c->foreground_color.blue,  c->foreground_color.green, c->foreground_color.red, c->foreground_color.alpha);
    for (i = 0; i < c->pixmap_width; i++)
    {
        /* Draw one bar of the CPU usage graph. */
        if (c->stats_cpu[drawing_cursor] != 0.0)
        {
            cairo_move_to(cr, i + 0.5, c->pixmap_height);
            cairo_line_to(cr, i + 0.5, c->pixmap_height - c->stats_cpu[drawing_cursor] * c->pixmap_height);
            cairo_stroke(cr);
        }

        /* Increment and wrap drawing cursor. */
        drawing_cursor += 1;
        if (drawing_cursor >= c->pixmap_width)
            drawing_cursor = 0;
    }

    /* draw a border in black */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, 0, c->pixmap_height);
    cairo_line_to(cr, c->pixmap_width, c->pixmap_height);
    cairo_line_to(cr, c->pixmap_width, 0);
    cairo_line_to(cr, 0, 0);
    cairo_stroke(cr);

    if (c->show_percentage)
    {
        int fontsize = 12;
        if (c->pixmap_width > 50) fontsize = c->pixmap_height / 3;
        char buffer[10];
        int val = 100 * c->stats_cpu[c->ring_cursor ? c->ring_cursor - 1 : c->pixmap_width - 1];
        sprintf (buffer, "%3d %%", val);
        cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size (cr, fontsize);
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_move_to (cr, (c->pixmap_width >> 1) - ((fontsize * 5) / 3), ((c->pixmap_height + fontsize) >> 1) - 1);
        cairo_show_text (cr, buffer);
    }

    /* check_cairo_status(cr); */
    cairo_destroy(cr);

    /* Redraw pixmap. */
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (cairo_image_surface_get_data (c->pixmap), GDK_COLORSPACE_RGB, TRUE, 8, c->pixmap_width, c->pixmap_height, c->pixmap_width *4, NULL, NULL);
    gtk_image_set_from_pixbuf (GTK_IMAGE (c->da), pixbuf);
    g_object_unref (pixbuf);
}

/* Periodic timer callback. */
static gboolean cpu_update(CPUPlugin * c)
{
    if (g_source_is_destroyed(g_main_current_source()))
        return FALSE;
    if ((c->stats_cpu != NULL) && (c->pixmap != NULL))
    {
        /* Open statistics file and scan out CPU usage. */
        struct cpu_stat cpu;
        char buffer[256];
        FILE *stat = fopen ("/proc/stat", "r");
        if (stat == NULL) return TRUE;
        fgets (buffer, 256, stat);
        fclose (stat);
        if (!strlen (buffer)) return TRUE;
        int fscanf_result = sscanf(buffer, "cpu %llu %llu %llu %llu", &cpu.u, &cpu.n, &cpu.s, &cpu.i);
        if (fscanf_result == 4)
        {
            /* Compute delta from previous statistics. */
            struct cpu_stat cpu_delta;
            cpu_delta.u = cpu.u - c->previous_cpu_stat.u;
            cpu_delta.n = cpu.n - c->previous_cpu_stat.n;
            cpu_delta.s = cpu.s - c->previous_cpu_stat.s;
            cpu_delta.i = cpu.i - c->previous_cpu_stat.i;

            /* Copy current to previous. */
            memcpy(&c->previous_cpu_stat, &cpu, sizeof(struct cpu_stat));

            /* Compute user+nice+system as a fraction of total.
             * Introduce this sample to ring buffer, increment and wrap ring buffer cursor. */
            float cpu_uns = cpu_delta.u + cpu_delta.n + cpu_delta.s;
            c->stats_cpu[c->ring_cursor] = cpu_uns / (cpu_uns + cpu_delta.i);
            c->ring_cursor += 1;
            if (c->ring_cursor >= c->pixmap_width)
                c->ring_cursor = 0;

            /* Redraw with the new sample. */
            redraw_pixmap(c);
        }
    }
    return TRUE;
}

/* Handler for configure_event on drawing area. */
//static void cpu_configuration_changed (LXPanel *panel, GtkWidget *p) XFCE
static void cpu_configuration_changed (XfcePanelPlugin *plugin, CPUPlugin *c)
{
    //CPUPlugin *c = lxpanel_plugin_get_data (p); XFCE

    /* Allocate pixmap and statistics buffer without border pixels. */
    //guint new_pixmap_height = panel_get_icon_size (panel) - (BORDER_SIZE << 1);
    guint new_pixmap_height = xfce_panel_plugin_get_size (plugin) - (BORDER_SIZE << 1);
    guint new_pixmap_width = (new_pixmap_height * 3) >> 1;
    if (new_pixmap_width < 42) new_pixmap_width = 42;//XFCE
    if ((new_pixmap_width > 0) && (new_pixmap_height > 0))
    {
        /* If statistics buffer does not exist or it changed size, reallocate and preserve existing data. */
        if ((c->stats_cpu == NULL) || (new_pixmap_width != c->pixmap_width))
        {
            CPUSample * new_stats_cpu = g_new0(typeof(*c->stats_cpu), new_pixmap_width);
            if (c->stats_cpu != NULL)
            {
                if (new_pixmap_width > c->pixmap_width)
                {
                    /* New allocation is larger.
                     * Introduce new "oldest" samples of zero following the cursor. */
                    memcpy(&new_stats_cpu[0],
                        &c->stats_cpu[0], c->ring_cursor * sizeof(CPUSample));
                    memcpy(&new_stats_cpu[new_pixmap_width - c->pixmap_width + c->ring_cursor],
                        &c->stats_cpu[c->ring_cursor], (c->pixmap_width - c->ring_cursor) * sizeof(CPUSample));
                }
                else if (c->ring_cursor <= new_pixmap_width)
                {
                    /* New allocation is smaller, but still larger than the ring buffer cursor.
                     * Discard the oldest samples following the cursor. */
                    memcpy(&new_stats_cpu[0],
                        &c->stats_cpu[0], c->ring_cursor * sizeof(CPUSample));
                    memcpy(&new_stats_cpu[c->ring_cursor],
                        &c->stats_cpu[c->pixmap_width - new_pixmap_width + c->ring_cursor], (new_pixmap_width - c->ring_cursor) * sizeof(CPUSample));
                }
                else
                {
                    /* New allocation is smaller, and also smaller than the ring buffer cursor.
                     * Discard all oldest samples following the ring buffer cursor and additional samples at the beginning of the buffer. */
                    memcpy(&new_stats_cpu[0],
                        &c->stats_cpu[c->ring_cursor - new_pixmap_width], new_pixmap_width * sizeof(CPUSample));
                    c->ring_cursor = 0;
                }
                g_free(c->stats_cpu);
            }
            c->stats_cpu = new_stats_cpu;
        }

        /* Allocate or reallocate pixmap. */
        c->pixmap_width = new_pixmap_width;
        c->pixmap_height = new_pixmap_height;
        if (c->pixmap)
            cairo_surface_destroy(c->pixmap);
        c->pixmap = cairo_image_surface_create(CAIRO_FORMAT_RGB24, c->pixmap_width, c->pixmap_height);
        /* check_cairo_surface_status(&c->pixmap); */

        /* Redraw pixmap at the new size. */
        redraw_pixmap(c);
    }
}

static gboolean
on_size_change (XfcePanelPlugin *plugin, gint size, void *data)
{
    cpu_configuration_changed(plugin, (CPUPlugin *)data);
    return TRUE;
}


/* Handler for expose_event on drawing area. */
static gboolean draw(GtkWidget * widget, cairo_t * cr, CPUPlugin * c)
{
    /* Draw the requested part of the pixmap onto the drawing area.
     * Translate it in both x and y by the border size. */
    if (c->pixmap != NULL)
    {
        cairo_set_source_rgb(cr, 0, 0, 0); // FIXME: use black color from style
        cairo_set_source_surface(cr, c->pixmap,
              BORDER_SIZE, BORDER_SIZE);
        cairo_paint(cr);
        /* check_cairo_status(cr); */
    }
    return FALSE;
}

//  Execute command on button press
static gboolean
on_button_press(GtkWidget *button, GdkEvent *event, gpointer data)
{
    if ( event->button.button == 1 ) //When left click
    {
        xfce_spawn_command_line (gdk_screen_get_default (),// screen
                                 "xfce4-taskmanager",// command
                                 FALSE, // in_terminal
                                 FALSE, // startup_notification
                                 TRUE,  // child process
                                 NULL); // error
        return TRUE;
    }
    return FALSE;
}


/* Plugin constructor. */
void
cpu_constructor (XfcePanelPlugin *plugin)
{
    /* Allocate plugin context and set into Plugin private data pointer. */
    CPUPlugin * c = g_new0(CPUPlugin, 1);
    GtkWidget * p;
    //int tmp_int;//XFCE
    //const char *str;// XFCE

    c->show_percentage = TRUE; // XFCE
    gdk_rgba_parse(&c->foreground_color, FG_COLOR); //XFCE
    gdk_rgba_parse(&c->background_color, BG_COLOR); //XFCE

    /* Allocate top level widget and set into Plugin widget pointer. */
    p = gtk_event_box_new();
    gtk_widget_set_has_window(p, FALSE);
    //lxpanel_plugin_set_data(p, c, cpu_destructor);

    /* Allocate drawing area as a child of top level widget. */
    c->da = gtk_image_new();
    //gtk_widget_add_events(c->da, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
    //                             GDK_BUTTON_MOTION_MASK);// XFCE
    gtk_container_add(GTK_CONTAINER(p), c->da);
    gtk_container_add(GTK_CONTAINER(plugin), p);// XFCE
    xfce_panel_plugin_add_action_widget (plugin, p); // XFCE

    /* Connect signals. */
    g_signal_connect(G_OBJECT(c->da), "draw", G_CALLBACK(draw), (gpointer) c);
    g_signal_connect (G_OBJECT (plugin), "size-changed", G_CALLBACK(on_size_change), (gpointer) c);// XFCE
    g_signal_connect (G_OBJECT (plugin), "free-data", G_CALLBACK (cpu_destructor), (gpointer) c);// XFCE
    g_signal_connect (G_OBJECT (p), "button-press-event", G_CALLBACK (on_button_press), NULL);//XFCE

    /* Show the widget.  Connect a timer to refresh the statistics. */
    gtk_widget_show(c->da);
    //cpu_configuration_changed (plugin, c);//XFCE
    gtk_widget_show_all(p);
    c->timer = g_timeout_add(1500, (GSourceFunc) cpu_update, (gpointer) c);
    //return p; XFCE
}

/* Plugin destructor. */
//static void cpu_destructor(gpointer user_data)
static void
cpu_destructor (XfcePanelPlugin *plugin, gpointer user_data)
{
    CPUPlugin * c = (CPUPlugin *)user_data;

    /* Disconnect the timer. */
    g_source_remove(c->timer);

    /* Deallocate memory. */
    cairo_surface_destroy(c->pixmap);
    g_free(c->stats_cpu);
    g_free(c);
}
/*
static gboolean cpu_apply_configuration (gpointer user_data)
{
	char colbuf[32];
    GtkWidget * p = user_data;
    CPUPlugin * c = lxpanel_plugin_get_data(p);
    config_group_set_int (c->settings, "ShowPercent", c->show_percentage);
#if GTK_CHECK_VERSION(3, 0, 0)
    sprintf (colbuf, "%s", gdk_rgba_to_string (&c->foreground_color));
#else
    sprintf (colbuf, "%s", gdk_color_to_string (&c->foreground_color));
#endif
    config_group_set_string (c->settings, "Foreground", colbuf);
#if GTK_CHECK_VERSION(3, 0, 0)
    sprintf (colbuf, "%s", gdk_rgba_to_string (&c->background_color));
#else
    sprintf (colbuf, "%s", gdk_color_to_string (&c->background_color));
#endif
    config_group_set_string (c->settings, "Background", colbuf);
    return FALSE;
}*/

/* Callback when the configuration dialog is to be shown. */
/*static GtkWidget *cpu_configure(LXPanel *panel, GtkWidget *p)
{
    CPUPlugin * dc = lxpanel_plugin_get_data(p);
    return lxpanel_generic_config_dlg(_("CPU Usage"), panel,
        cpu_apply_configuration, p,
        _("Show usage as percentage"), &dc->show_percentage, CONF_TYPE_BOOL,
        _("Foreground colour"), &dc->foreground_color, CONF_TYPE_COLOR,
        _("Background colour"), &dc->background_color, CONF_TYPE_COLOR,
        NULL);
}*/

//FM_DEFINE_MODULE(lxpanel_gtk, cpu)
XFCE_PANEL_PLUGIN_REGISTER(cpu_constructor);


/* Plugin descriptor. */
/*LXPanelPluginInit fm_module_init_lxpanel_gtk = {
    .name = N_("CPU Usage Monitor"),
    .config = cpu_configure,
    .description = N_("Display CPU usage"),
    .new_instance = cpu_constructor,
    .reconfigure = cpu_configuration_changed,
};*/
