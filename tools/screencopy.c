/*
 * screencopy - save the first output of a wlroots compositor as a PPM image.
 *
 *   screencopy out.ppm
 *
 * A wlroots compositor (sway) offers the wlr-screencopy protocol; the client
 * asks for the output, waits for the buffer description, copies into shared
 * memory and writes the pixels.  This is what grim does, in a small form that
 * only needs wayland-client.
 *
 * Build: make -C tools screencopy   (needs wayland-client and wayland-scanner)
 */
#include <wayland-client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "wlr-screencopy-client.h"

static struct wl_display* dpy;
static struct wl_shm* shm;
static struct wl_output* output;
static struct zwlr_screencopy_manager_v1* manager;
static struct zwlr_screencopy_frame_v1* frame;
static struct wl_buffer* buffer;
static void* data;
static int width,height,stride;
static unsigned format;
static int ready,done;

static void frame_buffer(void* d,struct zwlr_screencopy_frame_v1* f,uint32_t fmt,uint32_t w,uint32_t h,uint32_t s){
  char name[64];
  int fd,size;
  width=w; height=h; stride=s; format=fmt;
  snprintf(name,sizeof(name),"/fox-screencopy-%d",(int)getpid());
  fd=shm_open(name,O_RDWR|O_CREAT|O_EXCL,0600);
  if(fd<0){ perror("screencopy: shm_open"); done=1; return; }
  shm_unlink(name);
  size=stride*height;
  if(ftruncate(fd,size)<0){ perror("screencopy: ftruncate"); done=1; return; }
  data=mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  struct wl_shm_pool* pool=wl_shm_create_pool(shm,fd,size);
  buffer=wl_shm_pool_create_buffer(pool,0,width,height,stride,fmt);
  wl_shm_pool_destroy(pool);
  close(fd);
  zwlr_screencopy_frame_v1_copy(f, buffer);
  }

static void frame_flags(void* d,struct zwlr_screencopy_frame_v1* f,uint32_t fl){ }
static void frame_ready(void* d,struct zwlr_screencopy_frame_v1* f,uint32_t hi,uint32_t lo,uint32_t tv_nsec){
  ready=1;
  }
static void frame_failed(void* d,struct zwlr_screencopy_frame_v1* f){
  fprintf(stderr,"screencopy: frame failed\n");
  done=1;
  }
static void frame_damage(void* d,struct zwlr_screencopy_frame_v1* f,uint32_t x,uint32_t y,uint32_t w,uint32_t h){ }
static void frame_linux_dmabuf(void* d,struct zwlr_screencopy_frame_v1* f,uint32_t fmt,uint32_t w,uint32_t h){ }
static void frame_buffer_done(void* d,struct zwlr_screencopy_frame_v1* f){ }

static const struct zwlr_screencopy_frame_v1_listener frame_listener={
  frame_buffer,
  frame_flags,
  frame_ready,
  frame_failed,
  frame_damage,
  frame_linux_dmabuf,
  frame_buffer_done,
  };

static void shm_format(void* d,struct wl_shm* s,uint32_t fmt){ }

static const struct wl_shm_listener shm_listener={ shm_format };

static void reg_global(void* d,struct wl_registry* r,uint32_t name,const char* iface,uint32_t version){
  if(strcmp(iface,wl_shm_interface.name)==0){
    shm=wl_registry_bind(r,name,&wl_shm_interface,1);
    wl_shm_add_listener(shm,&shm_listener,0);
    }
  else if(strcmp(iface,zwlr_screencopy_manager_v1_interface.name)==0){
    manager=wl_registry_bind(r,name,&zwlr_screencopy_manager_v1_interface,1);
    }
  else if(strcmp(iface,wl_output_interface.name)==0 && !output){
    output=wl_registry_bind(r,name,&wl_output_interface,1);
    }
  }
static void reg_remove(void* d,struct wl_registry* r,uint32_t name){ }
static const struct wl_registry_listener reg_listener={ reg_global, reg_remove };

int main(int argc,char** argv){
  if(argc!=2){ fprintf(stderr,"usage: screencopy OUT.ppm\n"); return 2; }
  dpy=wl_display_connect(0);
  if(!dpy){ fprintf(stderr,"screencopy: no wayland display\n"); return 1; }
  struct wl_registry* reg=wl_display_get_registry(dpy);
  wl_registry_add_listener(reg,&reg_listener,0);
  wl_display_roundtrip(dpy);
  if(!shm || !manager || !output){ fprintf(stderr,"screencopy: missing globals\n"); return 1; }
  frame=zwlr_screencopy_manager_v1_capture_output(manager,0,output);
  zwlr_screencopy_frame_v1_add_listener(frame,&frame_listener,0);
  while(!ready && !done && wl_display_dispatch(dpy)>=0){}
  if(!ready) return 1;
  FILE* fp=fopen(argv[1],"wb");
  if(!fp){ fprintf(stderr,"screencopy: cannot write\n"); return 1; }
  fprintf(fp,"P6\n%d %d\n255\n",width,height);
  for(int y=0; y<height; y++){
    unsigned char* row=(unsigned char*)data+y*stride;
    for(int x=0; x<width; x++){
      unsigned char rgb[3];
      unsigned char* p=row+x*4;
      // ARGB8888/XRGB8888 are stored B,G,R,X on little endian; the ABGR/RGBA
      // formats a GL renderer may offer are stored R,G,B,X
      if(format==WL_SHM_FORMAT_ARGB8888 || format==WL_SHM_FORMAT_XRGB8888){
        rgb[0]=p[2]; rgb[1]=p[1]; rgb[2]=p[0];
        }
      else{
        rgb[0]=p[0]; rgb[1]=p[1]; rgb[2]=p[2];
        }
      fwrite(rgb,1,3,fp);
      }
    }
  fclose(fp);
  return 0;
  }
