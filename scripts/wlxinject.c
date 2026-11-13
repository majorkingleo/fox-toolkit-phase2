/*
 * wlxinject - drive the pointer of a wlroots compositor from a script.
 *
 * A headless sway has no input devices at all, so its seat reports no
 * capabilities and neither swaymsg nor wlrctl can produce a click:  wlrctl
 * creates a wlr virtual pointer and destroys it again before the compositor
 * has even delivered the button to the client.  This program creates one
 * virtual pointer, keeps it alive for the whole gesture and then moves,
 * presses and releases it.
 *
 * Usage:  wlxinject <x> <y> [<action> ...]
 *         wlxinject 40 46 down up
 *
 * Actions:  down, up, move, sleep:<ms>, rightdown, rightup
 *
 * The coordinates are layout coordinates, the same ones grim reports.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <wayland-client.h>
#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"

#define BTN_LEFT 272
#define BTN_RIGHT 273

static struct wl_seat* seat;
static struct zwlr_virtual_pointer_manager_v1* manager;

static void registryGlobal(void* data,struct wl_registry* registry,uint32_t name,const char* interface,uint32_t version){
  (void)data;
  if(strcmp(interface,"wl_seat")==0) seat=wl_registry_bind(registry,name,&wl_seat_interface,version<5?version:5);
  else if(strcmp(interface,"zwlr_virtual_pointer_manager_v1")==0) manager=wl_registry_bind(registry,name,&zwlr_virtual_pointer_manager_v1_interface,1);
  }

static void registryGlobalRemove(void* data,struct wl_registry* registry,uint32_t name){
  (void)data; (void)registry; (void)name;
  }

static const struct wl_registry_listener registryListener={
  registryGlobal,
  registryGlobalRemove
  };

static uint32_t timestamp(void){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (uint32_t)(ts.tv_sec*1000+ts.tv_nsec/1000000);
  }

static void pause_ms(unsigned ms){
  struct timespec ts;
  ts.tv_sec=ms/1000;
  ts.tv_nsec=(long)(ms%1000)*1000000L;
  nanosleep(&ts,0);
  }

int main(int argc,char* argv[]){
  struct wl_display* display;
  struct wl_registry* registry;
  struct zwlr_virtual_pointer_v1* pointer;
  int x=0,y=0,i;
  uint32_t extentw=1024,extenth=640;
  if(argc<3){
    fprintf(stderr,"usage: wlxinject <x> <y> [down|up|rightdown|rightup|move|sleep:ms] ...\n");
    return 2;
    }
  x=atoi(argv[1]);
  y=atoi(argv[2]);
  if(getenv("WLXINJECT_WIDTH")) extentw=(uint32_t)atoi(getenv("WLXINJECT_WIDTH"));
  if(getenv("WLXINJECT_HEIGHT")) extenth=(uint32_t)atoi(getenv("WLXINJECT_HEIGHT"));
  display=wl_display_connect(0);
  if(!display){
    fprintf(stderr,"wlxinject: cannot connect to the compositor\n");
    return 1;
    }
  registry=wl_display_get_registry(display);
  wl_registry_add_listener(registry,&registryListener,0);
  wl_display_roundtrip(display);
  if(!manager){
    fprintf(stderr,"wlxinject: the compositor has no virtual pointer manager\n");
    return 1;
    }
  pointer=zwlr_virtual_pointer_manager_v1_create_virtual_pointer(manager,seat ? seat : 0);
  if(!pointer){
    fprintf(stderr,"wlxinject: cannot create a virtual pointer\n");
    return 1;
    }
  wl_display_roundtrip(display);
  pause_ms(200);                                        // Let the seat pick it up
  for(i=3; i<argc; i++){
    if(strcmp(argv[i],"down")==0){
      zwlr_virtual_pointer_v1_button(pointer,timestamp(),BTN_LEFT,WL_POINTER_BUTTON_STATE_PRESSED);
      }
    else if(strcmp(argv[i],"up")==0){
      zwlr_virtual_pointer_v1_button(pointer,timestamp(),BTN_LEFT,WL_POINTER_BUTTON_STATE_RELEASED);
      }
    else if(strcmp(argv[i],"rightdown")==0){
      zwlr_virtual_pointer_v1_button(pointer,timestamp(),BTN_RIGHT,WL_POINTER_BUTTON_STATE_PRESSED);
      }
    else if(strcmp(argv[i],"rightup")==0){
      zwlr_virtual_pointer_v1_button(pointer,timestamp(),BTN_RIGHT,WL_POINTER_BUTTON_STATE_RELEASED);
      }
    else if(strcmp(argv[i],"move")==0){
      zwlr_virtual_pointer_v1_motion_absolute(pointer,timestamp(),(uint32_t)x,(uint32_t)y,extentw,extenth);
      }
    else if(strncmp(argv[i],"sleep:",6)==0){
      pause_ms((unsigned)atoi(argv[i]+6));
      }
    else{
      fprintf(stderr,"wlxinject: unknown action %s\n",argv[i]);
      return 2;
      }
    zwlr_virtual_pointer_v1_frame(pointer);
    wl_display_flush(display);
    wl_display_roundtrip(display);
    }
  pause_ms(200);
  zwlr_virtual_pointer_v1_destroy(pointer);
  wl_display_flush(display);
  wl_display_disconnect(display);
  return 0;
  }
