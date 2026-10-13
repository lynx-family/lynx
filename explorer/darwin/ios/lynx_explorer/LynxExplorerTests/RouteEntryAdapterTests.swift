// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import UIKit
import XCTest

@testable import LynxExplorer

#if canImport(SparklingMethod) && canImport(Sparkling_Router)
  import SparklingMethod
  import Sparkling_Router
#endif

final class RouteEntryAdapterTests: XCTestCase {
  override func tearDown() {
    RouteCoordinatorBridge.install(nil)
    super.tearDown()
  }

  @MainActor
  func testExplorerCommonGlobalPropsProviderEmitsProductionValueShape() {
    let provider = ExplorerCommonGlobalPropsProvider(
      userInterfaceStyle: { .dark },
      preferredTheme: { "PreferredDark" },
      safeAreaInsets: { UIEdgeInsets(top: 59, left: 0, bottom: 34, right: 0) })

    let props = provider.makeGlobalProps()

    XCTAssertEqual(
      props,
      [
        "isNotchScreen": true,
        "preferredTheme": "PreferredDark",
        "theme": "Dark",
      ])
    XCTAssertTrue(props["isNotchScreen"]?.base is Bool)
    XCTAssertTrue(props["preferredTheme"]?.base is String)
    XCTAssertTrue(props["theme"]?.base is String)
  }

  @MainActor
  func testExplorerCommonGlobalPropsProviderOmitsAbsentPreferenceAndDetectsFlatScreen() {
    let provider = ExplorerCommonGlobalPropsProvider(
      userInterfaceStyle: { .light },
      preferredTheme: { nil },
      safeAreaInsets: { UIEdgeInsets(top: 20, left: 0, bottom: 0, right: 0) })

    XCTAssertEqual(
      provider.makeGlobalProps(),
      [
        "isNotchScreen": false,
        "theme": "Light",
      ])
  }

  func testStartupRouteUsesEnvironmentBeforeColdURLAndUniversalLink() {
    let environmentURL = "https://example.com/environment.lynx.bundle"
    let coldURL = URL(string: "lynx://open?url=https%3A%2F%2Fexample.com%2Fcold.lynx.bundle")!
    let universalURL = URL(string: "https://example.com/universal.lynx.bundle")!
    let activity = NSUserActivity(activityType: NSUserActivityTypeBrowsingWeb)
    activity.webpageURL = universalURL

    let request = RouteEntryAdapter.startupRequest(
      environment: ["lynx_initial_url": environmentURL],
      launchOptions: [
        .url: coldURL,
        .userActivityDictionary: ["activity": activity],
      ],
      homepageURL: "file://lynx?local://homepage.lynx.bundle",
      isRunningTests: false)

    XCTAssertEqual(request?.input, environmentURL)
    XCTAssertEqual(request?.requestedContainer, .automatic)
    XCTAssertEqual(request?.source, .startup)
    XCTAssertEqual(request?.presentation, .push)
  }

  func testStartupRouteFallsBackFromColdURLToUniversalLinkThenHomepage() {
    let coldURL = URL(string: "lynx://open?url=https%3A%2F%2Fexample.com%2Fcold.lynx.bundle")!
    let universalURL = URL(string: "https://example.com/universal.lynx.bundle")!
    let homepageURL = "file://lynx?local://homepage.lynx.bundle"
    let activity = NSUserActivity(activityType: NSUserActivityTypeBrowsingWeb)
    activity.webpageURL = universalURL

    let coldRequest = RouteEntryAdapter.startupRequest(
      environment: [:],
      launchOptions: [
        .url: coldURL,
        .userActivityDictionary: ["activity": activity],
      ],
      homepageURL: homepageURL,
      isRunningTests: false)
    let universalRequest = RouteEntryAdapter.startupRequest(
      environment: [:],
      launchOptions: [.userActivityDictionary: ["activity": activity]],
      homepageURL: homepageURL,
      isRunningTests: false)
    let homepageRequest = RouteEntryAdapter.startupRequest(
      environment: [:],
      launchOptions: [:],
      homepageURL: homepageURL,
      isRunningTests: false)

    XCTAssertEqual(coldRequest?.input, coldURL.absoluteString)
    XCTAssertEqual(coldRequest?.source, .externalURL)
    XCTAssertEqual(universalRequest?.input, universalURL.absoluteString)
    XCTAssertEqual(universalRequest?.source, .universalLink)
    XCTAssertEqual(homepageRequest?.input, homepageURL)
    XCTAssertEqual(homepageRequest?.source, .startup)
  }

  func testXCTestStartupSkipsOnlyHomepageFallback() {
    let homepageURL = "file://lynx?local://homepage.lynx.bundle"
    let explicitURL = "https://example.com/explicit.lynx.bundle"

    XCTAssertNil(
      RouteEntryAdapter.startupRequest(
        environment: [:],
        launchOptions: [:],
        homepageURL: homepageURL,
        isRunningTests: true))
    XCTAssertEqual(
      RouteEntryAdapter.startupRequest(
        environment: ["lynx_initial_url": explicitURL],
        launchOptions: [:],
        homepageURL: homepageURL,
        isRunningTests: true)?.input,
      explicitURL)
  }

  @MainActor
  func testColdCustomURLCallbackIsSuppressedOnceWithoutBlockingLaterOpen() {
    let environment = makeEnvironment()
    let url = URL(string: "lynx://open?url=https%3A%2F%2Fexample.com%2Fcold.lynx.bundle")!
    let startupExpectation = expectation(description: "cold startup")
    let duplicateExpectation = expectation(description: "duplicate callback")

    environment.bridge.openStartup(
      environment: [:],
      launchOptions: [UIApplication.LaunchOptionsKey.url.rawValue: url],
      homepageURL: "file://lynx?local://homepage.lynx.bundle",
      isRunningTests: true
    ) { _ in
      startupExpectation.fulfill()
    }
    environment.bridge.openExternalURL(url) { value in
      XCTAssertEqual((value as? [String: Any])?["code"] as? String, "duplicate_cold_delivery")
      duplicateExpectation.fulfill()
    }

    wait(for: [startupExpectation, duplicateExpectation], timeout: 2)
    XCTAssertEqual(environment.parser.received.count, 1)
    XCTAssertEqual(environment.navigation.openedPolicies, [.push])

    let laterExpectation = expectation(description: "later user open")
    environment.bridge.openExternalURL(url) { _ in
      laterExpectation.fulfill()
    }
    wait(for: [laterExpectation], timeout: 2)
    XCTAssertEqual(environment.parser.received.count, 2)
    XCTAssertEqual(environment.navigation.openedPolicies, [.push, .push])
  }

  @MainActor
  func testColdUniversalLinkCallbackIsSuppressedOnce() {
    let environment = makeEnvironment()
    let url = URL(string: "https://example.com/cold-universal.lynx.bundle")!
    let activity = NSUserActivity(activityType: NSUserActivityTypeBrowsingWeb)
    activity.webpageURL = url
    let startupExpectation = expectation(description: "cold universal startup")
    let duplicateExpectation = expectation(description: "duplicate universal callback")

    environment.bridge.openStartup(
      environment: [:],
      launchOptions: [
        UIApplication.LaunchOptionsKey.userActivityDictionary.rawValue: ["activity": activity]
      ],
      homepageURL: "file://lynx?local://homepage.lynx.bundle",
      isRunningTests: true
    ) { _ in
      startupExpectation.fulfill()
    }
    environment.bridge.openUniversalLinkURL(url) { value in
      XCTAssertEqual((value as? [String: Any])?["code"] as? String, "duplicate_cold_delivery")
      duplicateExpectation.fulfill()
    }

    wait(for: [startupExpectation, duplicateExpectation], timeout: 2)
    XCTAssertEqual(environment.parser.received.count, 1)
    XCTAssertEqual(environment.parser.received.first?.source, .universalLink)
    XCTAssertEqual(environment.navigation.openedPolicies, [.push])
  }

  @MainActor
  func testEnvironmentStartupDoesNotSuppressUnselectedColdURL() {
    let environment = makeEnvironment()
    let environmentURL = "https://example.com/environment.lynx.bundle"
    let coldURL = URL(string: "lynx://open?url=https%3A%2F%2Fexample.com%2Fcold.lynx.bundle")!
    let startupExpectation = expectation(description: "environment startup")
    let coldURLExpectation = expectation(description: "unselected cold URL callback")

    environment.bridge.openStartup(
      environment: ["lynx_initial_url": environmentURL],
      launchOptions: [UIApplication.LaunchOptionsKey.url.rawValue: coldURL],
      homepageURL: "file://lynx?local://homepage.lynx.bundle",
      isRunningTests: true
    ) { _ in
      startupExpectation.fulfill()
    }
    environment.bridge.openExternalURL(coldURL) { value in
      XCTAssertEqual((value as? [String: Any])?["code"] as? String, "ok")
      coldURLExpectation.fulfill()
    }

    wait(for: [startupExpectation, coldURLExpectation], timeout: 2)
    XCTAssertEqual(
      environment.parser.received.map(\.input),
      [environmentURL, coldURL.absoluteString])
    XCTAssertEqual(environment.navigation.openedPolicies, [.push, .push])
  }

  @MainActor
  func testColdURLStartupDoesNotSuppressUnselectedUniversalLink() {
    let environment = makeEnvironment()
    let coldURL = URL(string: "lynx://open?url=https%3A%2F%2Fexample.com%2Fcold.lynx.bundle")!
    let universalURL = URL(string: "https://example.com/universal.lynx.bundle")!
    let activity = NSUserActivity(activityType: NSUserActivityTypeBrowsingWeb)
    activity.webpageURL = universalURL
    let startupExpectation = expectation(description: "cold URL startup")
    let universalExpectation = expectation(description: "unselected universal callback")

    environment.bridge.openStartup(
      environment: [:],
      launchOptions: [
        UIApplication.LaunchOptionsKey.url.rawValue: coldURL,
        UIApplication.LaunchOptionsKey.userActivityDictionary.rawValue: ["activity": activity],
      ],
      homepageURL: "file://lynx?local://homepage.lynx.bundle",
      isRunningTests: true
    ) { _ in
      startupExpectation.fulfill()
    }
    environment.bridge.openUniversalLinkURL(universalURL) { value in
      XCTAssertEqual((value as? [String: Any])?["code"] as? String, "ok")
      universalExpectation.fulfill()
    }

    wait(for: [startupExpectation, universalExpectation], timeout: 2)
    XCTAssertEqual(
      environment.parser.received.map(\.input),
      [coldURL.absoluteString, universalURL.absoluteString])
    XCTAssertEqual(environment.navigation.openedPolicies, [.push, .push])
  }

  func testColdDeliverySuppressionExpires() {
    var currentDate = Date(timeIntervalSince1970: 1_000)
    let suppressor = ColdRouteDeliverySuppressor(
      lifetime: 1,
      now: { currentDate })
    let input = "https://example.com/cold.lynx.bundle"

    suppressor.remember(input: input, source: .externalURL)
    currentDate.addTimeInterval(2)

    XCTAssertFalse(suppressor.shouldSuppress(input: input, source: .externalURL))
  }

  func testHotCustomURLPreservesEncodedWrapperVerbatim() {
    let input =
      "lynx://open?url=hybrid%3A%2F%2Flynxview_page%3Fbundle%3Dchild.lynx.bundle%26token%3D100%2525"
    let request = RouteEntryAdapter.hotURLRequest(for: URL(string: input)!)

    XCTAssertEqual(request.input, input)
    XCTAssertEqual(request.requestedContainer, .automatic)
    XCTAssertEqual(request.source, .externalURL)
    XCTAssertEqual(request.presentation, .push)
  }

  func testDebugBridgeUsesReplaceTopForLocalAndResetForRemote() {
    let local = RouteEntryAdapter.debugBridgeRequest(
      for: "file://lynx?local://homepage.lynx.bundle")
    let remote = RouteEntryAdapter.debugBridgeRequest(
      for: "https://example.com/home.lynx.bundle")

    XCTAssertEqual(local.requestedContainer, .automatic)
    XCTAssertEqual(local.source, .debugBridge)
    XCTAssertEqual(local.presentation, .replaceTop)
    XCTAssertEqual(remote.source, .debugBridge)
    XCTAssertEqual(remote.presentation, .resetAndPush)
  }

  func testModuleScannerUniversalAndSparklingRouterRequestsUseExplicitPolicies() {
    let module = RouteEntryAdapter.moduleRequest(
      for: "https://example.com/module.lynx.bundle",
      containerName: "sparkling")
    let scanner = RouteEntryAdapter.scannerRequest(
      for: "https://example.com/scanned.lynx.bundle",
      container: .legacy)
    let universal = RouteEntryAdapter.universalLinkRequest(
      for: URL(string: "https://example.com/universal.lynx.bundle")!)
    let sparklingRouter = RouteEntryAdapter.sparklingRouterRequest(
      for: "hybrid://lynxview_page?bundle=child.lynx.bundle")

    XCTAssertEqual(module?.requestedContainer, .sparkling)
    XCTAssertEqual(module?.source, .nativeModule)
    XCTAssertEqual(module?.presentation, .push)
    XCTAssertEqual(scanner?.requestedContainer, .legacy)
    XCTAssertEqual(scanner?.source, .scanner)
    XCTAssertEqual(scanner?.presentation, .replaceTop)
    XCTAssertEqual(universal.source, .universalLink)
    XCTAssertEqual(universal.presentation, .push)
    XCTAssertEqual(sparklingRouter.requestedContainer, .sparkling)
    XCTAssertEqual(sparklingRouter.source, .sparklingRouter)
    XCTAssertEqual(sparklingRouter.presentation, .push)
    XCTAssertEqual(RouteEntryAdapter.scannerContainerChoices, [.legacy, .sparkling])
    XCTAssertNil(
      RouteEntryAdapter.moduleRequest(
        for: "https://example.com/module.lynx.bundle",
        containerName: "unsupported"))
  }

  @MainActor
  func testExplorerModuleForwardsContainerAndCallsCallbackExactlyOnce() {
    let environment = makeEnvironment()
    RouteCoordinatorBridge.install(environment.bridge)
    let expectation = expectation(description: "module callback")
    var callbackCount = 0
    var payload: [String: Any]?

    ExplorerModule().openRoute(
      "https://example.com/module.lynx.bundle",
      container: "sparkling"
    ) { value in
      callbackCount += 1
      payload = value as? [String: Any]
      expectation.fulfill()
    }

    wait(for: [expectation], timeout: 2)
    XCTAssertEqual(callbackCount, 1)
    XCTAssertEqual(payload?["success"] as? Bool, true)
    XCTAssertEqual(environment.parser.received.first?.requestedContainer, .sparkling)
    XCTAssertEqual(environment.parser.received.first?.source, .nativeModule)
    XCTAssertEqual(environment.navigation.openedPolicies, [.push])
  }

  @MainActor
  func testExplorerModuleInvalidContainerCallsCallbackExactlyOnce() {
    let environment = makeEnvironment()
    RouteCoordinatorBridge.install(environment.bridge)
    let expectation = expectation(description: "invalid container callback")
    var callbackCount = 0
    var payload: [String: Any]?

    ExplorerModule().openRoute(
      "https://example.com/module.lynx.bundle",
      container: "future"
    ) { value in
      callbackCount += 1
      payload = value as? [String: Any]
      expectation.fulfill()
    }

    wait(for: [expectation], timeout: 2)
    XCTAssertEqual(callbackCount, 1)
    XCTAssertEqual(payload?["success"] as? Bool, false)
    XCTAssertEqual(payload?["code"] as? String, "invalid_parameter")
    XCTAssertTrue(environment.parser.received.isEmpty)
  }

  @MainActor
  func testExplorerModuleMalformedRouteCallsCallbackExactlyOnce() {
    let environment = makeEnvironment(parserError: RouteError.malformedURL("not a URL"))
    RouteCoordinatorBridge.install(environment.bridge)
    let expectation = expectation(description: "malformed route callback")
    var callbackCount = 0
    var payload: [String: Any]?

    ExplorerModule().openRoute("not a URL", container: "automatic") { value in
      callbackCount += 1
      payload = value as? [String: Any]
      expectation.fulfill()
    }

    wait(for: [expectation], timeout: 2)
    XCTAssertEqual(callbackCount, 1)
    XCTAssertEqual(payload?["success"] as? Bool, false)
    XCTAssertEqual(payload?["code"] as? String, "malformed_url")
  }

  @MainActor
  func testExplorerModuleLegacyOpenSchemaUsesAutomaticCoordinatorRoute() {
    let environment = makeEnvironment()
    RouteCoordinatorBridge.install(environment.bridge)
    let expectation = expectation(description: "legacy module open")
    let input = "https://example.com/legacy-module.lynx.bundle"

    ExplorerModule().openSchema(input)
    DispatchQueue.main.async {
      expectation.fulfill()
    }

    wait(for: [expectation], timeout: 2)
    XCTAssertEqual(environment.parser.received.count, 1)
    XCTAssertEqual(environment.parser.received.first?.input, input)
    XCTAssertEqual(environment.parser.received.first?.requestedContainer, .automatic)
    XCTAssertEqual(environment.parser.received.first?.source, .nativeModule)
    XCTAssertEqual(environment.navigation.openedPolicies, [.push])
  }

  @MainActor
  func testRouteErrorPresenterUpdatesAlertsButNotActionSheets() {
    let alert = UIAlertController(
      title: "Old alert",
      message: "Old message",
      preferredStyle: .alert)
    XCTAssertTrue(
      ExplorerRouteErrorPresenter.updatePresentedAlertIfPossible(
        alert,
        title: "New alert",
        message: "New message"))
    XCTAssertEqual(alert.title, "New alert")
    XCTAssertEqual(alert.message, "New message")

    let actionSheet = UIAlertController(
      title: "Existing choices",
      message: "Choose a route",
      preferredStyle: .actionSheet)
    XCTAssertFalse(
      ExplorerRouteErrorPresenter.updatePresentedAlertIfPossible(
        actionSheet,
        title: "Unable to Open",
        message: "Malformed URL"))
    XCTAssertEqual(actionSheet.title, "Existing choices")
    XCTAssertEqual(actionSheet.message, "Choose a route")
  }

  @MainActor
  func testRouteErrorPresenterTreatsNavigationBusyAsNonBlocking() {
    XCTAssertFalse(
      ExplorerRouteErrorPresenter.shouldPresent(RouteError.navigationBusy))
    XCTAssertTrue(
      ExplorerRouteErrorPresenter.shouldPresent(RouteError.malformedURL("not a URL")))
  }

  @MainActor
  func testRouteErrorPresenterDismissesNestedActionSheetBeforePresentingAlert() {
    UIView.setAnimationsEnabled(false)
    defer { UIView.setAnimationsEnabled(true) }

    let root = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    let window = UIWindow(frame: UIScreen.main.bounds)
    window.rootViewController = navigationController
    window.makeKeyAndVisible()
    root.loadViewIfNeeded()

    let actionSheet = UIAlertController(
      title: "Open With",
      message: "Choose a container",
      preferredStyle: .actionSheet)
    actionSheet.addAction(UIAlertAction(title: "Cancel", style: .cancel))
    root.present(actionSheet, animated: false)
    XCTAssertTrue(
      ExplorerRouteErrorPresenter.deepestPresentedViewController(
        from: navigationController) === actionSheet)

    ExplorerRouteErrorPresenter(navigationController: navigationController)
      .presentRouteError(RouteError.malformedURL("not a URL"))

    let alertExpectation = expectation(
      for: NSPredicate { _, _ in
        guard let alert = root.presentedViewController as? UIAlertController else {
          return false
        }
        return alert.preferredStyle == .alert
          && alert.title == "Unable to Open (malformed_url)"
      },
      evaluatedWith: root)
    wait(for: [alertExpectation], timeout: 2)

    let presentedAlert = root.presentedViewController as? UIAlertController
    XCTAssertEqual(presentedAlert?.preferredStyle, .alert)
    XCTAssertEqual(presentedAlert?.title, "Unable to Open (malformed_url)")
  }

  @MainActor
  func testExplorerModuleNavigateBackCallsCallbackExactlyOnce() {
    let environment = makeEnvironment()
    RouteCoordinatorBridge.install(environment.bridge)
    let expectation = expectation(description: "navigate back callback")
    var callbackCount = 0
    var payload: [String: Any]?

    ExplorerModule().navigateBack { value in
      callbackCount += 1
      payload = value as? [String: Any]
      expectation.fulfill()
    }

    wait(for: [expectation], timeout: 2)
    XCTAssertEqual(callbackCount, 1)
    XCTAssertEqual(payload?["success"] as? Bool, true)
    XCTAssertEqual(environment.navigation.closeAnimations, [true])
  }

  @MainActor
  func testScannerPresentationUsesCoordinatorNavigationHost() {
    let environment = makeEnvironment()
    RouteCoordinatorBridge.install(environment.bridge)
    let expectation = expectation(description: "scanner presentation callback")
    var callbackCount = 0

    environment.bridge.presentScanner { value in
      callbackCount += 1
      XCTAssertEqual((value as? [String: Any])?["success"] as? Bool, true)
      expectation.fulfill()
    }

    wait(for: [expectation], timeout: 2)
    XCTAssertEqual(callbackCount, 1)
    XCTAssertEqual(environment.navigation.openedPolicies, [.push])
  }

  #if canImport(SparklingMethod) && canImport(Sparkling_Router) && canImport(Sparkling)
    @MainActor
    func testSparklingRouterOpenUsesDescriptorAnimationDefault() {
      let environment = makeEnvironment()
      RouteCoordinatorBridge.install(environment.bridge)
      let service = RouterServiceImpl(bridgeProvider: { environment.bridge })
      let callbackExpectation = expectation(description: "router open callback")
      let params = OpenMethodParamModel()
      params.scheme = "https://example.com/child.lynx.bundle"

      service.openScheme(withParams: params) { status, _ in
        XCTAssertEqual(status.code, .succeeded)
        callbackExpectation.fulfill()
      }

      wait(for: [callbackExpectation], timeout: 2)
      XCTAssertEqual(environment.navigation.openedAnimations, [true])
    }

    @MainActor
    func testSparklingRouterOpenAndCloseMirrorCoordinatorResults() {
      let environment = makeEnvironment()
      RouteCoordinatorBridge.install(environment.bridge)
      let caller = UIViewController()
      let service = RouterServiceImpl(
        bridgeProvider: { environment.bridge },
        callingViewControllerResolver: { _ in caller })
      let openExpectation = expectation(description: "router open callback")
      let closeExpectation = expectation(description: "router close callback")
      let openParams = OpenMethodParamModel()
      openParams.scheme = "https://example.com/child.lynx.bundle"
      openParams.animated = false
      let closeParams = CloseMethodParamModel()
      closeParams.animated = true
      var openCallbackCount = 0
      var closeCallbackCount = 0

      service.openScheme(withParams: openParams) { status, _ in
        openCallbackCount += 1
        XCTAssertEqual(status.code, .succeeded)
        openExpectation.fulfill()
      }
      service.closeContainer(withParams: closeParams) { status, _ in
        closeCallbackCount += 1
        XCTAssertEqual(status.code, .succeeded)
        closeExpectation.fulfill()
      }

      wait(for: [openExpectation, closeExpectation], timeout: 2)
      XCTAssertEqual(openCallbackCount, 1)
      XCTAssertEqual(closeCallbackCount, 1)
      XCTAssertEqual(environment.parser.received.first?.input, openParams.scheme)
      XCTAssertEqual(environment.parser.received.first?.requestedContainer, .sparkling)
      XCTAssertEqual(environment.parser.received.first?.source, .sparklingRouter)
      XCTAssertEqual(environment.navigation.openedPolicies, [.push])
      XCTAssertTrue(environment.navigation.closeAnimations.isEmpty)
      XCTAssertEqual(environment.navigation.targetCloses.count, 1)
      XCTAssertTrue(environment.navigation.targetCloses[0].viewController === caller)
      XCTAssertTrue(environment.navigation.targetCloses[0].animated)
    }

    @MainActor
    func testSparklingRouterRejectsMismatchedCloseContainerID() {
      let environment = makeEnvironment()
      let service = RouterServiceImpl(
        bridgeProvider: { environment.bridge },
        callingViewControllerResolver: { _ in
          XCTFail("A mismatched container ID must not resolve a view controller.")
          return UIViewController()
        })
      let callbackExpectation = expectation(description: "invalid close callback")
      let params = CloseMethodParamModel()
      params.containerID = "different-container"

      service.closeContainer(withParams: params) { status, _ in
        XCTAssertEqual(status.code, .invalidInputParameter)
        XCTAssertTrue(status.message?.contains("containerID") == true)
        callbackExpectation.fulfill()
      }

      wait(for: [callbackExpectation], timeout: 2)
      XCTAssertTrue(environment.navigation.targetCloses.isEmpty)
    }

    func testSparklingRouterCloseContainerIDContract() {
      XCTAssertTrue(
        RouterServiceImpl.closeTargetsCurrentContainer(
          requestedID: nil,
          currentID: nil))
      XCTAssertTrue(
        RouterServiceImpl.closeTargetsCurrentContainer(
          requestedID: "",
          currentID: "current-container"))
      XCTAssertTrue(
        RouterServiceImpl.closeTargetsCurrentContainer(
          requestedID: "current-container",
          currentID: "current-container"))
      XCTAssertFalse(
        RouterServiceImpl.closeTargetsCurrentContainer(
          requestedID: "different-container",
          currentID: "current-container"))
      XCTAssertFalse(
        RouterServiceImpl.closeTargetsCurrentContainer(
          requestedID: "explicit-container",
          currentID: nil))
    }

    @MainActor
    func testSparklingRouterExtraReachesChildDescriptorContextInput() {
      let environment = makeEnvironment()
      let service = RouterServiceImpl(bridgeProvider: { environment.bridge })
      let callbackExpectation = expectation(description: "router extra callback")
      let params = OpenMethodParamModel()
      params.scheme = "https://example.com/extra.lynx.bundle"
      params.extra = [
        "launch_extra": "from-method",
        "launch_count": NSNumber(value: 3),
      ]

      service.openScheme(withParams: params) { status, _ in
        XCTAssertEqual(status.code, .succeeded)
        callbackExpectation.fulfill()
      }

      wait(for: [callbackExpectation], timeout: 2)
      XCTAssertEqual(environment.sparklingLauncher.received.count, 1)
      XCTAssertEqual(
        environment.sparklingLauncher.received[0].extras["launch_extra"],
        "from-method")
      XCTAssertEqual(environment.sparklingLauncher.received[0].extras["launch_count"], 3)
    }

    @MainActor
    func testSparklingRouterSystemBrowserBypassesChildNavigation() {
      let environment = makeEnvironment()
      var openedURL: String?
      let service = RouterServiceImpl(
        bridgeProvider: { environment.bridge },
        systemBrowserOpener: { input in
          openedURL = input
          return true
        })
      let callbackExpectation = expectation(description: "system browser callback")
      let params = OpenMethodParamModel()
      params.scheme = "https://example.com/browser"
      params.useSysBrowser = true

      service.openScheme(withParams: params) { status, _ in
        XCTAssertEqual(status.code, .succeeded)
        callbackExpectation.fulfill()
      }

      wait(for: [callbackExpectation], timeout: 2)
      XCTAssertEqual(openedURL, params.scheme)
      XCTAssertTrue(environment.parser.received.isEmpty)
      XCTAssertTrue(environment.navigation.openedPolicies.isEmpty)
    }

    func testSparklingRouterCurrentThreadEntryRunsUIWorkAndCompletionOnMain() {
      let browserExpectation = expectation(description: "system browser runs on main")
      let callbackExpectation = expectation(description: "current-thread callback")
      callbackExpectation.assertForOverFulfill = true
      let service = RouterServiceImpl(
        systemBrowserOpener: { _ in
          XCTAssertTrue(Thread.isMainThread)
          browserExpectation.fulfill()
          return true
        })
      let params = OpenMethodParamModel()
      params.scheme = "https://example.com/current-thread"
      params.useSysBrowser = true

      DispatchQueue.global(qos: .userInitiated).async {
        XCTAssertFalse(Thread.isMainThread)
        service.openScheme(withParams: params) { status, _ in
          XCTAssertTrue(Thread.isMainThread)
          XCTAssertEqual(status.code, .succeeded)
          callbackExpectation.fulfill()
        }
      }

      wait(for: [browserExpectation, callbackExpectation], timeout: 2)
    }

    @MainActor
    func testSparklingRouterRecognizesAllPinnedReplacementPolicies() {
      let environment = makeEnvironment()
      let caller = UIViewController()
      let service = RouterServiceImpl(
        bridgeProvider: { environment.bridge },
        callingViewControllerResolver: { _ in caller })
      let replaceTypes = [
        "alwaysCloseBeforeOpen",
        "alwaysCloseAfterOpen",
        "onlyCloseAfterOpenSucceed",
      ]
      let callbacks = replaceTypes.map { expectation(description: "replace \($0)") }

      for (index, replaceType) in replaceTypes.enumerated() {
        let params = OpenMethodParamModel()
        params.scheme = "https://example.com/replace-\(index).lynx.bundle"
        params.replace = true
        params.replaceType = replaceType
        params.animated = false
        service.openScheme(withParams: params) { status, _ in
          XCTAssertEqual(status.code, .succeeded)
          callbacks[index].fulfill()
        }
      }

      wait(for: callbacks, timeout: 2)
      XCTAssertEqual(environment.navigation.openedPolicies, [.push])
      XCTAssertEqual(environment.navigation.targetCloses.count, 1)
      XCTAssertTrue(environment.navigation.targetCloses[0].viewController === caller)
      XCTAssertFalse(environment.navigation.targetCloses[0].animated)
      XCTAssertEqual(environment.navigation.replacements.count, 2)
      XCTAssertTrue(
        environment.navigation.replacements.allSatisfy {
          $0.sourceViewController === caller
        })
    }

    @MainActor
    func testSparklingRouterRejectsUnknownReplacementInsteadOfSilentlyPushing() {
      let environment = makeEnvironment()
      let service = RouterServiceImpl(
        bridgeProvider: { environment.bridge },
        callingViewControllerResolver: { _ in UIViewController() })
      let callbackExpectation = expectation(description: "invalid replacement callback")
      let params = OpenMethodParamModel()
      params.scheme = "https://example.com/replace.lynx.bundle"
      params.replace = true
      params.replaceType = "futureReplacement"

      service.openScheme(withParams: params) { status, _ in
        XCTAssertEqual(status.code, .invalidInputParameter)
        XCTAssertTrue(status.message?.contains("replaceType") == true)
        callbackExpectation.fulfill()
      }

      wait(for: [callbackExpectation], timeout: 2)
      XCTAssertTrue(environment.parser.received.isEmpty)
      XCTAssertTrue(environment.navigation.openedPolicies.isEmpty)
      XCTAssertTrue(environment.navigation.replacements.isEmpty)
    }

    @MainActor
    func testSparklingRouterRejectsUnsupportedInterceptorExplicitly() {
      let environment = makeEnvironment()
      let service = RouterServiceImpl(bridgeProvider: { environment.bridge })
      let callbackExpectation = expectation(description: "unsupported interceptor callback")
      let params = OpenMethodParamModel()
      params.scheme = "https://example.com/interceptor.lynx.bundle"
      params.interceptor = "login"

      service.openScheme(withParams: params) { status, _ in
        XCTAssertEqual(status.code, .notImplemented)
        XCTAssertTrue(status.message?.contains("interceptor") == true)
        callbackExpectation.fulfill()
      }

      wait(for: [callbackExpectation], timeout: 2)
      XCTAssertTrue(environment.parser.received.isEmpty)
    }
  #endif

  @MainActor
  private func makeEnvironment(parserError: Error? = nil) -> EntryTestEnvironment {
    let parser = EntryFakeParser(error: parserError)
    let navigation = EntryFakeNavigationHost()
    let sparklingLauncher = EntryFakeLauncher()
    let coordinator = RouteCoordinator(
      parser: parser,
      legacyLauncher: EntryFakeLauncher(),
      sparklingLauncher: sparklingLauncher,
      navigationHost: navigation,
      errorPresenter: EntryFakeErrorPresenter())
    return EntryTestEnvironment(
      bridge: RouteCoordinatorBridge(coordinator: coordinator),
      parser: parser,
      navigation: navigation,
      sparklingLauncher: sparklingLauncher)
  }
}

private struct EntryTestEnvironment {
  let bridge: RouteCoordinatorBridge
  let parser: EntryFakeParser
  let navigation: EntryFakeNavigationHost
  let sparklingLauncher: EntryFakeLauncher
}

private final class EntryFakeParser: LaunchDescriptorParsing {
  struct Received {
    let input: String
    let requestedContainer: RequestedContainer
    let source: RouteSource
  }

  let error: Error?
  private(set) var received: [Received] = []

  init(error: Error?) {
    self.error = error
  }

  func parse(
    _ input: String,
    requestedContainer: RequestedContainer,
    source: RouteSource
  ) throws -> LaunchDescriptor {
    received.append(
      Received(input: input, requestedContainer: requestedContainer, source: source))
    if let error {
      throw error
    }
    let resolved: ResolvedContainer = requestedContainer == .sparkling ? .sparkling : .legacy
    return LaunchDescriptor(
      originalInput: input,
      resource: .remote(URL(string: input)!),
      initialData: nil,
      commonGlobalProps: [:],
      pageGlobalProps: [:],
      pageName: nil,
      viewport: nil,
      appearance: AppearanceOptions(backgroundColor: nil, transparentStatusBar: false),
      navigation: NavigationOptions(
        navigationHidden: false,
        fullScreen: false,
        title: nil,
        titleColor: nil,
        barColor: nil,
        backButtonStyle: nil,
        orientation: nil),
      presentation: PresentationOptions(animated: true),
      queryItems: [],
      extras: [:],
      requestedContainer: requestedContainer,
      container: resolved,
      source: source,
      canonicalSparklingScheme: resolved == .sparkling ? input : nil)
  }
}

@MainActor
private final class EntryFakeLauncher: ContainerLaunching {
  private(set) var received: [LaunchDescriptor] = []

  func makeViewController(for descriptor: LaunchDescriptor) throws -> UIViewController {
    received.append(descriptor)
    return UIViewController()
  }
}

@MainActor
private final class EntryFakeNavigationHost: NavigationHosting {
  struct Replacement {
    let viewController: UIViewController
    let sourceViewController: UIViewController
    let animated: Bool
  }

  struct TargetClose {
    let viewController: UIViewController
    let animated: Bool
  }

  private(set) var openedPolicies: [RouteStackPolicy] = []
  private(set) var openedAnimations: [Bool] = []
  private(set) var closeAnimations: [Bool] = []
  private(set) var replacements: [Replacement] = []
  private(set) var targetCloses: [TargetClose] = []

  func open(
    _ viewController: UIViewController,
    policy: RouteStackPolicy,
    animated: Bool
  ) throws {
    openedPolicies.append(policy)
    openedAnimations.append(animated)
  }

  func close(animated: Bool) throws {
    closeAnimations.append(animated)
  }

  func close(_ viewController: UIViewController, animated: Bool) throws {
    targetCloses.append(TargetClose(viewController: viewController, animated: animated))
  }

  func replace(
    _ viewController: UIViewController,
    replacing sourceViewController: UIViewController,
    animated: Bool
  ) throws {
    replacements.append(
      Replacement(
        viewController: viewController,
        sourceViewController: sourceViewController,
        animated: animated))
  }

  func beginInteractiveClose() throws {}

  func commitInteractiveClose() throws {}

  func endInteractiveClose(completed _: Bool) {}
}

@MainActor
private final class EntryFakeErrorPresenter: RouteErrorPresenting {
  func presentRouteError(_ error: Error) {}
}
