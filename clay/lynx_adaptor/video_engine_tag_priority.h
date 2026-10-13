// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_VIDEO_ENGINE_TAG_PRIORITY_H_
#define CLAY_LYNX_ADAPTOR_VIDEO_ENGINE_TAG_PRIORITY_H_

#include <string>

namespace lynx {

inline constexpr char kClayPlatformVideoEngineTag[] = "x-video-engine";
inline constexpr char kClayBuiltinVideoEngineTag[] = "clay-video-engine";

// Pure, platform-independent priority for resolving the video-engine element
// tag, extracted so the routing can be unit tested without a live PropBundle or
// a platform-specific build. Priority:
//   1. Any tag other than x-video-engine is returned unchanged.
//   2. When Clay C++ video elements are NOT enabled (e.g. Android, which reuses
//      the XElement platform video), x-video-engine stays x-video-engine.
//   3. When Clay C++ video elements ARE enabled (iOS), the video-engine element
//      defaults to Clay's built-in clay-video-engine view and only stays on the
//      platform x-video-engine view when the element explicitly opts in via
//      use-platform-video. iOS therefore uses the Clay built-in video by
//      default.
inline std::string ResolveVideoEngineNodeTag(const std::string& tag,
                                             bool clay_cpp_video_enabled,
                                             bool use_platform_video) {
  if (tag != kClayPlatformVideoEngineTag || !clay_cpp_video_enabled) {
    return tag;
  }
  return use_platform_video ? std::string(kClayPlatformVideoEngineTag)
                            : std::string(kClayBuiltinVideoEngineTag);
}

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_VIDEO_ENGINE_TAG_PRIORITY_H_
