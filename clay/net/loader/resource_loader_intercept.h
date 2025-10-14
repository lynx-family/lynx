// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_LOADER_RESOURCE_LOADER_INTERCEPT_H_
#define CLAY_NET_LOADER_RESOURCE_LOADER_INTERCEPT_H_

#include <functional>
#include <string>

using ResourceLoaderShouldInterceptUrlCallback =
    std::function<void(const char* origin_url, bool should_decode,
                       char* intercept_url, int max_path_length)>;
namespace clay {

class ResourceLoaderIntercept {
 public:
  ResourceLoaderIntercept() = default;
  virtual ~ResourceLoaderIntercept() = default;

  void BindShouldInterceptUrlCallback(
      ResourceLoaderShouldInterceptUrlCallback callback) {
    should_intercept_url_callback_ = callback;
  }

  std::string ShouldInterceptUrl(const std::string& origin_url,
                                 bool should_decode = false) {
    if (should_intercept_url_callback_) {
      std::size_t max_path_length =
          origin_url.size() > 1024 ? origin_url.size() : 1024;
      char* intercept_url = new char[max_path_length + 1];
      std::snprintf(intercept_url, max_path_length + 1, "%s",
                    origin_url.c_str());
      should_intercept_url_callback_(origin_url.c_str(), should_decode,
                                     intercept_url, max_path_length + 1);
      std::string intercept_url_str = intercept_url;
      delete[] intercept_url;
      return intercept_url_str;
    }
    return origin_url;
  }

 private:
  ResourceLoaderShouldInterceptUrlCallback should_intercept_url_callback_;
};
}  // namespace clay

#endif  // CLAY_NET_LOADER_RESOURCE_LOADER_INTERCEPT_H_
