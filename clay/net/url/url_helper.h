// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_URL_URL_HELPER_H_
#define CLAY_NET_URL_URL_HELPER_H_

#include <algorithm>
#include <string>
#include <string_view>

#include "build/build_config.h"
#include "clay/net/url/url_parse.h"
#include "clay/net/url/url_parse_internal.h"

namespace clay {
namespace url {

static constexpr char kDataScheme[] = "data";

#if OS_ANDROID
static constexpr char kContentProviderScheme[] = "content";
static constexpr char kAssetScheme[] = "asset";
static constexpr char kResScheme[] = "res";

enum class UriSchemeType {
  kNet = 0,
  kLocalFile,
  kData,
  kContentProvider,
  kAsset,
  kRes,
  kInvalid,
};
#else   // OS_ANDROID
enum class UriSchemeType {
  kNet = 0,
  kLocalFile,
  kData,
  kInvalid,
};
#endif  // OS_ANDROID

UriSchemeType ParseUriScheme(std::string_view uri);

}  // namespace url
}  // namespace clay

#endif  // CLAY_NET_URL_URL_HELPER_H_
