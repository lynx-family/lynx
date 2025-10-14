// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_FLOW_FLOW_TEST_UTILS_H_
#define CLAY_FLOW_FLOW_TEST_UTILS_H_

#include <string>

namespace clay {

const std::string& GetGoldenDir();

void SetGoldenDir(const std::string& dir);

const std::string& GetFontFile();

void SetFontFile(const std::string& dir);

}  // namespace clay

#endif  // CLAY_FLOW_FLOW_TEST_UTILS_H_
