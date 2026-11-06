// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_FIXTURE_REPLAY_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_FIXTURE_REPLAY_HARMONY_H_

#include <string>

#include "base/include/expected.h"

namespace lynx {
namespace devtool {

// The caller owns an empty, private directory and removes it on failure or
// after both the replay view and its JS context release the extracted assets.
base::expected<std::string, std::string> LoadFixtureForReplay(
    const std::string& archive, const std::string& directory);

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_FIXTURE_REPLAY_HARMONY_H_
