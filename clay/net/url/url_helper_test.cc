// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/url/url_helper.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace url {

class UrlHelperTest : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

// Test case for empty URL
TEST_F(UrlHelperTest, EmptyUrl) { EXPECT_EQ(TrimUrl(""), ""); }

// Test case for URL with only whitespace
TEST_F(UrlHelperTest, WhitespaceOnlyUrl) {
  EXPECT_EQ(TrimUrl("   "), "");
  EXPECT_EQ(TrimUrl("\t\n\r"), "");
  EXPECT_EQ(TrimUrl(" \t \n "), "");
}

// Test case for URL with leading and trailing whitespace
TEST_F(UrlHelperTest, UrlWithWhitespace) {
  EXPECT_EQ(TrimUrl("  http://example.com  "), "http://example.com");
  EXPECT_EQ(TrimUrl("\thttps://example.com\n"), "https://example.com");
  EXPECT_EQ(TrimUrl(" \t http://example.com/path \n "),
            "http://example.com/path");
}

// Test case for HTTP URL without port
TEST_F(UrlHelperTest, HttpUrlWithoutPort) {
  EXPECT_EQ(TrimUrl("http://example.com"), "http://example.com");
  EXPECT_EQ(TrimUrl("http://example.com/path"), "http://example.com/path");
  EXPECT_EQ(TrimUrl("http://example.com/path?query=value"),
            "http://example.com/path?query=value");
}

// Test case for HTTP URL with default port (80)
TEST_F(UrlHelperTest, HttpUrlWithDefaultPort) {
  EXPECT_EQ(TrimUrl("http://example.com:80"), "http://example.com");
  EXPECT_EQ(TrimUrl("http://example.com:80/path"), "http://example.com/path");
  EXPECT_EQ(TrimUrl("http://example.com:80/path?query=value"),
            "http://example.com/path?query=value");
}

// Test case for HTTP URL with non-default port
TEST_F(UrlHelperTest, HttpUrlWithNonDefaultPort) {
  EXPECT_EQ(TrimUrl("http://example.com:8080"), "http://example.com:8080");
  EXPECT_EQ(TrimUrl("http://example.com:3000/path"),
            "http://example.com:3000/path");
}

// Test case for HTTPS URL without port
TEST_F(UrlHelperTest, HttpsUrlWithoutPort) {
  EXPECT_EQ(TrimUrl("https://example.com"), "https://example.com");
  EXPECT_EQ(TrimUrl("https://example.com/path"), "https://example.com/path");
  EXPECT_EQ(TrimUrl("https://example.com/path?query=value"),
            "https://example.com/path?query=value");
}

// Test case for HTTPS URL with default port (443)
TEST_F(UrlHelperTest, HttpsUrlWithDefaultPort) {
  EXPECT_EQ(TrimUrl("https://example.com:443"), "https://example.com");
  EXPECT_EQ(TrimUrl("https://example.com:443/path"),
            "https://example.com/path");
  EXPECT_EQ(TrimUrl("https://example.com:443/path?query=value"),
            "https://example.com/path?query=value");
}

// Test case for HTTPS URL with non-default port
TEST_F(UrlHelperTest, HttpsUrlWithNonDefaultPort) {
  EXPECT_EQ(TrimUrl("https://example.com:8443"), "https://example.com:8443");
  EXPECT_EQ(TrimUrl("https://example.com:9000/path"),
            "https://example.com:9000/path");
}

// Test case for URL with fragment
TEST_F(UrlHelperTest, UrlWithFragment) {
  EXPECT_EQ(TrimUrl("http://example.com#fragment"), "http://example.com");
  EXPECT_EQ(TrimUrl("http://example.com/path#fragment"),
            "http://example.com/path");
  EXPECT_EQ(TrimUrl("http://example.com/path?query=value#fragment"),
            "http://example.com/path?query=value");
  EXPECT_EQ(TrimUrl("https://example.com:8443/path#fragment"),
            "https://example.com:8443/path");
}

// Test case for URL with IPv4 address
TEST_F(UrlHelperTest, UrlWithIPv4Address) {
  EXPECT_EQ(TrimUrl("http://192.168.1.1"), "http://192.168.1.1");
  EXPECT_EQ(TrimUrl("http://192.168.1.1:8080/path"),
            "http://192.168.1.1:8080/path");
  EXPECT_EQ(TrimUrl("https://127.0.0.1:443"), "https://127.0.0.1");
}

// Test case for URL with IPv6 address
TEST_F(UrlHelperTest, UrlWithIPv6Address) {
  EXPECT_EQ(TrimUrl("http://[2001:db8::1]"), "http://[2001:db8::1]");
  EXPECT_EQ(TrimUrl("http://[2001:db8::1]:8080/path"),
            "http://[2001:db8::1]:8080/path");
  EXPECT_EQ(TrimUrl("https://[2001:db8::1]:443"), "https://[2001:db8::1]");
}

// Test case for non-HTTP/HTTPS URL
TEST_F(UrlHelperTest, NonHttpUrl) {
  // These should remain unchanged
  EXPECT_EQ(TrimUrl("file:///path/to/file"), "file:///path/to/file");
  EXPECT_EQ(TrimUrl("data:image/"
                    "png;base64,"
                    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42m"
                    "NkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg=="),
            "data:image/"
            "png;base64,"
            "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDw"
            "AChwGA60e6kgAAAABJRU5ErkJggg==");
  EXPECT_EQ(TrimUrl("ftp://ftp.example.com"), "ftp://ftp.example.com");
}

// Test case for complex URL with multiple components
TEST_F(UrlHelperTest, ComplexUrl) {
  EXPECT_EQ(TrimUrl("  "
                    "https://example.com:443/path/to/"
                    "resource?query=value&other=param#fragment  "),
            "https://example.com/path/to/resource?query=value&other=param");
  EXPECT_EQ(TrimUrl("http://example.com:80/path?query=value#fragment"),
            "http://example.com/path?query=value");
}

// Test case for edge cases
TEST_F(UrlHelperTest, EdgeCases) {
  // URL with just scheme
  EXPECT_EQ(TrimUrl("http://"), "http://");
  EXPECT_EQ(TrimUrl("https://"), "https://");

  // URL with scheme and host only
  EXPECT_EQ(TrimUrl("http://example.com"), "http://example.com");

  // URL with trailing slash
  EXPECT_EQ(TrimUrl("http://example.com/"), "http://example.com/");
  EXPECT_EQ(TrimUrl("http://example.com:80/"), "http://example.com/");
}

// Test case for case-insensitive HTTP/HTTPS scheme
TEST_F(UrlHelperTest, CaseInsensitiveScheme) {
  // HTTP variations
  EXPECT_EQ(TrimUrl("HTTP://example.com"), "http://example.com");
  EXPECT_EQ(TrimUrl("HTTP://example.com:80"), "http://example.com");
  EXPECT_EQ(TrimUrl("HTTP://example.com:8080/path"),
            "http://example.com:8080/path");
  EXPECT_EQ(TrimUrl("HTTP://example.com/path#fragment"),
            "http://example.com/path");

  // HTTPS variations
  EXPECT_EQ(TrimUrl("HTTPS://example.com"), "https://example.com");
  EXPECT_EQ(TrimUrl("HTTPS://example.com:443"), "https://example.com");
  EXPECT_EQ(TrimUrl("HTTPS://example.com:8443/path"),
            "https://example.com:8443/path");
  EXPECT_EQ(TrimUrl("HTTPS://example.com/path#fragment"),
            "https://example.com/path");

  // Mixed case variations
  EXPECT_EQ(TrimUrl("Http://Example.Com"), "http://Example.Com");
  EXPECT_EQ(TrimUrl("Http://Example.Com:80"), "http://Example.Com");
  EXPECT_EQ(TrimUrl("Https://Example.Com:443/path"),
            "https://Example.Com/path");
  EXPECT_EQ(TrimUrl("hTtP://eXaMpLe.CoM:8080/path?query=value#fragment"),
            "http://eXaMpLe.CoM:8080/path?query=value");
}

}  // namespace url
}  // namespace clay
