// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_H_
#define CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/net/resource_type.h"
#include "clay/net/url/uri.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

// This class send a http(s) request.
class HttpResourceFetcher {
 public:
  HttpResourceFetcher(const url::Uri& uri, size_t request_seq, int retry_time);
  virtual ~HttpResourceFetcher();

  static const char* GetPlatformCAFile();
  static const char* GetPlatformCAPath();
  static constexpr int kMaxRetryTime = 3;

  virtual void Load() = 0;
  virtual RawResource LoadSync() = 0;
  virtual void Cancel() { failed_time_ += kMaxRetryTime; }
  virtual bool IsHostLoader() { return false; }

  int FailedTime() const { return failed_time_; }
  void SetHeaders(std::map<std::string, std::string>&& headers);
  void SetMethod(const std::string& method);
  void SetBody(std::string&& body);

 protected:
  // seconds
  static constexpr int kDefaultConnectionTimeout = 15;
  static constexpr int kDefaultIOTimeout = 5;

  url::Uri uri_;
  size_t request_seq_;
  std::map<std::string, std::string> headers_;
  std::string method_;
  std::string body_;

 private:
  FRIEND_TEST(HttpResourceFetcherTest, Load);

  int failed_time_;
};

}  // namespace clay

#endif  // CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_H_
