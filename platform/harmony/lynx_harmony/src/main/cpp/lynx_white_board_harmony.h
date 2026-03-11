// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_WHITE_BOARD_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_WHITE_BOARD_HARMONY_H_

#include <js_native_api.h>

#include <memory>

#include "core/shared_data/lynx_white_board.h"

namespace lynx {
namespace harmony {

class LynxWhiteBoard {
 public:
  static void Init(napi_env env, napi_value exports);
  static napi_value New(napi_env env, napi_callback_info info);

  LynxWhiteBoard();
  ~LynxWhiteBoard() = default;

  const std::shared_ptr<tasm::WhiteBoard>& GetWhiteBoard() const {
    return white_board_;
  }

 private:
  std::shared_ptr<tasm::WhiteBoard> white_board_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_WHITE_BOARD_HARMONY_H_
