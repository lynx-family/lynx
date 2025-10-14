// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_LYNX_MODULE_LYNX_UI_METHOD_TYPES_H_
#define CLAY_UI_LYNX_MODULE_LYNX_UI_METHOD_TYPES_H_

namespace clay {

// should sync with LynxUIMethodConstants.java in Lynx.
enum class LynxUIMethodResult {
  kSuccess = 0,
  kUnknown = 1,
  kNodeNotFound = 2,
  kMethodNotFound = 3,
  kParamInvalid = 4,
  kSelectorNotSupported = 5,
  kNoUiForNode = 6,
  kInvalidStateError = 7,
  kOperationError = 8
};

}  // namespace clay

#endif  // CLAY_UI_LYNX_MODULE_LYNX_UI_METHOD_TYPES_H_
