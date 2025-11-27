// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_PLATFORM_HANDLER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_PLATFORM_HANDLER_H_

#include <Windows.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "clay/fml/logging.h"

namespace clay {

// A scoped wrapper for GlobalAlloc/GlobalFree.
class ScopedGlobalMemory {
 public:
  // Allocates |bytes| bytes of global memory with the given flags.
  ScopedGlobalMemory(unsigned int flags, size_t bytes) {
    memory_ = ::GlobalAlloc(flags, bytes);
    if (!memory_) {
      FML_LOG(ERROR) << "Unable to allocate global memory: "
                     << ::GetLastError();
    }
  }

  ~ScopedGlobalMemory() {
    if (memory_) {
      if (::GlobalFree(memory_) != nullptr) {
        FML_LOG(ERROR) << "Failed to free global allocation: "
                       << ::GetLastError();
      }
    }
  }

  // Prevent copying.
  ScopedGlobalMemory(ScopedGlobalMemory const&) = delete;
  ScopedGlobalMemory& operator=(ScopedGlobalMemory const&) = delete;

  // Returns the memory pointer, which will be nullptr if allocation failed.
  void* get() { return memory_; }

  void* release() {
    void* memory = memory_;
    memory_ = nullptr;
    return memory;
  }

 private:
  HGLOBAL memory_;
};

// A scoped wrapper for GlobalLock/GlobalUnlock.
class ScopedGlobalLock {
 public:
  // Attempts to acquire a global lock on |memory| for the life of this object.
  ScopedGlobalLock(HGLOBAL memory) {
    source_ = memory;
    if (memory) {
      locked_memory_ = ::GlobalLock(memory);
      if (!locked_memory_) {
        FML_LOG(ERROR) << "Unable to acquire global lock: " << ::GetLastError();
      }
    }
  }

  ~ScopedGlobalLock() {
    if (locked_memory_) {
      if (!::GlobalUnlock(source_)) {
        DWORD error = ::GetLastError();
        if (error != NO_ERROR) {
          FML_LOG(ERROR) << "Unable to release global lock: "
                         << ::GetLastError();
        }
      }
    }
  }

  // Prevent copying.
  ScopedGlobalLock(ScopedGlobalLock const&) = delete;
  ScopedGlobalLock& operator=(ScopedGlobalLock const&) = delete;

  // Returns the locked memory pointer, which will be nullptr if acquiring the
  // lock failed.
  void* get() { return locked_memory_; }

 private:
  HGLOBAL source_;
  void* locked_memory_;
};

class FlutterWindowsEngine;
class ScopedClipboardInterface;

class PlatformHandler {
 public:
  explicit PlatformHandler(
      FlutterWindowsEngine* engine,
      std::optional<std::function<std::unique_ptr<ScopedClipboardInterface>()>>
          scoped_clipboard_provider = std::nullopt);

  virtual ~PlatformHandler();

  // Sets the clipboard's plain text to |text|.
  void SetPlainText(const std::string& text);
  // Gets plain text from the clipboard.
  const std::string& GetPlainText() const;

 private:
  // A reference to the Flutter engine.
  FlutterWindowsEngine* engine_;

  // A scoped clipboard provider that can be passed in for mocking in tests.
  // Use this to acquire clipboard in each operation to avoid blocking clipboard
  // unnecessarily. See flutter/flutter#103205.
  std::function<std::unique_ptr<ScopedClipboardInterface>()>
      scoped_clipboard_provider_;

  mutable std::string last_clipboard_text_;
};

// A public interface for ScopedClipboard, so that it can be injected into
// PlatformHandler.
class ScopedClipboardInterface {
 public:
  virtual ~ScopedClipboardInterface(){};

  // Attempts to open the clipboard for the given window, returning the error
  // code in the case of failure and 0 otherwise.
  virtual int Open(HWND window) = 0;

  // Returns true if there is string data available to get.
  virtual bool HasString() = 0;

  // Returns string data from the clipboard.
  //
  // If getting a string fails, returns the error code.
  //
  // Open(...) must have succeeded to call this method.
  virtual std::variant<std::wstring, int> GetString() = 0;

  // Sets the string content of the clipboard, returning the error code on
  // failure and 0 otherwise.
  //
  // Open(...) must have succeeded to call this method.
  virtual int SetString(const std::wstring string) = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_PLATFORM_HANDLER_H_
