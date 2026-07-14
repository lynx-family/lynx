// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_PLATFORM_NODE_TAG_RESOLVER_H_
#define CLAY_LYNX_ADAPTOR_PLATFORM_NODE_TAG_RESOLVER_H_

#include <string>
#include <string_view>

#include "build/build_config.h"
#include "clay/lynx_adaptor/prop_bundle_impl.h"
#include "clay/ui/common/attribute_utils.h"

namespace lynx {

inline constexpr char kUsePlatformVideoAttribute[] = "use-platform-video";

inline std::string ResolveClayPlatformNodeTag(
    const std::string& tag, const tasm::PropBundle* painting_data) {
#if defined(OS_IOS) && defined(ENABLE_CLAY_CPP_VIDEO_ELEMENTS)
  constexpr char kVideoEngineTag[] = "x-video-engine";
  constexpr char kClayVideoEngineTag[] = "clay-video-engine";

  if (tag != kVideoEngineTag) {
    return tag;
  }

  bool use_platform_video = false;
  auto* props = static_cast<const PropBundleImpl*>(painting_data);
  if (props) {
    auto iter = props->map().find(kUsePlatformVideoAttribute);
    if (iter != props->map().end()) {
      use_platform_video = clay::attribute_utils::GetBool(iter->second, false);
    }
  }
  return use_platform_video ? kVideoEngineTag : kClayVideoEngineTag;
#endif
  return tag;
}

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_PLATFORM_NODE_TAG_RESOLVER_H_
