// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_EVENT_KEY_EVENT_H_
#define CLAY_UI_EVENT_KEY_EVENT_H_

#include <stdint.h>

#include <string>

#include "clay/ui/event/keyboard_key.h"

namespace clay {

enum class KeyEventType {
  kDown = 0,
  kUp,
  kRepeat,
  kCommitText,
  kCommitComposingText,
};

class KeyEvent {
 public:
  KeyEvent(uint64_t timestamp, KeyEventType type, PhysicalKeyboardKey physical,
           LogicalKeyboardKey logical, bool synthesized,
           const std::string& character)
      : timestamp_(timestamp),
        type_(type),
        physical_(physical),
        logical_(logical),
        synthesized_(synthesized),
        character_(character) {}

  KeyEvent(uint64_t timestamp, KeyEventType type, uint64_t physical,
           uint64_t logical, bool synthesized, const std::string& character)
      : KeyEvent(timestamp, type, static_cast<PhysicalKeyboardKey>(physical),
                 static_cast<LogicalKeyboardKey>(logical), synthesized,
                 character) {}

  ~KeyEvent() = default;

  uint64_t GetTimestamp() const { return timestamp_; }
  KeyEventType GetType() const { return type_; }
  PhysicalKeyboardKey GetPhysical() const { return physical_; }
  LogicalKeyboardKey GetLogical() const { return logical_; }
  bool GetSynthesized() const { return synthesized_; }
  const std::string& GetCharacter() const { return character_; }
  std::string ToString() const;

 private:
  uint64_t timestamp_;
  KeyEventType type_;
  PhysicalKeyboardKey physical_;
  LogicalKeyboardKey logical_;
  // True if the event does not correspond to a native event.
  //
  // The value is 1 for true, and 0 for false.
  bool synthesized_;
  std::string character_;
};

}  // namespace clay
#endif  // CLAY_UI_EVENT_KEY_EVENT_H_
