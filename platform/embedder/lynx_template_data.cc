// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus_context/json_parser.h"
#include "platform/embedder/lynx_template_data_priv.h"

LYNX_EXTERN_C lynx_template_data_t* lynx_template_data_create_from_json(
    const char* json) {
  lynx_template_data_t* template_data = new lynx_template_data_t();
  template_data->template_data = std::make_shared<lynx::tasm::TemplateData>();
  if (json) {
    template_data->template_data->SetValue(
        lynx::lepus::jsonValueTolepusValue(json));
  }

  return template_data;
}

LYNX_EXTERN_C void lynx_template_data_mark_state(
    lynx_template_data_t* template_data, const char* name) {
  template_data->template_data->SetPreprocessorName(
      std::string(name ? name : ""));
}

LYNX_EXTERN_C void lynx_template_data_set_read_only(
    lynx_template_data_t* template_data, int read_only) {
  template_data->template_data->SetReadOnly(read_only);
}

LYNX_EXTERN_C void lynx_template_data_release(
    lynx_template_data_t* template_data) {
  delete template_data;
}
