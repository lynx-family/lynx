// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import XCTest

@testable import LynxExplorer

final class GlobalPropsMergerTests: XCTestCase {
  func testOrdinaryPropsUseCommonThenContainerThenPagePrecedence() {
    let merged = GlobalPropsMerger.merge(
      common: [
        "commonOnly": "common",
        "containerWins": "common",
        "pageWins": "common",
      ],
      container: [
        "containerOnly": "container",
        "containerWins": "container",
        "pageWins": "container",
      ],
      page: [
        "pageOnly": "page",
        "pageWins": "page",
      ])

    XCTAssertEqual(merged["commonOnly"] as? String, "common")
    XCTAssertEqual(merged["containerOnly"] as? String, "container")
    XCTAssertEqual(merged["containerWins"] as? String, "container")
    XCTAssertEqual(merged["pageOnly"] as? String, "page")
    XCTAssertEqual(merged["pageWins"] as? String, "page")
  }

  func testCapabilityAndIdentityPropsCannotBeSpoofedByAnyInputLayer() {
    let spoofed: [String: AnyHashable] = [
      "containerID": "attacker-id",
      "containerType": "legacy",
      "sparkling_available": false,
      "sparkling_host_capabilities": "storage,media",
      "sparklingNavigation": false,
      "explorerSupportsExplicitRouteOwnership": false,
      "explorer_supports_explicit_route_ownership": false,
      "explorerSupportsSparklingContainer": false,
      "explorer_supports_sparkling_container": false,
      "spk-container-id": "attacker-sdk-id",
      "spk_pipe": "attacker-pipe",
    ]

    let merged = GlobalPropsMerger.merge(
      common: spoofed,
      container: spoofed,
      page: spoofed)

    XCTAssertNil(merged["containerID"])
    XCTAssertEqual(merged["containerType"] as? String, "sparkling")
    XCTAssertEqual(merged["sparklingAvailable"] as? Bool, true)
    XCTAssertEqual(
      merged["sparklingHostCapabilities"] as? String,
      "navigation,globalProps,appearance")
    XCTAssertEqual(merged["sparklingNavigation"] as? Bool, true)
    XCTAssertEqual(merged["explorerSupportsExplicitRouteOwnership"] as? Bool, true)
    XCTAssertEqual(merged["explorerSupportsSparklingContainer"] as? Bool, true)
    XCTAssertNil(merged["explorer_supports_explicit_route_ownership"])
    XCTAssertNil(merged["explorer_supports_sparkling_container"])
    XCTAssertNil(merged["spk-container-id"])
    XCTAssertNil(merged["spk_pipe"])
  }

  func testSDKStablePropsBeatCommonButRemainPageOverridable() {
    let merged = GlobalPropsMerger.merge(
      common: [
        "SPK_version": "spoofed-common-version",
        "os": "spoofed-common-os",
        "containerInitTime": 1,
        "screenWidth": 1,
        "ordinary": "common",
      ],
      container: [:],
      page: [
        "SPK_version": "page-version",
        "screenWidth": 750,
        "ordinary": "page",
      ])

    XCTAssertNil(merged["os"])
    XCTAssertNil(merged["containerInitTime"])
    XCTAssertEqual(merged["SPK_version"] as? String, "page-version")
    XCTAssertEqual(merged["screenWidth"] as? Int, 750)
    XCTAssertEqual(merged["ordinary"] as? String, "page")
  }

  func testSDKVersionStableKeyIsDroppedFromCommonButRemainsPageOverridable() {
    let stableOwned = GlobalPropsMerger.merge(
      common: ["SPK_version": "spoofed-common-version"],
      container: [:],
      page: [:])
    XCTAssertNil(stableOwned["SPK_version"])

    let pageOwned = GlobalPropsMerger.merge(
      common: ["SPK_version": "spoofed-common-version"],
      container: [:],
      page: ["SPK_version": "page-version"])
    XCTAssertEqual(pageOwned["SPK_version"] as? String, "page-version")
  }

  func testValuesAreUnwrappedBeforeBridgingToSparkling() {
    let merged = GlobalPropsMerger.merge(
      common: ["ordinaryBool": true],
      container: [:],
      page: [:])

    XCTAssertTrue(type(of: merged["ordinaryBool"]!) == Bool.self)
    XCTAssertTrue(type(of: merged["sparklingNavigation"]!) == Bool.self)
    XCTAssertTrue(
      type(of: merged["explorerSupportsExplicitRouteOwnership"]!) == Bool.self)
    XCTAssertTrue(
      type(of: merged["explorerSupportsSparklingContainer"]!) == Bool.self)
  }
}
