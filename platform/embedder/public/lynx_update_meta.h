// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_UPDATE_META_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_UPDATE_META_H_

#include <memory>

#include "capi/lynx_update_meta_capi.h"
#include "lynx_template_data.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief LynxUpdateMeta is the update meta of Lynx. It provides methods for
 * setting update data and global props.
 */
class LynxUpdateMeta {
 public:
  LynxUpdateMeta() : update_meta_(lynx_update_meta_create()) {}

  ~LynxUpdateMeta() { lynx_update_meta_release(update_meta_); }

  /**
   * @apidoc
   * @brief Set the update data of the current LynxUpdateMeta.
   * @param data The update data to be set.
   */
  void SetUpdateData(std::shared_ptr<LynxTemplateData> data) {
    lynx_update_meta_set_update_data(update_meta_, data->Impl());
  }

  /**
   * @apidoc
   * @brief Set the global props of the current LynxUpdateMeta.
   * @param global_props The global props to be set.
   */
  void SetGlobalProps(std::shared_ptr<LynxTemplateData> global_props) {
    lynx_update_meta_set_global_props(update_meta_, global_props->Impl());
  }

  lynx_update_meta_t* Impl() { return update_meta_; }

  LynxUpdateMeta(const LynxUpdateMeta&) = delete;
  LynxUpdateMeta& operator=(const LynxUpdateMeta&) = delete;

 private:
  lynx_update_meta_t* update_meta_;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_UPDATE_META_H_
