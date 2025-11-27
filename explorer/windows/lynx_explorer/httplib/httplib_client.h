// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef EXPLORER_WINDOWS_LYNX_EXPLORER_HTTPLIB_HTTPLIB_CLIENT_H_
#define EXPLORER_WINDOWS_LYNX_EXPLORER_HTTPLIB_HTTPLIB_CLIENT_H_

#include <string>

namespace lynx {
namespace example {
class HttplibClient {
 public:
  HttplibClient() = delete;
  ~HttplibClient() = delete;

  static std::string Get(const std::string& url);
};
}  // namespace example
}  // namespace lynx

#endif  // EXPLORER_WINDOWS_LYNX_EXPLORER_HTTPLIB_HTTPLIB_CLIENT_H_
