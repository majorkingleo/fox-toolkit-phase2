/*
 * wlclip - set or read the clipboard of a Wayland compositor from a script.
 *
 * The gate for the clipboard needs an application on the other side that is not
 * FOX, so that a copy is really handed to a peer and a paste really comes from
 * one.  wlclip is that peer.
 *
 * It uses the wlr data control protocol rather than wl_data_device, because a
 * compositor sends the clipboard selection of wl_data_device only to the client
 * that holds the keyboard focus:  a script has no window and never gets it.  The
 * data control protocol exists for exactly this case (clipboard managers), and
 * it also sets the selection without a serial, since no user event is behind it.
 *
 * Usage:
 *   wlclip --read [<mime>]          print the clipboard, exit 1 when it is empty
 *   wlclip --write <text> [<secs>]  offer a text for a while (default 5 s)
 *   wlclip --read-primary [<mime>]  the same for the primary selection
 *   wlclip --write-primary <text> [<secs>]
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <wayland-client.h>
#include "wlr-data-control-unstable-v1-client-protocol.h"

#define DEFAULT_MIME "text/plain;charset=utf-8"

static struct wl_display* display;
static struct wl_seat* seat;
static struct zwlr_data_control_manager_v1* manager;
static int managerversion;
static struct zwlr_data_control_device_v1* device;
static struct zwlr_data_control_source_v1* source;
static struct zwlr_data_control_offer_v1* offer;
static struct zwlr_data_control_offer_v1* primaryoffer;
static char offermime[8][128];
static char primarymime[8][128];
static int numoffermimes;
static int numprimarymimes;
static const char* writetext;

static void writeAll(int fd,const char* text){
  size_t size=strlen(text),done=0;
  ssize_t n;
  while(done<size){
    n=write(fd,text+done,size-done);
    if(n<=0) break;
    done+=n;
    }
  close(fd);
  }


/*******************************************************************************/

// The source side: a peer asks for a mime type and gets the text on a pipe
static void sourceSend(void* data,struct zwlr_data_control_source_v1* s,const char* mime,int32_t fd){
  (void)data; (void)s;
  fprintf(stderr,"wlclip: peer asks for %s\n",mime);
  writeAll(fd,writetext?writetext:"");
  }


static void sourceCancelled(void* data,struct zwlr_data_control_source_v1* s){
  (void)data;
  fprintf(stderr,"wlclip: cancelled\n");
  zwlr_data_control_source_v1_destroy(s);
  if(source==s) source=NULL;
  }


static const struct zwlr_data_control_source_v1_listener sourceListener={
  sourceSend,
  sourceCancelled
  };


/*******************************************************************************/

// The offer side: the mime types the clipboard holds
static void offerOffer(void* data,struct zwlr_data_control_offer_v1* o,const char* mime){
  (void)data;
  if(o==primaryoffer){
    if(numprimarymimes<8){
      snprintf(primarymime[numprimarymimes],sizeof(primarymime[0]),"%s",mime);
      numprimarymimes++;
      }
    return;
    }
  if(numoffermimes<8){
    snprintf(offermime[numoffermimes],sizeof(offermime[0]),"%s",mime);
    numoffermimes++;
    }
  }


static const struct zwlr_data_control_offer_v1_listener offerListener={
  offerOffer
  };


static void takeOffer(struct zwlr_data_control_offer_v1* o){
  // The compositor announces the same offer twice, once as data_offer and once
  // as the selection; the second time there is nothing to do and the offer must
  // not be destroyed, or its mime types are lost
  if(o==offer) return;
  if(offer && offer!=primaryoffer){
    zwlr_data_control_offer_v1_destroy(offer);
    }
  offer=o;
  numoffermimes=0;
  if(o){
    zwlr_data_control_offer_v1_add_listener(o,&offerListener,NULL);
    }
  }


static void deviceDataOffer(void* data,struct zwlr_data_control_device_v1* d,struct zwlr_data_control_offer_v1* o){
  (void)data; (void)d;
  takeOffer(o);
  }

static void deviceSelection(void* data,struct zwlr_data_control_device_v1* d,struct zwlr_data_control_offer_v1* o){
  (void)data; (void)d;
  takeOffer(o);
  fprintf(stderr,"wlclip: clipboard %s\n",o?"offered":"empty");
  }


static void deviceFinished(void* data,struct zwlr_data_control_device_v1* d){
  (void)data; (void)d;
  fprintf(stderr,"wlclip: the compositor took the data device away\n");
  }


static void devicePrimarySelection(void* data,struct zwlr_data_control_device_v1* d,struct zwlr_data_control_offer_v1* o){
  (void)data; (void)d;
  if(o==primaryoffer) return;
  if(primaryoffer && primaryoffer!=offer){
    zwlr_data_control_offer_v1_destroy(primaryoffer);
    }
  primaryoffer=o;
  numprimarymimes=0;
  if(o){
    zwlr_data_control_offer_v1_add_listener(o,&offerListener,NULL);
    }
  fprintf(stderr,"wlclip: selection %s\n",o?"offered":"empty");
  }


static const struct zwlr_data_control_device_v1_listener deviceListener={
  deviceDataOffer,
  deviceSelection,
  deviceFinished,
  devicePrimarySelection
  };


/*******************************************************************************/

static void registryGlobal(void* data,struct wl_registry* registry,uint32_t name,const char* interface,uint32_t version){
  (void)data;
  if(strcmp(interface,wl_seat_interface.name)==0){
    seat=wl_registry_bind(registry,name,&wl_seat_interface,version<5?version:5);
    }
  else if(strcmp(interface,zwlr_data_control_manager_v1_interface.name)==0){
    managerversion=version<2?version:2;
    manager=wl_registry_bind(registry,name,&zwlr_data_control_manager_v1_interface,managerversion);
    }
  }


static void registryGlobalRemove(void* data,struct wl_registry* registry,uint32_t name){
  (void)data; (void)registry; (void)name;
  }


static const struct wl_registry_listener registryListener={
  registryGlobal,
  registryGlobalRemove
  };


// Wait for the compositor to deliver the selection
static int waitForOffer(unsigned ms){
  struct pollfd pfd;
  while(ms>0 && !offer){
    pfd.fd=wl_display_get_fd(display);
    pfd.events=POLLIN;
    pfd.revents=0;
    if(poll(&pfd,1,50)>0){
      wl_display_dispatch(display);
      }
    ms-=50;
    }
  return offer!=NULL;
  }


int main(int argc,char** argv){
  struct wl_registry* registry;
  int i;
  const char* mime=DEFAULT_MIME;
  int fd[2];
  char* data=NULL;
  size_t size=0,alloced=0;
  ssize_t n;
  struct pollfd pfd;
  struct timespec ts;

  if(argc<2){
    fprintf(stderr,"usage: wlclip --read [<mime>] | --write <text> [<secs>] | --read-primary | --write-primary <text> [<secs>]\n");
    return 2;
    }
  display=wl_display_connect(NULL);
  if(!display){
    fprintf(stderr,"wlclip: cannot connect to a compositor\n");
    return 1;
    }
  registry=wl_display_get_registry(display);
  wl_registry_add_listener(registry,&registryListener,NULL);
  wl_display_roundtrip(display);
  if(!manager || !seat){
    fprintf(stderr,"wlclip: no data control manager or seat\n");
    return 1;
    }

  // The primary selection of the data control protocol arrived in version 2
  if(strstr(argv[1],"primary") && managerversion<2){
    fprintf(stderr,"wlclip: the compositor data control protocol is version %d, it has no primary selection\n",managerversion);
    return 1;
    }
  device=zwlr_data_control_manager_v1_get_data_device(manager,seat);
  zwlr_data_control_device_v1_add_listener(device,&deviceListener,NULL);
  wl_display_roundtrip(display);

  // Offer a text to whoever asks for it
  if(strcmp(argv[1],"--write")==0 || strcmp(argv[1],"--write-primary")==0){
    int primary=strcmp(argv[1],"--write-primary")==0;
    int secs=argc>3?atoi(argv[3]):5;
    writetext=argc>2?argv[2]:"";
    source=zwlr_data_control_manager_v1_create_data_source(manager);
    zwlr_data_control_source_v1_add_listener(source,&sourceListener,NULL);
    zwlr_data_control_source_v1_offer(source,"text/plain;charset=utf-8");
    zwlr_data_control_source_v1_offer(source,"UTF8_STRING");
    zwlr_data_control_source_v1_offer(source,"text/plain");
    if(primary){
      zwlr_data_control_device_v1_set_primary_selection(device,source);
      }
    else{
      zwlr_data_control_device_v1_set_selection(device,source);
      }
    wl_display_flush(display);
    fprintf(stderr,"wlclip: offering \"%s\" on the %s for %d s\n",writetext,primary?"selection":"clipboard",secs);
    for(i=0; i<secs*10; i++){
      ts.tv_sec=0;
      ts.tv_nsec=100000000L;
      nanosleep(&ts,NULL);
      wl_display_dispatch_pending(display);
      pfd.fd=wl_display_get_fd(display);
      pfd.events=POLLIN;
      pfd.revents=0;
      if(poll(&pfd,1,0)>0) wl_display_dispatch(display);
      }
    return 0;
    }

  // Read what the clipboard or the primary selection holds
  if(argc>2) mime=argv[2];
  if(strcmp(argv[1],"--read-primary")==0){
    for(i=0; i<30 && !primaryoffer; i++){
      pfd.fd=wl_display_get_fd(display);
      pfd.events=POLLIN;
      pfd.revents=0;
      if(poll(&pfd,1,100)>0) wl_display_dispatch(display);
      }
    if(!primaryoffer){
      fprintf(stderr,"wlclip: the selection is empty\n");
      return 1;
      }
    for(i=0; i<numprimarymimes; i++) fprintf(stderr,"wlclip: offer %s\n",primarymime[i]);
    offer=primaryoffer;
    }
  else if(!waitForOffer(3000)){
    fprintf(stderr,"wlclip: the clipboard is empty\n");
    return 1;
    }
  for(i=0; i<numoffermimes && offer!=primaryoffer; i++) fprintf(stderr,"wlclip: offer %s\n",offermime[i]);
  if(pipe(fd)<0) return 1;
  zwlr_data_control_offer_v1_receive(offer,mime,fd[1]);
  close(fd[1]);
  wl_display_flush(display);
  wl_display_roundtrip(display);
  while(true){
    pfd.fd=fd[0];
    pfd.events=POLLIN;
    pfd.revents=0;
    if(poll(&pfd,1,2000)<=0) break;
    if(size+4096>alloced){
      char* grown;
      alloced=size+4096;
      if(!(grown=realloc(data,alloced))) break;
      data=grown;
      }
    n=read(fd[0],data+size,alloced-size);
    if(n<=0) break;
    size+=n;
    }
  close(fd[0]);
  if(size==0){
    fprintf(stderr,"wlclip: the peer sent nothing for %s\n",mime);
    return 1;
    }
  fwrite(data,1,size,stdout);
  if(data[size-1]!='\n') fputc('\n',stdout);
  free(data);
  wl_display_disconnect(display);
  return 0;
  }
