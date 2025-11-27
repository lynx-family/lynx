// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstring>

#include "platform/embedder/lynx_load_meta_priv.h"

LYNX_EXTERN_C lynx_load_meta_t* lynx_load_meta_create() {
  return new lynx_load_meta_t();
}

LYNX_EXTERN_C void lynx_load_meta_set_url(lynx_load_meta_t* meta,
                                          const char* url) {
  if (url) {
    meta->url = url;
  }
}

LYNX_EXTERN_C void lynx_load_meta_set_template_bundle(
    lynx_load_meta_t* meta, lynx_template_bundle_t* bundle) {
  meta->template_bundle = bundle->template_bundle;
}

LYNX_EXTERN_C void lynx_load_meta_set_binary_data(
    lynx_load_meta_t* meta, uint8_t* data, size_t length,
    void (*dtor)(uint8_t*, size_t, void*), void* opaque) {
  if (meta->binary_data.data) {
    if (meta->binary_data.dtor) {
      meta->binary_data.dtor(meta->binary_data.data, meta->binary_data.length,
                             meta->binary_data.opaque);
    } else {
      delete[] meta->binary_data.data;
    }
  }
  if (dtor) {
    meta->binary_data.data = data;
    meta->binary_data.length = length;
    meta->binary_data.dtor = dtor;
    meta->binary_data.opaque = opaque;
  } else {
    meta->binary_data.data = new uint8_t[length];
    meta->binary_data.length = length;
    meta->binary_data.dtor = nullptr;
    memcpy(meta->binary_data.data, data, length);
  }
}

LYNX_EXTERN_C void lynx_load_meta_set_initial_data(lynx_load_meta_t* meta,
                                                   lynx_template_data_t* data) {
  meta->initial_data = data->template_data;
}

LYNX_EXTERN_C void lynx_load_meta_set_global_props(lynx_load_meta_t* meta,
                                                   lynx_template_data_t* data) {
  meta->global_props = data->template_data;
}

LYNX_EXTERN_C void lynx_load_meta_release(lynx_load_meta_t* meta) {
  if (meta->binary_data.data) {
    if (meta->binary_data.dtor) {
      meta->binary_data.dtor(meta->binary_data.data, meta->binary_data.length,
                             meta->binary_data.opaque);
    } else {
      delete[] meta->binary_data.data;
    }
  }
  delete meta;
}

int lynx_load_meta_is_template_bundle_valid(lynx_load_meta_t* meta) {
  return meta->template_bundle != nullptr;
}
