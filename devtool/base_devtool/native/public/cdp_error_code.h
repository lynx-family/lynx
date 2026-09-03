// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_ERROR_CODE_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_ERROR_CODE_H_

namespace lynx {
namespace devtool {

/**
 * Error codes returned in the "error.code" field of a CDP response.
 *
 * The negative values follow the JSON-RPC 2.0 specification, which the Chrome
 * DevTools Protocol builds upon. kServerError is the implementation-defined
 * server error reserved by JSON-RPC (-32000 to -32099) that CDP uses for
 * generic backend failures.
 */
enum class CDPErrorCode : int {
  kParseError = -32700,
  kInvalidRequest = -32600,
  kMethodNotFound = -32601,
  kInvalidParams = -32602,
  kInternalError = -32603,
  kServerError = -32000,
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_ERROR_CODE_H_
