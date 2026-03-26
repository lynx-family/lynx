// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/frame_timing_listener_impl.h"

#include "platform/embedder/lynx_view_clients.h"

namespace lynx {
namespace embedder {

void FrameTimingListenerImpl::OnFrameTiming(int64_t frame_start_time_in_ns,
                                            int64_t frame_finish_time_in_ns) {
  for (auto* client : clients_) {
    client->OnFrameTiming(frame_start_time_in_ns, frame_finish_time_in_ns);
  }
}

void FrameTimingListenerImpl::AddClient(LynxViewClients* client) {
  if (std::find(clients_.begin(), clients_.end(), client) == clients_.end()) {
    clients_.emplace_back(client);
  }
}

void FrameTimingListenerImpl::RemoveAllClients() { clients_.clear(); }

}  // namespace embedder
}  // namespace lynx
