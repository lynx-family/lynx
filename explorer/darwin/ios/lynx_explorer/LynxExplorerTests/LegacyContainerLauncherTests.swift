// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import UIKit
import XCTest

@testable import LynxExplorer

final class LegacyContainerLauncherTests: XCTestCase {
  private let parser = LaunchDescriptorParser(recorderURLPrefix: "file://lynxrecorder?")

  @MainActor
  func testSparklingDescriptorIsRejectedWithoutCreatingLegacyController() throws {
    let descriptor = try parser.parse(
      "https://example.com/page.lynx.bundle",
      requestedContainer: .sparkling,
      source: .nativeModule)

    XCTAssertThrowsError(
      try LegacyContainerLauncher().makeViewController(for: descriptor)
    ) { error in
      XCTAssertEqual(
        error as? RouteError,
        .containerMismatch(expected: .legacy, actual: .sparkling))
    }
  }

  @MainActor
  func testRemoteLaunchCopiesAllLegacyOptionsWithoutSparklingCapabilities() throws {
    let descriptor = try parser.parse(
      "https://example.com/page.lynx.bundle?animated=0&width=750&height=1334"
        + "&orientation=landscape&hidden_nav=1&fullscreen=1&title=First"
        + "&title_color=%23ffffff&bar_color=%23000000&back_button_style=dark"
        + "&container_bg_color=%23123456&trans_status_bar=1&enable_napi_addon=1",
      requestedContainer: .automatic,
      source: .nativeModule)

    let viewController = try LegacyContainerLauncher().makeViewController(for: descriptor)
    let params = try XCTUnwrap(viewController.value(forKey: "params") as? NSDictionary)

    XCTAssertEqual(
      viewController.value(forKey: "url") as? String,
      descriptor.originalInput)
    XCTAssertEqual(params["animated"] as? String, "0")
    XCTAssertEqual(params["width"] as? String, "750")
    XCTAssertEqual(params["height"] as? String, "1334")
    XCTAssertEqual(params["orientation"] as? String, "landscape")
    XCTAssertEqual(params["hidden_nav"] as? String, "1")
    XCTAssertEqual(params["fullscreen"] as? String, "1")
    XCTAssertEqual(params["title"] as? String, "First")
    XCTAssertEqual(params["title_color"] as? String, "#ffffff")
    XCTAssertEqual(params["bar_color"] as? String, "#000000")
    XCTAssertEqual(params["back_button_style"] as? String, "dark")
    XCTAssertEqual(params["container_bg_color"] as? String, "#123456")
    XCTAssertEqual(params["trans_status_bar"] as? String, "1")
    XCTAssertEqual(params["enable_napi_addon"] as? String, "1")
    XCTAssertNil(params["containerID"])
    XCTAssertNil(params["containerType"])
    XCTAssertNil(params["sparklingNavigation"])
    XCTAssertNil(params["sparklingAvailable"])
  }

  @MainActor
  func testCapabilityAliasesCannotSpoofSparklingGlobalProps() throws {
    let descriptor = try parser.parse(
      "https://example.com/page.lynx.bundle"
        + "?containerType=sparkling&container_type=sparkling"
        + "&containerID=fake&container_id=fake"
        + "&sparklingNavigation=true&sparkling_navigation=true"
        + "&sparklingAvailable=true&sparkling_available=true"
        + "&explorerSupportsExplicitRouteOwnership=false"
        + "&explorer_supports_explicit_route_ownership=false"
        + "&explorerSupportsSparklingContainer=false"
        + "&explorer_supports_sparkling_container=false"
        + "&spkPipe=fake&spk_pipe=fake&spk_containerID=fake",
      requestedContainer: .automatic,
      source: .nativeModule)

    let viewController = try LegacyContainerLauncher().makeViewController(for: descriptor)
    let params = try XCTUnwrap(viewController.value(forKey: "params") as? NSDictionary)

    for key in descriptor.extras.keys {
      XCTAssertNil(params[key], "Legacy parameters must strip reserved capability key \(key)")
    }
  }

  @MainActor
  func testTypedOptionsFillParametersWhenDescriptorExtrasAreEmpty() throws {
    let descriptor = LaunchDescriptor(
      originalInput: "https://example.com/typed.lynx.bundle",
      resource: .remote(try XCTUnwrap(URL(string: "https://example.com/typed.lynx.bundle"))),
      initialData: nil,
      commonGlobalProps: [:],
      pageGlobalProps: [:],
      pageName: nil,
      viewport: ViewportOptions(widthInPixels: 640, heightInPixels: 1136),
      appearance: AppearanceOptions(
        backgroundColor: "#123456",
        transparentStatusBar: true),
      navigation: NavigationOptions(
        navigationHidden: true,
        fullScreen: true,
        title: "Typed",
        titleColor: "#ffffff",
        barColor: "#000000",
        backButtonStyle: "dark",
        orientation: .landscape),
      presentation: PresentationOptions(animated: false),
      debugOptions: DebugOptions(enableNAPIAddon: true),
      queryItems: [],
      extras: [:],
      requestedContainer: .legacy,
      container: .legacy,
      source: .nativeModule,
      canonicalSparklingScheme: nil)

    let viewController = try LegacyContainerLauncher().makeViewController(for: descriptor)
    let params = try XCTUnwrap(viewController.value(forKey: "params") as? NSDictionary)

    XCTAssertEqual(params["animated"] as? Bool, false)
    XCTAssertEqual(params["width"] as? Double, 640)
    XCTAssertEqual(params["height"] as? Double, 1136)
    XCTAssertEqual(params["orientation"] as? String, "landscape")
    XCTAssertEqual(params["hidden_nav"] as? Bool, true)
    XCTAssertEqual(params["fullscreen"] as? Bool, true)
    XCTAssertEqual(params["title"] as? String, "Typed")
    XCTAssertEqual(params["title_color"] as? String, "#ffffff")
    XCTAssertEqual(params["bar_color"] as? String, "#000000")
    XCTAssertEqual(params["back_button_style"] as? String, "dark")
    XCTAssertEqual(params["container_bg_color"] as? String, "#123456")
    XCTAssertEqual(params["trans_status_bar"] as? Bool, true)
    XCTAssertEqual(params["enable_napi_addon"] as? Bool, true)
  }

  @MainActor
  func testPageNameFillsLegacyInitialPageParameter() throws {
    let descriptor = makeDescriptor(pageName: "details")

    let viewController = try LegacyContainerLauncher().makeViewController(for: descriptor)
    let params = try XCTUnwrap(viewController.value(forKey: "params") as? NSDictionary)

    XCTAssertEqual(params["initial_page"] as? String, "details")
  }

  @MainActor
  func testPageNameDoesNotOverrideRawLegacyInitialPageParameter() throws {
    let descriptor = makeDescriptor(
      pageName: "typed-page",
      extras: ["initial_page": "raw-page"])

    let viewController = try LegacyContainerLauncher().makeViewController(for: descriptor)
    let params = try XCTUnwrap(viewController.value(forKey: "params") as? NSDictionary)

    XCTAssertEqual(params["initial_page"] as? String, "raw-page")
  }

  @MainActor
  func testSemanticInitialDataAndGlobalPropLayersAreForwardedToLegacyShell() throws {
    let initialData = try JSONSerialization.data(withJSONObject: ["answer": 42])
    let descriptor = makeDescriptor(
      initialData: initialData,
      commonGlobalProps: [
        "theme": "common",
        "commonOnly": "common-value",
      ],
      pageGlobalProps: [
        "theme": "page",
        "pageOnly": "page-value",
      ],
      extras: ["theme": "parameter"])

    let viewController = try XCTUnwrap(
      try LegacyContainerLauncher().makeViewController(for: descriptor)
        as? LynxViewShellViewController)

    XCTAssertEqual(viewController.launchInitialData?["answer"] as? Int, 42)
    XCTAssertEqual(viewController.commonGlobalProps?["theme"] as? String, "common")
    XCTAssertEqual(
      viewController.commonGlobalProps?["commonOnly"] as? String,
      "common-value")
    XCTAssertEqual(viewController.pageGlobalProps?["theme"] as? String, "page")
    XCTAssertEqual(viewController.pageGlobalProps?["pageOnly"] as? String, "page-value")
  }

  @MainActor
  func testSemanticGlobalPropLayersStripEveryReservedCapabilityAlias() throws {
    let reserved: [String: AnyHashable] = [
      "containerType": "sparkling",
      "container_id": "fake",
      "sparkling-navigation": true,
      "sparkling_available": true,
      "explorerSupportsExplicitRouteOwnership": false,
      "explorerSupportsSparklingContainer": false,
      "spk_pipe": "fake",
      "spk-containerID": "fake",
    ]
    let descriptor = makeDescriptor(
      commonGlobalProps: reserved,
      pageGlobalProps: reserved)

    let viewController = try XCTUnwrap(
      try LegacyContainerLauncher().makeViewController(for: descriptor)
        as? LynxViewShellViewController)

    XCTAssertTrue(viewController.commonGlobalProps?.isEmpty == true)
    XCTAssertTrue(viewController.pageGlobalProps?.isEmpty == true)
  }

  @MainActor
  func testInvalidSemanticInitialDataIsRejectedBeforeCreatingLegacyShell() throws {
    let invalidJSON = makeDescriptor(initialData: Data("not-json".utf8))
    let arrayJSON = makeDescriptor(
      initialData: try JSONSerialization.data(withJSONObject: ["not", "an", "object"]))
    let launcher = LegacyContainerLauncher()

    XCTAssertThrowsError(try launcher.makeViewController(for: invalidJSON)) { error in
      XCTAssertEqual(
        error as? RouteError,
        .invalidParameter(name: "initial_data", value: "invalid JSON"))
    }
    XCTAssertThrowsError(try launcher.makeViewController(for: arrayJSON)) { error in
      XCTAssertEqual(
        error as? RouteError,
        .invalidParameter(name: "initial_data", value: "expected JSON object"))
    }
  }

  @MainActor
  func testBackToBackLaunchesHaveIndependentParameters() throws {
    let first = try parser.parse(
      "https://example.com/first.lynx.bundle?title=First",
      requestedContainer: .automatic,
      source: .nativeModule)
    let second = try parser.parse(
      "https://example.com/second.lynx.bundle?title=Second",
      requestedContainer: .automatic,
      source: .nativeModule)
    let launcher = LegacyContainerLauncher()

    let firstController = try launcher.makeViewController(for: first)
    let secondController = try launcher.makeViewController(for: second)
    let firstParams = try XCTUnwrap(firstController.value(forKey: "params") as? NSDictionary)
    let secondParams = try XCTUnwrap(secondController.value(forKey: "params") as? NSDictionary)

    XCTAssertFalse(firstParams === secondParams)
    XCTAssertEqual(firstParams["title"] as? String, "First")
    XCTAssertEqual(secondParams["title"] as? String, "Second")
  }

  @MainActor
  func testLocalBundleIsResolvedByExistingLegacyResourceProvider() throws {
    let descriptor = try parser.parse(
      "file://lynx?local://homepage.lynx.bundle?title=Home",
      requestedContainer: .automatic,
      source: .manualInput)

    let viewController = try LegacyContainerLauncher().makeViewController(for: descriptor)

    XCTAssertTrue(
      (viewController.value(forKey: "url") as? String)?.hasSuffix(
        "/Resource/homepage.lynx.bundle") == true)
    XCTAssertNotNil(viewController.value(forKey: "data") as? Data)
    let params = try XCTUnwrap(viewController.value(forKey: "params") as? NSDictionary)
    XCTAssertEqual(params["title"] as? String, "Home")
  }

  @MainActor
  func testRecorderDescriptorCreatesRecorderControllerWithoutNavigationMutation() throws {
    let input = "file://lynxrecorder?session=sample"
    let descriptor = try parser.parse(
      input,
      requestedContainer: .automatic,
      source: .recorder)

    let viewController = try LegacyContainerLauncher().makeViewController(for: descriptor)

    XCTAssertEqual(NSStringFromClass(type(of: viewController)), "LynxRecorderViewController")
    XCTAssertEqual(viewController.value(forKey: "url") as? String, input)
    XCTAssertNil(viewController.navigationController)
  }

  private func makeDescriptor(
    initialData: Data? = nil,
    commonGlobalProps: [String: AnyHashable] = [:],
    pageGlobalProps: [String: AnyHashable] = [:],
    pageName: String? = nil,
    extras: [String: String] = [:]
  ) -> LaunchDescriptor {
    LaunchDescriptor(
      originalInput: "https://example.com/semantic.lynx.bundle",
      resource: .remote(URL(string: "https://example.com/semantic.lynx.bundle")!),
      initialData: initialData,
      commonGlobalProps: commonGlobalProps,
      pageGlobalProps: pageGlobalProps,
      pageName: pageName,
      viewport: nil,
      appearance: AppearanceOptions(
        backgroundColor: nil,
        transparentStatusBar: false),
      navigation: NavigationOptions(
        navigationHidden: false,
        fullScreen: false,
        title: nil,
        titleColor: nil,
        barColor: nil,
        backButtonStyle: nil,
        orientation: nil),
      presentation: PresentationOptions(animated: true),
      debugOptions: DebugOptions(enableNAPIAddon: false),
      queryItems: [],
      extras: extras,
      requestedContainer: .legacy,
      container: .legacy,
      source: .nativeModule,
      canonicalSparklingScheme: nil)
  }
}
