// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/shell/platform/windows/window_mouse_drop_handler.h"

#include <Shlobj.h>
#include <shellapi.h>

#include <cstdio>
#include <list>
#include <string>
#include <unordered_map>

#include "clay/fml/logging.h"
#include "clay/fml/platform/win/wstring_conversion.h"
#include "clay/shell/platform/windows/platform_handler.h"
#include "clay/ui/gesture/drag_drop_manager.h"

namespace clay {

WindowMouseDropHandle::WindowMouseDropHandle(WindowBindingHandler* delegate,
                                             FlutterWindowsEngine* engine)
    : engine_(engine), window_dwEffect_(0) {
  auto target = delegate->GetRenderTarget();
  window_handle_ = std::get<HWND>(target);
  auto ret = RegisterDragDrop(window_handle_, this);
  if (ret != 0) {
    FML_DLOG(WARNING) << "RegisterDragDrop failed: " << ret;
  }
}

WindowMouseDropHandle::~WindowMouseDropHandle() {
  RevokeDragDrop(window_handle_);
}

HRESULT STDMETHODCALLTYPE WindowMouseDropHandle::DragEnter(
    IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) {
  window_dwEffect_ = DROPEFFECT_NONE;
  std::string drag_type;
  // Check the data object for supported formats
  FORMATETC formatEtc = {CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1,
                         TYMED_HGLOBAL};
  if (pDataObj->QueryGetData(&formatEtc) == S_OK) {
    // Set the drop effect to "copy"
    window_dwEffect_ = DROPEFFECT_COPY;
    drag_type = kDragTextType;
  }

  // Check the data object for dropped files
  formatEtc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  if (pDataObj->QueryGetData(&formatEtc) == S_OK) {
    // Set the drop effect to drop file path
    window_dwEffect_ = DROPEFFECT_LINK;
    drag_type = kDragFileType;
  }

  // Return the drop effect
  *pdwEffect = window_dwEffect_;

  POINT point = {pt.x, pt.y};
  ScreenToClient(window_handle_, &point);

  if (engine_) {
    engine_->PerformDragEnterAndOver(FloatPoint(point.x, point.y));
  }
  return 0;
}

HRESULT STDMETHODCALLTYPE WindowMouseDropHandle::DragOver(DWORD grfKeyState,
                                                          POINTL pt,
                                                          DWORD* pdwEffect) {
  // Return the drop effect
  *pdwEffect = window_dwEffect_;
  std::string drag_type;
  if (window_dwEffect_ == DROPEFFECT_COPY) {
    drag_type = kDragTextType;
  } else if (window_dwEffect_ == DROPEFFECT_LINK) {
    drag_type = kDragFileType;
  }
  POINT point = {pt.x, pt.y};
  ScreenToClient(window_handle_, &point);

  if (engine_) {
    engine_->PerformDragEnterAndOver(FloatPoint(point.x, point.y));
  }
  return 0;
}

HRESULT WindowMouseDropHandle::DragLeave() {
  if (engine_) {
    engine_->PerformDragLeave();
  }
  return 0;
}

HRESULT STDMETHODCALLTYPE WindowMouseDropHandle::Drop(IDataObject* pDataObj,
                                                      DWORD grfKeyState,
                                                      POINTL pt,
                                                      DWORD* pdwEffect) {
  std::string drag_type;
  // construct a FORMATETC object
  FORMATETC fmtetc;
  STGMEDIUM stgmed = {TYMED_HGLOBAL};
  if (window_dwEffect_ == DROPEFFECT_COPY) {
    drag_type = kDragTextType;
    fmtetc = {CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  } else if (window_dwEffect_ == DROPEFFECT_LINK) {
    drag_type = kDragFileType;
    fmtetc = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  } else {
    return 0;
  }

  POINT point = {pt.x, pt.y};
  ScreenToClient(window_handle_, &point);
  bool get_data_success = false;

  std::string drag_content_text = "";
  std::list<std::unordered_map<std::string, std::string>> drag_content_files;
  // See if the dataobject contains any TEXT stored as a HGLOBAL
  if (pDataObj->QueryGetData(&fmtetc) == S_OK) {
    // Yippie! the data is there, so go get it!
    if (pDataObj->GetData(&fmtetc, &stgmed) == S_OK) {
      ScopedGlobalLock locked_data(stgmed.hGlobal);
      auto data = locked_data.get();
      if (data != nullptr) {
        get_data_success = true;
        if (window_dwEffect_ == DROPEFFECT_COPY) {
          LPWSTR pszText = (LPWSTR)data;
          std::wstring wide(pszText);
          std::string drop_text = fml::WideStringToUtf8(wide);
          FML_DLOG(WARNING)
              << "WindowMouseDropHandle::Drop text: " << drop_text;
          drag_content_text = drop_text;
        } else if (window_dwEffect_ == DROPEFFECT_LINK) {
          std::unordered_map<std::string, std::string> file_args;
          auto files = DragQueryFile(reinterpret_cast<HDROP>(data), 0xFFFFFFFF,
                                     nullptr, 0);
          UINT file_name_length = 0;
          TCHAR* file_name = nullptr;
          for (unsigned int i = 0; i < files; ++i) {
            // Get length of file names.
            file_name_length =
                DragQueryFile(reinterpret_cast<HDROP>(data), i, nullptr, 0);
            if (file_name_length <= 0) {
              FML_DLOG(WARNING)
                  << "WindowMouseDropHandle::Drop file name length <=0";
              continue;
            }
            file_name = new TCHAR[file_name_length + 1];
            if (!file_name) {
              FML_DLOG(WARNING)
                  << "WindowMouseDropHandle::Drop file name init fail";
              continue;
            }
            DragQueryFile(reinterpret_cast<HDROP>(data), i, file_name,
                          file_name_length + 1);
            SHFILEINFO fileInfo = {0};
            if (SHGetFileInfo(
                    file_name, 0, &fileInfo, sizeof(fileInfo),
                    SHGFI_DISPLAYNAME | SHGFI_TYPENAME | SHGFI_ATTRIBUTES)) {
              int64_t file_size = 0;
              std::wstring wide(file_name);
              std::string path = fml::WideStringToUtf8(wide);
              FML_DLOG(WARNING) << "WindowMouseDropHandle::Drop file: " << path;
              std::wstring wide_name(fileInfo.szDisplayName);
              std::string file_display_name = fml::WideStringToUtf8(wide_name);
              std::wstring wide_type(fileInfo.szTypeName);
              std::string file_type = fml::WideStringToUtf8(wide_type);
              std::string file_last_modify_time;

              WIN32_FIND_DATAW findData;
              HANDLE hFind = ::FindFirstFile(file_name, &findData);
              if (hFind != INVALID_HANDLE_VALUE) {
                ULARGE_INTEGER fileSize;
                fileSize.LowPart = findData.nFileSizeLow;
                fileSize.HighPart = findData.nFileSizeHigh;
                file_size = fileSize.QuadPart;

                FILETIME ftLastWrite = findData.ftLastWriteTime;
                FILETIME ftLocal;
                if (FileTimeToLocalFileTime(&ftLastWrite, &ftLocal)) {
                  SYSTEMTIME st;
                  if (FileTimeToSystemTime(&ftLocal, &st)) {
                    TCHAR time_str[20];
                    wsprintf(time_str, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
                             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
                             st.wSecond);
                    std::wstring wide_date(time_str);
                    file_last_modify_time = fml::WideStringToUtf8(wide_date);
                  }
                }
              }
              ::FindClose(hFind);

              file_args[kDragDropPathKey] = path;
              file_args[kDragDropNameKey] = file_display_name;
              file_args[kDragDropTypeKey] = file_type;
              file_args[kDragDropSizeKey] = std::to_string(file_size);
              file_args[kDragDropLastModifiedKey] = file_last_modify_time;
              drag_content_files.push_back(file_args);
            }
            delete[] file_name;
            file_name = nullptr;
          }
        }
      }
      // release the data using the COM API
      ReleaseStgMedium(&stgmed);
    }
  }
  if (get_data_success) {
    if (engine_) {
      engine_->PerformDragDrop(FloatPoint(point.x, point.y), drag_type,
                               drag_content_text, drag_content_files);
    }
  }

  return 0;
}

HRESULT STDMETHODCALLTYPE
WindowMouseDropHandle::QueryInterface(const IID& riid, void** ppvObject) {
  if (riid == IID_IDropTarget) {
    *ppvObject = this;  // or static_cast<IUnknown*> if preferred
    // AddRef() if doing things properly
    // but then you should probably handle IID_IUnknown as well;
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE WindowMouseDropHandle::AddRef() { return 1; }
ULONG STDMETHODCALLTYPE WindowMouseDropHandle::Release() { return 0; }

}  // namespace clay
