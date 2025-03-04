// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/ssr/client/ssr_data_update_manager.h"

#include <string>

#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/services/ssr/client/ssr_event_utils.h"

namespace lynx {
namespace ssr {
namespace {

// Get all updated keys, including sub-keys in table and array index, such as
// 'a.b.1'.
void GetUpdatedKeysHelper(const lepus::Value &data,
                          std::vector<base::String> &keys_updated,
                          std::string &paths) {
  if (data.IsArray() || data.IsTable()) {
    tasm::ForEachLepusValue(
        data, [&keys_updated, &paths](const auto &key, const auto &value) {
          if (!value.IsUndefined()) {
            std::size_t original_size = paths.size();
            if (!paths.empty()) {
              paths.append(".");
            }
            paths.append(key.IsInt64() ? std::to_string(key.Int64())
                                       : key.ToString());
            GetUpdatedKeysHelper(value, keys_updated, paths);
            paths.resize(original_size);
          }
        });
  } else {
    keys_updated.push_back(paths);
  }
}
}  // namespace

void SsrDataUpdateManager::UpdateDomIfUpdated(
    const std::vector<base::String> &keys_updated, const lepus::Value &dict) {
  for (const auto &key : keys_updated) {
    auto placeholder_itr = node_placeholder_map_.find(key);
    if (placeholder_itr == node_placeholder_map_.end()) {
      continue;
    }
    // Use a vector to mark update-infos visited to avoid repeat updates.
    std::vector<bool> info_visited_vec(update_info_vec_.size(), false);
    auto &info_index_vec = placeholder_itr->second;
    for (const auto info_index : info_index_vec) {
      if (info_index >= update_info_vec_.size()) {
        continue;
      }
      if (info_visited_vec[info_index]) {
        continue;
      }
      info_visited_vec[info_index] = true;
      auto &update_info = update_info_vec_.at(info_index);

      // Process placeholders updating
      for (auto &string_info : update_info.sub_value_with_placeholder_vec) {
        lepus::Value res =
            ReplacePlaceholdersForString(string_info.value, dict);
        // If the address isn't nullptr, it means that string_info.value is a
        // sub-value in a table or array, so we replace the contents in
        // string_info.address to get the final result. Otherwise,
        // string_info.value is not a sub-value, we can replace the contents in
        // origin_value directly.
        if (string_info.address != nullptr) {
          *(string_info.address) = res;
        } else {
          *(&update_info.origin_value) = res;
        }
      }

      // Update node
      if (update_info.node == nullptr) {
        continue;
      }
      switch (update_info.type) {
        case kAttr: {
          update_info.node->SetDynamicAttribute(update_info.update_key,
                                                update_info.origin_value);
          update_info.node->element()->SetAttribute(update_info.update_key,
                                                    update_info.origin_value);
        } break;
        case kDataset: {
          update_info.node->SetDataSet(update_info.update_key,
                                       update_info.origin_value);
          update_info.node->element()->SetDataSet(
              {{update_info.update_key, update_info.origin_value}});
        } break;
        case kEvent: {
          auto event_itr =
              update_info.node->static_events().find(update_info.update_key);
          if (event_itr != update_info.node->static_events().end()) {
            auto &piper_event_vec = *(event_itr->second->piper_event_vec());
            if (update_info.index_for_event_piper < piper_event_vec.size()) {
              piper_event_vec[update_info.index_for_event_piper]
                  .piper_func_args_ = update_info.origin_value;
            }
          }
        } break;
        default:
          break;
      }
    }
  }
}

lepus::Value SsrDataUpdateManager::GetScriptIfUpdated(
    const std::vector<base::String> &keys_updated, const lepus::Value &dict) {
  if (script_placeholder_info_.first.empty()) {
    return lepus::Value();
  }

  bool found = false;
  for (const auto &placeholder : script_placeholder_info_.first) {
    for (const auto &key_updated : keys_updated) {
      if (placeholder == key_updated) {
        found = true;
        break;
      }
    }
    if (found) {
      break;
    }
  }
  return found ? ReplacePlaceholdersForString(script_placeholder_info_.second,
                                              dict, nullptr, true)
               : lepus::Value();
}

void SsrDataUpdateManager::CollectNodeUpdateInfoIfNeeded(
    const std::vector<base::String> &placeholder_keys,
    lepus::Value &origin_value, const base::String &origin_name,
    SsrPlaceholderStringInfoVec &placeholder_vec, tasm::RadonNode *node,
    SsrNodeUpdateType type, size_t index_for_event_piper) {
  if (placeholder_keys.empty()) {
    return;
  }
  SsrPlaceholderNodeUpdateInfo info = {node,        origin_value,
                                       origin_name, std::move(placeholder_vec),
                                       type,        index_for_event_piper};
  update_info_vec_.emplace_back(std::move(info));
  auto info_index = update_info_vec_.size() - 1;
  for (const auto &placeholder_key : placeholder_keys) {
    node_placeholder_map_[placeholder_key].emplace_back(info_index);
  }
}

void SsrDataUpdateManager::CollectScriptInfoIfNeeded(
    const std::vector<base::String> &placeholder_keys,
    const lepus::Value &value) {
  if (!placeholder_keys.empty()) {
    script_placeholder_info_ = {std::move(placeholder_keys), value};
  }
}

// Collet updated keys which can match the pattern of keys in
// node_placeholder_map_, so that we can find related ops to update.
// static
void SsrDataUpdateManager::GetUpdatedKeys(
    const lepus::Value &data, std::vector<base::String> &keys_updated) {
  std::string str;
  GetUpdatedKeysHelper(data, keys_updated, str);
}

}  // namespace ssr
}  // namespace lynx
