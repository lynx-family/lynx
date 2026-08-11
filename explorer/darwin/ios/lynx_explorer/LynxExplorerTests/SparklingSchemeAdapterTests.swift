// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore futureview

import XCTest

@testable import LynxExplorer

final class SparklingSchemeAdapterTests: XCTestCase {
  func testBundlePageProducesSemanticResolutionAndPreservesOuterQueryItems() throws {
    let input =
      "hybrid://lynxview_page?bundle=home.lynx.bundle&title=Explorer&tag=first&tag=second"
    let adapter = makeAdapter(
      extras: [
        "bundle": "home.lynx.bundle",
        "title": "Explorer",
        "tag": "second",
      ])

    let resolution = try adapter.resolve(input)

    XCTAssertEqual(resolution.resource, .localBundle("home.lynx.bundle"))
    XCTAssertEqual(resolution.canonicalScheme, input)
    XCTAssertEqual(
      resolution.outerQueryItems,
      [
        LaunchQueryItem(name: "bundle", value: "home.lynx.bundle"),
        LaunchQueryItem(name: "title", value: "Explorer"),
        LaunchQueryItem(name: "tag", value: "first"),
        LaunchQueryItem(name: "tag", value: "second"),
      ])
    XCTAssertEqual(resolution.normalizedExtras["tag"], "second")
  }

  func testURLPageUsesOfficialTargetAndOuterValuesOverrideNormalizedInnerExtras() throws {
    let target = "https://example.com/page.lynx.bundle?inner=base%20value&shared=inside"
    let input =
      "hybrid://lynxview?url=\(percentEncoded(target))&shared=outside&title=Remote"
    let adapter = makeAdapter(
      url: target,
      extras: [
        "url": target,
        "inner": "base value",
        "shared": "inside",
      ])

    let resolution = try adapter.resolve(input)

    XCTAssertEqual(resolution.resource, .remote(URL(string: target)!))
    XCTAssertEqual(resolution.normalizedExtras["inner"], "base value")
    XCTAssertEqual(resolution.normalizedExtras["shared"], "outside")
    XCTAssertEqual(resolution.normalizedExtras["title"], "Remote")
    XCTAssertEqual(resolution.canonicalScheme, input)
  }

  func testDuplicateAndMixedTargetsFailBeforeOfficialResolution() {
    var callCount = 0
    let adapter = SparklingSchemeAdapter { _, _ in
      callCount += 1
      return self.parameters(extras: ["bundle": "unused.lynx.bundle"])
    }

    assertRouteError(.ambiguousTarget("bundle")) {
      _ = try adapter.resolve(
        "hybrid://lynxview_page?bundle=first.lynx.bundle&bundle=second.lynx.bundle")
    }
    assertRouteError(.ambiguousTarget("url")) {
      _ = try adapter.resolve(
        "hybrid://lynxview_page?url=&url=https%3A%2F%2Fexample.com%2Fpage.lynx.bundle")
    }
    assertRouteError(.ambiguousTarget("bundle or url")) {
      _ = try adapter.resolve(
        "hybrid://lynxview_page?bundle=&url=https%3A%2F%2Fexample.com%2Fpage.lynx.bundle")
    }
    XCTAssertEqual(callCount, 0)
  }

  func testMissingAndEmptyTargetsAreTypedErrors() {
    let adapter = makeAdapter(extras: [:])

    assertRouteError(.missingTarget("bundle or url")) {
      _ = try adapter.resolve("hybrid://lynxview_page?title=Missing")
    }
    assertRouteError(.missingTarget("bundle")) {
      _ = try adapter.resolve("hybrid://lynxview_page?bundle=")
    }
    assertRouteError(.missingTarget("url")) {
      _ = try adapter.resolve("hybrid://lynxview_page?url=")
    }
  }

  func testNonHybridAndUnresolvableInputsAreMalformedSparklingSchemes() {
    let nonHybrid = "https://example.com/page.lynx.bundle"
    assertRouteError(.malformedSparklingScheme(nonHybrid)) {
      _ = try makeAdapter(extras: [:]).resolve(nonHybrid)
    }

    let unresolvable = "hybrid://lynxview_page?bundle=home.lynx.bundle"
    assertRouteError(.malformedSparklingScheme(unresolvable)) {
      _ = try makeAdapter(canResolve: false, extras: [:]).resolve(unresolvable)
    }
  }

  func testWebCardAndUnknownContainersAreExplicitlyUnsupported() {
    let web = "hybrid://webview?url=https%3A%2F%2Fexample.com"
    assertRouteError(.unsupportedHybridHost("webview")) {
      _ = try makeAdapter(
        engine: .web,
        container: .unknown,
        url: "https://example.com",
        extras: ["url": "https://example.com"]
      ).resolve(web)
    }

    let card = "hybrid://lynxview_card?bundle=card.lynx.bundle"
    assertRouteError(.unsupportedHybridHost("lynxview_card")) {
      _ = try makeAdapter(
        container: .card,
        extras: ["bundle": "card.lynx.bundle"]
      ).resolve(card)
    }

    let unknown = "hybrid://futureview?bundle=future.lynx.bundle"
    assertRouteError(.unsupportedHybridHost("futureview")) {
      _ = try makeAdapter(
        engine: .unknown,
        container: .unknown,
        extras: ["bundle": "future.lynx.bundle"]
      ).resolve(unknown)
    }
  }

  func testOfficialTargetsMustRemainValidAfterResolution() {
    let invalidRemote = "hybrid://lynxview_page?url=https%3A%2F%2Fexample.com%2Fpage"
    assertRouteError(.malformedSparklingScheme(invalidRemote)) {
      _ = try makeAdapter(
        url: "ftp://example.com/page",
        extras: ["url": "ftp://example.com/page"]
      ).resolve(invalidRemote)
    }

    let missingBundle = "hybrid://lynxview_page?bundle=page.lynx.bundle"
    assertRouteError(.malformedSparklingScheme(missingBundle)) {
      _ = try makeAdapter(extras: [:]).resolve(missingBundle)
    }
  }

  func testParserUsesNormalizedExtrasWithoutReplacingOrderedOuterQueryItems() throws {
    let input =
      "hybrid://lynxview_page?bundle=page.lynx.bundle&shared=outer&tag=first&tag=second"
    let resolution = SparklingSchemeResolution(
      resource: .localBundle("page.lynx.bundle"),
      canonicalScheme: input,
      outerQueryItems: [
        LaunchQueryItem(name: "bundle", value: "page.lynx.bundle"),
        LaunchQueryItem(name: "shared", value: "outer"),
        LaunchQueryItem(name: "tag", value: "first"),
        LaunchQueryItem(name: "tag", value: "second"),
      ],
      normalizedExtras: [
        "bundle": "page.lynx.bundle",
        "innerOnly": "from-target",
        "shared": "outer",
        "tag": "second",
      ])
    let parser = LaunchDescriptorParser(
      recorderURLPrefix: "file://lynxrecorder?",
      sparklingSchemeResolver: StubSparklingSchemeResolver(resolution: resolution))

    let descriptor = try parser.parse(
      input,
      requestedContainer: .legacy,
      source: .externalURL)

    XCTAssertEqual(descriptor.container, .sparkling)
    XCTAssertEqual(descriptor.queryItems, resolution.outerQueryItems)
    XCTAssertEqual(descriptor.pageGlobalProps["innerOnly"] as? String, "from-target")
    XCTAssertEqual(descriptor.pageGlobalProps["shared"] as? String, "outer")
    XCTAssertEqual(descriptor.pageGlobalProps["tag"] as? String, "second")
    XCTAssertEqual(descriptor.extras["innerOnly"], "from-target")
  }

  #if canImport(Sparkling)
    func testProductionAdapterUsesOfficialBundleAndURLParsers() throws {
      let adapter = SparklingSchemeAdapter()
      let bundle = try adapter.resolve(
        "hybrid://lynxview_page?bundle=home.lynx.bundle&title=Bundle")
      let target = "https://example.com/page.lynx.bundle?inner=value"
      let remote = try adapter.resolve(
        "hybrid://lynxview?url=\(percentEncoded(target))&title=Remote")

      XCTAssertEqual(bundle.resource, .localBundle("home.lynx.bundle"))
      XCTAssertEqual(bundle.normalizedExtras["title"], "Bundle")
      XCTAssertEqual(remote.resource, .remote(URL(string: target)!))
      XCTAssertEqual(remote.normalizedExtras["inner"], "value")
      XCTAssertEqual(remote.normalizedExtras["title"], "Remote")
    }

    func testProductionAdapterRejectsOfficialNonPageSchemes() {
      assertRouteError(.unsupportedHybridHost("lynxview_card")) {
        _ = try SparklingSchemeAdapter().resolve(
          "hybrid://lynxview_card?bundle=card.lynx.bundle")
      }
      assertRouteError(.unsupportedHybridHost("webview")) {
        _ = try SparklingSchemeAdapter().resolve(
          "hybrid://webview?url=https%3A%2F%2Fexample.com")
      }
    }
  #else
    func testProductionAdapterMakesEveryHybridCandidateUnavailableBeforeGrammarValidation() {
      let parser = LaunchDescriptorParser(recorderURLPrefix: "file://lynxrecorder?")

      for input in [
        "hybrid://lynxview_page?bundle=home.lynx.bundle",
        "hybrid://lynxview_page",
        "hybrid://lynxview_page?bundle=",
        "hybrid://lynxview_page?bundle=first.lynx.bundle&bundle=second.lynx.bundle",
        "hybrid://lynxview_page?bundle=&url=https%3A%2F%2Fexample.com%2Fpage.lynx.bundle",
      ] {
        assertRouteError(.sparklingUnavailable) {
          _ = try parser.parse(
            input,
            requestedContainer: .legacy,
            source: .manualInput)
        }
      }
    }
  #endif

  private func makeAdapter(
    canResolve: Bool = true,
    engine: SparklingSchemeEngineKind = .lynx,
    container: SparklingSchemeContainerKind = .page,
    url: String? = nil,
    extras: [String: String]
  ) -> SparklingSchemeAdapter {
    let result = parameters(
      canResolve: canResolve,
      engine: engine,
      container: container,
      url: url,
      extras: extras)
    return SparklingSchemeAdapter { _, _ in result }
  }

  private func parameters(
    canResolve: Bool = true,
    engine: SparklingSchemeEngineKind = .lynx,
    container: SparklingSchemeContainerKind = .page,
    url: String? = nil,
    extras: [String: String]
  ) -> SparklingSchemeParameters {
    SparklingSchemeParameters(
      canResolve: canResolve,
      engine: engine,
      container: container,
      url: url,
      extras: extras)
  }

  private func percentEncoded(_ value: String) -> String {
    var allowed = CharacterSet.urlQueryAllowed
    allowed.remove(charactersIn: "&=+?#")
    return value.addingPercentEncoding(withAllowedCharacters: allowed)!
  }

  private func assertRouteError(
    _ expected: RouteError,
    file: StaticString = #filePath,
    line: UInt = #line,
    operation: () throws -> Void
  ) {
    XCTAssertThrowsError(try operation(), file: file, line: line) { error in
      XCTAssertEqual(error as? RouteError, expected, file: file, line: line)
    }
  }
}

private struct StubSparklingSchemeResolver: SparklingSchemeResolving {
  let resolution: SparklingSchemeResolution

  func resolve(_ scheme: String) throws -> SparklingSchemeResolution {
    resolution
  }
}
