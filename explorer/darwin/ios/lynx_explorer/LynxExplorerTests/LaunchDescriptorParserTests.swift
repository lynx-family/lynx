// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import XCTest

@testable import LynxExplorer

final class LaunchDescriptorParserTests: XCTestCase {
  private let recorderPrefix = "file://lynxrecorder?"

  private var parser: LaunchDescriptorParser {
    LaunchDescriptorParser(
      recorderURLPrefix: recorderPrefix,
      maximumWrapperDepth: 2,
      sparklingSchemeResolver: TestSparklingSchemeResolver())
  }

  func testAppearanceAndNavigationDefaultsAreSemantic() throws {
    let descriptor = try parser.parse(
      "https://example.com/home.lynx.bundle",
      requestedContainer: .automatic,
      source: .manualInput
    )

    XCTAssertEqual(
      descriptor.appearance,
      AppearanceOptions(backgroundColor: nil, transparentStatusBar: false)
    )
    XCTAssertEqual(
      descriptor.navigation,
      NavigationOptions(
        navigationHidden: false,
        fullScreen: false,
        title: nil,
        titleColor: nil,
        barColor: nil,
        backButtonStyle: nil,
        orientation: nil
      )
    )
  }

  func testRawHTTPAndHTTPSRespectRequestedContainer() throws {
    let automatic = try parser.parse(
      "http://example.com/home.lynx.bundle?title=HTTP",
      requestedContainer: .automatic,
      source: .manualInput
    )
    let legacy = try parser.parse(
      "https://example.com/home.lynx.bundle",
      requestedContainer: .legacy,
      source: .scanner
    )
    let sparkling = try parser.parse(
      "https://example.com/home.lynx.bundle",
      requestedContainer: .sparkling,
      source: .manualInput
    )

    XCTAssertEqual(
      automatic.resource, .remote(URL(string: "http://example.com/home.lynx.bundle?title=HTTP")!))
    XCTAssertEqual(automatic.container, .legacy)
    XCTAssertEqual(automatic.source, .manualInput)
    XCTAssertEqual(legacy.container, .legacy)
    XCTAssertEqual(legacy.source, .scanner)
    XCTAssertEqual(sparkling.container, .sparkling)
  }

  func testLocalResourcePreservesNestedPathAndQueryItems() throws {
    let descriptor = try parser.parse(
      "file://lynx?local://examples/navigation/child.lynx.bundle?title=Child&animated=0",
      requestedContainer: .automatic,
      source: .manualInput
    )

    XCTAssertEqual(descriptor.resource, .localBundle("examples/navigation/child.lynx.bundle"))
    XCTAssertEqual(descriptor.container, .legacy)
    XCTAssertEqual(
      descriptor.queryItems,
      [
        LaunchQueryItem(name: "title", value: "Child"),
        LaunchQueryItem(name: "animated", value: "0"),
      ]
    )
    XCTAssertFalse(descriptor.presentation.animated)
  }

  func testRawAndLocalResourcesCanExplicitlyUseSparkling() throws {
    let local = try parser.parse(
      "file://lynx?local://homepage.lynx.bundle",
      requestedContainer: .sparkling,
      source: .manualInput
    )
    let remote = try parser.parse(
      "https://example.com/home.lynx.bundle",
      requestedContainer: .sparkling,
      source: .manualInput
    )

    XCTAssertEqual(local.container, .sparkling)
    XCTAssertEqual(remote.container, .sparkling)
    XCTAssertNil(local.canonicalSparklingScheme)
    XCTAssertNil(remote.canonicalSparklingScheme)
  }

  func testRecorderUsesLegacyForAutomaticAndLegacyRequests() throws {
    let input = "file://lynxrecorder?record_path=session.json&replay=1"
    let automatic = try parser.parse(
      input,
      requestedContainer: .automatic,
      source: .recorder
    )
    let legacy = try parser.parse(
      input,
      requestedContainer: .legacy,
      source: .recorder
    )

    XCTAssertEqual(automatic.resource, .recorder(input))
    XCTAssertEqual(automatic.container, .legacy)
    XCTAssertEqual(legacy.container, .legacy)
    XCTAssertEqual(automatic.extras["record_path"], "session.json")
  }

  func testRecorderRejectsExplicitSparkling() {
    assertRouteError(.recorderUnsupportedInSparkling) {
      _ = try parser.parse(
        "file://lynxrecorder?record_path=session.json",
        requestedContainer: .sparkling,
        source: .manualInput
      )
    }
  }

  func testCanonicalBundleForcesSparklingEvenWhenLegacyWasRequested() throws {
    let input = "hybrid://lynxview_page?bundle=nav-basic.lynx.bundle&title=Child"
    let descriptor = try parser.parse(
      input,
      requestedContainer: .legacy,
      source: .manualInput
    )

    XCTAssertEqual(descriptor.requestedContainer, .legacy)
    XCTAssertEqual(descriptor.container, .sparkling)
    XCTAssertEqual(descriptor.resource, .localBundle("nav-basic.lynx.bundle"))
    XCTAssertEqual(descriptor.pageGlobalProps["title"] as? String, "Child")
    XCTAssertEqual(descriptor.canonicalSparklingScheme, input)
  }

  func testCanonicalAppearanceAliasesAndPageNameBecomeTypedSemantics() throws {
    let input = [
      "hybrid://lynxview_page?bundle=home.lynx.bundle",
      "hidden_nav=0",
      "hide_nav_bar=1",
      "bar_color=%23111111",
      "nav_bar_color=%23222222",
      "hide_status_bar=true",
      "initial_page=details",
    ].joined(separator: "&")

    let descriptor = try parser.parse(
      input,
      requestedContainer: .automatic,
      source: .manualInput)

    XCTAssertTrue(descriptor.navigation.navigationHidden)
    XCTAssertEqual(descriptor.navigation.barColor, "#222222")
    XCTAssertTrue(descriptor.appearance.statusBarHidden)
    XCTAssertEqual(descriptor.pageName, "details")
  }

  func testCanonicalNamesTakePrecedenceOverLegacyAliases() throws {
    let descriptor = try parser.parse(
      "hybrid://lynxview_page?bundle=home.lynx.bundle"
        + "&hide_nav_bar=0&hidden_nav=1"
        + "&nav_bar_color=%23222222&bar_color=%23111111",
      requestedContainer: .automatic,
      source: .manualInput)

    XCTAssertFalse(descriptor.navigation.navigationHidden)
    XCTAssertEqual(descriptor.navigation.barColor, "#222222")
  }

  func testEverySupportedCanonicalPageHostForcesSparkling() throws {
    for host in ["lynxview", "lynxview_page"] {
      let descriptor = try parser.parse(
        "hybrid://\(host)?bundle=home.lynx.bundle",
        requestedContainer: .automatic,
        source: .manualInput
      )
      XCTAssertEqual(descriptor.container, .sparkling)
    }
  }

  func testCanonicalURLStyleDecodesRemoteTargetOnce() throws {
    let input =
      "hybrid://lynxview?url=https%3A%2F%2Fexample.com%2Fpage.lynx.bundle%3Fa%3D1%2525&title=Remote"
    let descriptor = try parser.parse(
      input,
      requestedContainer: .automatic,
      source: .externalURL
    )

    XCTAssertEqual(
      descriptor.resource,
      .remote(URL(string: "https://example.com/page.lynx.bundle?a=1%25")!)
    )
    XCTAssertEqual(descriptor.container, .sparkling)
    XCTAssertEqual(descriptor.canonicalSparklingScheme, input)
  }

  func testCanonicalMixedResourceTargetsAreAmbiguous() {
    let input =
      "hybrid://lynxview_page?bundle=local.lynx.bundle&url=https%3A%2F%2Fexample.com%2Fremote.lynx.bundle"

    assertRouteError(.ambiguousTarget("bundle or url")) {
      _ = try parser.parse(
        input,
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
  }

  func testCanonicalMixedResourceTargetKeysAreAmbiguousEvenWhenOneIsEmpty() {
    for input in [
      "hybrid://lynxview_page?bundle=&url=https%3A%2F%2Fexample.com%2Fremote.lynx.bundle",
      "hybrid://lynxview_page?bundle=local.lynx.bundle&url=",
    ] {
      assertRouteError(.ambiguousTarget("bundle or url")) {
        _ = try parser.parse(
          input,
          requestedContainer: .automatic,
          source: .manualInput
        )
      }
    }
  }

  func testEncodedWrapperIsUnwrappedAndRetainsOuterOriginalInput() throws {
    let canonical = "hybrid://lynxview_page?bundle=nav-basic.lynx.bundle&title=Child"
    let wrapper = "lynx://open?url=\(percentEncoded(canonical))"
    let descriptor = try parser.parse(
      wrapper,
      requestedContainer: .automatic,
      source: .externalURL
    )

    XCTAssertEqual(descriptor.originalInput, wrapper)
    XCTAssertEqual(descriptor.canonicalSparklingScheme, canonical)
    XCTAssertEqual(descriptor.resource, .localBundle("nav-basic.lynx.bundle"))
    XCTAssertEqual(descriptor.container, .sparkling)
  }

  func testUnescapedLegacyWrapperPreservesTheNestedURLTail() throws {
    let target =
      "https://example.com/page.lynx.bundle?title=Nested&fullscreen=true"
    let wrapper = "lynx://open?url=\(target)"

    let descriptor = try parser.parse(
      wrapper,
      requestedContainer: .automatic,
      source: .externalURL)

    XCTAssertEqual(descriptor.originalInput, wrapper)
    XCTAssertEqual(descriptor.resource, .remote(URL(string: target)!))
    XCTAssertEqual(descriptor.navigation.title, "Nested")
    XCTAssertTrue(descriptor.navigation.fullScreen)
    XCTAssertEqual(
      descriptor.queryItems,
      [
        LaunchQueryItem(name: "title", value: "Nested"),
        LaunchQueryItem(name: "fullscreen", value: "true"),
      ])
  }

  func testNestedWrappersAreReclassifiedAtEachBoundedLevel() throws {
    let target = "https://example.com/home.lynx.bundle?title=Nested"
    let first = "lynx://open?url=\(percentEncoded(target))"
    let second = "lynx://open?url=\(percentEncoded(first))"
    let descriptor = try parser.parse(
      second,
      requestedContainer: .legacy,
      source: .externalURL
    )

    XCTAssertEqual(descriptor.originalInput, second)
    XCTAssertEqual(descriptor.resource, .remote(URL(string: target)!))
    XCTAssertEqual(descriptor.container, .legacy)
  }

  func testWrapperDepthIsBounded() {
    let parser = LaunchDescriptorParser(
      recorderURLPrefix: recorderPrefix,
      maximumWrapperDepth: 1,
      sparklingSchemeResolver: TestSparklingSchemeResolver())
    let target = "https://example.com/home.lynx.bundle"
    let first = "lynx://open?url=\(percentEncoded(target))"
    let second = "lynx://open?url=\(percentEncoded(first))"

    assertRouteError(.wrapperDepthExceeded(1)) {
      _ = try parser.parse(second, requestedContainer: .automatic, source: .externalURL)
    }
  }

  func testWrapperRejectsMissingEmptyAndDuplicateTargets() {
    assertRouteError(.missingTarget("url")) {
      _ = try parser.parse("lynx://open", requestedContainer: .automatic, source: .externalURL)
    }
    assertRouteError(.missingTarget("url")) {
      _ = try parser.parse("lynx://open?url=", requestedContainer: .automatic, source: .externalURL)
    }
    assertRouteError(.ambiguousTarget("url")) {
      _ = try parser.parse(
        "lynx://open?url=https%3A%2F%2Fa.example&url=https%3A%2F%2Fb.example",
        requestedContainer: .automatic,
        source: .externalURL
      )
    }
  }

  func testLocalQueryPercentDecodesAmpersandEqualsAndPercentExactlyOnce() throws {
    let descriptor = try parser.parse(
      "file://lynx?local://nested/path/home.lynx.bundle?token=a%26b%3Dc&period=100%25&still_encoded=%2525",
      requestedContainer: .automatic,
      source: .manualInput
    )

    XCTAssertEqual(descriptor.resource, .localBundle("nested/path/home.lynx.bundle"))
    XCTAssertEqual(descriptor.extras["token"], "a&b=c")
    XCTAssertEqual(descriptor.extras["period"], "100%")
    XCTAssertEqual(descriptor.extras["still_encoded"], "%25")
  }

  func testDuplicateOrdinaryKeysRemainOrderedAndUseLastValueForConvenienceViews() throws {
    let descriptor = try parser.parse(
      "https://example.com/home.lynx.bundle?tag=first&tag=second&empty=&flag",
      requestedContainer: .automatic,
      source: .manualInput
    )

    XCTAssertEqual(
      descriptor.queryItems,
      [
        LaunchQueryItem(name: "tag", value: "first"),
        LaunchQueryItem(name: "tag", value: "second"),
        LaunchQueryItem(name: "empty", value: ""),
        LaunchQueryItem(name: "flag", value: nil),
      ]
    )
    XCTAssertEqual(descriptor.extras["tag"], "second")
    XCTAssertEqual(descriptor.extras["empty"], "")
    XCTAssertNil(descriptor.extras["flag"])
    XCTAssertEqual(descriptor.pageGlobalProps["tag"] as? String, "second")
  }

  func testCanonicalResourceKeysRejectDuplicates() {
    assertRouteError(.ambiguousTarget("bundle")) {
      _ = try parser.parse(
        "hybrid://lynxview_page?bundle=a.lynx.bundle&bundle=b.lynx.bundle",
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
    assertRouteError(.ambiguousTarget("url")) {
      _ = try parser.parse(
        "hybrid://lynxview_page?url=https%3A%2F%2Fa.example&url=https%3A%2F%2Fb.example",
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
  }

  func testCompleteLegacyOptionMapping() throws {
    let input = [
      "https://example.com/home.lynx.bundle?animated=yes",
      "hidden_nav=false",
      "fullscreen=1",
      "title=Raw%20Title",
      "title_color=%23AABBCC",
      "bar_color=%23010203",
      "back_button_style=dark",
      "container_bg_color=%23112233",
      "trans_status_bar=false",
      "width=1170",
      "height=2532",
      "orientation=landscape",
      "enable_napi_addon=true",
      "initial_page=details",
    ].joined(separator: "&")
    let descriptor = try parser.parse(
      input,
      requestedContainer: .automatic,
      source: .manualInput
    )

    XCTAssertTrue(descriptor.presentation.animated)
    XCTAssertTrue(descriptor.navigation.fullScreen)
    XCTAssertTrue(descriptor.navigation.navigationHidden, "fullscreen must force hidden navigation")
    XCTAssertEqual(descriptor.navigation.title, "Raw Title")
    XCTAssertEqual(descriptor.navigation.titleColor, "#AABBCC")
    XCTAssertEqual(descriptor.navigation.barColor, "#010203")
    XCTAssertEqual(descriptor.navigation.backButtonStyle, "dark")
    XCTAssertEqual(descriptor.navigation.orientation, .landscape)
    XCTAssertEqual(descriptor.appearance.backgroundColor, "#112233")
    XCTAssertTrue(
      descriptor.appearance.transparentStatusBar,
      "fullscreen must force transparent status-bar appearance"
    )
    XCTAssertEqual(descriptor.viewport, ViewportOptions(widthInPixels: 1170, heightInPixels: 2532))
    XCTAssertTrue(descriptor.debugOptions.enableNAPIAddon)
    XCTAssertEqual(descriptor.extras["initial_page"], "details")
    XCTAssertEqual(descriptor.pageGlobalProps["initialPage"] as? String, "details")
    XCTAssertEqual(descriptor.pageName, "details")
    XCTAssertTrue(descriptor.commonGlobalProps.isEmpty)
    XCTAssertNil(descriptor.initialData)
  }

  func testViewportUsesLegacyIntegerParsingSemantics() throws {
    let exponent = try parser.parse(
      "https://example.com/home.lynx.bundle?width=1e3&height=2e3",
      requestedContainer: .sparkling,
      source: .manualInput)
    let prefixed = try parser.parse(
      "https://example.com/home.lynx.bundle?width=12px&height=34px",
      requestedContainer: .sparkling,
      source: .manualInput)
    let fractional = try parser.parse(
      "https://example.com/home.lynx.bundle?width=0.5&height=0.5",
      requestedContainer: .sparkling,
      source: .manualInput)

    XCTAssertEqual(
      exponent.viewport,
      ViewportOptions(widthInPixels: 1, heightInPixels: 2))
    XCTAssertEqual(
      prefixed.viewport,
      ViewportOptions(widthInPixels: 12, heightInPixels: 34))
    XCTAssertNil(fractional.viewport)
  }

  func testOrientationUsesLegacyLowerCaseOnlySemantics() throws {
    let descriptor = try parser.parse(
      "https://example.com/home.lynx.bundle?orientation=LANDSCAPE",
      requestedContainer: .legacy,
      source: .manualInput)

    XCTAssertNil(descriptor.navigation.orientation)
    XCTAssertEqual(descriptor.extras["orientation"], "LANDSCAPE")
  }

  func testTruthyAndFalseBooleanCompatibility() throws {
    for value in ["1", "true", "TRUE", "yes", "YES"] {
      let descriptor = try parser.parse(
        "https://example.com/home.lynx.bundle?hidden_nav=\(value)",
        requestedContainer: .automatic,
        source: .manualInput
      )
      XCTAssertTrue(descriptor.navigation.navigationHidden, "expected \(value) to be true")
    }
    for value in ["0", "false", "FALSE", "no", "NO"] {
      let descriptor = try parser.parse(
        "https://example.com/home.lynx.bundle?hidden_nav=\(value)",
        requestedContainer: .automatic,
        source: .manualInput
      )
      XCTAssertFalse(descriptor.navigation.navigationHidden, "expected \(value) to be false")
    }
  }

  func testLegacyInputsPreserveNSStringBooleanCompatibility() throws {
    for value in ["2", "-1", "Y", "T", "1foo"] {
      let descriptor = try parser.parse(
        "https://example.com/home.lynx.bundle?animated=\(value)&hidden_nav=\(value)",
        requestedContainer: .automatic,
        source: .manualInput)

      XCTAssertTrue(descriptor.presentation.animated, value)
      XCTAssertTrue(descriptor.navigation.navigationHidden, value)
    }

    let unknown = try parser.parse(
      "https://example.com/home.lynx.bundle?animated=sometimes&fullscreen=sometimes",
      requestedContainer: .legacy,
      source: .manualInput)
    XCTAssertFalse(unknown.presentation.animated)
    XCTAssertFalse(unknown.navigation.fullScreen)
  }

  func testLegacyBareAndPartialOptionsRemainNonFatal() throws {
    let bare = try parser.parse(
      "https://example.com/home.lynx.bundle?animated&hidden_nav",
      requestedContainer: .automatic,
      source: .manualInput)
    let empty = try parser.parse(
      "https://example.com/home.lynx.bundle?animated=",
      requestedContainer: .legacy,
      source: .manualInput)
    let partialViewport = try parser.parse(
      "https://example.com/home.lynx.bundle?width=100&orientation=diagonal",
      requestedContainer: .legacy,
      source: .manualInput)

    XCTAssertTrue(bare.presentation.animated)
    XCTAssertFalse(bare.navigation.navigationHidden)
    XCTAssertFalse(empty.presentation.animated)
    XCTAssertNil(partialViewport.viewport)
    XCTAssertNil(partialViewport.navigation.orientation)
  }

  func testLegacyBareDuplicatesDoNotEraseTheLastValuedParameter() throws {
    let descriptor = try parser.parse(
      "https://example.com/home.lynx.bundle?title=First&title&animated=0&animated",
      requestedContainer: .automatic,
      source: .manualInput)

    XCTAssertEqual(descriptor.navigation.title, "First")
    XCTAssertFalse(descriptor.presentation.animated)
  }

  func testNAPIAddonKeepsTheShellsNarrowTruthyAllowList() throws {
    let descriptor = try parser.parse(
      "https://example.com/home.lynx.bundle?enable_napi_addon=2",
      requestedContainer: .automatic,
      source: .manualInput)

    XCTAssertFalse(descriptor.debugOptions.enableNAPIAddon)
    XCTAssertEqual(descriptor.extras["enable_napi_addon"], "2")
  }

  func testExplicitSparklingUsesOfficialFoundationBooleanSemantics() throws {
    let descriptor = try parser.parse(
      "https://example.com/home.lynx.bundle?animated=sometimes"
        + "&hidden_nav=sometimes&hide_status_bar=sometimes",
      requestedContainer: .sparkling,
      source: .manualInput)

    XCTAssertFalse(descriptor.presentation.animated)
    XCTAssertFalse(descriptor.navigation.navigationHidden)
    XCTAssertFalse(descriptor.appearance.statusBarHidden)
  }

  func testStatusBarTransparencyUsesCompatibleBooleanValues() throws {
    for value in ["1", "true", "TRUE", "yes", "YES"] {
      let descriptor = try parser.parse(
        "https://example.com/home.lynx.bundle?trans_status_bar=\(value)",
        requestedContainer: .automatic,
        source: .manualInput
      )
      XCTAssertTrue(descriptor.appearance.transparentStatusBar, "expected \(value) to be true")
    }
    for value in ["0", "false", "FALSE", "no", "NO"] {
      let descriptor = try parser.parse(
        "https://example.com/home.lynx.bundle?trans_status_bar=\(value)",
        requestedContainer: .automatic,
        source: .manualInput
      )
      XCTAssertFalse(descriptor.appearance.transparentStatusBar, "expected \(value) to be false")
    }
  }

  func testUnrecognizedSemanticValuesRemainRawAndNonFatal() throws {
    let descriptor = try parser.parse(
      "https://example.com/a?trans_status_bar=opaque"
        + "&width=nan&height=-1&orientation=upside-down",
      requestedContainer: .sparkling,
      source: .manualInput)

    XCTAssertFalse(descriptor.appearance.transparentStatusBar)
    XCTAssertNil(descriptor.viewport)
    XCTAssertNil(descriptor.navigation.orientation)
    XCTAssertEqual(descriptor.extras["trans_status_bar"], "opaque")
    XCTAssertEqual(descriptor.extras["width"], "nan")
    XCTAssertEqual(descriptor.extras["height"], "-1")
    XCTAssertEqual(descriptor.extras["orientation"], "upside-down")
  }

  func testUnknownKeysUseLegacyCamelCaseMappingWithoutCoercingValues() throws {
    let descriptor = try parser.parse(
      "https://example.com/a?__snake__case=value&_count=001&___=kept&mixed_Case=Raw",
      requestedContainer: .automatic,
      source: .manualInput
    )

    XCTAssertEqual(descriptor.pageGlobalProps["snakeCase"] as? String, "value")
    XCTAssertEqual(descriptor.pageGlobalProps["count"] as? String, "001")
    XCTAssertEqual(descriptor.pageGlobalProps["___"] as? String, "kept")
    XCTAssertEqual(descriptor.pageGlobalProps["mixedCase"] as? String, "Raw")
  }

  func testParserHasNoCrossRequestState() throws {
    let first = try parser.parse(
      "https://example.com/first?title=First&fullscreen=true",
      requestedContainer: .automatic,
      source: .manualInput
    )
    let second = try parser.parse(
      "https://example.com/second",
      requestedContainer: .automatic,
      source: .scanner
    )

    XCTAssertEqual(first.navigation.title, "First")
    XCTAssertTrue(first.navigation.fullScreen)
    XCTAssertNil(second.navigation.title)
    XCTAssertFalse(second.navigation.fullScreen)
    XCTAssertNil(second.appearance.backgroundColor)
    XCTAssertFalse(second.appearance.transparentStatusBar)
    XCTAssertTrue(second.queryItems.isEmpty)
    XCTAssertTrue(second.extras.isEmpty)
  }

  func testOriginalInputIsNotReplacedByTrimmedClassificationInput() throws {
    let input = "  https://example.com/home.lynx.bundle?title=Home  "
    let descriptor = try parser.parse(
      input,
      requestedContainer: .automatic,
      source: .manualInput
    )

    XCTAssertEqual(descriptor.originalInput, input)
    XCTAssertEqual(
      descriptor.resource, .remote(URL(string: "https://example.com/home.lynx.bundle?title=Home")!))
  }

  func testMalformedAndUnsupportedInputsHaveSpecificErrors() {
    assertRouteError(.emptyInput) {
      _ = try parser.parse("   ", requestedContainer: .automatic, source: .manualInput)
    }
    assertRouteError(.invalidPercentEncoding("https://example.com/%ZZ")) {
      _ = try parser.parse(
        "https://example.com/%ZZ",
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
    assertRouteError(.invalidPercentEncoding("https://example.com/a?token=%FF")) {
      _ = try parser.parse(
        "https://example.com/a?token=%FF",
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
    assertRouteError(.malformedURL("not a URL")) {
      _ = try parser.parse("not a URL", requestedContainer: .automatic, source: .manualInput)
    }
    assertRouteError(.unsupportedScheme("ftp")) {
      _ = try parser.parse(
        "ftp://example.com/home.lynx.bundle",
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
    assertRouteError(.unsupportedHybridHost("unknown")) {
      _ = try parser.parse(
        "hybrid://unknown?bundle=home.lynx.bundle",
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
    assertRouteError(.missingTarget("bundle or url")) {
      _ = try parser.parse(
        "hybrid://lynxview_page?title=Missing",
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
    assertRouteError(.malformedURL("file://other?local://home.lynx.bundle")) {
      _ = try parser.parse(
        "file://other?local://home.lynx.bundle",
        requestedContainer: .automatic,
        source: .manualInput
      )
    }
  }

  func testRouteErrorsExposeStableActionableCodesAndDescriptions() {
    let error = RouteError.invalidParameter(name: "animated", value: "maybe")

    XCTAssertEqual(error.code, "invalid_parameter")
    XCTAssertTrue(error.localizedDescription.contains("animated"))
    XCTAssertTrue(error.localizedDescription.contains("maybe"))
  }

  private func percentEncoded(_ value: String) -> String {
    value.addingPercentEncoding(withAllowedCharacters: .alphanumerics)!
  }

  private func assertRouteError(
    _ expected: RouteError,
    file: StaticString = #filePath,
    line: UInt = #line,
    _ expression: () throws -> Void
  ) {
    XCTAssertThrowsError(try expression(), file: file, line: line) { error in
      XCTAssertEqual(error as? RouteError, expected, file: file, line: line)
    }
  }
}

private struct TestSparklingSchemeResolver: SparklingSchemeResolving {
  func resolve(_ scheme: String) throws -> SparklingSchemeResolution {
    try SparklingSchemeAdapter { url, outerValues in
      let host = url.host?.lowercased() ?? ""
      let engine: SparklingSchemeEngineKind
      switch host {
      case "lynxview", "lynxview_page", "lynxview_card":
        engine = .lynx
      case "webview":
        engine = .web
      default:
        engine = .unknown
      }

      let container: SparklingSchemeContainerKind
      switch host {
      case "lynxview", "lynxview_page":
        container = .page
      case "lynxview_card":
        container = .card
      default:
        container = .unknown
      }

      var extras: [String: String] = [:]
      if let target = outerValues["url"],
        let targetComponents = URLComponents(string: target)
      {
        for item in targetComponents.queryItems ?? [] {
          if let value = item.value {
            extras[item.name] = value
          }
        }
      }
      extras.merge(outerValues) { _, outer in outer }
      return SparklingSchemeParameters(
        canResolve: outerValues["url"] != nil || outerValues["bundle"] != nil,
        engine: engine,
        container: container,
        url: outerValues["url"],
        extras: extras)
    }.resolve(scheme)
  }
}
