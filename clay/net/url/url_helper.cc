// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/url/url_helper.h"

#include "base/include/string/string_utils.h"
#include "clay/fml/file.h"

namespace clay {

namespace url {

namespace {

// Helper function to find port colon position
size_t FindPortColon(std::string_view url, size_t authority_start,
                     size_t authority_end) {
  if (authority_start >= url.size()) {
    return std::string::npos;
  }

  if (url[authority_start] == '[') {
    // IPv6 host: look for "]:port".
    size_t ipv6_end = url.find(']', authority_start + 1);
    if (ipv6_end != std::string::npos && ipv6_end + 1 < authority_end &&
        url[ipv6_end + 1] == ':') {
      return ipv6_end + 1;
    }
  } else {
    // Regular host: look for ":" before the path or query.
    size_t port_colon = url.find(':', authority_start);
    if (port_colon != std::string::npos && port_colon < authority_end) {
      return port_colon;
    }
  }

  return std::string::npos;
}

}  // namespace

UriSchemeType ParseUriScheme(std::string_view uri) {
  Component scheme;

  if (!ExtractScheme(uri.data(), uri.size(), &scheme)) {
    if (fml::IsFile(uri.data())) {
      return UriSchemeType::kLocalFile;
    }
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
#elif OS_WIN
  } else if (scheme_str.size() == 1 && std::isalpha(scheme_str[0])) {
    return UriSchemeType::kLocalFile;
  } else {
#else
  } else {
#endif
    return UriSchemeType::kInvalid;
  }
}

// Trim URL by removing unnecessary parts that don't affect loading.
std::string TrimUrl(std::string_view url) {
  if (url.empty()) {
    return "";
  }

  // Step 0: Remove leading and trailing whitespace using StringUtils
  std::string_view trimmed_url = lynx::base::TrimToStringView(url);
  if (trimmed_url.empty()) {
    return "";
  }

  // Step 1: Fast scheme check - early return for non-http(s) URLs
  std::string scheme_prefix = lynx::base::StringToLowerASCII(
      trimmed_url.substr(0, std::min<size_t>(trimmed_url.size(), 8)));
  bool is_http = lynx::base::BeginsWith(scheme_prefix, "http://");
  bool is_https = lynx::base::BeginsWith(scheme_prefix, "https://");

  if (!is_http && !is_https) {
    return std::string(trimmed_url);
  }

  // Step 2: Convert scheme to lowercase for http(s) URLs
  std::string normalized_url;
  size_t scheme_end = trimmed_url.find("://");
  if (scheme_end != std::string::npos) {
    std::string scheme = is_http ? "http" : "https";
    // Combine lowercase scheme with rest of URL
    normalized_url =
        scheme + "://" + std::string(trimmed_url.substr(scheme_end + 3));
    trimmed_url = normalized_url;
  } else {
    // No scheme separator found, return as-is
    return std::string(trimmed_url);
  }

  // Step 3: Remove URL fragment (#fragment), which doesn't affect loading.
  size_t fragment_pos = trimmed_url.find('#');
  if (fragment_pos != std::string::npos) {
    trimmed_url = trimmed_url.substr(0, fragment_pos);
  }

  // Step 4: Remove default ports (80 for http, 443 for https).
  size_t authority_start = is_http ? 7 : 8;
  size_t authority_end = trimmed_url.find_first_of("/?", authority_start);
  if (authority_end == std::string::npos) {
    authority_end = trimmed_url.size();
  }

  size_t port_colon =
      FindPortColon(trimmed_url, authority_start, authority_end);
  if (port_colon != std::string::npos) {
    std::string_view port_view =
        trimmed_url.substr(port_colon + 1, authority_end - port_colon - 1);
    std::string port(port_view);
    bool is_default_port =
        (is_http && port == "80") || (is_https && port == "443");
    if (is_default_port) {
      // Create result by combining parts without default port
      std::string result;
      result.reserve(trimmed_url.size() - (authority_end - port_colon));
      result.append(trimmed_url.substr(0, port_colon));
      result.append(trimmed_url.substr(authority_end));
      return result;
    }
  }

  // Return the processed URL as std::string
  return std::string(trimmed_url);
}

}  // namespace url
}  // namespace clay
