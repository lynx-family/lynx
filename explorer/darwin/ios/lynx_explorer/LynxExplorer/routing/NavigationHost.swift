// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import UIKit

enum RouteStackPolicy: Equatable, Hashable {
  case push
  case replaceTop
  case resetAndPush
}

@MainActor
protocol NavigationHosting: AnyObject {
  func open(
    _ viewController: UIViewController,
    policy: RouteStackPolicy,
    animated: Bool
  ) throws

  func close(animated: Bool) throws

  /// Removes a specific container from the owned stack without assuming it is
  /// still the top controller.
  func close(_ viewController: UIViewController, animated: Bool) throws

  /// Replaces a specific calling container while preserving controllers that
  /// may have been pushed above it.
  func replace(
    _ viewController: UIViewController,
    replacing sourceViewController: UIViewController,
    animated: Bool
  ) throws

  /// Reserves the stack for UIKit's cancellable system-pop transition without
  /// mutating it a second time.
  func beginInteractiveClose() throws

  /// Atomically commits a host-rendered interactive close while its stack
  /// reservation is still held.
  func commitInteractiveClose() throws

  /// Releases the reservation after UIKit reports completion or cancellation.
  func endInteractiveClose(completed: Bool)
}

/// The only routing component allowed to mutate Explorer's navigation stack.
///
/// Its navigation controller is supplied by the application composition root;
/// route handling never discovers UI through global application state.
@MainActor
final class NavigationHost: NavigationHosting {
  private let navigationController: UINavigationController
  private var isMutationInFlight = false
  private var isInteractiveCloseInFlight = false

  init(navigationController: UINavigationController) {
    self.navigationController = navigationController
    // Explorer owns every stack mutation. A managed container may temporarily
    // enable UIKit's gesture only after reserving this host.
    navigationController.interactivePopGestureRecognizer?.isEnabled = false
  }

  func open(
    _ viewController: UIViewController,
    policy: RouteStackPolicy,
    animated: Bool
  ) throws {
    try beginMutation(animated: animated)

    switch policy {
    case .push:
      navigationController.pushViewController(viewController, animated: animated)
    case .replaceTop:
      var stack = navigationController.viewControllers
      if stack.isEmpty {
        stack = [viewController]
        navigationController.setViewControllers(stack, animated: animated)
      } else if stack.count == 1 {
        // A sole controller is Explorer's root. Historical DebugBridge
        // single-top handling could not pop it and therefore pushed the new
        // page, leaving a closable homepage underneath.
        navigationController.pushViewController(viewController, animated: animated)
      } else {
        stack[stack.count - 1] = viewController
        navigationController.setViewControllers(stack, animated: animated)
      }
    case .resetAndPush:
      let stack: [UIViewController]
      if let root = navigationController.viewControllers.first {
        stack = [root, viewController]
      } else {
        stack = [viewController]
      }
      navigationController.setViewControllers(stack, animated: animated)
    }

    observeMutationCompletion(animated: animated)
  }

  func close(animated: Bool) throws {
    guard !isMutationInFlight, !isInteractiveCloseInFlight,
      navigationController.transitionCoordinator == nil
    else {
      throw RouteError.navigationBusy
    }
    guard navigationController.viewControllers.count > 1 else {
      throw RouteError.cannotCloseRoot
    }

    try beginMutation(animated: animated)
    navigationController.popViewController(animated: animated)
    observeMutationCompletion(animated: animated)
  }

  func close(_ viewController: UIViewController, animated: Bool) throws {
    let sourceIndex = try ownedIndex(of: viewController)
    guard sourceIndex > 0 else {
      throw RouteError.cannotCloseRoot
    }

    try beginMutation(animated: animated)
    if sourceIndex == navigationController.viewControllers.count - 1 {
      navigationController.popViewController(animated: animated)
    } else {
      var stack = navigationController.viewControllers
      stack.remove(at: sourceIndex)
      navigationController.setViewControllers(stack, animated: animated)
    }
    observeMutationCompletion(animated: animated)
  }

  func replace(
    _ viewController: UIViewController,
    replacing sourceViewController: UIViewController,
    animated: Bool
  ) throws {
    let sourceIndex = try ownedIndex(of: sourceViewController)
    guard sourceIndex > 0 else {
      throw RouteError.cannotCloseRoot
    }

    try beginMutation(animated: animated)
    var stack = navigationController.viewControllers
    stack.remove(at: sourceIndex)
    stack.append(viewController)
    navigationController.setViewControllers(stack, animated: animated)
    observeMutationCompletion(animated: animated)
  }

  func beginInteractiveClose() throws {
    guard !isMutationInFlight, !isInteractiveCloseInFlight,
      navigationController.transitionCoordinator == nil
    else {
      throw RouteError.navigationBusy
    }
    guard navigationController.viewControllers.count > 1 else {
      throw RouteError.cannotCloseRoot
    }
    isInteractiveCloseInFlight = true
  }

  func commitInteractiveClose() throws {
    guard isInteractiveCloseInFlight else {
      throw RouteError.navigationBusy
    }

    // Always release a reservation after a commit attempt. A failed atomic
    // commit must not leave all subsequent routes permanently blocked.
    defer { isInteractiveCloseInFlight = false }
    guard !isMutationInFlight, navigationController.transitionCoordinator == nil else {
      throw RouteError.navigationBusy
    }
    guard navigationController.viewControllers.count > 1 else {
      throw RouteError.cannotCloseRoot
    }

    navigationController.popViewController(animated: false)
  }

  func endInteractiveClose(completed _: Bool) {
    isInteractiveCloseInFlight = false
  }

  private func ownedIndex(of viewController: UIViewController) throws -> Int {
    guard let index = navigationController.viewControllers.firstIndex(where: {
      $0 === viewController
    }) else {
      throw RouteError.routeOwnerUnavailable
    }
    return index
  }

  private func beginMutation(animated: Bool) throws {
    // A coordinator may come from an interactive UIKit transition (for
    // example, the system back gesture) that this host did not initiate.
    guard !isMutationInFlight, !isInteractiveCloseInFlight,
      navigationController.transitionCoordinator == nil
    else {
      throw RouteError.navigationBusy
    }
    isMutationInFlight = animated
  }

  private func observeMutationCompletion(animated: Bool) {
    guard animated else {
      return
    }

    if let transitionCoordinator = navigationController.transitionCoordinator,
      transitionCoordinator.animate(alongsideTransition: nil, completion: { [weak self] _ in
        self?.isMutationInFlight = false
      })
    {
      return
    }

    // UIKit does not create a transition coordinator when the navigation
    // controller is not currently presented. Keep the mutation serialized for
    // the rest of this turn, then deterministically allow the next one.
    DispatchQueue.main.async { [weak self] in
      self?.isMutationInFlight = false
    }
  }
}
