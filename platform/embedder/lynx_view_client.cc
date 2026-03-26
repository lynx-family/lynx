// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_client_priv.h"

LYNX_EXTERN_C lynx_view_client_t* lynx_view_client_create(void* user_data) {
  lynx_view_client_t* client = new lynx_view_client_t();
  client->user_data = user_data;
  return client;
}

LYNX_EXTERN_C void* lynx_view_client_get_user_data(lynx_view_client_t* client) {
  return client->user_data;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_page_start(
    lynx_view_client_t* client, on_page_start on_page_start) {
  client->on_page_start = on_page_start;
}
LYNX_EXTERN_C void lynx_view_client_bind_on_load_success(
    lynx_view_client_t* client, on_load_success on_load_success) {
  client->on_load_success = on_load_success;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_first_screen(
    lynx_view_client_t* client, on_first_screen on_first_screen) {
  client->on_first_screen = on_first_screen;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_page_updated(
    lynx_view_client_t* client, on_page_updated on_page_updated) {
  client->on_page_updated = on_page_updated;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_data_updated(
    lynx_view_client_t* client, on_data_updated on_data_updated) {
  client->on_data_updated = on_data_updated;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_destroy(lynx_view_client_t* client,
                                                    on_destroy on_destroy) {
  client->on_destroy = on_destroy;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_runtime_ready(
    lynx_view_client_t* client, on_runtime_ready on_runtime_ready) {
  client->on_runtime_ready = on_runtime_ready;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_received_error(
    lynx_view_client_t* client, on_received_error on_received_error) {
  client->on_received_error = on_received_error;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_timing_setup(
    lynx_view_client_t* client, on_timing_setup on_timing_setup) {
  client->on_timing_setup = on_timing_setup;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_timing_update(
    lynx_view_client_t* client, on_timing_update on_timing_update) {
  client->on_timing_update = on_timing_update;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_enter_foreground(
    lynx_view_client_t* client, on_enter_foreground on_enter_foreground) {
  client->on_enter_foreground = on_enter_foreground;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_enter_background(
    lynx_view_client_t* client, on_enter_background on_enter_background) {
  client->on_enter_background = on_enter_background;
}

LYNX_EXTERN_C void lynx_view_client_bind_on_frame_timing(
    lynx_view_client_t* client, on_frame_timing on_frame_timing) {
  client->on_frame_timing = on_frame_timing;
}

LYNX_EXTERN_C void lynx_view_client_release(lynx_view_client_t* client) {
  delete client;
}
