// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_VIEW_CLIENT_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_VIEW_CLIENT_PRIV_H_

#include "platform/embedder/public/capi/lynx_view_client_capi.h"

struct lynx_view_client_t {
  void* user_data = nullptr;
  on_page_start on_page_start = nullptr;
  on_load_success on_load_success = nullptr;
  on_first_screen on_first_screen = nullptr;
  on_page_updated on_page_updated = nullptr;
  on_data_updated on_data_updated = nullptr;
  on_destroy on_destroy = nullptr;
  on_runtime_ready on_runtime_ready = nullptr;
  on_received_error on_received_error = nullptr;
  on_timing_setup on_timing_setup = nullptr;
  on_timing_update on_timing_update = nullptr;
  on_enter_foreground on_enter_foreground = nullptr;
  on_enter_background on_enter_background = nullptr;
  on_frame_timing on_frame_timing = nullptr;
};

#endif  // PLATFORM_EMBEDDER_LYNX_VIEW_CLIENT_PRIV_H_
