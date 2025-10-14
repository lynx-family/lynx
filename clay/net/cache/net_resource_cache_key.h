// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_CACHE_NET_RESOURCE_CACHE_KEY_H_
#define CLAY_NET_CACHE_NET_RESOURCE_CACHE_KEY_H_

#include <string>

#include "clay/net/url/uri.h"

namespace clay {

// Contains `Uri` and generate resource_id from it, make sure call Parse before
// use Uri
class NetResourceCacheKey {
 public:
  explicit NetResourceCacheKey(const std::string& uri);

  const url::Uri& Uri() const { return uri_; }

  const std::string& ResourceId();

  bool Valid() const;

 private:
  url::Uri uri_;

  // resource_id_ will be the name of cache file, we use SHA1 hash to ensure is
  // unique.
  std::string resource_id_;
};

}  // namespace clay

#endif  // CLAY_NET_CACHE_NET_RESOURCE_CACHE_KEY_H_
