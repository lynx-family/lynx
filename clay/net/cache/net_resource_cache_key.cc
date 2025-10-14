// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/cache/net_resource_cache_key.h"

#include <string>

#include "clay/common/sha1.h"

namespace clay {

namespace {
std::string GenerateResourceId(const std::string& uri) {
  return clay::SHA1HashString(uri);
}
}  // namespace

NetResourceCacheKey::NetResourceCacheKey(const std::string& uri) : uri_(uri) {}

bool NetResourceCacheKey::Valid() const { return uri_.Valid(); }

const std::string& NetResourceCacheKey::ResourceId() {
  if (!Valid()) {
    resource_id_ = "";
  } else if (resource_id_.empty()) {
    resource_id_ = GenerateResourceId(uri_.Spec());
  }

  return resource_id_;
}

}  // namespace clay
