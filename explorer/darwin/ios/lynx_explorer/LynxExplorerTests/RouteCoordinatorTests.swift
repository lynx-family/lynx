// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import UIKit
import XCTest

import ObjectiveC

@testable import LynxExplorer

final class RouteCoordinatorTests: XCTestCase {
  func testRouteCapabilitiesMatchTheLinkedBuildProducts() {
    XCTAssertTrue(RouteCoordinatorBridge.supportsExplicitRouteOwnership)

    #if canImport(Sparkling)
      XCTAssertTrue(RouteCoordinatorBridge.supportsSparklingContainer)
    #else
      XCTAssertFalse(RouteCoordinatorBridge.supportsSparklingContainer)
    #endif
  }

  @MainActor
  func testNavigationHostPushesOntoTheInjectedNavigationController() throws {
    let root = UIViewController()
    let child = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    let host = NavigationHost(navigationController: navigationController)

    try host.open(child, policy: .push, animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 2)
    XCTAssertTrue(navigationController.viewControllers[0] === root)
    XCTAssertTrue(navigationController.viewControllers[1] === child)
  }

  @MainActor
  func testNavigationHostReplaceTopPreservesTheRestOfTheStack() throws {
    let root = UIViewController()
    let previousTop = UIViewController()
    let replacement = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, previousTop], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    try host.open(replacement, policy: .replaceTop, animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 2)
    XCTAssertTrue(navigationController.viewControllers[0] === root)
    XCTAssertTrue(navigationController.viewControllers[1] === replacement)
  }

  @MainActor
  func testNavigationHostReplaceTopUsesNewControllerWhenStackIsEmpty() throws {
    let replacement = UIViewController()
    let navigationController = UINavigationController()
    let host = NavigationHost(navigationController: navigationController)

    try host.open(replacement, policy: .replaceTop, animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 1)
    XCTAssertTrue(navigationController.viewControllers[0] === replacement)
  }

  @MainActor
  func testNavigationHostReplaceTopPushesWhenOnlyRootExists() throws {
    let root = UIViewController()
    let replacement = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    let host = NavigationHost(navigationController: navigationController)

    try host.open(replacement, policy: .replaceTop, animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 2)
    XCTAssertTrue(navigationController.viewControllers[0] === root)
    XCTAssertTrue(navigationController.viewControllers[1] === replacement)
  }

  @MainActor
  func testNavigationHostResetAndPushPreservesOnlyOriginalRoot() throws {
    let root = UIViewController()
    let intermediate = UIViewController()
    let previousTop = UIViewController()
    let destination = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, intermediate, previousTop], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    try host.open(destination, policy: .resetAndPush, animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 2)
    XCTAssertTrue(navigationController.viewControllers[0] === root)
    XCTAssertTrue(navigationController.viewControllers[1] === destination)
  }

  @MainActor
  func testNavigationHostResetAndPushUsesDestinationWhenStackIsEmpty() throws {
    let destination = UIViewController()
    let navigationController = UINavigationController()
    let host = NavigationHost(navigationController: navigationController)

    try host.open(destination, policy: .resetAndPush, animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 1)
    XCTAssertTrue(navigationController.viewControllers[0] === destination)
  }

  @MainActor
  func testNavigationHostClosesOnlyWhenAParentRemains() throws {
    let root = UIViewController()
    let child = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, child], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    try host.close(animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 1)
    XCTAssertTrue(navigationController.viewControllers[0] === root)
  }

  @MainActor
  func testNavigationHostClosesCoveredCallingControllerWithoutPoppingTop() throws {
    let root = UIViewController()
    let caller = UIViewController()
    let coveredTop = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, caller, coveredTop], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    try host.close(caller, animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 2)
    XCTAssertTrue(navigationController.viewControllers[0] === root)
    XCTAssertTrue(navigationController.viewControllers[1] === coveredTop)
  }

  @MainActor
  func testNavigationHostReplacesCoveredCallerByAppendingDestination() throws {
    let root = UIViewController()
    let caller = UIViewController()
    let coveredTop = UIViewController()
    let destination = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, caller, coveredTop], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    try host.replace(destination, replacing: caller, animated: false)

    XCTAssertEqual(navigationController.viewControllers.count, 3)
    XCTAssertTrue(navigationController.viewControllers[0] === root)
    XCTAssertTrue(navigationController.viewControllers[1] === coveredTop)
    XCTAssertTrue(navigationController.viewControllers[2] === destination)
  }

  @MainActor
  func testNavigationHostRejectsTargetOutsideOwnedStack() {
    let root = UIViewController()
    let unknownCaller = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    let host = NavigationHost(navigationController: navigationController)

    XCTAssertThrowsError(try host.close(unknownCaller, animated: false)) { error in
      XCTAssertEqual(error as? RouteError, .routeOwnerUnavailable)
    }
    XCTAssertTrue(navigationController.topViewController === root)
  }

  @MainActor
  func testNavigationHostRejectsClosingTheRootController() {
    let root = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    let host = NavigationHost(navigationController: navigationController)

    XCTAssertThrowsError(try host.close(animated: false)) { error in
      XCTAssertEqual(error as? RouteError, .cannotCloseRoot)
    }
    XCTAssertEqual(navigationController.viewControllers.count, 1)
    XCTAssertTrue(navigationController.viewControllers[0] === root)
  }

  @MainActor
  func testNavigationHostDisablesUnownedSystemInteractivePopGesture() throws {
    let root = UIViewController()
    let child = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, child], animated: false)
    let gesture = try XCTUnwrap(navigationController.interactivePopGestureRecognizer)
    gesture.isEnabled = true

    _ = NavigationHost(navigationController: navigationController)

    XCTAssertFalse(gesture.isEnabled)
  }

  @MainActor
  func testNavigationHostSerializesAuthorizedInteractiveCloseUntilItEnds() throws {
    let root = UIViewController()
    let child = UIViewController()
    let destination = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, child], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    try host.beginInteractiveClose()
    XCTAssertThrowsError(try host.beginInteractiveClose()) { error in
      XCTAssertEqual(error as? RouteError, .navigationBusy)
    }
    XCTAssertThrowsError(try host.open(destination, policy: .push, animated: false)) { error in
      XCTAssertEqual(error as? RouteError, .navigationBusy)
    }

    host.endInteractiveClose(completed: false)

    XCTAssertNoThrow(try host.open(destination, policy: .push, animated: false))
    XCTAssertTrue(navigationController.topViewController === destination)
  }

  @MainActor
  func testNavigationHostCommitsReservedInteractiveCloseAtomically() throws {
    let root = UIViewController()
    let child = UIViewController()
    let destination = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, child], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    try host.beginInteractiveClose()
    XCTAssertThrowsError(try host.open(destination, policy: .push, animated: false)) { error in
      XCTAssertEqual(error as? RouteError, .navigationBusy)
    }

    try host.commitInteractiveClose()

    XCTAssertTrue(navigationController.topViewController === root)
    XCTAssertNoThrow(try host.open(destination, policy: .push, animated: false))
    XCTAssertTrue(navigationController.topViewController === destination)
  }

  @MainActor
  func testNavigationHostRejectsInteractiveCommitWithoutReservation() {
    let root = UIViewController()
    let child = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, child], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    XCTAssertThrowsError(try host.commitInteractiveClose()) { error in
      XCTAssertEqual(error as? RouteError, .navigationBusy)
    }
    XCTAssertTrue(navigationController.topViewController === child)
  }

  @MainActor
  func testNavigationHostRejectsInteractiveCloseAtRoot() {
    let navigationController = UINavigationController(rootViewController: UIViewController())
    let host = NavigationHost(navigationController: navigationController)

    XCTAssertThrowsError(try host.beginInteractiveClose()) { error in
      XCTAssertEqual(error as? RouteError, .cannotCloseRoot)
    }
  }

  @MainActor
  func testNavigationHostRejectsBackToBackAnimatedOpenThenRecovers() async throws {
    let root = UIViewController()
    let first = UIViewController()
    let second = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    let host = NavigationHost(navigationController: navigationController)

    try host.open(first, policy: .push, animated: true)

    XCTAssertThrowsError(try host.open(second, policy: .push, animated: true)) { error in
      XCTAssertEqual(error as? RouteError, .navigationBusy)
    }

    await nextMainRunLoop()
    XCTAssertNoThrow(try host.open(second, policy: .push, animated: false))
    XCTAssertTrue(navigationController.topViewController === second)
  }

  @MainActor
  func testNavigationHostRejectsBackToBackAnimatedCloseThenRecovers() async throws {
    let root = UIViewController()
    let child = UIViewController()
    let grandchild = UIViewController()
    let navigationController = UINavigationController(rootViewController: root)
    navigationController.setViewControllers([root, child, grandchild], animated: false)
    let host = NavigationHost(navigationController: navigationController)

    try host.close(animated: true)

    XCTAssertThrowsError(try host.close(animated: true)) { error in
      XCTAssertEqual(error as? RouteError, .navigationBusy)
    }

    await nextMainRunLoop()
    XCTAssertNoThrow(try host.close(animated: false))
    XCTAssertTrue(navigationController.topViewController === root)
  }

  @MainActor
  func testCoordinatorSelectsLegacyLauncherAndForwardsPolicyAndAnimation() {
    let descriptor = makeDescriptor(container: .legacy, animated: false)
    let environment = makeEnvironment(descriptor: descriptor)
    var completion: Result<Void, Error>?

    environment.coordinator.open(
      RouteRequest(
        input: descriptor.originalInput,
        requestedContainer: .automatic,
        source: .manualInput,
        stackPolicy: .replaceTop,
        animatedOverride: nil)
    ) { completion = $0 }

    assertSuccess(completion)
    XCTAssertEqual(environment.legacy.received, [descriptor])
    XCTAssertTrue(environment.sparkling.received.isEmpty)
    XCTAssertEqual(environment.navigation.opened.count, 1)
    XCTAssertEqual(environment.navigation.opened[0].policy, .replaceTop)
    XCTAssertFalse(environment.navigation.opened[0].animated)
  }

  @MainActor
  func testCoordinatorAnimationOverrideWinsOverDescriptor() {
    let descriptor = makeDescriptor(container: .sparkling, animated: true)
    let environment = makeEnvironment(descriptor: descriptor)

    environment.coordinator.open(
      RouteRequest(
        input: descriptor.originalInput,
        requestedContainer: .sparkling,
        source: .scanner,
        stackPolicy: .push,
        animatedOverride: false)
    ) { _ in }

    XCTAssertEqual(environment.sparkling.received, [descriptor])
    XCTAssertEqual(environment.navigation.opened.count, 1)
    XCTAssertFalse(environment.navigation.opened[0].animated)
  }

  @MainActor
  func testCoordinatorPassesRequestMetadataToParser() {
    let descriptor = makeDescriptor(container: .legacy)
    let environment = makeEnvironment(descriptor: descriptor)
    let request = RouteRequest(
      input: "file://lynx?local://child.lynx.bundle",
      requestedContainer: .legacy,
      source: .universalLink,
      stackPolicy: .resetAndPush,
      animatedOverride: nil)

    environment.coordinator.open(request) { _ in }

    XCTAssertEqual(environment.parser.received, [request.parseInput])
    XCTAssertEqual(environment.navigation.opened.first?.policy, .resetAndPush)
  }

  @MainActor
  func testCoordinatorEnrichesCommonPropsOnceAndPreservesDescriptorPrecedence() {
    let descriptor = makeDescriptor(
      container: .sparkling,
      commonGlobalProps: [
        "descriptorOnly": "descriptor",
        "theme": "DescriptorTheme",
      ],
      pageGlobalProps: ["theme": "PageTheme"])
    var providerCallCount = 0
    let environment = makeEnvironment(
      descriptor: descriptor,
      commonGlobalPropsProvider: {
        providerCallCount += 1
        return [
          "isNotchScreen": true,
          "providerOnly": "provider",
          "theme": "ProviderTheme",
        ]
      })

    environment.coordinator.open(makeRequest()) { _ in }

    XCTAssertEqual(providerCallCount, 1)
    XCTAssertEqual(environment.sparkling.received.count, 1)
    XCTAssertEqual(
      environment.sparkling.received.first?.commonGlobalProps,
      [
        "descriptorOnly": "descriptor",
        "isNotchScreen": true,
        "providerOnly": "provider",
        "theme": "DescriptorTheme",
      ])
    XCTAssertEqual(
      environment.sparkling.received.first?.pageGlobalProps,
      ["theme": "PageTheme"])
  }

  @MainActor
  func testCoordinatorMergesRouterExtraIntoChildDescriptorWithMethodPrecedence() {
    let descriptor = makeDescriptor(
      container: .sparkling,
      extras: [
        "queryOnly": "query",
        "shared": "query",
      ])
    let environment = makeEnvironment(descriptor: descriptor)
    let request = RouteRequest(
      input: descriptor.originalInput,
      requestedContainer: .sparkling,
      source: .sparklingRouter,
      stackPolicy: .push,
      animatedOverride: false,
      extra: [
        "methodOnly": 7,
        "shared": "method",
      ])

    environment.coordinator.open(request) { _ in }

    XCTAssertEqual(environment.sparkling.received.count, 1)
    XCTAssertEqual(environment.sparkling.received.first?.extras["queryOnly"], "query")
    XCTAssertEqual(environment.sparkling.received.first?.extras["methodOnly"], 7)
    XCTAssertEqual(environment.sparkling.received.first?.extras["shared"], "method")
  }

  @MainActor
  func testCoordinatorReplacesCallingControllerInsteadOfSilentlyPushing() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .sparkling))
    let caller = UIViewController()
    var completion: Result<Void, Error>?

    environment.coordinator.open(
      makeRequest(),
      replacement: RouteReplacement(
        sourceViewController: caller,
        policy: .onlyCloseAfterOpenSucceed)
    ) { completion = $0 }

    assertSuccess(completion)
    XCTAssertTrue(environment.navigation.opened.isEmpty)
    XCTAssertEqual(environment.navigation.replacements.count, 1)
    XCTAssertTrue(environment.navigation.replacements[0].sourceViewController === caller)
  }

  @MainActor
  func testAlwaysCloseBeforeOpenClosesCallerBeforeCreatingDestination() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .sparkling))
    let caller = UIViewController()
    var events: [String] = []
    environment.navigation.onTargetClose = { events.append("close") }
    environment.sparkling.onMake = { events.append("make") }
    environment.navigation.onOpen = { events.append("open") }
    var completion: Result<Void, Error>?

    environment.coordinator.open(
      makeRequest(),
      replacement: RouteReplacement(
        sourceViewController: caller,
        policy: .alwaysCloseBeforeOpen)
    ) { completion = $0 }

    assertSuccess(completion)
    XCTAssertEqual(events, ["close", "make", "open"])
    XCTAssertEqual(environment.navigation.targetCloses.count, 1)
    XCTAssertEqual(environment.navigation.opened.count, 1)
    XCTAssertTrue(environment.navigation.replacements.isEmpty)
  }

  @MainActor
  func testCoordinatorAlwaysReplacementPoliciesCloseCallerWhenOpenFails() {
    let caller = UIViewController()
    let policies: [RouteReplacementPolicy] = [
      .alwaysCloseBeforeOpen,
      .alwaysCloseAfterOpen,
      .onlyCloseAfterOpenSucceed,
    ]

    for policy in policies {
      let environment = makeEnvironment(descriptor: makeDescriptor(container: .sparkling))
      environment.sparkling.error = TestError.sparklingCreation

      environment.coordinator.open(
        makeRequest(),
        replacement: RouteReplacement(
          sourceViewController: caller,
          policy: policy)
      ) { _ in }

      let expectedCloseCount = policy == .onlyCloseAfterOpenSucceed ? 0 : 1
      XCTAssertEqual(
        environment.navigation.targetCloses.count,
        expectedCloseCount,
        "Unexpected failure close behavior for \(policy)")
    }
  }

  @MainActor
  func testParserFailureIsPresentedAndCompletedExactlyOnce() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))
    environment.parser.error = TestError.parsing
    var completions = 0
    var completedError: Error?

    environment.coordinator.open(makeRequest()) { result in
      completions += 1
      if case .failure(let error) = result {
        completedError = error
      }
    }

    XCTAssertEqual(completions, 1)
    XCTAssertEqual(completedError as? TestError, .parsing)
    XCTAssertEqual(environment.presenter.errors.count, 1)
    XCTAssertEqual(environment.presenter.errors.first as? TestError, .parsing)
    XCTAssertTrue(environment.legacy.received.isEmpty)
    XCTAssertTrue(environment.sparkling.received.isEmpty)
    XCTAssertTrue(environment.navigation.opened.isEmpty)
  }

  @MainActor
  func testLegacyLauncherFailureIsPresentedWithoutOpeningNavigation() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))
    environment.legacy.error = TestError.legacyCreation
    var completedError: Error?

    environment.coordinator.open(makeRequest()) { result in
      if case .failure(let error) = result {
        completedError = error
      }
    }

    XCTAssertEqual(completedError as? TestError, .legacyCreation)
    XCTAssertEqual(environment.legacy.received.count, 1)
    XCTAssertTrue(environment.sparkling.received.isEmpty)
    XCTAssertTrue(environment.navigation.opened.isEmpty)
    XCTAssertEqual(environment.presenter.errors.first as? TestError, .legacyCreation)
  }

  @MainActor
  func testSparklingLauncherFailureNeverFallsBackToLegacy() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .sparkling))
    environment.sparkling.error = TestError.sparklingCreation
    var completedError: Error?

    environment.coordinator.open(
      RouteRequest(
        input: "hybrid://lynxview_page?bundle=child.lynx.bundle",
        requestedContainer: .legacy,
        source: .externalURL,
        stackPolicy: .push,
        animatedOverride: nil)
    ) { result in
      if case .failure(let error) = result {
        completedError = error
      }
    }

    XCTAssertEqual(completedError as? TestError, .sparklingCreation)
    XCTAssertEqual(environment.sparkling.received.count, 1)
    XCTAssertTrue(environment.legacy.received.isEmpty)
    XCTAssertTrue(environment.navigation.opened.isEmpty)
    XCTAssertEqual(environment.presenter.errors.first as? TestError, .sparklingCreation)
  }

  @MainActor
  func testNavigationFailureIsPresentedAndCompletedOnce() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))
    environment.navigation.openError = TestError.navigation
    var completions = 0
    var completedError: Error?

    environment.coordinator.open(makeRequest()) { result in
      completions += 1
      if case .failure(let error) = result {
        completedError = error
      }
    }

    XCTAssertEqual(completions, 1)
    XCTAssertEqual(completedError as? TestError, .navigation)
    XCTAssertEqual(environment.presenter.errors.first as? TestError, .navigation)
  }

  @MainActor
  func testCoordinatorCloseUsesNavigationHostAndReportsSuccessOnce() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))
    var completions = 0
    var result: Result<Void, Error>?

    environment.coordinator.close {
      completions += 1
      result = $0
    }

    XCTAssertEqual(completions, 1)
    assertSuccess(result)
    XCTAssertEqual(environment.navigation.closeAnimations, [true])
    XCTAssertTrue(environment.presenter.errors.isEmpty)
  }

  @MainActor
  func testCoordinatorSerializesInteractiveCloseWithoutPresentingGestureDenial() {
    let descriptor = makeDescriptor(container: .sparkling)
    let environment = makeEnvironment(descriptor: descriptor)

    XCTAssertTrue(environment.coordinator.beginInteractiveCloseSynchronously())
    XCTAssertEqual(environment.navigation.interactiveCloseBeginCount, 1)

    environment.coordinator.endInteractiveCloseSynchronously(completed: false)
    XCTAssertEqual(environment.navigation.interactiveCloseCompletions, [false])
    XCTAssertTrue(environment.presenter.errors.isEmpty)

    environment.navigation.interactiveCloseError = RouteError.navigationBusy
    XCTAssertFalse(environment.coordinator.beginInteractiveCloseSynchronously())
    XCTAssertTrue(environment.presenter.errors.isEmpty)
  }

  @MainActor
  func testCoordinatorCommitsReservedInteractiveCloseWithoutPresentingGestureDenial() {
    let descriptor = makeDescriptor(container: .legacy)
    let environment = makeEnvironment(descriptor: descriptor)

    XCTAssertTrue(environment.coordinator.beginInteractiveCloseSynchronously())
    XCTAssertTrue(environment.coordinator.commitInteractiveCloseSynchronously())
    XCTAssertEqual(environment.navigation.interactiveCloseCommitCount, 1)
    XCTAssertTrue(environment.presenter.errors.isEmpty)

    environment.navigation.interactiveCloseError = RouteError.navigationBusy
    XCTAssertFalse(environment.coordinator.commitInteractiveCloseSynchronously())
    XCTAssertTrue(environment.presenter.errors.isEmpty)
  }

  @MainActor
  func testCoordinatorCloseForwardsExplicitAnimationChoice() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))

    environment.coordinator.close(animated: false) { _ in }

    XCTAssertEqual(environment.navigation.closeAnimations, [false])
  }

  @MainActor
  func testCoordinatorCloseFailureIsPresented() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))
    environment.navigation.closeError = RouteError.cannotCloseRoot
    var completedError: Error?

    environment.coordinator.close { result in
      if case .failure(let error) = result {
        completedError = error
      }
    }

    XCTAssertEqual(completedError as? RouteError, .cannotCloseRoot)
    XCTAssertEqual(environment.presenter.errors.first as? RouteError, .cannotCloseRoot)
  }

  @MainActor
  func testObjectiveCBridgeDispatchesCompletionExactlyOnceOnMainThread() {
    let descriptor = makeDescriptor(container: .legacy)
    let environment = makeEnvironment(descriptor: descriptor)
    let bridge = RouteCoordinatorBridge(coordinator: environment.coordinator)
    let callbackExpectation = expectation(description: "bridge callback")
    let input = descriptor.originalInput
    var callbackCount = 0
    var payload: [String: Any]?
    var callbackWasOnMain = false

    DispatchQueue.global(qos: .userInitiated).async {
      bridge.openURL(
        input,
        requestedContainer: .automatic,
        source: .manualInput,
        presentation: .push,
        animatedOverride: nil
      ) { value in
        callbackCount += 1
        payload = value as? [String: Any]
        callbackWasOnMain = Thread.isMainThread
        callbackExpectation.fulfill()
      }
    }

    wait(for: [callbackExpectation], timeout: 2)
    XCTAssertEqual(callbackCount, 1)
    XCTAssertTrue(callbackWasOnMain)
    XCTAssertEqual(payload?["success"] as? Bool, true)
    XCTAssertEqual(payload?["code"] as? String, "ok")
    XCTAssertEqual(payload?["message"] as? String, "")
    XCTAssertEqual(Set(payload?.keys.map { $0 } ?? []), Set(["success", "code", "message"]))
  }

  @MainActor
  func testObjectiveCBridgeMapsTypedFailureToStablePayload() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))
    environment.parser.error = RouteError.emptyInput
    let bridge = RouteCoordinatorBridge(coordinator: environment.coordinator)
    let callbackExpectation = expectation(description: "bridge error callback")
    var payload: [String: Any]?

    bridge.openURL(
      "",
      requestedContainer: .automatic,
      source: .nativeModule,
      presentation: .replaceTop,
      animatedOverride: NSNumber(value: false)
    ) { value in
      payload = value as? [String: Any]
      callbackExpectation.fulfill()
    }

    wait(for: [callbackExpectation], timeout: 2)
    XCTAssertEqual(payload?["success"] as? Bool, false)
    XCTAssertEqual(payload?["code"] as? String, "empty_input")
    XCTAssertEqual(
      payload?["message"] as? String,
      RouteError.emptyInput.localizedDescription)
    XCTAssertEqual(Set(payload?.keys.map { $0 } ?? []), Set(["success", "code", "message"]))
  }

  @MainActor
  func testConcurrentBridgeRequestsKeepTheirOwnDescriptors() {
    let parser = FakeParser { input, requestedContainer, source in
      makeDescriptor(
        input: input,
        container: requestedContainer == .sparkling ? .sparkling : .legacy,
        requestedContainer: requestedContainer,
        source: source)
    }
    let legacy = FakeLauncher()
    let sparkling = FakeLauncher()
    let navigation = FakeNavigationHost()
    let presenter = FakeErrorPresenter()
    let coordinator = RouteCoordinator(
      parser: parser,
      legacyLauncher: legacy,
      sparklingLauncher: sparkling,
      navigationHost: navigation,
      errorPresenter: presenter)
    let bridge = RouteCoordinatorBridge(coordinator: coordinator)
    let legacyCallback = expectation(description: "legacy callback")
    let sparklingCallback = expectation(description: "sparkling callback")
    let legacyInput = "https://example.com/legacy.lynx.bundle"
    let sparklingInput = "https://example.com/sparkling.lynx.bundle"

    DispatchQueue.global(qos: .userInitiated).async {
      bridge.openURL(
        legacyInput,
        requestedContainer: .legacy,
        source: .scanner,
        presentation: .push,
        animatedOverride: nil
      ) { _ in legacyCallback.fulfill() }
    }
    DispatchQueue.global(qos: .utility).async {
      bridge.openURL(
        sparklingInput,
        requestedContainer: .sparkling,
        source: .debugBridge,
        presentation: .resetAndPush,
        animatedOverride: nil
      ) { _ in sparklingCallback.fulfill() }
    }

    wait(for: [legacyCallback, sparklingCallback], timeout: 2)
    XCTAssertEqual(legacy.received.map(\.originalInput), [legacyInput])
    XCTAssertEqual(sparkling.received.map(\.originalInput), [sparklingInput])
    XCTAssertEqual(legacy.received.first?.source, .scanner)
    XCTAssertEqual(sparkling.received.first?.source, .debugBridge)
    XCTAssertEqual(Set(navigation.opened.map(\.policy)), Set([.push, .resetAndPush]))
  }

  @MainActor
  func testObjectiveCBridgeCloseReturnsCompletePayloadOnMainThread() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))
    let bridge = RouteCoordinatorBridge(coordinator: environment.coordinator)
    let callbackExpectation = expectation(description: "close callback")
    var payload: [String: Any]?
    var callbackWasOnMain = false

    DispatchQueue.global(qos: .userInitiated).async {
      bridge.close { value in
        payload = value as? [String: Any]
        callbackWasOnMain = Thread.isMainThread
        callbackExpectation.fulfill()
      }
    }

    wait(for: [callbackExpectation], timeout: 2)
    XCTAssertTrue(callbackWasOnMain)
    XCTAssertEqual(payload?["success"] as? Bool, true)
    XCTAssertEqual(payload?["code"] as? String, "ok")
    XCTAssertEqual(payload?["message"] as? String, "")
    XCTAssertEqual(Set(payload?.keys.map { $0 } ?? []), Set(["success", "code", "message"]))
  }

  @MainActor
  func testObjectiveCBridgeCloseForwardsAnimationChoice() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .legacy))
    let bridge = RouteCoordinatorBridge(coordinator: environment.coordinator)
    let callbackExpectation = expectation(description: "animated close callback")

    bridge.close(animated: false) { _ in callbackExpectation.fulfill() }

    wait(for: [callbackExpectation], timeout: 2)
    XCTAssertEqual(environment.navigation.closeAnimations, [false])
  }

  @MainActor
  func testHostBackBridgeConsumesEventAfterSynchronousCloseSucceeds() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .sparkling))
    let bridge = RouteCoordinatorBridge(coordinator: environment.coordinator)

    XCTAssertTrue(bridge.closeFromHostBack(animated: false))
    XCTAssertEqual(environment.navigation.closeAnimations, [false])
    XCTAssertTrue(environment.presenter.errors.isEmpty)
  }

  @MainActor
  func testHostBackBridgeStillConsumesEventWhenSynchronousCloseFails() {
    let environment = makeEnvironment(descriptor: makeDescriptor(container: .sparkling))
    environment.navigation.closeError = RouteError.navigationBusy
    let bridge = RouteCoordinatorBridge(coordinator: environment.coordinator)

    XCTAssertTrue(bridge.closeFromHostBack(animated: true))
    XCTAssertTrue(environment.navigation.closeAnimations.isEmpty)
    XCTAssertEqual(environment.presenter.errors.first as? RouteError, .navigationBusy)
  }

  @MainActor
  func testBackgroundInstallIsVisibleBeforeReturning() async {
    RouteCoordinatorBridge.install(nil)
    let bridge = RouteCoordinatorBridge(
      coordinator: makeEnvironment(descriptor: makeDescriptor(container: .legacy)).coordinator)

    let installedBridgeIsCurrent = await withCheckedContinuation { continuation in
      DispatchQueue.global(qos: .userInitiated).async {
        RouteCoordinatorBridge.install(bridge)
        continuation.resume(returning: RouteCoordinatorBridge.currentBridge() === bridge)
      }
    }

    XCTAssertTrue(installedBridgeIsCurrent)
    XCTAssertTrue(RouteCoordinatorBridge.currentBridge() === bridge)
    RouteCoordinatorBridge.install(nil)
  }

  @MainActor
  func testBackgroundInstallReleasesReplacedBridgeOnMainThread() async {
    RouteCoordinatorBridge.install(nil)
    var oldBridge: RouteCoordinatorBridge? = RouteCoordinatorBridge(
      coordinator: makeEnvironment(descriptor: makeDescriptor(container: .legacy)).coordinator)
    let replacement = RouteCoordinatorBridge(
      coordinator: makeEnvironment(descriptor: makeDescriptor(container: .sparkling)).coordinator)
    let releasedOnMain = LockedValue<Bool?>(nil)
    var probe: DeinitProbe? = DeinitProbe {
      releasedOnMain.value = Thread.isMainThread
    }
    objc_setAssociatedObject(
      oldBridge!,
      &bridgeDeinitProbeKey,
      probe,
      .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
    probe = nil
    weak var releasedBridge = oldBridge
    RouteCoordinatorBridge.install(oldBridge)
    oldBridge = nil

    await withCheckedContinuation { continuation in
      DispatchQueue.global(qos: .userInitiated).async {
        RouteCoordinatorBridge.install(replacement)
        continuation.resume()
      }
    }

    XCTAssertNil(releasedBridge)
    XCTAssertEqual(releasedOnMain.value, true)
    RouteCoordinatorBridge.install(nil)
  }

  @MainActor
  private func makeEnvironment(
    descriptor: LaunchDescriptor,
    commonGlobalPropsProvider: (@MainActor () -> [String: AnyHashable])? = nil
  ) -> TestEnvironment {
    let parser = FakeParser { _, _, _ in descriptor }
    let legacy = FakeLauncher()
    let sparkling = FakeLauncher()
    let navigation = FakeNavigationHost()
    let presenter = FakeErrorPresenter()
    let coordinator = RouteCoordinator(
      parser: parser,
      legacyLauncher: legacy,
      sparklingLauncher: sparkling,
      navigationHost: navigation,
      errorPresenter: presenter,
      commonGlobalPropsProvider: commonGlobalPropsProvider)
    return TestEnvironment(
      coordinator: coordinator,
      parser: parser,
      legacy: legacy,
      sparkling: sparkling,
      navigation: navigation,
      presenter: presenter)
  }

  private func makeRequest() -> RouteRequest {
    RouteRequest(
      input: "https://example.com/home.lynx.bundle",
      requestedContainer: .automatic,
      source: .manualInput,
      stackPolicy: .push,
      animatedOverride: nil)
  }

  private func assertSuccess(
    _ result: Result<Void, Error>?,
    file: StaticString = #filePath,
    line: UInt = #line
  ) {
    guard let result else {
      XCTFail("Expected completion", file: file, line: line)
      return
    }
    if case .failure(let error) = result {
      XCTFail("Expected success, got \(error)", file: file, line: line)
    }
  }

  @MainActor
  private func nextMainRunLoop() async {
    await withCheckedContinuation { continuation in
      DispatchQueue.main.async {
        continuation.resume()
      }
    }
  }
}

private struct TestEnvironment {
  let coordinator: RouteCoordinator
  let parser: FakeParser
  let legacy: FakeLauncher
  let sparkling: FakeLauncher
  let navigation: FakeNavigationHost
  let presenter: FakeErrorPresenter
}

private enum TestError: Error, Equatable {
  case parsing
  case legacyCreation
  case sparklingCreation
  case navigation
}

private var bridgeDeinitProbeKey: UInt8 = 0

private final class DeinitProbe {
  private let onDeinit: () -> Void

  init(onDeinit: @escaping () -> Void) {
    self.onDeinit = onDeinit
  }

  deinit {
    onDeinit()
  }
}

private final class LockedValue<Value>: @unchecked Sendable {
  private let lock = NSLock()
  private var storedValue: Value

  init(_ value: Value) {
    storedValue = value
  }

  var value: Value {
    get {
      lock.lock()
      defer { lock.unlock() }
      return storedValue
    }
    set {
      lock.lock()
      storedValue = newValue
      lock.unlock()
    }
  }
}

private final class FakeParser: LaunchDescriptorParsing {
  typealias Builder = (String, RequestedContainer, RouteSource) throws -> LaunchDescriptor

  struct Received: Equatable {
    let input: String
    let requestedContainer: RequestedContainer
    let source: RouteSource
  }

  private let builder: Builder
  var error: Error?
  private(set) var received: [Received] = []

  init(builder: @escaping Builder) {
    self.builder = builder
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
    return try builder(input, requestedContainer, source)
  }
}

private extension RouteRequest {
  var parseInput: FakeParser.Received {
    FakeParser.Received(
      input: input,
      requestedContainer: requestedContainer,
      source: source)
  }
}

@MainActor
private final class FakeLauncher: ContainerLaunching {
  var error: Error?
  var onMake: (() -> Void)?
  private(set) var received: [LaunchDescriptor] = []

  func makeViewController(for descriptor: LaunchDescriptor) throws -> UIViewController {
    onMake?()
    received.append(descriptor)
    if let error {
      throw error
    }
    return UIViewController()
  }
}

@MainActor
private final class FakeNavigationHost: NavigationHosting {
  struct Opened {
    let viewController: UIViewController
    let policy: RouteStackPolicy
    let animated: Bool
  }

  struct Replacement {
    let viewController: UIViewController
    let sourceViewController: UIViewController
    let animated: Bool
  }

  struct TargetClose {
    let viewController: UIViewController
    let animated: Bool
  }

  var openError: Error?
  var closeError: Error?
  var interactiveCloseError: Error?
  var onOpen: (() -> Void)?
  var onTargetClose: (() -> Void)?
  private(set) var opened: [Opened] = []
  private(set) var replacements: [Replacement] = []
  private(set) var targetCloses: [TargetClose] = []
  private(set) var closeAnimations: [Bool] = []
  private(set) var interactiveCloseBeginCount = 0
  private(set) var interactiveCloseCommitCount = 0
  private(set) var interactiveCloseCompletions: [Bool] = []

  func open(
    _ viewController: UIViewController,
    policy: RouteStackPolicy,
    animated: Bool
  ) throws {
    onOpen?()
    if let openError {
      throw openError
    }
    opened.append(Opened(viewController: viewController, policy: policy, animated: animated))
  }

  func close(animated: Bool) throws {
    if let closeError {
      throw closeError
    }
    closeAnimations.append(animated)
  }

  func close(_ viewController: UIViewController, animated: Bool) throws {
    onTargetClose?()
    if let closeError {
      throw closeError
    }
    targetCloses.append(TargetClose(viewController: viewController, animated: animated))
  }

  func replace(
    _ viewController: UIViewController,
    replacing sourceViewController: UIViewController,
    animated: Bool
  ) throws {
    if let openError {
      throw openError
    }
    replacements.append(
      Replacement(
        viewController: viewController,
        sourceViewController: sourceViewController,
        animated: animated))
  }

  func beginInteractiveClose() throws {
    if let interactiveCloseError {
      throw interactiveCloseError
    }
    interactiveCloseBeginCount += 1
  }

  func commitInteractiveClose() throws {
    if let interactiveCloseError {
      throw interactiveCloseError
    }
    interactiveCloseCommitCount += 1
  }

  func endInteractiveClose(completed: Bool) {
    interactiveCloseCompletions.append(completed)
  }
}

@MainActor
private final class FakeErrorPresenter: RouteErrorPresenting {
  private(set) var errors: [Error] = []

  func presentRouteError(_ error: Error) {
    errors.append(error)
  }
}

private func makeDescriptor(
  input: String = "https://example.com/home.lynx.bundle",
  container: ResolvedContainer,
  requestedContainer: RequestedContainer = .automatic,
  source: RouteSource = .manualInput,
  animated: Bool = true,
  commonGlobalProps: [String: AnyHashable] = [:],
  pageGlobalProps: [String: AnyHashable] = [:],
  extras: [String: AnyHashable] = [:]
) -> LaunchDescriptor {
  LaunchDescriptor(
    originalInput: input,
    resource: .remote(URL(string: input)!),
    initialData: nil,
    commonGlobalProps: commonGlobalProps,
    pageGlobalProps: pageGlobalProps,
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
    presentation: PresentationOptions(animated: animated),
    cachePolicy: .useProtocolCachePolicy,
    queryItems: [],
    extras: extras,
    requestedContainer: requestedContainer,
    container: container,
    source: source,
    canonicalSparklingScheme: container == .sparkling ? input : nil)
}
