#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <raylib.h>

#include "device.h"
#include "adapter.h"
#include "forward_decl.h"

typedef struct {
  void (*callback)(void);
  Rectangle buttonBox;
  Color color;
} Button;

typedef struct {
  Button* buttons;
  int size;
  int taken;
} ButtonArray;

typedef struct {
  Rectangle rec;
  Color color;
} Rec;

typedef struct {
  Rec* array;
  int size;
  int taken;
} RecArray;

typedef struct device_list{
  const char *label;
  Device *device;
  struct device_list *next;
  struct device_list *prev;
} device_list;

struct button_args{
    const char *label;
    void (*activate)(void);
    int posX;
    int posY;
    int width;
    int height;
    Color color;
};

//
// Variable definitions
//

device_list *devices = nullptr;
bool devices_lock = false;
Adapter *adapter = nullptr;
GDBusConnection *dbusConnection = nullptr;
ButtonArray *buttons = nullptr;
RecArray *boxes = nullptr;
bool ExitProgram = false;

void get_devs(void);
void button1_cb(void);
void discovery_cb(Adapter *adapter, Device *device);
void device_remove_cb(Adapter *adapter, Device *device);
void connect_button_cb();
void disconnect_button_cb();
void remove_button_cb();

void init_button_array(){
  buttons = MemAlloc(sizeof(ButtonArray));
  buttons->buttons = MemAlloc(sizeof(Button));
  buttons->size = 1;
}
void init_rec_array(){
  boxes = MemAlloc(sizeof(RecArray));
  boxes->array = MemAlloc(sizeof(Rec));
  boxes->size = 1;
}

void insert_rec(Rec rec) {
  if(boxes->taken == boxes->size){
    void* newPtr = MemRealloc(boxes->array, sizeof(Rectangle)*boxes->size*2);
    if(newPtr == NULL) {
      exit(1);
    } else {
      boxes->array = newPtr;
    }
    boxes->size *= 2;
  }
  boxes->array[boxes->taken] = rec;
  boxes->taken++;
}

void insert_button(Button button) {
  if(buttons->taken == buttons->size){
    void* newPtr = MemRealloc(buttons->buttons, sizeof(Button)*buttons->size*2);
    if(newPtr == NULL) {
      exit(1);
    } else {
      buttons->buttons = newPtr;
    }
    buttons->size *= 2;
  }
  buttons->buttons[buttons->taken] = button;
  buttons->taken++;
}

void insert_device(device_list dev) {
  while(device_lock);
  devices_lock = true;
  
  if(devices == nullptr) {
    devices = MemAlloc(device_list);
    devices->next = devices;
    devices->prev = devices;
  }
  device_list *last_dev = devices->prev;
  last_dev->next = MemAlloc(sizeof(device_list));
  memcpy(last_dev->next, &dev, sizeof(device_list));

  last_dev->next->next = devices;
  devices->prev = last_dev->next;
  devices_lock = false;
}

void remove_device(Device *device) {
  while (device_lock);
  device_lock = true;
  
  if(devices == nullptr) {
    printf("\n[ERROR] devices is equal to nullptr, something is wrong\n");
    return;
  }

  device_list *temp_dev = devices->next;
  while(temp_dev != devices) {
    if(temp_dev->dev != device) {
      temp_dev = temp_dev->next;
      continue;
    }

    temp_dev->prev->next = temp_dev->next;
    temp_dev->next->prev = temp_dev->prev;
    MemFree(temp_dev);
    device_lock = false;
  }
  printf("No such device in list\n");
  device_lock = false;
}

void make_button(struct button_args args){
  insert_button((Button){.callback = args.activate, .buttonBox = (Rectangle){.width = args.width, .height = args.height, .x = args.posX, .y = args.posY}, .color = args.color});
}

void DrawRecs(void) {
  for(int i = 0; i < boxes->taken; i++) {
    DrawRectangleRec(boxes->array[i].rec, boxes->array[i].color);
  }
}

void DrawButtons(void) {
  for(int i = 0; i < buttons->taken; i++) {
    DrawRectangleRec(buttons->buttons[i].buttonBox, buttons->buttons[i].color);
  }
}

void ProceedButtons(void) {
  Vector2 mousePos = GetMousePosition();
  printf("\n\033[1F%f %f", mousePos.x, mousePos.y);
  for(int i = 0; i < buttons->taken; i++) {
    Button *button = &buttons->buttons[i];

    if(CheckCollisionPointRec(mousePos, button->buttonBox)){

       if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	 button->color = BLACK;
       else
	 button->color = DARKGRAY;

       if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) button->callback();
    } else {
      button->color = GRAY;
    }
  }
}

void app_activate(){
    make_button((struct button_args){
        .label = "X",
        .width = 50, 50,
	.posX = 725, 25,
        .activate = button1_cb,
	.color = GRAY
      });

    insert_rec((Rec){
	.rec = {
	  .width = 500, 350,
	  .x = 50, 50 
	},
	.color = LIGHTGRAY
    });

    make_button((struct button_args){
        .label = "Connect",
        .width = 200, 40,
	.posX = 575, 100,
        .activate = connect_button_cb,
	.color = GRAY
    });
    make_button((struct button_args){
        .label = "Disconnect",
        .width = 200, 40,
	.posX = 575, 160,	
        .activate = disconnect_button_cb,
	.color = GRAY
      });
    make_button((struct button_args){
        .label = "Remove",
        .width = 200, 40,
	.posX = 575, 220,	
	.activate = remove_button_cb,
	.color = GRAY
      });

    adapter = binc_adapter_get_default(dbusConnection);
    binc_adapter_power_on(adapter);
    binc_adapter_start_discovery(adapter);
    binc_adapter_set_discovery_cb(adapter, discovery_cb);
    binc_adapter_set_device_removal_cb(adapter, device_remove_cb);
    get_devs();
}

void get_devs(void){
  /*GList *discovered = binc_adapter_get_devices(adapter);
    while(discovered){
      // Device *dev = discovered->data;
      // GList *devs_copy = devices;
        //struct list_row *dev_row = g_new0(struct list_row, 1);
        //dev_row->dev = dev;
        //make_row(dev_row);
        if(!devices){
          //  devices = g_list_alloc();
	    //  devices->data = dev_row;
        } else {
	  //GList *_ = g_list_append(devices, dev_row);
        }
next_device:
        discovered = discovered->next;
	}*/
  // printf("\rList rows: %d", g_list_length(devices));
    fflush(stdout);
}

void button1_cb(){
  ExitProgram = true;
}

void discovery_cb(Adapter *adapter, Device *device){
  return;
  printf("New device discovered: %s\n", binc_device_get_address(device));
   
}

void device_remove_cb(Adapter *adapter, Device *device){
  return;
 
    printf("Failed to remove device from list\n");
}

void connect_button_cb(){
  return;
    perror("Device was not found");
}

void disconnect_button_cb(){
  return;
    perror("Device was not found");
}

void remove_button_cb(){
  return;
  perror("Device was not found");
}

void free_buttons(void){
  MemFree(buttons->buttons);
  MemFree(buttons);
}

int main(int argc, char **argv){
    dbusConnection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    InitWindow(800, 450, "vblue");
    SetTargetFPS(60);
    init_button_array();
    init_rec_array();
    //   SetExitKey(0);
    app_activate();
    while(!WindowShouldClose()) {

      BeginDrawing();
      ClearBackground(WHITE);

      ProceedButtons();
      DrawButtons();
      DrawRecs();
      
      EndDrawing();
      if(ExitProgram)
	break;
    }
    buttons->taken = 0;
    //free_buttons();
    
    CloseWindow();
}
