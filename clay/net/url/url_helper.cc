// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/url/url_helper.h"

namespace clay {

namespace url {

UriSchemeType ParseUriScheme(std::string_view uri) {
  Component scheme;

  if (!ExtractScheme(uri.data(), uri.size(), &scheme)) {
    return UriSchemeType::kInvalid;
  }
  std::string scheme_str(uri.data() + scheme.begin, scheme.len);

  std::transform(scheme_str.begin(), scheme_str.end(), scheme_str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (scheme_str.compare(kHttpsScheme) == 0) {
    return UriSchemeType::kNet;
  } else if (scheme_str.compare(kHttpScheme) == 0) {
    return UriSchemeType::kNet;
  } else if (scheme_str.compare(kFtpScheme) == 0) {
    return UriSchemeType::kNet;
  } else if (scheme_str.compare(kDataScheme) == 0) {
    return UriSchemeType::kData;
  } else if (scheme_str.compare(kFileScheme) == 0) {
    return UriSchemeType::kLocalFile;
#if OS_ANDROID
  } else if (scheme_str.compare(kContentProviderScheme) == 0) {
    return UriSchemeType::kContentProvider;
  } else if (scheme_str.compare(kAssetScheme) == 0) {
    return UriSchemeType::kAsset;
  } else if (scheme_str.compare(kResScheme) == 0) {
    return UriSchemeType::kRes;
  } else {
#else
  } else {
#endif
    return UriSchemeType::kInvalid;
  }
}

}  // namespace url
}  // namespace clay
