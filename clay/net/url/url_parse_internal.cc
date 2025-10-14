// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/url/url_parse_internal.h"

#include <ctype.h>

#include "clay/net/url/url_parse.h"

namespace clay {
namespace url {

namespace {

static const char* kStandardSchemes[] = {
    kHttpsScheme, kHttpScheme, kFileScheme, kFtpScheme, kWssScheme, kWsScheme,
};

}  // namespace

bool IsURLSlash(char ch) { return ch == '/' || ch == '\\'; }

bool ShouldTrimFromURL(char ch) { return ch <= ' '; }

void TrimURL(const char* spec, int* begin, int* len, bool trim_path_end) {
  // Strip leading whitespace and control characters.
  while (*begin < *len && ShouldTrimFromURL(spec[*begin])) {
    (*begin)++;
  }

  if (trim_path_end) {
    // Strip trailing whitespace and control characters. We need the >i test
    // for when the input string is all blanks; we don't want to back past the
    // input.
    while (*len > *begin && ShouldTrimFromURL(spec[*len - 1])) {
      (*len)--;
    }
  }
}

int CountConsecutiveSlashes(const char* str, int begin_offset, int str_len) {
  int count = 0;
  while ((begin_offset + count) < str_len &&
         IsURLSlash(str[begin_offset + count])) {
    ++count;
  }
  return count;
}

bool CompareSchemeComponent(const char* spec, const Component& component,
                            const char* compare_to) {
  if (!component.is_nonempty()) {
    return compare_to[0] == 0;  // When component is empty, match empty scheme.
  }
  for (int i = 0; i < component.len; ++i) {
    if (tolower(spec[i]) != compare_to[i]) {
      return false;
    }
  }
  return true;
}

bool IsStandard(const char* spec, const Component& component) {
  if (!component.is_nonempty()) {
    return false;
  }

  for (auto& kStandardScheme : kStandardSchemes) {
    if (CompareSchemeComponent(spec, component, kStandardScheme)) {
      return true;
    }
  }
  return false;
}

// NOTE: Not implemented because file URLs are currently unsupported.
// void ParseFileURL(const char* url, int url_len, Parsed* parsed) {}
}  // namespace url
}  // namespace clay
