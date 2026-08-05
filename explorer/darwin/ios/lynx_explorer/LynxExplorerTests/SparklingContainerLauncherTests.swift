// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import UIKit
import XCTest

@testable import LynxExplorer

#if canImport(Lynx)
  import Lynx
#endif

#if canImport(Sparkling)
  import Sparkling
#endif

final class SparklingContainerLauncherTests: XCTestCase {
  func testSparklingAssetResolverUsesPackagedExtensionRoot() throws {
    let resourceRoot = URL(
      fileURLWithPath: "/tmp/LynxExplorer.app/Resource", isDirectory: true)
    let resolver = ExplorerSparklingAssetResolver(resourceRoot: resourceRoot)
    let assetURL = try XCTUnwrap(
      URL(string: "asset:///static/image/sparkling_icon.781d0b63.png"))

    XCTAssertEqual(
      resolver.localURL(for: assetURL)?.path,
      "/tmp/LynxExplorer.app/Resource/extensions/sparkling-go/static/image/sparkling_icon.781d0b63.png"
    )
  }

  func testSparklingAssetResolverRejectsExternalAndTraversalURLs() throws {
    let resourceRoot = URL(
      fileURLWithPath: "/tmp/LynxExplorer.app/Resource", isDirectory: true)
    let resolver = ExplorerSparklingAssetResolver(resourceRoot: resourceRoot)

    XCTAssertNil(
      resolver.localURL(for: try XCTUnwrap(URL(string: "https://example.com/icon.png"))))
    XCTAssertNil(
      resolver.localURL(for: try XCTUnwrap(URL(string: "asset:///../icon.png"))))
  }

  #if canImport(Sparkling)
  func testCanonicalInputPassesThroughUnchangedAfterValidation() throws {
    let canonical =
      "hybrid://lynxview_page?bundle=home.lynx.bundle&title=An%20%26%20B"
    let descriptor = makeDescriptor(
      resource: .localBundle("home.lynx.bundle"),
      canonicalSparklingScheme: canonical)
    let validator = RecordingSchemeResolver(resource: descriptor.resource)
    let builder = SparklingCanonicalSchemeBuilder(validator: validator)

    XCTAssertEqual(try builder.build(for: descriptor), canonical)
    XCTAssertEqual(validator.inputs, [canonical])
  }

  func testRemoteCanonicalBuildRoundTripsEncodingAndKeepsOneResourceTarget() throws {
    let target = try XCTUnwrap(
      URL(string: "https://example.com/page.lynx.bundle?token=a%26b&formula=x%3Dy"))
    let descriptor = makeDescriptor(
      resource: .remote(target),
      navigation: NavigationOptions(
        navigationHidden: false,
        fullScreen: false,
        title: "A & B = C",
        titleColor: nil,
        barColor: nil,
        backButtonStyle: nil,
        orientation: nil),
      queryItems: [
        LaunchQueryItem(name: "tag", value: "first"),
        LaunchQueryItem(name: "url", value: "https://attacker.invalid"),
        LaunchQueryItem(name: "tag", value: "second"),
        LaunchQueryItem(name: "title", value: "stale"),
      ])
    let validator = RecordingSchemeResolver(resource: descriptor.resource)

    let scheme = try SparklingCanonicalSchemeBuilder(validator: validator).build(for: descriptor)
    let components = try XCTUnwrap(URLComponents(string: scheme))
    let items = components.queryItems ?? []

    XCTAssertEqual(components.scheme, "hybrid")
    XCTAssertEqual(components.host, "lynxview_page")
    XCTAssertEqual(items.filter { $0.name == "url" }.map(\.value), [target.absoluteString])
    XCTAssertTrue(items.filter { $0.name == "bundle" }.isEmpty)
    XCTAssertEqual(items.filter { $0.name == "tag" }.map(\.value), ["first", "second"])
    XCTAssertEqual(items.filter { $0.name == "title" }.map(\.value), ["A & B = C"])
    XCTAssertEqual(validator.inputs, [scheme])
  }

  func testRemoteCanonicalBuildFiltersCaseInsensitiveResourceTargetAliases() throws {
    let target = try XCTUnwrap(
      URL(string: "https://example.com/page.lynx.bundle?URL=page-value"))
    let descriptor = makeDescriptor(
      resource: .remote(target),
      queryItems: [LaunchQueryItem(name: "URL", value: "page-value")])
    let validator = RecordingSchemeResolver(resource: descriptor.resource)

    let scheme = try SparklingCanonicalSchemeBuilder(validator: validator).build(for: descriptor)
    let items = try XCTUnwrap(URLComponents(string: scheme)?.queryItems)

    XCTAssertEqual(items.filter { $0.name == "url" }.map(\.value), [target.absoluteString])
    XCTAssertTrue(items.filter { $0.name == "URL" }.isEmpty)
    XCTAssertEqual(validator.inputs, [scheme])
  }

  func testLocalCanonicalBuildDropsConflictingTargetsAndMapsPublicAppearanceKeys() throws {
    let descriptor = makeDescriptor(
      resource: .localBundle("folder/page.lynx.bundle"),
      appearance: AppearanceOptions(
        backgroundColor: "#112233",
        transparentStatusBar: true),
      navigation: NavigationOptions(
        navigationHidden: true,
        fullScreen: true,
        title: "Local",
        titleColor: "#ffffff",
        barColor: "#000000",
        backButtonStyle: nil,
        orientation: nil),
      queryItems: [
        LaunchQueryItem(name: "bundle", value: "attacker.lynx.bundle"),
        LaunchQueryItem(name: "url", value: "https://attacker.invalid"),
        LaunchQueryItem(name: "custom", value: "one"),
        LaunchQueryItem(name: "custom", value: "two"),
      ])
    let validator = RecordingSchemeResolver(resource: descriptor.resource)

    let scheme = try SparklingCanonicalSchemeBuilder(validator: validator).build(for: descriptor)
    let items = try XCTUnwrap(URLComponents(string: scheme)?.queryItems)

    XCTAssertEqual(
      items.filter { $0.name == "bundle" }.map(\.value),
      ["folder/page.lynx.bundle"])
    XCTAssertTrue(items.filter { $0.name == "url" }.isEmpty)
    XCTAssertEqual(items.filter { $0.name == "custom" }.map(\.value), ["one", "two"])
    XCTAssertEqual(items.first(where: { $0.name == "hide_nav_bar" })?.value, "1")
    XCTAssertEqual(items.first(where: { $0.name == "hide_status_bar" })?.value, "0")
    XCTAssertEqual(items.first(where: { $0.name == "trans_status_bar" })?.value, "1")
    XCTAssertEqual(items.first(where: { $0.name == "nav_bar_color" })?.value, "#000000")
    XCTAssertEqual(items.first(where: { $0.name == "container_bg_color" })?.value, "#112233")
  }

  func testStatusBarCanBeHiddenWithoutFullscreen() throws {
    let descriptor = makeDescriptor(
      resource: .localBundle("page.lynx.bundle"),
      appearance: AppearanceOptions(
        backgroundColor: nil,
        transparentStatusBar: false,
        statusBarHidden: true))

    let scheme = try SparklingCanonicalSchemeBuilder(
      validator: RecordingSchemeResolver(resource: descriptor.resource)
    ).build(for: descriptor)
    let items = try XCTUnwrap(URLComponents(string: scheme)?.queryItems)

    XCTAssertEqual(items.first(where: { $0.name == "fullscreen" })?.value, "0")
    XCTAssertEqual(items.first(where: { $0.name == "hide_status_bar" })?.value, "1")
  }

  func testRecorderAndUnsupportedNavigationOptionsAreTypedFailures() {
    let builder = SparklingCanonicalSchemeBuilder(
      validator: RecordingSchemeResolver(resource: .localBundle("unused")))

    assertRouteError(.recorderUnsupportedInSparkling) {
      _ = try builder.build(
        for: makeDescriptor(resource: .recorder("file://lynxrecorder?sample")))
    }
    assertRouteError(.sparklingOptionUnsupported("orientation")) {
      _ = try builder.build(
        for: makeDescriptor(
          resource: .localBundle("page.lynx.bundle"),
          navigation: NavigationOptions(
            navigationHidden: false,
            fullScreen: false,
            title: nil,
            titleColor: nil,
            barColor: nil,
            backButtonStyle: nil,
            orientation: .landscape)))
    }
    assertRouteError(.sparklingOptionUnsupported("orientation")) {
      _ = try builder.build(
        for: makeDescriptor(
          resource: .localBundle("page.lynx.bundle"),
          queryItems: [LaunchQueryItem(name: "orientation", value: "diagonal")]))
    }
    let backStyleDescriptor = makeDescriptor(
      resource: .localBundle("page.lynx.bundle"),
      navigation: NavigationOptions(
        navigationHidden: false,
        fullScreen: false,
        title: nil,
        titleColor: nil,
        barColor: nil,
        backButtonStyle: "dark",
        orientation: nil))
    let backStyleScheme = try? builder.build(for: backStyleDescriptor)
    let backStyle = backStyleScheme.flatMap {
      URLComponents(string: $0)?.queryItems?
        .first(where: { $0.name == "back_button_style" })?.value
    }
    XCTAssertEqual(backStyle, "dark")
  }
  #endif

  @MainActor
  func testLegacyDescriptorIsRejectedBeforeSchemeOrRouterWork() {
    let schemeBuilder = RecordingCanonicalBuilder(result: "hybrid://lynxview_page?bundle=x")
    #if canImport(Sparkling)
      let router = RecordingSparklingRouter()
      let launcher = SparklingContainerLauncher(
        router: router,
        schemeBuilder: schemeBuilder)
    #else
      let launcher = SparklingContainerLauncher(schemeBuilder: schemeBuilder)
    #endif
    let descriptor = makeDescriptor(
      resource: .localBundle("legacy.lynx.bundle"),
      container: .legacy)

    XCTAssertThrowsError(try launcher.makeViewController(for: descriptor)) { error in
      XCTAssertEqual(
        error as? RouteError,
        .containerMismatch(expected: .sparkling, actual: .legacy))
    }
    XCTAssertTrue(schemeBuilder.descriptors.isEmpty)
    #if canImport(Sparkling)
      XCTAssertEqual(router.callCount, 0)
    #endif
  }

  #if canImport(Sparkling)
    @MainActor
    func testLauncherWiresOfficialContextViewportServicesAndInitialData() throws {
      let router = RecordingSparklingRouter()
      let canonical =
        "hybrid://lynxview_page?bundle=page.lynx.bundle&title=Canonical&query=value"
      let initialData = try JSONSerialization.data(withJSONObject: ["answer": 42])
      let descriptor = makeDescriptor(
        resource: .localBundle("page.lynx.bundle"),
        initialData: initialData,
        commonGlobalProps: [
          "theme": "common",
          "containerType": "spoofed",
        ],
        pageGlobalProps: [
          "theme": "page",
          "pageOnly": "value",
          "sparklingNavigation": false,
        ],
        pageName: "home",
        viewport: ViewportOptions(widthInPixels: 750, heightInPixels: 1334),
        navigation: NavigationOptions(
          navigationHidden: false,
          fullScreen: false,
          title: nil,
          titleColor: nil,
          barColor: nil,
          backButtonStyle: "dark",
          orientation: nil),
        debugOptions: DebugOptions(enableNAPIAddon: true),
        queryItems: [LaunchQueryItem(name: "launch_extra", value: "extra")])
      let launcher = SparklingContainerLauncher(
        router: router,
        schemeBuilder: RecordingCanonicalBuilder(result: canonical),
        displayMetrics: {
          SparklingDisplayMetrics(bounds: CGRect(x: 0, y: 0, width: 390, height: 844), scale: 2)
        })

      let controller = try launcher.makeViewController(for: descriptor)
      let context = try XCTUnwrap(router.context)
      let globalProps = try XCTUnwrap(context.globalProps as? [String: AnyHashable])

      XCTAssertTrue(controller === router.controller)
      XCTAssertEqual(router.url, canonical)
      XCTAssertEqual(router.frame, CGRect(x: 0, y: 0, width: 375, height: 667))
      XCTAssertEqual(globalProps["theme"], "page")
      XCTAssertEqual(globalProps["pageOnly"], "value")
      XCTAssertEqual(globalProps["containerType"], "sparkling")
      XCTAssertEqual(globalProps["sparklingNavigation"], true)
      XCTAssertEqual(globalProps["explorerSupportsExplicitRouteOwnership"], true)
      XCTAssertEqual(globalProps["explorerSupportsSparklingContainer"], true)
      XCTAssertNil(globalProps["containerID"])
      XCTAssertEqual(context.initialData?["answer"] as? Int, 42)
      XCTAssertEqual(context.queryItems?["initial_page"], "home")
      XCTAssertEqual(context.queryItems?["query"], "value")
      XCTAssertEqual(context.extra?["launch_extra"], "extra")
      XCTAssertEqual(context.originURL, canonical)
      XCTAssertNotNil(context.templateProvider)
      XCTAssertNotNil(context.imageFetcher)
      let loadErrorView = try XCTUnwrap(context.failedViewBuilder?(nil))
      XCTAssertEqual(loadErrorView.accessibilityIdentifier, "sparkling-load-error")
      var refreshCount = 0
      loadErrorView.register? { refreshCount += 1 }
      let retryButton = try XCTUnwrap(
        loadErrorView.subviews
          .compactMap { $0 as? UIStackView }
          .flatMap(\.arrangedSubviews)
          .first { $0.accessibilityIdentifier == "sparkling-load-error-retry" }
          as? UIButton)
      retryButton.sendActions(for: .touchUpInside)
      XCTAssertEqual(refreshCount, 1)
      XCTAssertNotNil(context.rawViewBuilderBlock)
      XCTAssertNotNil(context.navigationBarBackHandler)
      let backButton = try XCTUnwrap(context.leftNavigationBarButtonItemBuilder?(nil))
      XCTAssertEqual(backButton.icon.pngData(), UIImage(named: "back_dark")?.pngData())
      XCTAssertNotNil(context.interactivePopGestureDelegate)
      let loadingView = try XCTUnwrap(context.loadingView)
      let lifecycleDelegate = try XCTUnwrap(context.containerLifecycleDelegate)
      XCTAssertTrue((loadingView as AnyObject) === (lifecycleDelegate as AnyObject))
      let modules = try XCTUnwrap(context.lynxModule)
      XCTAssertNotNil(modules[NSStringFromClass(ExplorerModule.self)])
      XCTAssertNotNil(modules[NSStringFromClass(LynxNodeAPIModule.self)])
      XCTAssertNil(context.widthMode)
      XCTAssertNil(context.heightMode)

      let builder = LynxViewBuilder()
      builder.config = LynxConfig(provider: context.templateProvider)
      let sparklingTemplateFetcher = DemoTemplateResourceFetcher()
      builder.templateResourceFetcher = sparklingTemplateFetcher
      context.rawViewBuilderBlock?(builder)
      XCTAssertEqual(builder.screenSize, CGSize(width: 375, height: 667))
      XCTAssertTrue(builder.debuggable)
      XCTAssertNotNil(builder.genericResourceFetcher)
      // Sparkling owns resolution of the canonical bundle/url target through
      // its page resource provider. The host only supplies subresource
      // fetchers; replacing the top-level template fetcher would bypass that
      // SDK contract and treat bundle names as unsupported URLs.
      XCTAssertTrue(builder.templateResourceFetcher === sparklingTemplateFetcher)
      XCTAssertNotNil(builder.mediaResourceFetcher)
      XCTAssertNotNil(builder.lynxBackgroundRuntime)
    }

    @MainActor
    func testNAPIPageLifetimeIsRetainedOnlyWhileRouterContextOwnsRawBuilderClosure() throws {
      let router = RecordingSparklingRouter()
      var lifetime: LifetimeProbe? = LifetimeProbe()
      weak var weakLifetime = lifetime
      let launcher = SparklingContainerLauncher(
        router: router,
        schemeBuilder: RecordingCanonicalBuilder(
          result: "hybrid://lynxview_page?bundle=page.lynx.bundle"),
        lifetimeFactory: { enableNAPIAddon in
          XCTAssertTrue(enableNAPIAddon)
          return lifetime!
        })
      let descriptor = makeDescriptor(
        resource: .localBundle("page.lynx.bundle"),
        debugOptions: DebugOptions(enableNAPIAddon: true))

      _ = try launcher.makeViewController(for: descriptor)
      lifetime = nil

      XCTAssertNotNil(weakLifetime)
      router.context = nil
      XCTAssertNil(weakLifetime)
    }

    @MainActor
    func testRouterCreationFailurePropagatesWithoutAlternativeContainer() {
      let router = RecordingSparklingRouter()
      router.error = TestError.creation
      let launcher = SparklingContainerLauncher(
        router: router,
        schemeBuilder: RecordingCanonicalBuilder(
          result: "hybrid://lynxview_page?bundle=page.lynx.bundle"))

      XCTAssertThrowsError(
        try launcher.makeViewController(
          for: makeDescriptor(resource: .localBundle("page.lynx.bundle")))
      ) { error in
        XCTAssertEqual(error as? TestError, .creation)
      }
      XCTAssertEqual(router.callCount, 1)
    }

  #else
    @MainActor
    func testDisabledBuildFailsWithoutCallingAnotherContainer() {
      let launcher = SparklingContainerLauncher()

      assertRouteError(.sparklingUnavailable) {
        _ = try launcher.makeViewController(
          for: makeDescriptor(resource: .localBundle("page.lynx.bundle")))
      }
    }
  #endif

  private func makeDescriptor(
    resource: LaunchResource,
    initialData: Data? = nil,
    commonGlobalProps: [String: AnyHashable] = [:],
    pageGlobalProps: [String: AnyHashable] = [:],
    pageName: String? = nil,
    viewport: ViewportOptions? = nil,
    appearance: AppearanceOptions = AppearanceOptions(
      backgroundColor: nil,
      transparentStatusBar: false),
    navigation: NavigationOptions = NavigationOptions(
      navigationHidden: false,
      fullScreen: false,
      title: nil,
      titleColor: nil,
      barColor: nil,
      backButtonStyle: nil,
      orientation: nil),
    debugOptions: DebugOptions = DebugOptions(enableNAPIAddon: false),
    queryItems: [LaunchQueryItem] = [],
    container: ResolvedContainer = .sparkling,
    canonicalSparklingScheme: String? = nil
  ) -> LaunchDescriptor {
    LaunchDescriptor(
      originalInput: "test-input",
      resource: resource,
      initialData: initialData,
      commonGlobalProps: commonGlobalProps,
      pageGlobalProps: pageGlobalProps,
      pageName: pageName,
      viewport: viewport,
      appearance: appearance,
      navigation: navigation,
      presentation: PresentationOptions(animated: true),
      cachePolicy: .useProtocolCachePolicy,
      debugOptions: debugOptions,
      queryItems: queryItems,
      extras: queryItems.reduce(into: [:]) { extras, item in
        if let value = item.value {
          extras[item.name] = value
        }
      },
      requestedContainer: container == .sparkling ? .sparkling : .legacy,
      container: container,
      source: .manualInput,
      canonicalSparklingScheme: canonicalSparklingScheme)
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

private final class RecordingSchemeResolver: SparklingSchemeResolving {
  let resource: LaunchResource
  private(set) var inputs: [String] = []

  init(resource: LaunchResource) {
    self.resource = resource
  }

  func resolve(_ scheme: String) throws -> SparklingSchemeResolution {
    inputs.append(scheme)
    return SparklingSchemeResolution(
      resource: resource,
      canonicalScheme: scheme,
      outerQueryItems: [],
      normalizedExtras: [:])
  }
}

private final class RecordingCanonicalBuilder: SparklingCanonicalSchemeBuilding {
  let result: String
  private(set) var descriptors: [LaunchDescriptor] = []

  init(result: String) {
    self.result = result
  }

  func build(for descriptor: LaunchDescriptor) throws -> String {
    descriptors.append(descriptor)
    return result
  }
}

#if canImport(Sparkling)
  @MainActor
  private final class RecordingSparklingRouter: SparklingRouting {
    let controller = UIViewController()
    var error: Error?
    var url: String?
    var context: SPKContext?
    var frame: CGRect?
    private(set) var callCount = 0

    func create(
      withURL url: String,
      context: SPKContext,
      frame: CGRect
    ) throws -> UIViewController {
      callCount += 1
      self.url = url
      self.context = context
      self.frame = frame
      if let error {
        throw error
      }
      return controller
    }
  }

  @MainActor
  private final class LifetimeProbe: SparklingPageLifetimeConfiguring {
    func configure(builder: LynxViewBuilder, frame: CGRect) {}
  }
#endif

private enum TestError: Error, Equatable {
  case creation
}
