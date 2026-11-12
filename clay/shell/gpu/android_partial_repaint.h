// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_GPU_ANDROID_PARTIAL_REPAINT_H_
#define CLAY_SHELL_GPU_ANDROID_PARTIAL_REPAINT_H_

#include <GLES2/gl2.h>

#include <optional>
#include <string_view>

#include "base/include/string/string_utils.h"
#include "clay/common/trail_settings.h"

namespace clay {

class AndroidPartialRepaint {
 public:
  static bool IsRendererBlocked(std::string_view renderer) {
    // These drivers have rendering issues when repainting a subregion.
    constexpr std::string_view kBlockedPrefixes[] = {
        "Adreno (TM) 3",   "Mali-T",          "Mali-450 MP",
        "Adreno (TM) 506", "Adreno (TM) 510", "Adreno (TM) 530"};
    for (auto prefix : kBlockedPrefixes) {
      if (renderer.substr(0, prefix.size()) == prefix) {
        return true;
      }
    }

    const auto configured_prefixes =
        setting::CLAY_ANDROID_PARTIAL_REPAINT_BLOCKED_RENDERER_PREFIXES.value();
    bool blocked = false;
    lynx::base::SplitString(configured_prefixes, ',', /*trim=*/true,
                            [&](const char* prefix, size_t length, int) {
                              blocked = renderer.substr(0, length) ==
                                        std::string_view(prefix, length);
                              return !blocked;
                            });
    return blocked;
  }

  // Call with the renderer's GL context current. A missing GL_RENDERER
  // response does not establish a result for subsequent frames.
  bool IsSupported() const {
    if (!supported_) {
      const auto* renderer =
          reinterpret_cast<const char*>(glGetString(GL_RENDERER));
      if (renderer) {
        supported_ = !IsRendererBlocked(renderer);
      }
    }
    return supported_.value_or(false);
  }

 private:
  mutable std::optional<bool> supported_;
};

}  // namespace clay

#endif  // CLAY_SHELL_GPU_ANDROID_PARTIAL_REPAINT_H_
