// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/event/event_utils.h"

#include "clay/fml/logging.h"

namespace clay {

const char* EventTypeToString(ClayEventType type) {
  switch (type) {
    case kClayEventTypeUnknown:
      return "unknown";
    case kClayEventTypeTouchStart:
      return "touchstart";
    case kClayEventTypeTouchMove:
      return "touchmove";
    case kClayEventTypeTouchCancel:
      return "touchcancel";
    case kClayEventTypeTouchEnd:
      return "touchend";
    case kClayEventTypeTap:
      return "tap";
    case kClayEventTypeLongPress:
      return "longpress";
    case kClayEventTypeMouseUp:
      return "mouseup";
    case kClayEventTypeMouseDown:
      return "mousedown";
    case kClayEventTypeMouseMove:
      return "mousemove";
    case kClayEventTypeMouseClick:
      return "mouseclick";
    case kClayEventTypeMouseDoubleClick:
      return "mousedblclick";
    case kClayEventTypeMouseLongPress:
      return "mouselongpress";
    case kClayEventTypeMouseEnter:
      return "mouseenter";
    case kClayEventTypeMouseOver:
      return "mouseover";
    case kClayEventTypeMouseLeave:
      return "mouseleave";
    case kClayEventTypeDragEnter:
      return "dragenter";
    case kClayEventTypeDragOver:
      return "dragover";
    case kClayEventTypeDragLeave:
      return "dragleave";
    case kClayEventTypeDrop:
      return "drop";
    case kClayEventTypeWheel:
      return "wheel";
    case kClayEventTypeKeyDown:
      return "keydown";
    case kClayEventTypeKeyUp:
      return "keyup";
    case kClayEventTypeAnimationStart:
      return "animationstart";
    case kClayEventTypeAnimationRepeat:
      return "animationiteration";
    case kClayEventTypeAnimationEnd:
      return "animationend";
    case kClayEventTypeAnimationCancel:
      return "animationcancel";
    case kClayEventTypeTransitionStart:
      return "transitionstart";
    case kClayEventTypeTransitionEnd:
      return "transitionend";
    default:
      FML_DLOG(ERROR) << "unsupported event type:" << type;
      return "";
  }
  return "";
}

}  // namespace clay
