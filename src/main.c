#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtk/gtkshortcut.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "device.h"
#include "adapter.h"
#include "forward_decl.h"

GtkWidget *window;
GList *devices = nullptr;
Adapter *adapter = nullptr;
GDBusConnection *dbusConnection = nullptr;
GtkWidget *list;

struct margin{
    int margin_top;
    int margin_start;
    int margin_end;
    int margin_bottom;
};

struct button_args{
    const char *label;
    void (*activate)(GtkButton *self, gpointer data);
    int width;
    int height;
    int margin_top;
    int margin_start;
    int margin_end;
    int margin_bottom;
};

struct list_row{
    GtkWidget *row;
    Device* dev;
};

gboolean get_devs(gpointer user_data);
void discovery_cb(Adapter *adapter, Device *device);
void device_remove_cb(Adapter *adapter, Device *device);
void connect_button_cb(GtkButton *self, gpointer data);
void disconnect_button_cb(GtkButton *self, gpointer data);

static void set_margin(GtkWidget *widget, struct margin margin){
    gtk_widget_set_margin_start(widget, margin.margin_start);
    gtk_widget_set_margin_top(widget, margin.margin_top);
    gtk_widget_set_margin_end(widget, margin.margin_end);
    gtk_widget_set_margin_bottom(widget, margin.margin_bottom);
}

static GtkWidget *make_button(struct button_args args){
    GtkWidget *button = gtk_button_new();

    if(args.activate)
        g_signal_connect(button, "clicked", G_CALLBACK(args.activate), nullptr);
    if(args.label)
        gtk_button_set_label(GTK_BUTTON(button), args.label);

    gtk_widget_set_size_request(button, args.width, args.height);
    set_margin(button, (struct margin){
        .margin_bottom = args.margin_bottom,
        .margin_end = args.margin_end,
        .margin_top = args.margin_top,
        .margin_start = args.margin_start
    });

    return button;
}

static void append_childs(GtkBox *box, int childs_num, ...){
    va_list list;
    va_start(list);
    for(int i = 0; i < childs_num; i++)
        gtk_box_append(box, va_arg(list, GtkWidget*));
    va_end(list);
}

void button1_cb(GtkButton *self, gpointer data){
    gtk_window_close(GTK_WINDOW(window));
}

static void app_activate(GtkApplication *app, gpointer user_data){
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "vblue");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 450);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *box1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    set_margin(box1, (struct margin){
        .margin_top = 25,
        .margin_start = 25,
        .margin_end = 25,
        .margin_bottom = 25,
    });

    GtkWidget *buttons_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    set_margin(buttons_box, (struct margin){.margin_start = 15});

    GtkWidget *box3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    list = gtk_list_box_new();
    GtkWidget *scroll_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_window), list);
    gtk_widget_set_size_request(scroll_window, 535, 350);


    GtkWidget *button1 = make_button((struct button_args){
        .label = "X",
        .margin_top = 4,
        .margin_end = 4,
        .width = 50, 50,
        .activate = button1_cb
    });

    GtkWidget *button_connect = make_button((struct button_args){
        .label = "Connect",
        .width = 200, 40,
        .activate = connect_button_cb
    });
    GtkWidget *button_disconnect = make_button((struct button_args){
        .label = "Disconnect",
        .margin_top = 10,
        .width = 200, 40,
        .activate = disconnect_button_cb
    });
    GtkWidget *button_remove = make_button((struct button_args){
        .label = "Remove",
        .margin_top = 10,
        .width = 200, 40
    });

    GtkWidget *connected = gtk_label_new("Connected: [NULL]");
    gtk_widget_set_size_request(connected, 746, 0);

    append_childs(GTK_BOX(box1), 2,
                    scroll_window,
                    buttons_box
    );
    append_childs(GTK_BOX(buttons_box), 3,
                    button_connect,
                    button_disconnect,
                    button_remove
    );
    append_childs(GTK_BOX(box3), 2,
                    connected,
                    button1
    );
    append_childs(GTK_BOX(main_box), 2, 
                    box3,
                    box1
    );

    gtk_window_set_child(GTK_WINDOW(window), main_box);

    gtk_window_present(GTK_WINDOW(window));
    adapter = binc_adapter_get_default(dbusConnection);
    binc_adapter_power_on(adapter);
    binc_adapter_start_discovery(adapter);
    binc_adapter_set_discovery_cb(adapter, discovery_cb);
}

void make_row(struct list_row *list_row){
    list_row->row = gtk_list_box_row_new();
    const char *dev_name = binc_device_get_name(list_row->dev);
    GtkWidget *label = gtk_label_new(dev_name?dev_name:binc_device_get_address(list_row->dev));
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(list_row->row), label);
    gtk_list_box_append(GTK_LIST_BOX(list), list_row->row);
}

gboolean get_devs(gpointer user_data){
    GList *discovered = binc_adapter_get_devices(adapter);
    while(discovered){
        Device *dev = discovered->data;
        GList *devs_copy = devices;
        while(devs_copy){
            if(strcmp(binc_device_get_address(((struct list_row*)devs_copy->data)->dev), binc_device_get_address(dev)) == 0){
                goto next_device;
            }
            devs_copy = devs_copy->next;
        }
        struct list_row *dev_row = malloc(sizeof(struct list_row));
        memset(dev_row, 0, sizeof(struct list_row));
        dev_row->dev = dev;
        make_row(dev_row);
        if(!devices){
            devices = g_list_alloc();
            devices->data = dev_row;
        } else {
            GList *_ = g_list_append(devices, dev_row);
        }
next_device:
        discovered = discovered->next;
    }
    printf("\rList rows: %d", g_list_length(devices));
    fflush(stdout);
    return TRUE;
}

void discovery_cb(Adapter *adapter, Device *device){
    printf("New device discovered: %s\n", binc_device_get_address(device));
    struct list_row *row = g_new0(struct list_row, 1);
    row->dev = device;
    make_row(row);
    if(!devices){
        devices = g_list_alloc();
        devices->data = row;
    } else {
        GList *_ = g_list_append(devices, row);
    }
}

void device_remove_cb(Adapter *adapter, Device *device){
    g_assert(adapter != NULL);
    g_assert(device != NULL);

    struct list_row* row;
    GList *devs_copy = devices;
    while(devs_copy){
        row = (struct list_row*)devs_copy->data;
        if((strcmp(binc_device_get_address(((struct list_row*)devs_copy->data)->dev), binc_device_get_address(device)) == 0)){
            g_list_remove(devs_copy, row);
        }

        devs_copy = devs_copy->next;
    }
}

void connect_button_cb(GtkButton *self, gpointer data){
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(list));
    GList *devs_copy = devices;
    while(devs_copy){
        struct list_row *row = devs_copy->data;
        if(GTK_LIST_BOX_ROW(row->row) == selected){
            binc_device_connect(row->dev);
            return;
        }

        devs_copy = devs_copy->next;
    }
    perror("Device was not found");
}

void disconnect_button_cb(GtkButton *self, gpointer data){
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(list));
    GList *devs_copy = devices;
    if(selected != NULL)
        while(devs_copy){
            struct list_row *row = devs_copy->data;
            if(GTK_LIST_BOX_ROW(row->row) == selected){
                binc_device_disconnect(row->dev);
                return;
            }

            devs_copy = devs_copy->next;
        }
    perror("Device was not found");
}


static void app_shutdown(GtkApplication *app, gpointer user_data){
    g_list_free(devices);
    devices = nullptr;
}

int main(int argc, char **argv){
    GtkApplication *app;
    int status;
    dbusConnection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    app = gtk_application_new("org.valad47.vblue", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), nullptr);
    g_signal_connect(app, "shutdown", G_CALLBACK(app_shutdown), nullptr);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    g_object_unref(dbusConnection);

    return status;
}