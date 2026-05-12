// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_LYNX_TRAIL_HUB_H_
#define CORE_RENDERER_UTILS_LYNX_TRAIL_HUB_H_

#include <memory>
#include <optional>
#include <string>

#include "base/include/no_destructor.h"

namespace lynx {
namespace tasm {
/**
 * Singleton class to get trail value
 * TODO(zhoupeng): move trail-related code to this class from LynxEnv
 */
class LynxTrailHub {
 public:
  /**
   * Class helps to get trail value from platform.
   */
  class TrailImpl {
   public:
    virtual ~TrailImpl() = default;

    static std::unique_ptr<TrailImpl> Create();

    virtual std::optional<std::string> GetStringForTrailKey(
        const std::string& key) = 0;
  };

 public:
  static LynxTrailHub& GetInstance();

  /**
   * Get trail value by key
   */
  std::optional<std::string> GetStringForTrailKey(const std::string& key);

  ~LynxTrailHub() = default;
  LynxTrailHub(const LynxTrailHub&) = delete;
  LynxTrailHub& operator=(const LynxTrailHub&) = delete;
  LynxTrailHub(LynxTrailHub&&) = delete;
  LynxTrailHub& operator=(LynxTrailHub&&) = delete;

 private:
  LynxTrailHub();

  std::unique_ptr<TrailImpl> impl_{nullptr};

  friend class base::NoDestructor<LynxTrailHub>;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_LYNX_TRAIL_HUB_H_
