// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_SSR_CLIENT_SSR_DATA_UPDATE_MANAGER_H_
#define CORE_SERVICES_SSR_CLIENT_SSR_DATA_UPDATE_MANAGER_H_

#include <map>
#include <utility>
#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class RadonNode;
}
namespace ssr {

enum SsrNodeUpdateType : uint32_t { kAttr = 0, kDataset, kEvent };

struct SsrPlaceholderStringInfo {
  // value with placeholder
  lepus::Value value;
  // It will be assigned with the address of processed-value placed only when
  // the value above is a sub-value of a table or array.
  lepus::Value *address{nullptr};
};

using SsrPlaceholderStringInfoVec = std::vector<SsrPlaceholderStringInfo>;

// infos needed for an update operation
struct SsrPlaceholderNodeUpdateInfo {
  tasm::RadonNode *node{nullptr};
  // Value with placeholders
  lepus::Value origin_value;
  // The key corresponding to the value with placeholders, like 'text' in
  // attribute
  base::String update_key;
  // Sub string values with placeholder. if origin_value is a table, there may
  // be several string values with placeholder.
  SsrPlaceholderStringInfoVec sub_value_with_placeholder_vec;
  SsrNodeUpdateType type;
  size_t index_for_event_piper{0};
};

class SsrDataUpdateManager {
 public:
  // If there are placeholders in origin_value, collect related info and store
  // it in node_placeholder_map_.
  void CollectNodeUpdateInfoIfNeeded(
      const std::vector<base::String> &placeholder_keys,
      lepus::Value &origin_value, const base::String &origin_name,
      SsrPlaceholderStringInfoVec &placeholder_pair_vec, tasm::RadonNode *node,
      SsrNodeUpdateType type, size_t index = 0);

  // According to keys_updated to update related node info
  void UpdateDomIfUpdated(const std::vector<base::String> &keys_updated,
                          const lepus::Value &dict);

  // If there are placeholders in script, collect placeholders and script and
  // store it in script_placeholder_info_.
  void CollectScriptInfoIfNeeded(
      const std::vector<base::String> &placeholder_keys,
      const lepus::Value &value);

  // If placeholders in script are updated, replace them and return it.
  lepus::Value GetScriptIfUpdated(const std::vector<base::String> &keys_updated,
                                  const lepus::Value &dict);

  static void GetUpdatedKeys(const lepus::Value &data,
                             std::vector<base::String> &keys_updated);

 private:
  // The key is the placeholder key in node, the value is vector of indexes
  // mapping to update_info in update_info_vec
  std::map<base::String, std::vector<size_t>> node_placeholder_map_;
  std::vector<SsrPlaceholderNodeUpdateInfo> update_info_vec_;

  // The first value is set of placeholders in script, the second value is
  // original script value.
  std::pair<std::vector<base::String>, lepus::Value> script_placeholder_info_;
};

}  // namespace ssr
}  // namespace lynx

#endif  // CORE_SERVICES_SSR_CLIENT_SSR_DATA_UPDATE_MANAGER_H_
