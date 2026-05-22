// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_HEX_CODEC_H_
#define CLAY_FML_HEX_CODEC_H_

#include <string>
#include <string_view>

namespace fml {

std::string HexEncode(std::string_view input);

}  // namespace fml

#endif  // CLAY_FML_HEX_CODEC_H_
