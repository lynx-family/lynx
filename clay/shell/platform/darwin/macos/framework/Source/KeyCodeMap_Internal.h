// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <cinttypes>
#include <cstddef>

namespace clay {

template <typename Key, typename Value>
struct KeyCodeMapEntry {
  Key key;
  Value value;
};

template <typename Key, typename Value>
inline const KeyCodeMapEntry<Key, Value>* FindKeyCodeMapEntry(
    const KeyCodeMapEntry<Key, Value>* entries, size_t size, const Key& key) {
  const auto* end = entries + size;
  const auto* entry =
      std::lower_bound(entries, end, key,
                       [](const KeyCodeMapEntry<Key, Value>& entry,
                          const Key& value) { return entry.key < value; });
  return entry != end && entry->key == key ? entry : nullptr;
}

/**
 * Maps macOS-specific key code values representing |PhysicalKeyboardKey|.
 *
 * MacOS doesn't provide a scan code, but a virtual keycode to represent a
 * physical key.
 */
extern const KeyCodeMapEntry<uint32_t, uint64_t> keyCodeToPhysicalKey[];
extern const size_t keyCodeToPhysicalKeySize;

/**
 * A map from macOS key codes to Flutter's logical key values.
 *
 * This is used to derive logical keys that can't or shouldn't be derived from
 * |charactersIgnoringModifiers|.
 */
extern const KeyCodeMapEntry<uint32_t, uint64_t> keyCodeToLogicalKey[];
extern const size_t keyCodeToLogicalKeySize;

// Several mask constants. See KeyCodeMap.g.mm for their descriptions.

/**
 * Mask for the 32-bit value portion of the key code.
 */
extern const uint64_t kValueMask;

/**
 * The plane value for keys which have a Unicode representation.
 */
extern const uint64_t kUnicodePlane;

/**
 * The plane value for the private keys defined by the macOS embedding.
 */
extern const uint64_t kMacosPlane;

/**
 * Map |NSEvent.keyCode| to its corresponding bitmask of NSEventModifierFlags.
 *
 * This does not include CapsLock, for it is handled specially.
 */
extern const KeyCodeMapEntry<uint32_t, uint32_t> keyCodeToModifierFlag[];
extern const size_t keyCodeToModifierFlagSize;

/**
 * Map a bit of bitmask of NSEventModifierFlags to its corresponding
 * |NSEvent.keyCode|.
 *
 * This does not include CapsLock, for it is handled specially.
 */
extern const KeyCodeMapEntry<uint32_t, uint32_t> modifierFlagToKeyCode[];
extern const size_t modifierFlagToKeyCodeSize;

/**
 * The physical key for CapsLock, which needs special handling.
 */
extern const uint64_t kCapsLockPhysicalKey;

/**
 * The logical key for CapsLock, which needs special handling.
 */
extern const uint64_t kCapsLockLogicalKey;

/**
 * Bits in |NSEvent.modifierFlags| indicating whether a modifier key is pressed.
 *
 * These constants are not written in the official documentation, but derived
 * from experiments. This is currently the only way to know whether a one-side
 * modifier key (such as ShiftLeft) is pressed, instead of the general combined
 * modifier state (such as Shift).
 */
typedef enum {
  kModifierFlagControlLeft = 0x1,
  kModifierFlagShiftLeft = 0x2,
  kModifierFlagShiftRight = 0x4,
  kModifierFlagMetaLeft = 0x8,
  kModifierFlagMetaRight = 0x10,
  kModifierFlagAltLeft = 0x20,
  kModifierFlagAltRight = 0x40,
  kModifierFlagControlRight = 0x200,
} ModifierFlag;

/**
 * A character that Flutter wants to derive layout for, and guides on how to
 * derive it.
 */
typedef struct {
  // The key code for a key that prints `keyChar` in the US keyboard layout.
  uint16_t keyCode;

  // The printable string to derive logical key for.
  uint8_t keyChar;

  // If the goal is mandatory, the keyboard manager will make sure to find a
  // logical key for this character, falling back to the US keyboard layout.
  bool mandatory;
} LayoutGoal;

/**
 * All keys that Flutter wants to derive layout for, and guides on how to derive
 * them.
 */
extern const LayoutGoal layoutGoals[];
extern const size_t layoutGoalsSize;

}  // namespace clay
