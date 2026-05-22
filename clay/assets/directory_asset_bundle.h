// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_ASSETS_DIRECTORY_ASSET_BUNDLE_H_
#define CLAY_ASSETS_DIRECTORY_ASSET_BUNDLE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/unique_fd.h"
#include "clay/assets/asset_resolver.h"

namespace clay {

class DirectoryAssetBundle : public AssetResolver {
 public:
  DirectoryAssetBundle(fml::UniqueFD descriptor,
                       bool is_valid_after_asset_manager_change);

  ~DirectoryAssetBundle() override;

 private:
  const fml::UniqueFD descriptor_;
  bool is_valid_ = false;
  bool is_valid_after_asset_manager_change_ = false;

  // |AssetResolver|
  bool IsValid() const override;

  // |AssetResolver|
  bool IsValidAfterAssetManagerChange() const override;

  // |AssetResolver|
  AssetResolver::AssetResolverType GetType() const override;

  // |AssetResolver|
  std::unique_ptr<fml::Mapping> GetAsMapping(
      const std::string& asset_name) const override;

  // |AssetResolver|
  std::vector<std::unique_ptr<fml::Mapping>> GetAsMappings(
      const std::string& asset_pattern,
      const std::optional<std::string>& subdir) const override;

  BASE_DISALLOW_COPY_AND_ASSIGN(DirectoryAssetBundle);
};

}  // namespace clay

#endif  // CLAY_ASSETS_DIRECTORY_ASSET_BUNDLE_H_
