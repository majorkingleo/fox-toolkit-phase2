/********************************************************************************
*                                                                               *
*                   M a i n   F O X   I n c l u d e   F i l e                   *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
********************************************************************************/
#ifndef FX_H
#define FX_H

// Basic includes
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cmath>
#include <cstring>

// Intrinsics
#if defined(WIN32)
#if (_MSC_VER >= 1400)          // VC++ 2005 or newer
#if defined(_M_IX86) || defined(_M_X64)
#include <intrin.h>
#endif
#endif
#else
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif
#endif
#endif

// FOX defines
#include "fxver.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxkeys.h"
#include "fxmath.h"
#include "fxendian.h"
#include "fxascii.h"
#include "fxunicode.h"

// FOX classes
#include "FXhalf.h"
#include "FXException.h"
#include "FXAtomic.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXRandom.h"
#include "FXCallback.h"
#include "FXMutex.h"
#include "FXAutoThreadStorageKey.h"
#include "FXSemaphore.h"
#include "FXCompletion.h"
#include "FXRunnable.h"
#include "FXThread.h"
#include "FXPerformance.h"
#include "FXStream.h"
#include "FXPtrList.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXFile.h"
#include "FXMappedFile.h"
#include "FXPipe.h"
#include "FXMemoryStream.h"
#include "FXProcess.h"
#include "FXString.h"
#include "FXVariant.h"
#include "FXVariantArray.h"
#include "FXVariantMap.h"
#include "FXDictionary.h"
#include "FXDictionaryOf.h"
#include "FXReverseDictionary.h"
#include "FXReverseDictionaryOf.h"
#include "FXDLL.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXColors.h"
#include "FXObject.h"
#include "FXPath.h"
#include "FXSystem.h"
#include "FXStat.h"
#include "FXDir.h"
#include "FXStringDictionary.h"
#include "FXParseBuffer.h"
#include "FXJSON.h"
#include "FXJSONFile.h"
#include "FXJSONString.h"
#include "FXINI.h"
#include "FXINIFile.h"
#include "FXXML.h"
#include "FXXMLFile.h"
#include "FXXMLString.h"
#include "FXWorker.h"
#include "FXLFQueue.h"
#include "FXThreadPool.h"
#include "FXTaskGroup.h"
#include "FXParallel.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXCursor.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXId.h"
#include "FXDrawable.h"
#include "FXBitmap.h"
#include "FXImage.h"
#include "FXIcon.h"
#include "FXWindow.h"
#include "FXTheme.h"
#include "FXStyle.h"
#include "FXCSS.h"
#include "FXThemeCache.h"
#include "FXDesktopSettings.h"
#include "FXApp.h"
#include "FXGIFIcon.h"
#include "FXRegion.h"
#include "FXDC.h"
#include "FXDCWindow.h"
#include "FXFrame.h"
#include "FXSeparator.h"
#include "FXLabel.h"
#include "FXTextField.h"
#include "FXButton.h"
#include "FXCheckButton.h"
#include "FXRadioButton.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXHorizontalFrame.h"
#include "FXVerticalFrame.h"
#include "FXRootWindow.h"
#include "FXGroupBox.h"
#include "FXShell.h"
#include "FXSlider.h"
#include "FXToolTip.h"
#include "FXPopup.h"
#include "FXTopWindow.h"
#include "FXDialogBox.h"
#include "FXMainWindow.h"
#include "FXMenuPane.h"
#include "FXDragCorner.h"
#include "FXStatusBar.h"
#include "FXStatusLine.h"
#include "FXMessageBox.h"
#include "FXDockBar.h"
#include "FXToolBar.h"
#include "FXDockHandler.h"
#include "FXToolBarGrip.h"
#include "FXDebugTarget.h"
#include "FXRex.h"
#include "FXUnits.h"
#include "FXExpression.h"


#ifndef FX_NO_GLOBAL_NAMESPACE
using namespace FX;
#endif


#endif
