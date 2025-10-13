// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_template_data_priv.h"
#include "platform/embedder/lynx_update_meta_priv.h"

LYNX_EXTERN_C lynx_update_meta_t* lynx_update_meta_create() {
  return new lynx_update_meta_t();
}

LYNX_EXTERN_C void lynx_update_meta_set_update_data(
    lynx_update_meta_t* meta, lynx_template_data_t* data) {
  meta->update_data = data->template_data;
}

LYNX_EXTERN_C void lynx_update_meta_set_global_props(
    lynx_update_meta_t* meta, lynx_template_data_t* data) {
  meta->global_props = data->template_data;
}

LYNX_EXTERN_C void lynx_update_meta_release(lynx_update_meta_t* meta) {
  delete meta;
}
