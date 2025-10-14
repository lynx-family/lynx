// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_URL_URI_H_
#define CLAY_NET_URL_URI_H_

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "clay/net/url/url_parse.h"

namespace clay {

namespace url {

class Uri {
 public:
  Uri() = default;
  explicit Uri(const std::string& uri);

  const std::string& Spec() const { return spec_; }
  std::string_view PathView() const;

  std::string_view SchemeView() const;

  std::string_view SchemeHostPortView() const;
  std::string SchemeHostPort() const;

  const std::vector<std::string_view>& QueryKeys();
  const std::vector<std::string_view>& QueryValues();

  std::string_view GetQueryParameter(const std::string& key);

  bool Valid() const { return valid_; }

 private:
  // TODO(hanhaoshen): support port and query;
  // parse scheme+host and path from uri
  void DoParse();

  void DoParseQuery();

  bool valid_ = false;
  bool query_valid_ = false;

  // The origin data of uri.
  std::string spec_;

  Component scheme_parsed_;
  Component host_parsed_;
  Component port_parsed_;
  Component path_parsed_;
  Component query_parsed_;

  std::vector<std::string_view> query_keys_;
  std::vector<std::string_view> query_values_;
};

}  // namespace url
}  // namespace clay

#endif  // CLAY_NET_URL_URI_H_
