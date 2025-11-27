// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/windows/lynx_explorer/httplib/httplib_client.h"

#include "third_party/httplib/httplib.h"

namespace lynx {
namespace example {
std::string HttplibClient::Get(const std::string& url) {
  std::string::size_type pos = url.find("://");
  if (pos == std::string::npos) {
    return "";
  }
  pos = url.find('/', pos + 3);
  std::string scheme_host_port;
  std::string path_query;
  if (pos != std::string::npos) {
    scheme_host_port = url.substr(0, pos);
    path_query = url.substr(pos);
  } else {
    scheme_host_port = url;
  }
  httplib::Client client(scheme_host_port.c_str());
  httplib::Result res = client.Get(path_query.c_str());
  if (res.error() == httplib::Error::Success) {
    return res->body;
  } else {
    return "";
  }
}
}  // namespace example
}  // namespace lynx
