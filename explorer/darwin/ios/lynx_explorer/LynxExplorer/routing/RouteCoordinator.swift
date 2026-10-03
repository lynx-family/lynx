// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import UIKit
import os.log

#if canImport(Lynx)
  import Lynx
#endif

#if canImport(Sparkling)
  import Sparkling
#endif

private let explorerRouteLog = OSLog(
  subsystem: "org.lynx.explorer",
  category: "routing")

protocol LaunchDescriptorParsing {
  func parse(
    _ input: String,
    requestedContainer: RequestedContainer,
    source: RouteSource
  ) throws -> LaunchDescriptor
}

extension LaunchDescriptorParser: LaunchDescriptorParsing {}

@MainActor
protocol ContainerLaunching: AnyObject {
  func makeViewController(for descriptor: LaunchDescriptor) throws -> UIViewController
}

@MainActor
protocol RouteErrorPresenting: AnyObject {
  func presentRouteError(_ error: Error)
}

struct RouteRequest: Equatable {
  let input: String
  let requestedContainer: RequestedContainer
  let source: RouteSource
  let stackPolicy: RouteStackPolicy
  let animatedOverride: Bool?
  let extra: [String: AnyHashable]

  init(
    input: String,
    requestedContainer: RequestedContainer,
    source: RouteSource,
    stackPolicy: RouteStackPolicy,
    animatedOverride: Bool?,
    extra: [String: AnyHashable] = [:]
  ) {
    self.input = input
    self.requestedContainer = requestedContainer
    self.source = source
    self.stackPolicy = stackPolicy
    self.animatedOverride = animatedOverride
    self.extra = extra
  }
}

enum RouteReplacementPolicy: Equatable {
  case alwaysCloseBeforeOpen
  case alwaysCloseAfterOpen
  case onlyCloseAfterOpenSucceed
}

struct RouteReplacement {
  let sourceViewController: UIViewController
  let policy: RouteReplacementPolicy
}

/// Parses every route once, selects exactly one container, and delegates the
/// sole navigation mutation to `NavigationHosting`.
@MainActor
final class RouteCoordinator: NSObject {
  private let parser: LaunchDescriptorParsing
  private let legacyLauncher: ContainerLaunching
  private let sparklingLauncher: ContainerLaunching
  private let navigationHost: NavigationHosting
  private let errorPresenter: RouteErrorPresenting
  private let commonGlobalPropsProvider: @MainActor () -> [String: AnyHashable]

  init(
    parser: LaunchDescriptorParsing,
    legacyLauncher: ContainerLaunching,
    sparklingLauncher: ContainerLaunching,
    navigationHost: NavigationHosting,
    errorPresenter: RouteErrorPresenting,
    commonGlobalPropsProvider: (@MainActor () -> [String: AnyHashable])? = nil
  ) {
    self.parser = parser
    self.legacyLauncher = legacyLauncher
    self.sparklingLauncher = sparklingLauncher
    self.navigationHost = navigationHost
    self.errorPresenter = errorPresenter
    self.commonGlobalPropsProvider = commonGlobalPropsProvider ?? { [:] }
    super.init()
  }

  func open(
    _ request: RouteRequest,
    replacement: RouteReplacement? = nil,
    completion: @escaping (Result<Void, Error>) -> Void
  ) {
    do {
      var remainingReplacement = replacement
      if let replacement,
        replacement.policy == .alwaysCloseBeforeOpen
      {
        // This policy intentionally gives up the calling container even when
        // parsing or destination creation later fails. Use a non-animated
        // targeted mutation so the subsequent open is not rejected as a
        // concurrent UIKit transition.
        try navigationHost.close(
          replacement.sourceViewController,
          animated: false)
        remainingReplacement = nil
      }

      let parsedDescriptor = try parser.parse(
        request.input,
        requestedContainer: request.requestedContainer,
        source: request.source)
      let descriptor = parsedDescriptor
        .addingCommonGlobalPropDefaults(commonGlobalPropsProvider())
        .mergingExtras(request.extra)
      let launcher: ContainerLaunching
      switch descriptor.container {
      case .legacy:
        launcher = legacyLauncher
      case .sparkling:
        launcher = sparklingLauncher
      }

      // Deliberately do not catch a Sparkling failure and retry with Legacy.
      // Container ownership is final once the descriptor has been parsed.
      let viewController = try launcher.makeViewController(for: descriptor)
      let animated = request.animatedOverride ?? descriptor.presentation.animated
      if let replacement = remainingReplacement {
        // Both post-open policies have the same successful final stack. Use
        // one atomic host mutation so no asynchronous push/close interleaving
        // can remove the wrong controller.
        try navigationHost.replace(
          viewController,
          replacing: replacement.sourceViewController,
          animated: animated)
      } else {
        try navigationHost.open(
          viewController,
          policy: request.stackPolicy,
          animated: animated)
      }
      RouteSessionRecorder.record(descriptor)
      Self.logSuccess(container: descriptor.container)
      completion(.success(()))
    } catch {
      if let replacement,
        replacement.policy == .alwaysCloseAfterOpen
      {
        // This policy owns source teardown after both success and failure.
        // Keep the original open error as the public result.
        try? navigationHost.close(
          replacement.sourceViewController,
          animated: request.animatedOverride ?? true)
      }
      Self.logFailure(error)
      errorPresenter.presentRouteError(error)
      completion(.failure(error))
    }
  }

  func close(
    animated: Bool = true,
    completion: @escaping (Result<Void, Error>) -> Void
  ) {
    completion(closeSynchronously(animated: animated))
  }

  func close(
    _ viewController: UIViewController,
    animated: Bool,
    completion: @escaping (Result<Void, Error>) -> Void
  ) {
    do {
      try navigationHost.close(viewController, animated: animated)
      completion(.success(()))
    } catch {
      errorPresenter.presentRouteError(error)
      completion(.failure(error))
    }
  }

  @discardableResult
  fileprivate func closeSynchronously(animated: Bool = true) -> Result<Void, Error> {
    do {
      try navigationHost.close(animated: animated)
      return .success(())
    } catch {
      errorPresenter.presentRouteError(error)
      return .failure(error)
    }
  }

  @discardableResult
  func beginInteractiveCloseSynchronously() -> Bool {
    do {
      try navigationHost.beginInteractiveClose()
      return true
    } catch {
      Self.logFailure(error)
      return false
    }
  }

  @discardableResult
  func commitInteractiveCloseSynchronously() -> Bool {
    do {
      try navigationHost.commitInteractiveClose()
      return true
    } catch {
      Self.logFailure(error)
      return false
    }
  }

  func endInteractiveCloseSynchronously(completed: Bool) {
    navigationHost.endInteractiveClose(completed: completed)
  }

  func presentUtility(
    _ viewController: UIViewController,
    animated: Bool,
    completion: @escaping (Result<Void, Error>) -> Void
  ) {
    do {
      try navigationHost.open(viewController, policy: .push, animated: animated)
      completion(.success(()))
    } catch {
      errorPresenter.presentRouteError(error)
      completion(.failure(error))
    }
  }

  func rejectOpen(
    _ error: Error,
    completion: @escaping (Result<Void, Error>) -> Void
  ) {
    Self.logFailure(error)
    errorPresenter.presentRouteError(error)
    completion(.failure(error))
  }

  private static func logSuccess(container: ResolvedContainer) {
    let containerName = container == .sparkling ? "sparkling" : "legacy"
    #if canImport(Lynx)
      let lynxVersion = LynxVersion.versionString() ?? "unknown"
    #else
      let lynxVersion = "unavailable"
    #endif
    os_log(
      "%{public}@",
      log: explorerRouteLog,
      type: .info,
      "LynxExplorer route_success container=\(containerName) lynx_source=local lynx_version=\(lynxVersion)"
    )
  }

  private static func logFailure(_ error: Error) {
    let code = (error as? RouteError)?.code ?? "route_failed"
    os_log(
      "%{public}@",
      log: explorerRouteLog,
      type: .error,
      "LynxExplorer route_failure code=\(code)")
  }
}

private enum RouteSessionRecorder {
  static let storageKey = "explorerNativeLaunchSessionsV1"
  static let maximumCount = 20

  static func record(_ descriptor: LaunchDescriptor) {
    let source: String
    switch descriptor.source {
    case .scanner:
      source = "scan"
    case .manualInput:
      source = "input"
    case .externalURL, .universalLink, .debugBridge, .recorder:
      source = "external"
    case .startup, .nativeModule, .sparklingRouter:
      // Startup is the shell itself. Native-module launches are recorded by
      // the universal Home after the callback succeeds, preserving richer
      // source labels such as `showcase` without creating duplicates.
      return
    }

    let runtime = descriptor.container == .sparkling ? "sparkling" : "lynx"
    let theme = resolvedTheme(for: descriptor)
    let openedAt = Int64(Date().timeIntervalSince1970 * 1_000)
    let session: [String: Any] = [
      "id": "native-\(openedAt)-\(runtime)-\(descriptor.originalInput)",
      "url": descriptor.originalInput,
      "runtime": runtime,
      "source": source,
      "openedAt": openedAt,
      "fullscreen": descriptor.navigation.fullScreen,
      "hiddenNav": descriptor.navigation.navigationHidden,
      "theme": theme ?? NSNull(),
    ]

    var sessions = storedSessions().filter {
      ($0["url"] as? String) != descriptor.originalInput
        || ($0["runtime"] as? String) != runtime
    }
    sessions.insert(session, at: 0)
    if sessions.count > maximumCount {
      sessions.removeLast(sessions.count - maximumCount)
    }
    guard JSONSerialization.isValidJSONObject(sessions),
      let data = try? JSONSerialization.data(withJSONObject: sessions),
      let value = String(data: data, encoding: .utf8)
    else {
      return
    }
    UserDefaults.standard.set(value, forKey: storageKey)
  }

  private static func storedSessions() -> [[String: Any]] {
    guard let value = UserDefaults.standard.string(forKey: storageKey),
      let data = value.data(using: .utf8),
      let sessions = try? JSONSerialization.jsonObject(with: data) as? [[String: Any]]
    else {
      return []
    }
    return sessions
  }

  private static func resolvedTheme(for descriptor: LaunchDescriptor) -> String? {
    if let style = descriptor.navigation.backButtonStyle?.lowercased(),
      style == "dark" || style == "light"
    {
      return style
    }
    return descriptor.queryItems.first(where: {
      $0.name == "force_theme_style"
    }).flatMap { item in
      guard let value = item.value?.lowercased(), value == "dark" || value == "light"
      else { return nil }
      return value
    }
  }
}

@objc enum LXRouteSource: Int {
  case manualInput
  case scanner
  case startup
  case externalURL
  case universalLink
  case debugBridge
  case nativeModule
  case sparklingRouter
  case recorder

  fileprivate var routeSource: RouteSource {
    switch self {
    case .manualInput:
      return .manualInput
    case .scanner:
      return .scanner
    case .startup:
      return .startup
    case .externalURL:
      return .externalURL
    case .universalLink:
      return .universalLink
    case .debugBridge:
      return .debugBridge
    case .nativeModule:
      return .nativeModule
    case .sparklingRouter:
      return .sparklingRouter
    case .recorder:
      return .recorder
    }
  }
}

@objc enum LXRequestedContainer: Int {
  case automatic
  case legacy
  case sparkling

  fileprivate var requestedContainer: RequestedContainer {
    switch self {
    case .automatic:
      return .automatic
    case .legacy:
      return .legacy
    case .sparkling:
      return .sparkling
    }
  }
}

@objc enum LXRoutePresentation: Int {
  case push
  case replaceTop
  case resetAndPush

  fileprivate var stackPolicy: RouteStackPolicy {
    switch self {
    case .push:
      return .push
    case .replaceTop:
      return .replaceTop
    case .resetAndPush:
      return .resetAndPush
    }
  }
}

/// Objective-C-facing facade. The fully configured instance is installed by
/// Explorer's composition root; it never discovers a navigation controller or
/// constructs a partial coordinator on demand.
@objc(LXRouteCoordinator)
final class RouteCoordinatorBridge: NSObject, @unchecked Sendable {
  private static let installationLock = NSLock()
  private static var installedBridge: RouteCoordinatorBridge?

  private let coordinator: RouteCoordinator
  private let coldDeliverySuppressor: ColdRouteDeliverySuppressor

  /// Host-owned signal consumed through Lynx global props. Keep capability
  /// discovery out of page code: reading absent native methods can emit
  /// FUNCTION_NOT_FOUND diagnostics on Android and Harmony.
  @objc(supportsExplicitRouteOwnership)
  static var supportsExplicitRouteOwnership: Bool {
    true
  }

  /// Build capability is separate from route ownership: Legacy-only builds
  /// still own navigation through this coordinator, but cannot create a
  /// Sparkling container.
  @objc(supportsSparklingContainer)
  static var supportsSparklingContainer: Bool {
    #if canImport(Sparkling)
      true
    #else
      false
    #endif
  }

  /// The extension card reports the framework actually linked into this app,
  /// rather than duplicating a package version in frontend source.
  @objc(sparklingVersion)
  static var sparklingVersion: String? {
    #if canImport(Sparkling)
      guard
        let resourceURL = Bundle.main.url(
          forResource: "sparklingPageResource", withExtension: "bundle"),
        let resourceBundle = Bundle(url: resourceURL)
      else {
        return nil
      }
      return resourceBundle.object(
        forInfoDictionaryKey: "CFBundleShortVersionString") as? String
    #else
      nil
    #endif
  }

  @MainActor
  init(
    coordinator: RouteCoordinator,
    coldDeliverySuppressor: ColdRouteDeliverySuppressor = ColdRouteDeliverySuppressor()
  ) {
    self.coordinator = coordinator
    self.coldDeliverySuppressor = coldDeliverySuppressor
    super.init()
  }

  @objc(installBridge:)
  static func install(_ bridge: RouteCoordinatorBridge?) {
    if Thread.isMainThread {
      installOnMain(bridge)
      return
    }

    DispatchQueue.main.sync {
      installOnMain(bridge)
    }
  }

  private static func installOnMain(_ bridge: RouteCoordinatorBridge?) {
    dispatchPrecondition(condition: .onQueue(.main))

    let previousBridge: RouteCoordinatorBridge?
    installationLock.lock()
    previousBridge = installedBridge
    installedBridge = bridge
    installationLock.unlock()

    // Keep the previous ownership graph alive until after the lock is released.
    // Because this helper always runs on main, UIKit-owned objects also tear
    // down on main even when installation originated on a background queue.
    withExtendedLifetime(previousBridge) {}
  }

  @objc(currentBridge)
  static func currentBridge() -> RouteCoordinatorBridge? {
    installationLock.lock()
    defer { installationLock.unlock() }
    return installedBridge
  }

  @objc(openURL:requestedContainer:source:presentation:animatedOverride:callback:)
  func openURL(
    _ input: String,
    requestedContainer: LXRequestedContainer,
    source: LXRouteSource,
    presentation: LXRoutePresentation,
    animatedOverride: NSNumber?,
    callback: @escaping (Any) -> Void
  ) {
    let request = RouteRequest(
      input: input,
      requestedContainer: requestedContainer.requestedContainer,
      source: source.routeSource,
      stackPolicy: presentation.stackPolicy,
      animatedOverride: animatedOverride?.boolValue)

    // Objective-C callers bypass Swift global-actor checking. Always enqueue
    // the operation so parsing, launcher creation, navigation, and callbacks
    // are performed on the main thread.
    DispatchQueue.main.async { [coordinator] in
      coordinator.open(request) { result in
        callback(Self.callbackPayload(for: result))
      }
    }
  }

  @objc(closeWithCallback:)
  func close(callback: @escaping (Any) -> Void) {
    close(animated: true, callback: callback)
  }

  @objc(closeAnimated:callback:)
  func close(animated: Bool, callback: @escaping (Any) -> Void) {
    DispatchQueue.main.async { [coordinator] in
      coordinator.close(animated: animated) { result in
        callback(Self.callbackPayload(for: result))
      }
    }
  }

  /// Sparkling's public back hook is synchronous and is guaranteed to run on
  /// the main thread. Once Explorer installs this ownership hook, always
  /// consume the event: returning false would ask Sparkling to perform a
  /// coordinator-bypassing fallback pop after a busy/root failure.
  @MainActor
  func closeFromHostBack(animated: Bool) -> Bool {
    _ = coordinator.closeSynchronously(animated: animated)
    return true
  }

  @objc(beginInteractiveCloseFromHost)
  @MainActor
  func beginInteractiveCloseFromHost() -> Bool {
    coordinator.beginInteractiveCloseSynchronously()
  }

  @objc(commitInteractiveCloseFromHost)
  @MainActor
  func commitInteractiveCloseFromHost() -> Bool {
    coordinator.commitInteractiveCloseSynchronously()
  }

  @objc(endInteractiveCloseFromHostCompleted:)
  @MainActor
  func endInteractiveCloseFromHost(completed: Bool) {
    coordinator.endInteractiveCloseSynchronously(completed: completed)
  }

  @objc(openStartupWithEnvironment:launchOptions:homepageURL:isRunningTests:callback:)
  func openStartup(
    environment: [String: String],
    launchOptions: [String: Any],
    homepageURL: String,
    isRunningTests: Bool,
    callback: @escaping (Any) -> Void
  ) {
    let typedLaunchOptions = Dictionary(
      uniqueKeysWithValues: launchOptions.map {
        (UIApplication.LaunchOptionsKey(rawValue: $0.key), $0.value)
      })
    guard
      let request = RouteEntryAdapter.startupRequest(
        environment: environment,
        launchOptions: typedLaunchOptions,
        homepageURL: homepageURL,
        isRunningTests: isRunningTests)
    else {
      DispatchQueue.main.async {
        callback([
          "success": true,
          "code": "no_route",
          "message": "",
        ])
      }
      return
    }
    switch request.source {
    case .externalURL:
      coldDeliverySuppressor.remember(input: request.input, source: .externalURL)
    case .universalLink:
      coldDeliverySuppressor.remember(input: request.input, source: .universalLink)
    default:
      break
    }
    openEntry(request, callback: callback)
  }

  @objc(openExternalURL:callback:)
  func openExternalURL(
    _ url: URL,
    callback: @escaping (Any) -> Void
  ) {
    guard
      !coldDeliverySuppressor.shouldSuppress(
        input: url.absoluteString,
        source: .externalURL)
    else {
      completeSuppressedColdDelivery(callback: callback)
      return
    }
    openEntry(RouteEntryAdapter.hotURLRequest(for: url), callback: callback)
  }

  @objc(openUniversalLinkURL:callback:)
  func openUniversalLinkURL(
    _ url: URL,
    callback: @escaping (Any) -> Void
  ) {
    guard
      !coldDeliverySuppressor.shouldSuppress(
        input: url.absoluteString,
        source: .universalLink)
    else {
      completeSuppressedColdDelivery(callback: callback)
      return
    }
    openEntry(RouteEntryAdapter.universalLinkRequest(for: url), callback: callback)
  }

  @objc(openDebugURL:callback:)
  func openDebugURL(
    _ input: String,
    callback: @escaping (Any) -> Void
  ) {
    openEntry(RouteEntryAdapter.debugBridgeRequest(for: input), callback: callback)
  }

  @objc(openModuleURL:container:callback:)
  func openModuleURL(
    _ input: String,
    container: String,
    callback: @escaping (Any) -> Void
  ) {
    guard
      let request = RouteEntryAdapter.moduleRequest(
        for: input,
        containerName: container)
    else {
      rejectOpen(
        RouteError.invalidParameter(name: "container", value: container),
        callback: callback)
      return
    }
    openEntry(request, callback: callback)
  }

  @objc(openScannerURL:requestedContainer:callback:)
  func openScannerURL(
    _ input: String,
    requestedContainer: LXRequestedContainer,
    callback: @escaping (Any) -> Void
  ) {
    guard
      let request = RouteEntryAdapter.scannerRequest(
        for: input,
        container: requestedContainer.requestedContainer)
    else {
      rejectOpen(
        RouteError.invalidParameter(name: "container", value: nil),
        callback: callback)
      return
    }
    openEntry(request, callback: callback)
  }

  @objc(openSparklingRouterURL:animated:callback:)
  func openSparklingRouterURL(
    _ input: String,
    animated: NSNumber?,
    callback: @escaping (Any) -> Void
  ) {
    openSparklingRouterURL(
      input,
      animated: animated,
      extra: [:],
      replacement: nil,
      callback: callback)
  }

  func openSparklingRouterURL(
    _ input: String,
    animated: NSNumber?,
    extra: [String: AnyHashable],
    replacement: RouteReplacement?,
    callback: @escaping (Any) -> Void
  ) {
    let entry = RouteEntryAdapter.sparklingRouterRequest(
      for: input,
      animated: animated?.boolValue)
    let base = entry.routeRequest
    let request = RouteRequest(
      input: base.input,
      requestedContainer: base.requestedContainer,
      source: base.source,
      stackPolicy: base.stackPolicy,
      animatedOverride: base.animatedOverride,
      extra: extra)
    DispatchQueue.main.async { [coordinator] in
      coordinator.open(request, replacement: replacement) { result in
        callback(Self.callbackPayload(for: result))
      }
    }
  }

  func closeSparklingRouterViewController(
    _ viewController: UIViewController,
    animated: Bool,
    callback: @escaping (Any) -> Void
  ) {
    DispatchQueue.main.async { [coordinator] in
      coordinator.close(viewController, animated: animated) { result in
        callback(Self.callbackPayload(for: result))
      }
    }
  }

  @objc(presentScannerWithCallback:)
  func presentScanner(callback: @escaping (Any) -> Void) {
    DispatchQueue.main.async { [coordinator] in
      coordinator.presentUtility(ScanViewController(), animated: true) { result in
        callback(Self.callbackPayload(for: result))
      }
    }
  }

  private func openEntry(
    _ entry: RouteEntryRequest,
    callback: @escaping (Any) -> Void
  ) {
    DispatchQueue.main.async { [coordinator] in
      coordinator.open(entry.routeRequest) { result in
        callback(Self.callbackPayload(for: result))
      }
    }
  }

  private func rejectOpen(
    _ error: Error,
    callback: @escaping (Any) -> Void
  ) {
    DispatchQueue.main.async { [coordinator] in
      coordinator.rejectOpen(error) { result in
        callback(Self.callbackPayload(for: result))
      }
    }
  }

  private func completeSuppressedColdDelivery(
    callback: @escaping (Any) -> Void
  ) {
    DispatchQueue.main.async {
      callback([
        "success": true,
        "code": "duplicate_cold_delivery",
        "message": "",
      ])
    }
  }

  private static func callbackPayload(for result: Result<Void, Error>) -> [String: Any] {
    switch result {
    case .success:
      return [
        "success": true,
        "code": "ok",
        "message": "",
      ]
    case .failure(let error):
      let code = (error as? RouteError)?.code ?? "route_failed"
      return [
        "success": false,
        "code": code,
        "message": error.localizedDescription,
      ]
    }
  }
}
