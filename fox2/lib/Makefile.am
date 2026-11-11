## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include

lib_LTLIBRARIES = libFOX-1.7.la

libFOX_1_7_la_LIBADD  = $(X_LIBS) $(X_PRE_LIBS) $(X_BASE_LIBS) $(X_EXTRA_LIBS) $(GL_LIBS)
libFOX_1_7_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -export-dynamic -no-undefined

libFOX_1_7_la_SOURCES = \
FX88591Codec.cpp \
FXAccelTable.cpp \
FXApp.cpp \
fxascii.cpp \
FXAutoThreadStorageKey.cpp \
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
FXMetaClass.cpp \
FXMutex.cpp \
FXObject.cpp \
FXPacker.cpp \
FXPath.cpp \
FXPipe.cpp \
fxprintf.cpp \
fxpriv.cpp \
FXProcess.cpp \
FXRadioButton.cpp \
FXRectangle.cpp \
FXRegistry.cpp \
FXRootWindow.cpp \
fxscanf.cpp \
FXSeparator.cpp \
FXSettings.cpp \
FXShell.cpp \
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
fxpriv.h

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
