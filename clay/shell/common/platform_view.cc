// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/platform_view.h"

#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "clay/common/service/service_manager.h"

namespace clay {

PlatformView::PlatformView(
    std::shared_ptr<clay::ServiceManager> service_manager, Delegate& delegate,
    const TaskRunners& task_runners)
    : service_manager_(service_manager),
      delegate_(delegate),
      task_runners_(task_runners),
      weak_factory_(this) {}

PlatformView::~PlatformView() {}

bool PlatformView::DispatchPointerDataPacket(
    std::unique_ptr<PointerDataPacket> packet) {
  return delegate_.OnPlatformViewDispatchPointerDataPacket(
      pointer_data_packet_converter_.Convert(std::move(packet)));
}

void PlatformView::DispatchKeyEvent(std::unique_ptr<clay::KeyEvent> key_event,
                                    Delegate::KeyDataResponse callback) {
  delegate_.OnPlatformViewDispatchKeyEvent(std::move(key_event),
                                           std::move(callback));
}

#ifdef ENABLE_ACCESSIBILITY
void PlatformView::DispatchClaySemanticsAction(int virtual_view_id,
                                               int action) {
  delegate_.OnPlatformViewDispatchSemanticsAction(virtual_view_id, action);
}
#endif

void PlatformView::SetViewportMetrics(const ViewportMetrics& metrics) {
  delegate_.OnPlatformViewSetViewportMetrics(metrics);
}

void PlatformView::NotifyCreated() { delegate_.OnPlatformViewCreated(); }

void PlatformView::NotifyDestroyed() { delegate_.OnPlatformViewDestroyed(); }

void PlatformView::ScheduleFrame() { delegate_.OnPlatformViewScheduleFrame(); }

PointerDataDispatcherMaker PlatformView::GetDispatcherMaker() {
  return [](DefaultPointerDataDispatcher::Delegate& delegate) {
    return std::make_unique<DefaultPointerDataDispatcher>(delegate);
  };
}

fml::WeakPtr<PlatformView> PlatformView::GetWeakPtr() const {
  return weak_factory_.GetWeakPtr();
}

void PlatformView::OnPreEngineRestart() const {}

void PlatformView::SetNextFrameCallback(const fml::closure& closure) {
  if (!closure) {
    return;
  }

  delegate_.OnPlatformViewSetNextFrameCallback(closure);
}

std::unique_ptr<std::vector<std::string>>
PlatformView::ComputePlatformResolvedLocales(
    const std::vector<std::string>& supported_locale_data) {
  std::unique_ptr<std::vector<std::string>> out =
      std::make_unique<std::vector<std::string>>();
  return out;
}

void PlatformView::DispatchKeyDataPacket(
    std::unique_ptr<clay::KeyDataPacket> packet,
    Delegate::KeyDataResponse callback) {
  if (!packet) {
    return;
  }

  // unpack clay::KeyDataPacket
  auto& data = packet->data();
  const uint8_t* buf = data.data();
  uint64_t char_size = 0;
  if (data.size() < sizeof(char_size) + sizeof(KeyData)) {
    return;
  }
  memcpy(&char_size, buf, sizeof(char_size));
  buf += sizeof(char_size);
  if (data.size() != sizeof(char_size) + sizeof(KeyData) + char_size) {
    return;
  }
  KeyData key_data;
  memcpy(&key_data, buf, sizeof(KeyData));
  buf += sizeof(KeyData);
  auto character = std::string(buf, buf + char_size);

  // convert to clay::KeyEvent
  auto event = std::make_unique<clay::KeyEvent>(
      key_data.timestamp, static_cast<clay::KeyEventType>(key_data.type),
      static_cast<clay::PhysicalKeyboardKey>(key_data.physical),
      static_cast<clay::LogicalKeyboardKey>(key_data.logical),
      key_data.synthesized, character);

  DispatchKeyEvent(std::move(event), std::move(callback));
}
}  // namespace clay
