// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/url/url_helper.h"

namespace clay {

namespace url {

namespace {

// Bitmask for whitespace characters (space, tab, newline, carriage return, form
// feed, vertical tab)
static constexpr uint64_t kWhitespaceMask[] = {
    0x0000000000000000,  // 0x00-0x07
    0x0000000000000000,  // 0x08-0x0F
    0x0000000000000000,  // 0x10-0x17
    0x0000000000000000,  // 0x18-0x1F
    0x0000000000000001,  // 0x20 (space)
    0x0000000000000000,  // 0x21-0x27
    0x0000000000000000,  // 0x28-0x2F
    0x0000000000000000,  // 0x30-0x37
    0x0000000000000000,  // 0x38-0x3F
    0x0000000000000000,  // 0x40-0x47
    0x0000000000000000,  // 0x48-0x4F
    0x0000000000000000,  // 0x50-0x57
    0x0000000000000000,  // 0x58-0x5F
    0x0000000000000000,  // 0x60-0x67
    0x0000000000000000,  // 0x68-0x6F
    0x0000000000000000,  // 0x70-0x77
    0x0000000000000000,  // 0x78-0x7F
    0x0000000000000000,  // 0x80-0x87
    0x0000000000000000,  // 0x88-0x8F
    0x0000000000000000,  // 0x90-0x97
    0x0000000000000000,  // 0x98-0x9F
    0x0000000000000000,  // 0xA0-0xA7
    0x0000000000000000,  // 0xA8-0xAF
    0x0000000000000000,  // 0xB0-0xB7
    0x0000000000000000,  // 0xB8-0xBF
    0x0000000000000000,  // 0xC0-0xC7
    0x0000000000000000,  // 0xC8-0xCF
    0x0000000000000000,  // 0xD0-0xD7
    0x0000000000000000,  // 0xD8-0xDF
    0x0000000000000000,  // 0xE0-0xE7
    0x0000000000000000,  // 0xE8-0xEF
    0x0000000000000000,  // 0xF0-0xF7
    0x0000000000000000,  // 0xF8-0xFF
};

// Special case for control characters (0x00-0x1F)
static constexpr bool IsControlWhitespace(unsigned char c) {
  return c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

// Check if a character is whitespace using bitmask for faster lookup
bool IsWhitespace(unsigned char c) {
  if (c < 32) {
    return IsControlWhitespace(c);
  }
  size_t index = c / 8;
  size_t bit = c % 8;
  return (kWhitespaceMask[index] >> bit) & 1;
}

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

// Helper function to check if string starts with a prefix
bool StartsWith(std::string_view str, std::string_view prefix) {
  return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

}  // namespace

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

// Trim URL by removing unnecessary parts that don't affect loading.
std::string TrimUrl(std::string_view url) {
  if (url.empty()) {
    return "";
  }

  // Step 0: Remove leading and trailing whitespace using bitmask for faster
  // lookup
  size_t first_non_space = 0;
  while (first_non_space < url.size() &&
         IsWhitespace(static_cast<unsigned char>(url[first_non_space]))) {
    ++first_non_space;
  }
  if (first_non_space == url.size()) {
    // The entire string is whitespace
    return "";
  }
  size_t last_non_space = url.size() - 1;
  while (last_non_space > first_non_space &&
         IsWhitespace(static_cast<unsigned char>(url[last_non_space]))) {
    --last_non_space;
  }

  // Create string view of trimmed content
  std::string_view trimmed_url =
      url.substr(first_non_space, last_non_space - first_non_space + 1);

  // Step 1: Fast scheme check - early return for non-http(s) URLs
  if (trimmed_url.size() < 7 || (!StartsWith(trimmed_url, "http://") &&
                                 !StartsWith(trimmed_url, "https://"))) {
    return std::string(trimmed_url);
  }

  // Step 2: Remove URL fragment (#fragment), which doesn't affect loading.
  size_t fragment_pos = trimmed_url.find('#');
  if (fragment_pos != std::string::npos) {
    trimmed_url = trimmed_url.substr(0, fragment_pos);
  }

  // Step 3: Remove default ports (80 for http, 443 for https).
  bool is_http = StartsWith(trimmed_url, "http://");
  bool is_https = StartsWith(trimmed_url, "https://");

  if (is_http || is_https) {
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
  }

  // Return the processed URL as std::string
  return std::string(trimmed_url);
}

}  // namespace url
}  // namespace clay
