// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import UIKit

/// A source-specific request before it crosses the single coordinator boundary.
/// Keeping these policies as values makes every UIKit/DebugBridge/module entry
/// use the same tested precedence and stack semantics.
struct RouteEntryRequest: Equatable {
  let input: String
  let requestedContainer: RequestedContainer
  let source: RouteSource
  let presentation: RouteStackPolicy
  let animatedOverride: Bool?

  init(
    input: String,
    requestedContainer: RequestedContainer,
    source: RouteSource,
    presentation: RouteStackPolicy,
    animatedOverride: Bool? = nil
  ) {
    self.input = input
    self.requestedContainer = requestedContainer
    self.source = source
    self.presentation = presentation
    self.animatedOverride = animatedOverride
  }

  var routeRequest: RouteRequest {
    RouteRequest(
      input: input,
      requestedContainer: requestedContainer,
      source: source,
      stackPolicy: presentation,
      animatedOverride: animatedOverride)
  }
}

enum RouteEntryAdapter {
  static let scannerContainerChoices: [RequestedContainer] = [.legacy, .sparkling]

  static func startupRequest(
    environment: [String: String],
    launchOptions: [UIApplication.LaunchOptionsKey: Any],
    homepageURL: String,
    isRunningTests: Bool
  ) -> RouteEntryRequest? {
    if let initialURL = environment["lynx_initial_url"], !initialURL.isRouteBlank {
      return RouteEntryRequest(
        input: initialURL,
        requestedContainer: .automatic,
        source: .startup,
        presentation: .push)
    }

    if let launchInput = coldLaunchInput(in: launchOptions) {
      return RouteEntryRequest(
        input: launchInput,
        requestedContainer: .automatic,
        source: .externalURL,
        presentation: .push)
    }

    if let universalLinkInput = coldUniversalLinkInput(in: launchOptions),
      let webpageURL = URL(string: universalLinkInput)
    {
      return universalLinkRequest(for: webpageURL)
    }

    // The host test bundle still needs the fully configured coordinator and
    // native modules. Only suppress the implicit homepage, which otherwise
    // starts a Lynx runtime before XCTest has injected the test bundle.
    guard !isRunningTests, !homepageURL.isRouteBlank else {
      return nil
    }
    return RouteEntryRequest(
      input: homepageURL,
      requestedContainer: .automatic,
      source: .startup,
      presentation: .push)
  }

  static func hotURLRequest(for url: URL) -> RouteEntryRequest {
    RouteEntryRequest(
      input: url.absoluteString,
      requestedContainer: .automatic,
      source: .externalURL,
      presentation: .push)
  }

  static func universalLinkRequest(for url: URL) -> RouteEntryRequest {
    RouteEntryRequest(
      input: url.absoluteString,
      requestedContainer: .automatic,
      source: .universalLink,
      presentation: .push)
  }

  static func debugBridgeRequest(for input: String) -> RouteEntryRequest {
    let components = URLComponents(string: input.trimmingCharacters(in: .whitespacesAndNewlines))
    let isLocalExplorerURL =
      components?.scheme?.lowercased() == "file"
      && components?.host?.lowercased() == "lynx"
      && components?.percentEncodedQuery?.lowercased().hasPrefix("local://") == true
    return RouteEntryRequest(
      input: input,
      requestedContainer: .automatic,
      source: .debugBridge,
      presentation: isLocalExplorerURL ? .replaceTop : .resetAndPush)
  }

  static func moduleRequest(
    for input: String,
    containerName: String
  ) -> RouteEntryRequest? {
    let requestedContainer: RequestedContainer
    switch containerName.trimmingCharacters(in: .whitespacesAndNewlines).lowercased() {
    case "automatic":
      requestedContainer = .automatic
    case "legacy":
      requestedContainer = .legacy
    case "sparkling":
      requestedContainer = .sparkling
    default:
      return nil
    }
    return RouteEntryRequest(
      input: input,
      requestedContainer: requestedContainer,
      source: .nativeModule,
      presentation: .push)
  }

  static func scannerRequest(
    for input: String,
    container: RequestedContainer
  ) -> RouteEntryRequest? {
    guard scannerContainerChoices.contains(container), !input.isRouteBlank else {
      return nil
    }
    return RouteEntryRequest(
      input: input,
      requestedContainer: container,
      source: .scanner,
      presentation: .replaceTop)
  }

  static func sparklingRouterRequest(
    for input: String,
    animated: Bool? = nil
  ) -> RouteEntryRequest {
    RouteEntryRequest(
      input: input,
      requestedContainer: .sparkling,
      source: .sparklingRouter,
      presentation: .push,
      animatedOverride: animated)
  }

  static func coldLaunchInput(
    in launchOptions: [UIApplication.LaunchOptionsKey: Any]
  ) -> String? {
    if let launchURL = launchOptions[.url] as? URL {
      return launchURL.absoluteString
    }
    if let launchURL = launchOptions[.url] as? NSURL {
      return launchURL.absoluteString
    }
    return nil
  }

  static func coldUniversalLinkInput(
    in launchOptions: [UIApplication.LaunchOptionsKey: Any]
  ) -> String? {
    guard
      let activityValue = launchOptions[.userActivityDictionary],
      let activity = userActivity(in: activityValue)
    else {
      return nil
    }
    return activity.webpageURL?.absoluteString
  }

  private static func userActivity(in value: Any) -> NSUserActivity? {
    if let activity = value as? NSUserActivity {
      return activity
    }
    if let dictionary = value as? [AnyHashable: Any] {
      for nestedValue in dictionary.values {
        if let activity = userActivity(in: nestedValue) {
          return activity
        }
      }
    }
    if let array = value as? [Any] {
      for nestedValue in array {
        if let activity = userActivity(in: nestedValue) {
          return activity
        }
      }
    }
    return nil
  }
}

/// UIKit may deliver a cold URL/user activity in launch options and then
/// immediately repeat it through the corresponding AppDelegate callback.
/// Suppression is one-shot and time-bounded so a later user-initiated open of
/// the same URL remains valid.
final class ColdRouteDeliverySuppressor {
  private struct PendingDelivery {
    let input: String
    let source: RouteSource
    let expiresAt: Date
  }

  private let lock = NSLock()
  private let lifetime: TimeInterval
  private let now: () -> Date
  private var pendingDeliveries: [PendingDelivery] = []

  init(
    lifetime: TimeInterval = 5,
    now: @escaping () -> Date = Date.init
  ) {
    self.lifetime = lifetime
    self.now = now
  }

  func remember(input: String?, source: RouteSource) {
    guard let input, !input.isRouteBlank else {
      return
    }
    lock.lock()
    defer { lock.unlock() }
    let currentDate = now()
    pendingDeliveries.removeAll { $0.expiresAt <= currentDate || $0.source == source }
    pendingDeliveries.append(
      PendingDelivery(
        input: input,
        source: source,
        expiresAt: currentDate.addingTimeInterval(lifetime)))
  }

  func shouldSuppress(input: String, source: RouteSource) -> Bool {
    lock.lock()
    defer { lock.unlock() }
    let currentDate = now()
    pendingDeliveries.removeAll { $0.expiresAt <= currentDate }
    guard
      let index = pendingDeliveries.firstIndex(where: {
        $0.input == input && $0.source == source
      })
    else {
      return false
    }
    pendingDeliveries.remove(at: index)
    return true
  }
}

/// Supplies the host-owned common props that both Explorer containers should
/// receive. Dependencies are closures so the values are sampled for each
/// route and can be tested without changing process-wide UIKit state.
@MainActor
struct ExplorerCommonGlobalPropsProvider {
  private let userInterfaceStyle: @MainActor () -> UIUserInterfaceStyle
  private let preferredTheme: @MainActor () -> String?
  private let safeAreaInsets: @MainActor () -> UIEdgeInsets

  init(
    userInterfaceStyle: @escaping @MainActor () -> UIUserInterfaceStyle,
    preferredTheme: @escaping @MainActor () -> String?,
    safeAreaInsets: @escaping @MainActor () -> UIEdgeInsets
  ) {
    self.userInterfaceStyle = userInterfaceStyle
    self.preferredTheme = preferredTheme
    self.safeAreaInsets = safeAreaInsets
  }

  func makeGlobalProps() -> [String: AnyHashable] {
    let insets = safeAreaInsets()
    var props: [String: AnyHashable] = [
      "isNotchScreen": insets.top > 20 || insets.bottom > 0 || insets.left > 0
        || insets.right > 0,
      "theme": userInterfaceStyle() == .dark ? "Dark" : "Light",
    ]
    if let preferredTheme = preferredTheme() {
      props["preferredTheme"] = preferredTheme
    }
    return props
  }

  static func live(
    navigationController: UINavigationController
  ) -> ExplorerCommonGlobalPropsProvider {
    ExplorerCommonGlobalPropsProvider(
      userInterfaceStyle: {
        UIScreen.main.traitCollection.userInterfaceStyle
      },
      preferredTheme: {
        UserDefaults.standard.object(forKey: "preferredTheme") as? String
      },
      safeAreaInsets: {
        let view = navigationController.viewIfLoaded
        let windowInsets = view?.window?.safeAreaInsets ?? .zero
        return windowInsets == .zero ? view?.safeAreaInsets ?? .zero : windowInsets
      })
  }
}

@MainActor
final class ExplorerRouteErrorPresenter: RouteErrorPresenting {
  private weak var navigationController: UINavigationController?

  init(navigationController: UINavigationController) {
    self.navigationController = navigationController
  }

  func presentRouteError(_ error: Error) {
    // Busy is an expected, transient serialization result. Callers still get
    // the typed failure and routing logs still record it, but interrupting the
    // user with an alert makes rapid taps and cancelled gestures much worse.
    guard Self.shouldPresent(error) else {
      return
    }
    guard let navigationController else {
      return
    }
    let code = (error as? RouteError)?.code ?? "route_failed"
    let title = "Unable to Open (\(code))"
    let message = error.localizedDescription

    let presentedViewController = Self.deepestPresentedViewController(
      from: navigationController)
    if Self.updatePresentedAlertIfPossible(
      presentedViewController,
      title: title,
      message: message)
    {
      return
    }

    let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: "OK", style: .default))
    if let actionSheet = presentedViewController as? UIAlertController,
      actionSheet.preferredStyle == .actionSheet
    {
      actionSheet.dismiss(animated: true) { [weak navigationController] in
        guard let navigationController else {
          return
        }
        Self.present(alert, from: navigationController)
      }
      return
    }
    let presenter = presentedViewController
      ?? navigationController.visibleViewController
      ?? navigationController
    presenter.present(alert, animated: true)
  }

  static func shouldPresent(_ error: Error) -> Bool {
    (error as? RouteError) != .navigationBusy
  }

  static func updatePresentedAlertIfPossible(
    _ viewController: UIViewController?,
    title: String,
    message: String
  ) -> Bool {
    guard
      let alert = viewController as? UIAlertController,
      alert.preferredStyle == .alert
    else {
      return false
    }
    alert.title = title
    alert.message = message
    return true
  }

  static func deepestPresentedViewController(
    from navigationController: UINavigationController
  ) -> UIViewController? {
    if let presented = deepestPresentedViewController(
      startingAt: navigationController.presentedViewController)
    {
      return presented
    }
    return deepestPresentedViewController(
      startingAt: navigationController.visibleViewController?.presentedViewController)
  }

  private static func deepestPresentedViewController(
    startingAt viewController: UIViewController?
  ) -> UIViewController? {
    var current = viewController
    while let presented = current?.presentedViewController {
      current = presented
    }
    return current
  }

  private static func present(
    _ alert: UIAlertController,
    from navigationController: UINavigationController
  ) {
    let presenter = navigationController.visibleViewController ?? navigationController
    presenter.present(alert, animated: true)
  }
}

/// The application composition root. This is the only production location
/// that creates a coordinator and binds it to Explorer's navigation stack.
@objc(LXExplorerRouteComposition)
@MainActor
final class ExplorerRouteComposition: NSObject {
  @objc(makeBridgeWithNavigationController:)
  static func makeBridge(
    navigationController: UINavigationController
  ) -> RouteCoordinatorBridge {
    let navigationHost = NavigationHost(navigationController: navigationController)
    let recorderURLPrefix = LXExplorerRecorderURLPrefix() ?? ""
    let commonGlobalPropsProvider = ExplorerCommonGlobalPropsProvider.live(
      navigationController: navigationController)
    let coordinator = RouteCoordinator(
      parser: LaunchDescriptorParser(recorderURLPrefix: recorderURLPrefix),
      legacyLauncher: LegacyContainerLauncher(),
      sparklingLauncher: SparklingContainerLauncher(),
      navigationHost: navigationHost,
      errorPresenter: ExplorerRouteErrorPresenter(
        navigationController: navigationController),
      commonGlobalPropsProvider: commonGlobalPropsProvider.makeGlobalProps)
    return RouteCoordinatorBridge(coordinator: coordinator)
  }
}

extension String {
  fileprivate var isRouteBlank: Bool {
    trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
  }
}
