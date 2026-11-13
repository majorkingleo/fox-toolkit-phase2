## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include

lib_LTLIBRARIES = libFOX-1.7.la

libFOX_1_7_la_LIBADD  = $(X_LIBS) $(X_PRE_LIBS) $(X_BASE_LIBS) $(X_EXTRA_LIBS) $(GL_LIBS)
libFOX_1_7_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -export-dynamic -no-undefined

if COND_OSWINDOWS
AM_CXXFLAGS += -DFOXDLL -DFOXDLL_EXPORTS -DWIN32 @FOX_NDEBUG@ -D_WINDOWS -DUNICODE -D_USRDLL -D_CRT_SECURE_NO_WARNINGS
libFOX_1_7_la_LDFLAGS += -lcomctl32 -lwsock32 -limm32 -lmsimg32
endif

libFOX_1_7_la_SOURCES = \
FX88591Codec.cpp \
FXAccelTable.cpp \
FXApp.cpp \
fxascii.cpp \
FXButton.cpp \
fxchar.cpp \
FXCheckButton.cpp \
FXColors.cpp \
FXComposeContext.cpp \
FXComposite.cpp \
FXCursor.cpp \
FXDC.cpp \
FXDCWindow.cpp \
FXDebugTarget.cpp \
FXDir.cpp \
FXDragCorner.cpp \
FXDrawable.cpp \
fxezquantize.cpp \
FXFile.cpp \
FXFont.cpp \
FXFrame.cpp \
fxfsquantize.cpp \
FXGIFIcon.cpp \
fxgifio.cpp \
FXGroupBox.cpp \
FXHash.cpp \
FXHorizontalFrame.cpp \
FXIcon.cpp \
FXId.cpp \
FXImage.cpp \
FXIO.cpp \
FXIODevice.cpp \
fxkeysym.cpp \
FXLabel.cpp \
FXMainWindow.cpp \
FXMemoryStream.cpp \
FXMenuPane.cpp \
FXMetaClass.cpp \
FXMutex.cpp \
FXObject.cpp \
FXPacker.cpp \
FXPath.cpp \
FXPipe.cpp \
FXPopup.cpp \
fxprintf.cpp \
fxpriv.cpp \
FXProcess.cpp \
FXRadioButton.cpp \
FXRectangle.cpp \
FXRegion.cpp \
FXRegistry.cpp \
FXRootWindow.cpp \
fxscanf.cpp \
FXSeparator.cpp \
FXSettings.cpp \
FXShell.cpp \
FXSlider.cpp \
FXStat.cpp \
FXStatusBar.cpp \
FXStatusLine.cpp \
FXStream.cpp \
FXString.cpp \
FXStringDictionary.cpp \
fxstrtod.cpp \
fxstrtoll.cpp \
fxstrtoull.cpp \
FXSystem.cpp \
FXTextCodec.cpp \
FXThread.cpp \
FXToolTip.cpp \
FXTopWindow.cpp \
FXTranslator.cpp \
fxunicode.cpp \
FXUTF16Codec.cpp \
fxutils.cpp \
FXVerticalFrame.cpp \
FXVisual.cpp \
FXWindow.cpp \
fxwuquantize.cpp \
fxxbmio.cpp \
FXStyle.cpp \
FXTheme.cpp \
FXCSS.cpp \
FXThemeCache.cpp \
FXDesktopSettings.cpp \
fxpriv.h

# Platform seam: exactly one implementation is compiled into the library
if COND_OSWINDOWS
libFOX_1_7_la_SOURCES += FXApp_win32.cpp FXWindow_win32.cpp FXFont_win32.cpp FXIODevice_win32.cpp FXMutex_win32.cpp FXAutoThreadStorageKey_win32.cpp FXPipe_win32.cpp FXDir_win32.cpp FXFile_win32.cpp FXProcess_win32.cpp FXStat_win32.cpp FXSystem_win32.cpp FXPath_win32.cpp FXThread_win32.cpp FXCursor_win32.cpp FXVisual_win32.cpp FXDCWindow_win32.cpp FXTopWindow_win32.cpp FXRegion_win32.cpp fxutils_win32.cpp fxkeyboard_win32.cpp FXStyle_win32.cpp FXImage_win32.cpp FXRootWindow_win32.cpp
else
libFOX_1_7_la_SOURCES += FXApp_x11.cpp FXWindow_x11.cpp FXFont_x11.cpp FXIODevice_x11.cpp FXMutex_x11.cpp FXAutoThreadStorageKey_x11.cpp FXPipe_x11.cpp FXDir_x11.cpp FXFile_x11.cpp FXProcess_x11.cpp FXStat_x11.cpp FXSystem_x11.cpp FXPath_x11.cpp FXThread_x11.cpp FXCursor_x11.cpp FXVisual_x11.cpp FXDCWindow_x11.cpp FXTopWindow_x11.cpp FXRegion_x11.cpp fxutils_x11.cpp FXStyle_x11.cpp FXImage_x11.cpp FXRootWindow_x11.cpp
endif

noinst_HEADERS = fxplatform.h fxplatformwindow.h fxapppriv.h fxiopriv.h fxpathpriv.h

EXTRA_DIST = \
blank_mask.xbm \
blank.xbm \
crosshair_mask.xbm \
crosshair.xbm \
dndask_mask.xbm \
dndask.xbm \
dndcopy_mask.xbm \
dndcopy.xbm \
dndlink_mask.xbm \
dndlink.xbm \
dndmove_mask.xbm \
dndmove.xbm \
dontdrop_mask.xbm \
dontdrop.xbm \
drag_mask.xbm \
drag.xbm \
hand_mask.xbm \
hand.xbm \
helparrow_mask.xbm \
helparrow.xbm \
hsplit_mask.xbm \
hsplit.xbm \
ne_mask.xbm \
ne.xbm \
nw_mask.xbm \
nw.xbm \
resizeleft_mask.xbm \
resizeleft.xbm \
resizetopleft_mask.xbm \
resizetopleft.xbm \
resizetop_mask.xbm \
resizetopright_mask.xbm \
resizetopright.xbm \
resizetop.xbm \
rotate_mask.xbm \
rotate.xbm \
se_mask.xbm \
se.xbm \
swatch_mask.xbm \
swatch.xbm \
sw_mask.xbm \
sw.xbm \
vsplit_mask.xbm \
vsplit.xbm \
xsplit_mask.xbm \
xsplit.xbm
