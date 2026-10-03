// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_SHARED_DATA_WHITE_BOARD_INSPECTOR_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_SHARED_DATA_WHITE_BOARD_INSPECTOR_IMPL_H_

#include <memory>
#include <optional>
#include <unordered_map>

#include "core/shared_data/white_board_inspector.h"
#include "devtool/base_devtool/native/public/cdp_error_code.h"

namespace lynx {
namespace devtool {

class WhiteBoardInspectorDelegate;

class WhiteBoardInspectorImpl : public tasm::WhiteBoardInspector {
 public:
  WhiteBoardInspectorImpl() = default;
  ~WhiteBoardInspectorImpl() override = default;

  void InsertDelegate(
      const std::shared_ptr<WhiteBoardInspectorDelegate>& delegate,
      int view_id);
  void RemoveDelegate(int view_id);

  std::optional<CDPErrorCode> SetSharedData(const std::string& key,
                                            const std::string& value,
                                            std::string& error_message);
  std::optional<CDPErrorCode> GetSharedData(
      std::vector<std::pair<std::string, std::string>>& shared_data,
      std::string& error_message);
  std::optional<CDPErrorCode> RemoveSharedData(const std::string& key,
                                               std::string& error_message);
  std::optional<CDPErrorCode> ClearSharedData(std::string& error_message);

  void OnSharedDataAdded(const std::string& key,
                         const pub::Value& value) override;
  void OnSharedDataUpdated(const std::string& key,
                           const pub::Value& value) override;
  void OnSharedDataRemoved(const std::string& key) override;
  void OnSharedDataCleared() override;

 private:
  std::unordered_map<int, std::weak_ptr<WhiteBoardInspectorDelegate>>
      delegates_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_SHARED_DATA_WHITE_BOARD_INSPECTOR_IMPL_H_
