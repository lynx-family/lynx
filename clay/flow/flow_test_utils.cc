// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "base/include/no_destructor.h"

namespace clay {

namespace {

std::string& GoldenDir() {
  static lynx::base::NoDestructor<std::string> golden_dir;
  return *golden_dir;
}

std::string& FontFile() {
  static lynx::base::NoDestructor<std::string> font_file;
  return *font_file;
}

}  // namespace

const std::string& GetGoldenDir() { return GoldenDir(); }

void SetGoldenDir(const std::string& dir) { GoldenDir() = dir; }

const std::string& GetFontFile() { return FontFile(); }

void SetFontFile(const std::string& file) { FontFile() = file; }

}  // namespace clay
