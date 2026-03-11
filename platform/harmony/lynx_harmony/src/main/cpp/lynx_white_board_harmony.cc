// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_white_board_harmony.h"

#include "base/include/platform/harmony/napi_util.h"
#include "core/base/harmony/napi_convert_helper.h"

namespace lynx {
namespace harmony {

LynxWhiteBoard::LynxWhiteBoard()
    : white_board_(std::make_shared<tasm::WhiteBoard>()) {}

void LynxWhiteBoard::Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {};

  napi_value cons;
  napi_define_class(env, "LynxWhiteBoard", NAPI_AUTO_LENGTH, New, nullptr, 0,
                    properties, &cons);
  napi_set_named_property(env, exports, "LynxWhiteBoard", cons);
}

napi_value LynxWhiteBoard::New(napi_env env, napi_callback_info info) {
  napi_value js_this;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

  LynxWhiteBoard* white_board = new LynxWhiteBoard();
  napi_wrap(
      env, js_this, white_board,
      [](napi_env env, void* data, void* hint) {
        delete static_cast<LynxWhiteBoard*>(data);
      },
      nullptr, nullptr);

  return js_this;
}

}  // namespace harmony
}  // namespace lynx
