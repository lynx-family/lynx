// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_LIST_NODE_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_LIST_NODE_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/utils/diff_algorithm.h"

namespace lynx {
namespace tasm {

namespace myers_diff {
struct DiffResult;
}

class TemplateAssembler;

class ListNode {
 public:
  ListNode();

  virtual void RenderComponentAtIndex(uint32_t row,
                                      int64_t operationId = 0) = 0;
  virtual void UpdateComponent(uint32_t sign, uint32_t row,
                               int64_t operationId = 0) = 0;

  virtual void RemoveComponent(uint32_t sign) = 0;

  virtual void AppendComponentInfo(std::unique_ptr<ListComponentInfo> info) = 0;

  /*
   * List New Arch API
   * prepare the component at specified id.
   * return the ui sign of the prepared component.
   */
  virtual int32_t ComponentAtIndex(uint32_t index, int64_t operationId = 0,
                                   bool enable_reuse_notification = false) {
    return 0;
  }

  /*
   * List New Arch API
   * Recycle the element of the component with specified sign.
   * Its element might be reused later upon a call of `ComponentAtIndex`.
   */
  virtual void EnqueueComponent(int32_t sign) {}

  /*
   * Native list with fiber arch API.
   */
  virtual void ComponentAtIndexes(
      const fml::RefPtr<lepus::CArray>& index_array,
      const fml::RefPtr<lepus::CArray>& operation_id_array,
      bool enable_reuse_notification = false) {}

  const std::vector<uint32_t>& fullspan() const {
    return platform_info_.fullspan_;
  }

  const std::vector<uint32_t>& sticky_top() const {
    return platform_info_.stick_top_items_;
  }

  const std::vector<uint32_t>& sticky_bottom() const {
    return platform_info_.stick_bottom_items_;
  }

  const std::vector<std::string>& component_info() const {
    return platform_info_.components_;
  }

  const std::vector<double>& estimated_height() const {
    return platform_info_.estimated_heights_;
  }

  const std::vector<double>& estimated_height_px() const {
    return platform_info_.estimated_heights_px_;
  }

  const std::vector<double>& estimated_main_axis_size_px() const {
    return platform_info_.estimated_main_axis_size_px_;
  }

  const std::vector<std::string>& item_keys() const {
    return platform_info_.item_keys_;
  }

  bool Diffable() const { return platform_info_.diffable_list_result_; }
  bool NewArch() const { return platform_info_.new_arch_list_; }
  bool EnableMoveOperation() const {
    return platform_info_.enable_move_operation_;
  }
  const myers_diff::DiffResult& DiffResult() const {
    return platform_info_.update_actions_;
  }
  void ClearDiffResult() { platform_info_.update_actions_.Clear(); }

 protected:
  struct PlatformInfo {
    std::vector<std::string> components_;
    std::vector<uint32_t> fullspan_;
    std::vector<uint32_t> stick_top_items_;
    std::vector<uint32_t> stick_bottom_items_;
    std::vector<double> estimated_heights_;
    std::vector<double> estimated_heights_px_;
    std::vector<double> estimated_main_axis_size_px_;

    // record the item_key of each item, so that we can figure out that whether
    // a item_key is still in our list
    std::vector<std::string> item_keys_;

    myers_diff::DiffResult update_actions_;
    bool diffable_list_result_{false};
    bool new_arch_list_{false};
    bool enable_move_operation_{false};
    bool enable_plug_{false};

    void Generate(
        const std::vector<std::unique_ptr<ListComponentInfo>>& components) {
      fullspan_.clear();
      stick_top_items_.clear();
      stick_bottom_items_.clear();
      components_.clear();
      estimated_heights_.clear();
      estimated_heights_px_.clear();
      estimated_main_axis_size_px_.clear();
      item_keys_.clear();

      const auto size = components.size();
      components_.reserve(size);
      estimated_heights_.reserve(size);
      estimated_heights_px_.reserve(size);
      estimated_main_axis_size_px_.reserve(size);
      item_keys_.reserve(size);

      for (auto i = size_t{}; i < components.size(); ++i) {
        const auto& info = *components[i];
        components_.emplace_back(info.name_);
        estimated_heights_.emplace_back(info.estimated_height_);
        estimated_heights_px_.emplace_back(info.estimated_height_px_);
        estimated_main_axis_size_px_.emplace_back(
            info.estimated_main_axis_size_px_);
        item_keys_.emplace_back(info.diff_key_.StdString());
        if (info.type_ == ListComponentInfo::Type::HEADER) {
          fullspan_.push_back(static_cast<uint32_t>(i));
        } else if (info.type_ == ListComponentInfo::Type::FOOTER) {
          fullspan_.push_back(static_cast<uint32_t>(i));
        } else if (info.type_ == ListComponentInfo::Type::LIST_ROW) {
          fullspan_.push_back(static_cast<uint32_t>(i));
        }
        if (info.stick_top_) {
          stick_top_items_.push_back(static_cast<uint32_t>(i));
        }
        if (info.stick_bottom_) {
          stick_bottom_items_.push_back(static_cast<uint32_t>(i));
        }
      }
    }
  };

  PlatformInfo platform_info_;
  virtual void FilterComponents(
      std::vector<std::unique_ptr<ListComponentInfo>>& components,
      TemplateAssembler* tasm);
  virtual bool HasComponent(const std::string& component_name,
                            const std::string& current_entry) = 0;

  std::vector<std::unique_ptr<ListComponentInfo>> components_;
  bool MyersDiff(
      const std::vector<std::unique_ptr<ListComponentInfo>>& old_components,
      const std::vector<std::unique_ptr<ListComponentInfo>>& new_components,
      bool force_update_all = false);
  bool MyersDiff(
      const std::vector<std::unique_ptr<ListComponentInfo>>& old_components,
      bool force_update_all = false);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_LIST_NODE_H_
