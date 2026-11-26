## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include

lib_LTLIBRARIES = libFOX-2.0.la

libFOX_2_0_la_LIBADD  = $(PLATFORM_LIBS) $(GL_LIBS) $(LIBS)
libFOX_2_0_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -export-dynamic -no-undefined

if COND_OSWINDOWS
AM_CXXFLAGS += -DFOXDLL -DFOXDLL_EXPORTS -DWIN32 @FOX_NDEBUG@ -D_WINDOWS -DUNICODE -D_USRDLL -D_CRT_SECURE_NO_WARNINGS
libFOX_2_0_la_LDFLAGS += -lcomctl32 -lwsock32 -limm32 -lmsimg32 -lgdiplus
endif

libFOX_2_0_la_SOURCES = \
FX88591Codec.cpp \
FXAccelTable.cpp \
FXApp.cpp \
FXArrowButton.cpp \
fxascii.cpp \
FXBMPIcon.cpp \
FXBitmap.cpp \
FXBMPImage.cpp \
FXButton.cpp \
FXCanvas.cpp \
fxchar.cpp \
FXCheckButton.cpp \
FXColorBar.cpp \
FXColorDialog.cpp \
FXColorList.cpp \
FXColorRing.cpp \
FXColors.cpp \
FXColorSelector.cpp \
FXColorWell.cpp \
FXColorWheel.cpp \
FXComposeContext.cpp \
FXComposite.cpp \
FXCursor.cpp \
FXDC.cpp \
FXDCWindow.cpp \
FXDebugTarget.cpp \
FXDir.cpp \
FXDialogBox.cpp \
FXDirVisitor.cpp \
FXDragCorner.cpp \
FXDrawable.cpp \
fxezquantize.cpp \
fxmath.cpp \
fxparsegeometry.cpp \
FXFile.cpp \
FXFont.cpp \
FXFontDialog.cpp \
FXFontSelector.cpp \
FXFrame.cpp \
fxfsquantize.cpp \
FXGIFIcon.cpp \
FXGlyphs.cpp \
fxbmpio.cpp \
fxgifio.cpp \
FXComboBox.cpp \
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
FXList.cpp \
FXMenuButton.cpp \
FXMenuCaption.cpp \
FXMenuCascade.cpp \
FXMenuCheck.cpp \
FXMenuCommand.cpp \
FXMenuPane.cpp \
FXMenuRadio.cpp \
FXMenuSeparator.cpp \
FXMenuTitle.cpp \
FXMetaClass.cpp \
FXMutex.cpp \
FXObject.cpp \
FXPacker.cpp \
FXPath.cpp \
FXPicker.cpp \
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
FXScrollArea.cpp \
FXSearchDialog.cpp \
FXScrollBar.cpp \
FXScrollWindow.cpp \
FXObjectList.cpp \
FXSpinner.cpp \
FXStatusBar.cpp \
FXStatusLine.cpp \
FXStream.cpp \
FXString.cpp \
FXStringDictionary.cpp \
FXSwitcher.cpp \
FXConsole.cpp \
FXTerminal.cpp \
fxterminalparser.cpp \
fxterminalscreen.cpp \
fxterminalwidth.cpp \
FXGLCanvas.cpp \
FXGLObject.cpp \
FXGLViewer.cpp \
FXGLContext.cpp \
FXGLVisual.cpp \
fxglwayland.cpp \
FXGlobVisitor.cpp \
FXShutter.cpp \
FXMDIClient.cpp \
FXMDIChild.cpp \
FXMDIButton.cpp \
fxcrc.cpp \
fxxpmio.cpp \
fxtifio.cpp \
fxtargaio.cpp \
fxrgbio.cpp \
fxrasio.cpp \
fxqoifio.cpp \
fxppmio.cpp \
fxpcxio.cpp \
fxpngio.cpp \
fxwebpio.cpp \
fxjxlio.cpp \
fxjpegio.cpp \
fxjp2io.cpp \
fxiffio.cpp \
fxexeio.cpp \
fxddsio.cpp \
FXPrintDialog.cpp \
FXDCPrint.cpp \
FXToolBarTab.cpp \
FXRulerView.cpp \
FXRuler.cpp \
FXGradientBar.cpp \
FXImageView.cpp \
FXBitmapView.cpp \
FXBitmapFrame.cpp \
FXGIFCursor.cpp \
FXCURCursor.cpp \
FXJXLIcon.cpp \
FXJXLImage.cpp \
FXWEBPIcon.cpp \
FXWEBPImage.cpp \
FXJPGIcon.cpp \
FXJPGImage.cpp \
FXJP2Icon.cpp \
FXJP2Image.cpp \
FXIFFIcon.cpp \
FXIFFImage.cpp \
FXEXEIcon.cpp \
FXEXEImage.cpp \
FXDDSIcon.cpp \
FXDDSImage.cpp \
FXXPMIcon.cpp \
FXXPMImage.cpp \
FXXBMIcon.cpp \
FXXBMImage.cpp \
FXTIFIcon.cpp \
FXTIFImage.cpp \
FXTGAIcon.cpp \
FXTGAImage.cpp \
FXRGBIcon.cpp \
FXRGBImage.cpp \
FXRASIcon.cpp \
FXRASImage.cpp \
FXQOIFIcon.cpp \
FXQOIFImage.cpp \
FXSVGImage.cpp \
FXSVGIcon.cpp \
fxsvgio.cpp \
FXPPMIcon.cpp \
FXPPMImage.cpp \
FXPNGIcon.cpp \
FXPNGImage.cpp \
FXPCXIcon.cpp \
FXPCXImage.cpp \
FXICOIcon.cpp \
FXICOImage.cpp \
FXTable.cpp \
FX7Segment.cpp \
FXImageFrame.cpp \
FXCalendar.cpp \
FXCalendarView.cpp \
FXWizard.cpp \
FXProgressDialog.cpp \
FXReplaceDialog.cpp \
FXKnob.cpp \
FXDate.cpp \
FXDirDialog.cpp \
FXDirSelector.cpp \
FXFileDialog.cpp \
FXFileSelector.cpp \
FXRecentFiles.cpp \
FXPathBox.cpp \
FXDriveBox.cpp \
FXDirBox.cpp \
FXInputDialog.cpp \
FXChoiceBox.cpp \
FXRealSpinner.cpp \
FXRangeSlider.cpp \
FXDial.cpp \
FXTriStateButton.cpp \
FXToggleButton.cpp \
FXTreeListBox.cpp \
FXListBox.cpp \
FXOptionMenu.cpp \
FXScrollPane.cpp \
FXFileList.cpp \
FXDirList.cpp \
FXFileProgressDialog.cpp \
FXProgressBar.cpp \
FXIconList.cpp \
FXGIFImage.cpp \
FXFileAssociations.cpp \
FXIconCache.cpp \
FXIconSource.cpp \
FXFileStream.cpp \
FXURL.cpp \
FXRealSlider.cpp \
FXGauge.cpp \
FXDataTarget.cpp \
FXDockHandler.cpp \
FXDockTitle.cpp \
FXToolBarGrip.cpp \
FXToolBarShell.cpp \
FXDockSite.cpp \
FXDockBar.cpp \
FXToolBar.cpp \
FXMessageBox.cpp \
FXMenuBar.cpp \
FXTreeList.cpp \
FX4Splitter.cpp \
FXSplitter.cpp \
FXHeader.cpp \
FXSpring.cpp \
FXTabBar.cpp \
FXTabBook.cpp \
FXTabItem.cpp \
fxtabbify.cpp \
fxstrtod.cpp \
fxstrtoll.cpp \
fxstrtoull.cpp \
FXSystem.cpp \
FXSystemTime.cpp \
FXSystemTimeFormat.cpp \
FXSystemTimeParse.cpp \
FXText.cpp \
FXTextCodec.cpp \
FXTextField.cpp \
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
FXCP1252Codec.cpp \
FXComplexd.cpp \
FXComplexf.cpp \
FXExtentd.cpp \
FXExtentf.cpp \
FXMat2d.cpp \
FXMat2f.cpp \
FXMat3d.cpp \
FXMat3f.cpp \
FXMat4d.cpp \
FXMat4f.cpp \
FXQuatd.cpp \
FXQuatf.cpp \
FXRanged.cpp \
FXRangef.cpp \
FXSphered.cpp \
FXSpheref.cpp \
FXVec2d.cpp \
FXVec2f.cpp \
FXVec3d.cpp \
FXVec3f.cpp \
FXVec4d.cpp \
FXVec4f.cpp \
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
FXMatrix.cpp \
FXParseBuffer.cpp \
FXPtrList.cpp \
FXRandom.cpp \
FXRaster.cpp \
FXRects.cpp \
FXReverseDictionary.cpp \
FXRex.cpp \
FXThreadPool.cpp \
FXTaskGroup.cpp \
FXUnits.cpp \
FXUndoList.cpp \
FXUTF32Codec.cpp \
FXUTF8Codec.cpp \
FXVariant.cpp \
FXVariantArray.cpp \
FXVariantMap.cpp \
FXWorker.cpp \
FXWinMap.cpp \
FXXML.cpp \
FXXMLFile.cpp \
FXXMLString.cpp \
FXAxis.cpp \
FXChart.cpp \
FXChartUtils.cpp \
FXCurve.cpp \
FXData.cpp \
FX2DChart.cpp \
FX2DPlot.cpp \
FXFollowView.cpp \
FXInteractivePlot.cpp \
icons.cpp \
icons.h \
fxpriv.h

# Platform seam: exactly one implementation is compiled into the library
if COND_OSWINDOWS
libFOX_2_0_la_SOURCES += FXApp_win32.cpp FXWindow_win32.cpp FXFont_win32.cpp FXIODevice_win32.cpp FXMutex_win32.cpp FXAutoThreadStorageKey_win32.cpp FXPipe_win32.cpp FXDir_win32.cpp FXFile_win32.cpp FXProcess_win32.cpp FXStat_win32.cpp FXSystem_win32.cpp FXPath_win32.cpp FXThread_win32.cpp FXCursor_win32.cpp FXVisual_win32.cpp FXGLVisual_win32.cpp FXDCWindow_win32.cpp FXTopWindow_win32.cpp FXRegion_win32.cpp fxutils_win32.cpp fxkeyboard_win32.cpp FXStyle_win32.cpp FXImage_win32.cpp FXBitmap_win32.cpp FXRootWindow_win32.cpp FXIcon_win32.cpp FXPopup_win32.cpp FXShell_win32.cpp FXToolTip_win32.cpp FXMainWindow_win32.cpp FXDragCorner_win32.cpp FXDesktopSettings_win32.cpp fxpriv_win32.cpp fxshadow_win32.cpp FXComposeContext_win32.cpp FXRegistry_win32.cpp FXCheckButton_win32.cpp FXSlider_win32.cpp FXSemaphore_win32.cpp FXMappedFile_win32.cpp FXSystemTime_win32.cpp fxosver_win32.cpp fxpty_win32.cpp
else
# POSIX: shared by the X11 and the Wayland platform
libFOX_2_0_la_SOURCES += FXAutoThreadStorageKey_posix.cpp FXDir_posix.cpp FXFile_posix.cpp FXIODevice_posix.cpp FXMappedFile_posix.cpp FXMutex_posix.cpp FXPath_posix.cpp FXPipe_posix.cpp FXProcess_posix.cpp FXSemaphore_posix.cpp FXStat_posix.cpp FXSystemTime_posix.cpp FXSystem_posix.cpp FXThread_posix.cpp fxosver_posix.cpp fxutils_posix.cpp fxpty_posix.cpp
# Unix: windowing independent, shared by the X11 and the Wayland platform
libFOX_2_0_la_SOURCES += FXCheckButton_unix.cpp FXDesktopSettings_unix.cpp FXRegistry_unix.cpp FXShell_unix.cpp FXSlider_unix.cpp
if COND_WAYLAND
AM_CXXFLAGS += -DFOX_WAYLAND
AM_CPPFLAGS =
BUILT_SOURCES =
CLEANFILES =
nodist_libFOX_2_0_la_SOURCES =

# Client glue generated from the vendored protocol XML, the same list the CMake
# build uses; the headers land in the build tree and are found through INCLUDES
WP_GEN = $(top_builddir)/lib/protocols
AM_CPPFLAGS += -I$(WP_GEN)

$(WP_GEN)/xdg-shell-client-protocol.h: $(top_srcdir)/protocols/stable/xdg-shell/xdg-shell.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/xdg-shell-protocol.c: $(top_srcdir)/protocols/stable/xdg-shell/xdg-shell.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/viewporter-client-protocol.h: $(top_srcdir)/protocols/stable/viewporter/viewporter.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/viewporter-protocol.c: $(top_srcdir)/protocols/stable/viewporter/viewporter.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/tablet-v2-client-protocol.h: $(top_srcdir)/protocols/stable/tablet/tablet-v2.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/tablet-v2-protocol.c: $(top_srcdir)/protocols/stable/tablet/tablet-v2.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/xdg-decoration-unstable-v1-client-protocol.h: $(top_srcdir)/protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/xdg-decoration-unstable-v1-protocol.c: $(top_srcdir)/protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/primary-selection-unstable-v1-client-protocol.h: $(top_srcdir)/protocols/unstable/primary-selection/primary-selection-unstable-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/primary-selection-unstable-v1-protocol.c: $(top_srcdir)/protocols/unstable/primary-selection/primary-selection-unstable-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/text-input-unstable-v3-client-protocol.h: $(top_srcdir)/protocols/unstable/text-input/text-input-unstable-v3.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/text-input-unstable-v3-protocol.c: $(top_srcdir)/protocols/unstable/text-input/text-input-unstable-v3.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/xdg-output-unstable-v1-client-protocol.h: $(top_srcdir)/protocols/unstable/xdg-output/xdg-output-unstable-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/xdg-output-unstable-v1-protocol.c: $(top_srcdir)/protocols/unstable/xdg-output/xdg-output-unstable-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/fractional-scale-v1-client-protocol.h: $(top_srcdir)/protocols/staging/fractional-scale/fractional-scale-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/fractional-scale-v1-protocol.c: $(top_srcdir)/protocols/staging/fractional-scale/fractional-scale-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/cursor-shape-v1-client-protocol.h: $(top_srcdir)/protocols/staging/cursor-shape/cursor-shape-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/cursor-shape-v1-protocol.c: $(top_srcdir)/protocols/staging/cursor-shape/cursor-shape-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/xdg-activation-v1-client-protocol.h: $(top_srcdir)/protocols/staging/xdg-activation/xdg-activation-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/xdg-activation-v1-protocol.c: $(top_srcdir)/protocols/staging/xdg-activation/xdg-activation-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/xdg-toplevel-icon-v1-client-protocol.h: $(top_srcdir)/protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/xdg-toplevel-icon-v1-protocol.c: $(top_srcdir)/protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/xdg-dialog-v1-client-protocol.h: $(top_srcdir)/protocols/staging/xdg-dialog/xdg-dialog-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/xdg-dialog-v1-protocol.c: $(top_srcdir)/protocols/staging/xdg-dialog/xdg-dialog-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

$(WP_GEN)/idle-inhibit-unstable-v1-client-protocol.h: $(top_srcdir)/protocols/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) client-header $< $@

$(WP_GEN)/idle-inhibit-unstable-v1-protocol.c: $(top_srcdir)/protocols/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml
	@$(MKDIR_P) $(WP_GEN)
	$(WAYLAND_SCANNER) private-code $< $@

BUILT_SOURCES += $(WP_GEN)/xdg-shell-client-protocol.h \
	$(WP_GEN)/viewporter-client-protocol.h \
	$(WP_GEN)/tablet-v2-client-protocol.h \
	$(WP_GEN)/xdg-decoration-unstable-v1-client-protocol.h \
	$(WP_GEN)/primary-selection-unstable-v1-client-protocol.h \
	$(WP_GEN)/text-input-unstable-v3-client-protocol.h \
	$(WP_GEN)/xdg-output-unstable-v1-client-protocol.h \
	$(WP_GEN)/fractional-scale-v1-client-protocol.h \
	$(WP_GEN)/cursor-shape-v1-client-protocol.h \
	$(WP_GEN)/xdg-activation-v1-client-protocol.h \
	$(WP_GEN)/xdg-toplevel-icon-v1-client-protocol.h \
	$(WP_GEN)/xdg-dialog-v1-client-protocol.h \
	$(WP_GEN)/idle-inhibit-unstable-v1-client-protocol.h

nodist_libFOX_2_0_la_SOURCES += $(WP_GEN)/xdg-shell-protocol.c \
	$(WP_GEN)/viewporter-protocol.c \
	$(WP_GEN)/tablet-v2-protocol.c \
	$(WP_GEN)/xdg-decoration-unstable-v1-protocol.c \
	$(WP_GEN)/primary-selection-unstable-v1-protocol.c \
	$(WP_GEN)/text-input-unstable-v3-protocol.c \
	$(WP_GEN)/xdg-output-unstable-v1-protocol.c \
	$(WP_GEN)/fractional-scale-v1-protocol.c \
	$(WP_GEN)/cursor-shape-v1-protocol.c \
	$(WP_GEN)/xdg-activation-v1-protocol.c \
	$(WP_GEN)/xdg-toplevel-icon-v1-protocol.c \
	$(WP_GEN)/xdg-dialog-v1-protocol.c \
	$(WP_GEN)/idle-inhibit-unstable-v1-protocol.c

CLEANFILES += $(WP_GEN)/xdg-shell-client-protocol.h \
	$(WP_GEN)/viewporter-client-protocol.h \
	$(WP_GEN)/tablet-v2-client-protocol.h \
	$(WP_GEN)/xdg-decoration-unstable-v1-client-protocol.h \
	$(WP_GEN)/primary-selection-unstable-v1-client-protocol.h \
	$(WP_GEN)/text-input-unstable-v3-client-protocol.h \
	$(WP_GEN)/xdg-output-unstable-v1-client-protocol.h \
	$(WP_GEN)/fractional-scale-v1-client-protocol.h \
	$(WP_GEN)/cursor-shape-v1-client-protocol.h \
	$(WP_GEN)/xdg-activation-v1-client-protocol.h \
	$(WP_GEN)/xdg-toplevel-icon-v1-client-protocol.h \
	$(WP_GEN)/xdg-dialog-v1-client-protocol.h \
	$(WP_GEN)/xdg-shell-protocol.c \
	$(WP_GEN)/viewporter-protocol.c \
	$(WP_GEN)/tablet-v2-protocol.c \
	$(WP_GEN)/xdg-decoration-unstable-v1-protocol.c \
	$(WP_GEN)/primary-selection-unstable-v1-protocol.c \
	$(WP_GEN)/text-input-unstable-v3-protocol.c \
	$(WP_GEN)/xdg-output-unstable-v1-protocol.c \
	$(WP_GEN)/fractional-scale-v1-protocol.c \
	$(WP_GEN)/cursor-shape-v1-protocol.c \
	$(WP_GEN)/xdg-activation-v1-protocol.c \
	$(WP_GEN)/xdg-toplevel-icon-v1-protocol.c \
	$(WP_GEN)/idle-inhibit-unstable-v1-client-protocol.h \
	$(WP_GEN)/idle-inhibit-unstable-v1-protocol.c
libFOX_2_0_la_SOURCES += FXApp_wayland.cpp fxactivation_wayland.cpp FXRegion_wayland.cpp FXVisual_wayland.cpp FXGLVisual_wayland.cpp FXImage_wayland.cpp FXBitmap_wayland.cpp FXIcon_wayland.cpp FXFont_wayland.cpp FXStyle_wayland.cpp FXCursor_wayland.cpp FXDCWindow_wayland.cpp FXWindow_wayland.cpp FXRootWindow_wayland.cpp FXTopWindow_wayland.cpp FXMainWindow_wayland.cpp FXPopup_wayland.cpp FXToolTip_wayland.cpp FXDragCorner_wayland.cpp FXComposeContext_wayland.cpp FXDesktopSettings_wayland.cpp fxclipboard_wayland.cpp fxcsd_wayland.cpp fxidle_wayland.cpp fxscale_wayland.cpp fxtextinput_wayland.cpp fxtoplevelicon_wayland.cpp fxwayland.cpp fxseat_wayland.cpp fxplatform_wayland.cpp fxplatformwindow_wayland.cpp
else
AM_CXXFLAGS += -DFOX_X11
libFOX_2_0_la_SOURCES += FXApp_x11.cpp FXComposeContext_x11.cpp FXCursor_x11.cpp FXDCWindow_x11.cpp FXDragCorner_x11.cpp FXFont_x11.cpp FXIcon_x11.cpp FXImage_x11.cpp FXBitmap_x11.cpp FXMainWindow_x11.cpp FXPopup_x11.cpp FXRegion_x11.cpp FXRootWindow_x11.cpp FXStyle_x11.cpp FXToolTip_x11.cpp FXTopWindow_x11.cpp FXVisual_x11.cpp FXGLVisual_x11.cpp FXWindow_x11.cpp fxpriv_x11.cpp
endif
endif

noinst_HEADERS = fxplatform.h fxplatformwindow.h fxapppriv.h fxiopriv.h fxpathpriv.h fxglyphs.h fxglpriv.h fxraster.h fxrects.h fxstipples.h fxwinmap.h fxterminalparser.h fxterminalscreen.h fxterminalwidth.h fxpty.h

EXTRA_DIST = nanosvg.h nanosvgrast.h nanosvg-LICENSE.txt \
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
