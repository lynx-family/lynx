// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_LYNX_GET_UI_RESULT_H_
#define CORE_RENDERER_DOM_LYNX_GET_UI_RESULT_H_

#include <string>
#include <utility>
#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {
class LynxGetUIResult {
 public:
  LynxGetUIResult() : err_code_(UNKNOWN) {}

  LynxGetUIResult(std::vector<int32_t> &&nodes, int32_t err_code,
                  const std::string &selector)
      : ui_impl_ids(std::move(nodes)), err_code_(err_code) {
    err_msg_ = FormatErrMsg(selector);
  }

  LynxGetUIResult(std::vector<int32_t> &&nodes, int32_t err_code,
                  const std::string &selector, const std::string &err_msg)
      : ui_impl_ids(std::move(nodes)), err_code_(err_code), err_msg_(err_msg) {}

  static LynxGetUIResult UnknownError(const std::string &err_msg) {
    LynxGetUIResult result;
    result.err_msg_ = err_msg;
    return result;
  }

  lepus::Value StatusAsLepusValue() const {
    auto result_dict = lepus::Dictionary::Create();
    BASE_STATIC_STRING_DECL(kCode, "code");
    BASE_STATIC_STRING_DECL(kData, "data");
    result_dict->SetValue(kCode, ErrCode());
    result_dict->SetValue(kData, ErrMsg());
    return lepus::Value(std::move(result_dict));
  }

  bool Success() const { return err_code_ == SUCCESS; }

  const std::vector<int32_t> &UiImplIds() const { return ui_impl_ids; }
  int32_t ErrCode() const { return err_code_; }
  const std::string &ErrMsg() const { return err_msg_; }

  // also in LynxUIMethodConstants.java
  static constexpr int32_t SUCCESS = 0;
  static constexpr int32_t UNKNOWN = 1;
  static constexpr int32_t NODE_NOT_FOUND = 2;
  static constexpr int32_t METHOD_NOT_FOUND = 3;
  static constexpr int32_t PARAM_INVALID = 4;
  static constexpr int32_t SELECTOR_NOT_SUPPORTED = 5;
  static constexpr int32_t NO_UI_FOR_NODE = 6;
  static constexpr int32_t INVALID_STATE_ERROR = 7;
  static constexpr int32_t OPERATION_ERROR = 8;

 private:
  std::vector<int32_t> ui_impl_ids;
  int32_t err_code_;
  std::string err_msg_;

  std::string FormatErrMsg(const std::string &selector) const {
    if (Success()) {
      return "success";
    } else {
      switch (ErrCode()) {
        case NODE_NOT_FOUND:
          return "no node found for selector '" + selector + "'";
        case NO_UI_FOR_NODE:
          return "node '" + selector + "' does not have a LynxUI";
        case SELECTOR_NOT_SUPPORTED:
          return "selector '" + selector +
                 "' not supported. currently ID(#id), Class(.class), Child(#a "
                 "> #b), Descendant(#a #b), and Descendant(across component "
                 "form, #a >>> #b) selectors are supported.";
        default:
          return "unknown error";
      }
    }
  }
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_LYNX_GET_UI_RESULT_H_
