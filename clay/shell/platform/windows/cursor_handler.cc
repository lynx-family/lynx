// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/cursor_handler.h"

#include <windows.h>

#include <cstring>
#include <vector>

#include "clay/shell/platform/windows/flutter_windows_engine.h"
#include "clay/shell/platform/windows/flutter_windows_view.h"
#include "clay/ui/platform/cursor_types.h"

namespace clay {

CursorHandler::CursorHandler(FlutterWindowsEngine* engine)
    : engine_(engine), weak_factory_(this) {}

void CursorHandler::ActivateSystemCursor(int type, const char* path) {
  auto kind = static_cast<clay::CursorTypes>(type);
  FlutterWindowsView* view = engine_->view();
  if (view == nullptr) {
    FML_LOG(ERROR) << "Cursor is not available in Windows headless mode";
    return;
  }
  view->UpdateFlutterCursor(kind);
}

HCURSOR GetCursorFromBuffer(const std::vector<uint8_t>& buffer, double hot_x,
                            double hot_y, int width, int height) {
  HCURSOR cursor = nullptr;
  HDC display_dc = GetDC(NULL);
  // Flutter should returns rawBGRA, which has 8bits * 4channels.
  BITMAPINFO bmi;
  memset(&bmi, 0, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = width * height * 4;
  // Create the pixmap DIB section
  uint8_t* pixels = 0;
  HBITMAP bitmap =
      CreateDIBSection(display_dc, &bmi, DIB_RGB_COLORS, (void**)&pixels, 0, 0);
  ReleaseDC(0, display_dc);
  if (!bitmap || !pixels) {
    return nullptr;
  }
  int bytes_per_line = width * 4;
  for (int y = 0; y < height; ++y) {
    memcpy(pixels + y * bytes_per_line, &buffer[bytes_per_line * y],
           bytes_per_line);
  }
  HBITMAP mask;
  GetMaskBitmaps(bitmap, mask);
  ICONINFO icon_info;
  icon_info.fIcon = 0;
  icon_info.xHotspot = hot_x;
  icon_info.yHotspot = hot_y;
  icon_info.hbmMask = mask;
  icon_info.hbmColor = bitmap;
  cursor = CreateIconIndirect(&icon_info);
  DeleteObject(mask);
  DeleteObject(bitmap);
  return cursor;
}

void GetMaskBitmaps(HBITMAP bitmap, HBITMAP& mask_bitmap) {
  HDC h_dc = ::GetDC(NULL);
  HDC h_main_dc = ::CreateCompatibleDC(h_dc);
  HDC h_and_mask_dc = ::CreateCompatibleDC(h_dc);

  // Get the dimensions of the source bitmap
  BITMAP bm;
  ::GetObject(bitmap, sizeof(BITMAP), &bm);
  mask_bitmap = ::CreateCompatibleBitmap(h_dc, bm.bmWidth, bm.bmHeight);

  // Select the bitmaps to DC
  HBITMAP h_old_main_bitmap = (HBITMAP)::SelectObject(h_main_dc, bitmap);
  HBITMAP h_old_and_mask_bitmap =
      (HBITMAP)::SelectObject(h_and_mask_dc, mask_bitmap);

  // Scan each pixel of the souce bitmap and create the masks
  COLORREF main_bit_pixel;
  for (int x = 0; x < bm.bmWidth; ++x) {
    for (int y = 0; y < bm.bmHeight; ++y) {
      main_bit_pixel = ::GetPixel(h_main_dc, x, y);
      if (main_bit_pixel == RGB(0, 0, 0)) {
        ::SetPixel(h_and_mask_dc, x, y, RGB(255, 255, 255));
      } else {
        ::SetPixel(h_and_mask_dc, x, y, RGB(0, 0, 0));
      }
    }
  }
  ::SelectObject(h_main_dc, h_old_main_bitmap);
  ::SelectObject(h_and_mask_dc, h_old_and_mask_bitmap);

  ::DeleteDC(h_and_mask_dc);
  ::DeleteDC(h_main_dc);

  ::ReleaseDC(NULL, h_dc);
}

}  // namespace clay
