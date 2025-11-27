// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_MOUSE_DROP_HANDLER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_MOUSE_DROP_HANDLER_H_

#include <windows.h>

#include "clay/shell/platform/windows/flutter_windows_engine.h"
#include "clay/shell/platform/windows/window_binding_handler.h"

namespace clay {
class WindowMouseDropHandle : public IDropTarget {
 public:
  WindowMouseDropHandle(WindowBindingHandler* delegate,
                        FlutterWindowsEngine* engine);

  ~WindowMouseDropHandle();

  HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD grfKeyState,
                                      POINTL pt, DWORD* pdwEffect) override;
  HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt,
                                     DWORD* pdwEffect) override;
  HRESULT STDMETHODCALLTYPE DragLeave() override;
  HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD grfKeyState,
                                 POINTL pt, DWORD* pdwEffect) override;

  HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid,
                                           void** ppvObject) override;
  ULONG STDMETHODCALLTYPE AddRef() override;
  ULONG STDMETHODCALLTYPE Release() override;

 private:
  FlutterWindowsEngine* engine_;

  HWND window_handle_;
  DWORD window_dwEffect_;
};
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_MOUSE_DROP_HANDLER_H_
