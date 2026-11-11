/********************************************************************************
*                                                                              *
*              F X T h e m e I m p o r t   (theme converter)                   *
*                                                                              *
********************************************************************************/
#include "fx.h"
#include <cstdio>
#include <cstring>

using namespace FX;


// Print usage
static void usage(const char* program){
  std::fprintf(stderr,"usage: %s [-c] [-o output.foxtheme] [-n name] source\n",program);
  std::fprintf(stderr,"  source is a GTK theme directory, a gtk.css, or a gtkrc\n");
  std::fprintf(stderr,"  -c  use the theme cache, importing and storing on a miss\n");
  }


// Print one role of the resolved theme
static void showColor(const char* label,FXTheme& theme,FXint role){
  FXColor clr=theme.color(role);
  std::printf("  %-20s #%02X%02X%02X\n",label,FXREDVAL(clr),FXGREENVAL(clr),FXBLUEVAL(clr));
  }


int main(int argc,char* argv[]){
  const char* output=nullptr;
  const char* name=nullptr;
  const char* source=nullptr;
  FXbool usecache=false;
  FXbool parsed=false;
  FXTheme theme;
  FXCSS css;
  FXString text;
  FXFile file;
  int i;

  // Parse the command line
  for(i=1; i<argc; i++){
    if(std::strcmp(argv[i],"-o")==0 && (i+1)<argc){
      output=argv[++i];
      }
    else if(std::strcmp(argv[i],"-n")==0 && (i+1)<argc){
      name=argv[++i];
      }
    else if(std::strcmp(argv[i],"-c")==0){
      usecache=true;
      }
    else if(argv[i][0]=='-'){
      usage(argv[0]);
      return 1;
      }
    else{
      source=argv[i];
      }
    }
  if(!source){
    usage(argv[0]);
    return 1;
    }

  // Resolve the source: from the cache when possible, otherwise by importing
  theme.setClassic();
  if(usecache && FXThemeCache::fetch(source,theme)){
    std::printf("cache       : hit %s\n",FXThemeCache::location(source).text());
    }
  else{
    if(!css.load(source)){
      std::fprintf(stderr,"fxthemeimport: cannot read '%s'\n",source);
      return 1;
      }
    theme.setClassic();
    css.apply(theme);
    parsed=true;
    if(usecache){
      bool stored=FXThemeCache::store(source,theme);
      std::printf("cache       : miss, imported%s\n",stored?" and stored":"");
      }
    }

  std::printf("source      : %s\n",parsed?css.getSource().text():source);
  if(parsed){
    std::printf("rules       : %d\n",css.getRules());
    std::printf("warnings    : %d\n",css.getWarnings());
    }
  std::printf("colors:\n");
  showColor("window_bg",theme,ROLE_WINDOW_BG);
  showColor("surface",theme,ROLE_SURFACE);
  showColor("surface_hilite",theme,ROLE_SURFACE_HILITE);
  showColor("surface_shadow",theme,ROLE_SURFACE_SHADOW);
  showColor("text",theme,ROLE_TEXT);
  showColor("text_disabled",theme,ROLE_TEXT_DISABLED);
  showColor("border",theme,ROLE_BORDER);
  showColor("selection_bg",theme,ROLE_SELECTION_BG);
  showColor("selection_text",theme,ROLE_SELECTION_TEXT);
  showColor("tooltip_bg",theme,ROLE_TOOLTIP_BG);
  showColor("tooltip_text",theme,ROLE_TOOLTIP_TEXT);
  showColor("accent",theme,ROLE_ACCENT);
  std::printf("metrics:\n");
  std::printf("  %-20s %d\n","padding",theme.metric(METRIC_PADDING));
  std::printf("  %-20s %d\n","border_width",theme.metric(METRIC_BORDER_WIDTH));
  std::printf("  %-20s %d\n","focus_width",theme.metric(METRIC_FOCUS_WIDTH));
  std::printf("  %-20s %d\n","corner_radius",theme.metric(METRIC_CORNER_RADIUS));
  std::printf("  %-20s %s\n","font_ui",theme.font(FONT_UI).getFont().text());
  // Write the theme file when asked
  if(output){
    text=theme.describe(name?name:"imported",source);    file.open(output,FXIO::Writing);
    if(!file.isOpen()){
      std::fprintf(stderr,"fxthemeimport: cannot write '%s'\n",output);
      return 1;
      }
    file.writeBlock(text.text(),text.length());
    file.close();
    std::printf("written     : %s (%d bytes)\n",output,text.length());
    }
  return css.getWarnings()?2:0;
  }
