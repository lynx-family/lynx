// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_LOAD_META_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_LOAD_META_PRIV_H_

#include <memory>
#include <string>

#include "core/renderer/data/template_data.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "platform/embedder/lynx_template_bundle_priv.h"
#include "platform/embedder/lynx_template_data_priv.h"
#include "platform/embedder/public/capi/lynx_load_meta_capi.h"

struct lynx_load_meta_t {
  std::string url;
  std::shared_ptr<lynx::tasm::LynxTemplateBundle> template_bundle = nullptr;

  struct binary_data {
    uint8_t* data = nullptr;
    size_t length = 0;
    void (*dtor)(uint8_t*, size_t, void*) = nullptr;
    void* opaque = nullptr;
  } binary_data;

  std::shared_ptr<lynx::tasm::TemplateData> initial_data = nullptr;
  std::shared_ptr<lynx::tasm::TemplateData> global_props = nullptr;
};

int lynx_load_meta_is_template_bundle_valid(lynx_load_meta_t* meta);

#endif  // PLATFORM_EMBEDDER_LYNX_LOAD_META_PRIV_H_
