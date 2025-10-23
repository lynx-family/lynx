// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

namespace clay {

static std::string gGoldenDir;
static std::string gFontFile;

const std::string& GetGoldenDir() { return gGoldenDir; }

void SetGoldenDir(const std::string& dir) { gGoldenDir = dir; }

const std::string& GetFontFile() { return gFontFile; }

void SetFontFile(const std::string& file) { gFontFile = file; }

}  // namespace clay
