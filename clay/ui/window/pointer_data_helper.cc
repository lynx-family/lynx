// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/window/pointer_data_helper.h"

namespace clay {

// Returns the PointerData::Change for the given ClayPointerPhase.
PointerData::Change PointerDataHelper::ToPointerDataChange(
    ClayPointerPhase phase) {
  switch (phase) {
    case kClayPointerPhaseCancel:
      return PointerData::Change::kCancel;
    case kClayPointerPhaseUp:
      return PointerData::Change::kUp;
    case kClayPointerPhaseDown:
      return PointerData::Change::kDown;
    case kClayPointerPhaseMove:
      return PointerData::Change::kMove;
    case kClayPointerPhaseAdd:
      return PointerData::Change::kAdd;
    case kClayPointerPhaseRemove:
      return PointerData::Change::kRemove;
    case kClayPointerPhaseHover:
      return PointerData::Change::kHover;
    case kClayPointerPhasePanZoomStart:
      return PointerData::Change::kPanZoomStart;
    case kClayPointerPhasePanZoomUpdate:
      return PointerData::Change::kPanZoomUpdate;
    case kClayPointerPhasePanZoomEnd:
      return PointerData::Change::kPanZoomEnd;
  }
  return PointerData::Change::kCancel;
}

// Returns the PointerData::DeviceKind for the given
// ClayPointerDeviceKind.
PointerData::DeviceKind PointerDataHelper::ToPointerDataKind(
    ClayPointerDeviceKind device_kind) {
  switch (device_kind) {
    case kClayPointerDeviceKindMouse:
      return PointerData::DeviceKind::kMouse;
    case kClayPointerDeviceKindTouch:
      return PointerData::DeviceKind::kTouch;
    case kClayPointerDeviceKindStylus:
      return PointerData::DeviceKind::kStylus;
    case kClayPointerDeviceKindTrackpad:
      return PointerData::DeviceKind::kTrackpad;
  }
  return PointerData::DeviceKind::kMouse;
}

// Returns the PointerData::SignalKind for the given
// ClayPointerSignalKind.
PointerData::SignalKind PointerDataHelper::ToPointerDataSignalKind(
    ClayPointerSignalKind kind) {
  switch (kind) {
    case kClayPointerSignalKindNone:
      return PointerData::SignalKind::kNone;
    case kClayPointerSignalKindScroll:
      return PointerData::SignalKind::kScroll;
    case kClayPointerSignalKindScrollInertiaCancel:
      return PointerData::SignalKind::kScrollInertiaCancel;
    case kClayPointerSignalKindScale:
      return PointerData::SignalKind::kScale;
  }
  return PointerData::SignalKind::kNone;
}

// Returns the buttons to synthesize for a PointerData from a
// ClayPointerEvent with no type or buttons set.
int64_t PointerDataHelper::PointerDataButtonsForLegacyEvent(
    PointerData::Change change) {
  switch (change) {
    case PointerData::Change::kDown:
    case PointerData::Change::kMove:
      // These kinds of change must have a non-zero `buttons`, otherwise
      // gesture recognizers will ignore these events.
      return kPointerButtonMousePrimary;
    case PointerData::Change::kCancel:
    case PointerData::Change::kAdd:
    case PointerData::Change::kRemove:
    case PointerData::Change::kHover:
    case PointerData::Change::kUp:
    case PointerData::Change::kPanZoomStart:
    case PointerData::Change::kPanZoomUpdate:
    case PointerData::Change::kPanZoomEnd:
      return 0;
  }
  return 0;
}

}  // namespace clay
