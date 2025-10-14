// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_LOADER_DATA_IMAGE_LOADER_H_
#define CLAY_NET_LOADER_DATA_IMAGE_LOADER_H_

#include <functional>
#include <string>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/resource_type.h"

namespace clay {

class DataImageLoader : public ResourceLoader {
 public:
  explicit DataImageLoader(fml::RefPtr<fml::TaskRunner> task_runner);

  void Load(const std::string& src,
            const std::function<void(const uint8_t*, size_t)>& callback,
            const ResourceType resource_type = ResourceType::kOthers,
            bool need_redirect = false) override;

  RawResource LoadSync(const std::string& src,
                       const ResourceType resource_type = ResourceType::kOthers,
                       bool need_redirect = false) override;

  // Not support cancel
  void CancelAll() override {}

 private:
  fml::RefPtr<fml::TaskRunner> task_runner_;
};

}  // namespace clay

#endif  // CLAY_NET_LOADER_DATA_IMAGE_LOADER_H_
