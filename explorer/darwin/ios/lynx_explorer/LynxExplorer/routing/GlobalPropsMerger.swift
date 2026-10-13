// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore containerid containertype explorersupportsexplicitrouteownership
// cspell:ignore explorersupportssparklingcontainer explorersparklingversion
// cspell:ignore sparklingavailable sparklinghostcapabilities sparklingnavigation
// cspell:ignore spkcontainerid spkpipe

import Foundation

/// Single source of truth for reserved capability keys. Every input boundary
/// (`GlobalPropsMerger`, `LegacyContainerLauncher`, and the Objective-C
/// `LynxViewShellViewController`) reserves this same set so container-owned
/// identity and capability values can never be impersonated by caller input.
/// Keys are pre-normalized; always compare through `isReservedCapabilityKey(_:)`.
/// Add any new container-owned key to `normalized` below and nowhere else, so
/// the boundaries can never drift out of sync again.
@objc final class ExplorerReservedKeys: NSObject {
  private static let normalized: Set<String> = [
    "containerid",
    "containertype",
    "explorersupportsexplicitrouteownership",
    "explorersupportssparklingcontainer",
    "explorersparklingversion",
    "sparklingavailable",
    "sparklinghostcapabilities",
    "sparklingnavigation",
    "spkcontainerid",
    "spkpipe",
  ]

  /// Strips `_`/`-` and lowercases so every spelling maps to one entry.
  static func normalizedKey(_ key: String) -> String {
    key.filter { $0 != "_" && $0 != "-" }.lowercased()
  }

  /// Objective-C and Swift entry point for the reserved-key check.
  @objc static func isReservedCapabilityKey(_ key: String) -> Bool {
    normalized.contains(normalizedKey(key))
  }
}

/// Merges caller-supplied props while keeping Sparkling-owned identity and
/// capability values outside every untrusted input layer.
enum GlobalPropsMerger {
  /// The pinned SDK creates these values after Explorer supplies its context
  /// props. Drop collisions only from the lowest-precedence common layer so
  /// Sparkling's stable values win, while an explicit page prop can still
  /// override them according to the public common < stable/container < page
  /// contract. Remove this inventory when Sparkling exposes layered props.
  private static let sdkStableKeys: Set<String> = [
    "SPK_version",
    "accessibleMode",
    "bottomHeight",
    "containerInitTime",
    "contentHeight",
    "deviceModel",
    "fullUrl",
    "isAppBackground",
    "isIPhoneX",
    "isIPhoneXMax",
    "isLowPowerMode",
    "isPad",
    "language",
    "lynxSdkVersion",
    "originUrl",
    "os",
    "osVersion",
    "queryItems",
    "safeAreaHeight",
    "screenHeight",
    "screenOrientation",
    "screenWidth",
    "sparklingVersion",
    "statusBarHeight",
    "topHeight",
  ]

  static func merge(
    common: [String: AnyHashable],
    container: [String: AnyHashable],
    page: [String: AnyHashable]
  ) -> [String: Any] {
    var result: [String: Any] = [:]

    for (key, value) in common
    where !ExplorerReservedKeys.isReservedCapabilityKey(key)
      && !sdkStableKeys.contains(key) {
      result[key] = value.base
    }
    for layer in [container, page] {
      for (key, value) in layer
      where !ExplorerReservedKeys.isReservedCapabilityKey(key) {
        result[key] = value.base
      }
    }

    // The SDK writes the final containerID after creating its Lynx wrapper.
    result.removeValue(forKey: "containerID")
    result["containerType"] = "sparkling"
    result["explorerSupportsExplicitRouteOwnership"] = true
    result["explorerSupportsSparklingContainer"] = true
    result["sparklingAvailable"] = true
    // Sparkling Go treats an absent capability list as its standalone host,
    // where every demo is available. Explorer declares only modules it has
    // integrated so unsupported examples remain out of the embedded UI.
    result["sparklingHostCapabilities"] = "navigation,globalProps,appearance"
    result["sparklingNavigation"] = true
    return result
  }
}
