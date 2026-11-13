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
fxmath.cpp \
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
FXSystemTime.cpp \
FXSystemTimeFormat.cpp \
FXSystemTimeParse.cpp \
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
FX88592Codec.cpp \
FXArray.cpp \
FXCompletion.cpp \
FXDictionary.cpp \
FXException.cpp \
FXExpression.cpp \
FXhalf.cpp \
FXINI.cpp \
FXINIFile.cpp \
FXJSON.cpp \
FXJSONFile.cpp \
FXJSONString.cpp \
FXKOI8RCodec.cpp \
FXLFQueue.cpp \
FXMappedFile.cpp \
FXParseBuffer.cpp \
FXPtrList.cpp \
FXRandom.cpp \
FXRaster.cpp \
FXReverseDictionary.cpp \
FXRex.cpp \
FXThreadPool.cpp \
FXTaskGroup.cpp \
FXUnits.cpp \
FXUTF32Codec.cpp \
FXUTF8Codec.cpp \
FXVariant.cpp \
FXVariantArray.cpp \
FXVariantMap.cpp \
FXWorker.cpp \
FXXML.cpp \
FXXMLFile.cpp \
FXXMLString.cpp \
fxpriv.h

# Platform seam: exactly one implementation is compiled into the library
if COND_OSWINDOWS
libFOX_1_7_la_SOURCES += FXApp_win32.cpp FXWindow_win32.cpp FXFont_win32.cpp FXIODevice_win32.cpp FXMutex_win32.cpp FXAutoThreadStorageKey_win32.cpp FXPipe_win32.cpp FXDir_win32.cpp FXFile_win32.cpp FXProcess_win32.cpp FXStat_win32.cpp FXSystem_win32.cpp FXPath_win32.cpp FXThread_win32.cpp FXCursor_win32.cpp FXVisual_win32.cpp FXDCWindow_win32.cpp FXTopWindow_win32.cpp FXRegion_win32.cpp fxutils_win32.cpp fxkeyboard_win32.cpp FXStyle_win32.cpp FXImage_win32.cpp FXRootWindow_win32.cpp FXIcon_win32.cpp FXPopup_win32.cpp FXShell_win32.cpp FXToolTip_win32.cpp FXMainWindow_win32.cpp FXDragCorner_win32.cpp FXDesktopSettings_win32.cpp fxpriv_win32.cpp FXComposeContext_win32.cpp FXRegistry_win32.cpp FXCheckButton_win32.cpp FXSlider_win32.cpp FXSemaphore_win32.cpp FXMappedFile_win32.cpp FXSystemTime_win32.cpp fxosver_win32.cpp
else
# POSIX: shared by the X11 and the Wayland platform
libFOX_1_7_la_SOURCES += FXAutoThreadStorageKey_posix.cpp FXDir_posix.cpp FXFile_posix.cpp FXIODevice_posix.cpp FXMappedFile_posix.cpp FXMutex_posix.cpp FXPath_posix.cpp FXPipe_posix.cpp FXProcess_posix.cpp FXSemaphore_posix.cpp FXStat_posix.cpp FXSystemTime_posix.cpp FXSystem_posix.cpp FXThread_posix.cpp fxosver_posix.cpp fxutils_posix.cpp
# Unix: windowing independent, shared by the X11 and the Wayland platform
libFOX_1_7_la_SOURCES += FXCheckButton_unix.cpp FXDesktopSettings_unix.cpp FXRegistry_unix.cpp FXShell_unix.cpp FXSlider_unix.cpp
if COND_WAYLAND
AM_CXXFLAGS += -DFOX_WAYLAND
else
AM_CXXFLAGS += -DFOX_X11
libFOX_1_7_la_SOURCES += FXApp_x11.cpp FXComposeContext_x11.cpp FXCursor_x11.cpp FXDCWindow_x11.cpp FXDragCorner_x11.cpp FXFont_x11.cpp FXIcon_x11.cpp FXImage_x11.cpp FXMainWindow_x11.cpp FXPopup_x11.cpp FXRegion_x11.cpp FXRootWindow_x11.cpp FXStyle_x11.cpp FXToolTip_x11.cpp FXTopWindow_x11.cpp FXVisual_x11.cpp FXWindow_x11.cpp fxpriv_x11.cpp
endif
endif

noinst_HEADERS = fxplatform.h fxplatformwindow.h fxapppriv.h fxiopriv.h fxpathpriv.h fxraster.h fxstipples.h

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
