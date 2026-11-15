/*
 * kwininject - drive the pointer of a KWin session from a script.
 *
 * KWin exposes org_kde_kwin_fake_input to its clients; this program binds it
 * and sends absolute motion and button events.  It is the KWin counterpart of
 * wlxinject, which only exists on wlroots compositors (sway), and is what a
 * test needs to drag a window border or press a widget on the desktop the
 * users actually run.
 *
 * Usage:  kwininject <x> <y> [<action> ...]
 *         kwininject 512 384 move sleep:300 down move:700,500 up
 *
 * Actions:  move, move:<x>,<y>, down, up, rightdown, rightup, sleep:<ms>,
 *           wheel:<notches>
 *
 * The coordinates are layout coordinates of the output, the same ones the
 * compositor reports for the windows.
 *
 * Build: make -C tools kwininject   (needs the protocol XML in
 * scripts/protocols and libwayland-client)
 */
#include <wayland-client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "org-kde-kwin-fake-input-client-protocol.h"

#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111

static struct org_kde_kwin_fake_input* fake;

static void registryGlobal(void* data,struct wl_registry* registry,uint32_t name,const char* interface,uint32_t version){
  (void)data;
  if(strcmp(interface,org_kde_kwin_fake_input_interface.name)==0){
    fake=wl_registry_bind(registry,name,&org_kde_kwin_fake_input_interface,version<3?version:3);
    }
  }

static void registryGlobalRemove(void* data,struct wl_registry* registry,uint32_t name){
  (void)data; (void)registry; (void)name;
  }

static const struct wl_registry_listener registryListener={
  registryGlobal,
  registryGlobalRemove
  };

static void pause_ms(unsigned ms){
  struct timespec ts;
  ts.tv_sec=ms/1000;
  ts.tv_nsec=(long)(ms%1000)*1000000L;
  nanosleep(&ts,nullptr);
  }

static void move_pointer(struct wl_display* display,int x,int y){
  org_kde_kwin_fake_input_pointer_motion_absolute(fake,wl_fixed_from_int(x),wl_fixed_from_int(y));
  wl_display_flush(display);
  }

int main(int argc,char* argv[]){
  struct wl_display* display;
  struct wl_registry* registry;
  int x=0,y=0,i;
  if(argc<3){
    fprintf(stderr,"usage: kwininject <x> <y> [move|move:x,y|down|up|rightdown|rightup|sleep:ms|wheel:n] ...\n");
    return 2;
    }
  x=atoi(argv[1]);
  y=atoi(argv[2]);
  display=wl_display_connect(nullptr);
  if(!display){
    fprintf(stderr,"kwininject: cannot connect to the compositor\n");
    return 1;
    }
  registry=wl_display_get_registry(display);
  wl_registry_add_listener(registry,&registryListener,nullptr);
  wl_display_roundtrip(display);
  if(!fake){
    fprintf(stderr,"kwininject: the compositor has no org_kde_kwin_fake_input\n");
    return 1;
    }
  // Ask for permission; a compositor may ignore everything until it is given
  org_kde_kwin_fake_input_authenticate(fake,"fox test tool","scripted pointer input");
  wl_display_flush(display);
  wl_display_roundtrip(display);
  pause_ms(100);
  move_pointer(display,x,y);
  pause_ms(100);
  for(i=3; i<argc; i++){
    if(strcmp(argv[i],"down")==0){
      org_kde_kwin_fake_input_button(fake,BTN_LEFT,1);
      }
    else if(strcmp(argv[i],"up")==0){
      org_kde_kwin_fake_input_button(fake,BTN_LEFT,0);
      }
    else if(strcmp(argv[i],"rightdown")==0){
      org_kde_kwin_fake_input_button(fake,BTN_RIGHT,1);
      }
    else if(strcmp(argv[i],"rightup")==0){
      org_kde_kwin_fake_input_button(fake,BTN_RIGHT,0);
      }
    else if(strcmp(argv[i],"move")==0){
      move_pointer(display,x,y);
      }
    else if(strncmp(argv[i],"move:",5)==0){
      int nx=x,ny=y;
      if(sscanf(argv[i]+5,"%d,%d",&nx,&ny)==2){
        x=nx;
        y=ny;
        move_pointer(display,x,y);
        }
      else{
        fprintf(stderr,"kwininject: bad position %s\n",argv[i]);
        return 2;
        }
      }
    else if(strncmp(argv[i],"sleep:",6)==0){
      pause_ms((unsigned)atoi(argv[i]+6));
      }
    else if(strncmp(argv[i],"wheel:",6)==0){
      org_kde_kwin_fake_input_axis(fake,0,wl_fixed_from_int(atoi(argv[i]+6)*10));
      }
    else{
      fprintf(stderr,"kwininject: unknown action %s\n",argv[i]);
      return 2;
      }
    wl_display_flush(display);
    wl_display_roundtrip(display);
    }
  pause_ms(100);
  wl_display_flush(display);
  wl_display_disconnect(display);
  return 0;
  }
