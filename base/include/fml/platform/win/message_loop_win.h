// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_PLATFORM_WIN_MESSAGE_LOOP_WIN_H_
#define BASE_INCLUDE_FML_PLATFORM_WIN_MESSAGE_LOOP_WIN_H_

#include <windows.h>

#include <atomic>

#include "base/include/fml/macros.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/unique_object.h"

namespace lynx {
namespace fml {

class MessageLoopWin : public MessageLoopImpl {
 private:
  struct UniqueHandleTraits {
    static HANDLE InvalidValue() { return NULL; }
    static bool IsValid(HANDLE value) { return value != NULL; }
    static void Free(HANDLE value) { CloseHandle(value); }
  };

  bool running_;
  fml::UniqueObject<HANDLE, UniqueHandleTraits> timer_;
  uint32_t timer_resolution_;

  MessageLoopWin();

  ~MessageLoopWin() override;

  void Run() override;

  void Terminate() override;

  void WakeUp(fml::TimePoint time_point) override;

  FML_FRIEND_MAKE_REF_COUNTED(MessageLoopWin);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MessageLoopWin);
  BASE_DISALLOW_COPY_AND_ASSIGN(MessageLoopWin);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::MessageLoopWin;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_PLATFORM_WIN_MESSAGE_LOOP_WIN_H_
