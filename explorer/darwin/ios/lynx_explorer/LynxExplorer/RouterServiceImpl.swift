// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Foundation

#if canImport(SparklingMethod) && canImport(Sparkling_Router) && canImport(Sparkling)
  import Sparkling
  import SparklingMethod
  import Sparkling_Router
  import UIKit

  /// Routes sparkling-navigation open/close calls through LynxExplorer's
  /// single route coordinator and reports the actual navigation result.
  final class RouterServiceImpl: RouterService {
    typealias BridgeProvider = () -> RouteCoordinatorBridge?
    typealias CallingViewControllerResolver = (PipeContainer?) -> UIViewController?
    typealias SystemBrowserOpener = (String?) -> Bool

    private let bridgeProvider: BridgeProvider
    private let callingViewControllerResolver: CallingViewControllerResolver
    private let systemBrowserOpener: SystemBrowserOpener

    init(
      bridgeProvider: @escaping BridgeProvider = {
        RouteCoordinatorBridge.currentBridge()
      },
      callingViewControllerResolver: @escaping CallingViewControllerResolver = { container in
        guard let responder = container as? UIResponder else {
          return nil
        }
        return SPKRouter.viewController(for: responder)
      },
      systemBrowserOpener: @escaping SystemBrowserOpener = { input in
        SPKRouter.openInSystemBrowser(withURL: input)
      }
    ) {
      self.bridgeProvider = bridgeProvider
      self.callingViewControllerResolver = callingViewControllerResolver
      self.systemBrowserOpener = systemBrowserOpener
    }

    func closeContainer(
      withParams params: CloseMethodParamModel,
      completion: @escaping PipeMethod.CompletionBlock
    ) {
      performOnMain {
        self.closeContainerOnMain(withParams: params, completion: completion)
      }
    }

    private func closeContainerOnMain(
      withParams params: CloseMethodParamModel,
      completion: @escaping PipeMethod.CompletionBlock
    ) {
      let pipeContainer = params.context?.pipeContainer
      guard
        Self.closeTargetsCurrentContainer(
          requestedID: params.containerID,
          currentID: pipeContainer?.spk_containerID)
      else {
        completion(
          .invalidParameter(
            message: "The containerID does not identify the calling container."),
          nil)
        return
      }

      guard let bridge = bridgeProvider() else {
        completion(.failed(message: "The route coordinator is unavailable."), nil)
        return
      }
      guard
        let callingViewController = callingViewControllerResolver(pipeContainer)
      else {
        completion(.failed(message: "The calling container is unavailable."), nil)
        return
      }

      bridge.closeSparklingRouterViewController(
        callingViewController,
        animated: params.animated
      ) { payload in
        Self.complete(with: payload, completion: completion)
      }
    }

    func openScheme(
      withParams params: OpenMethodParamModel,
      completion: @escaping PipeMethod.CompletionBlock
    ) {
      performOnMain {
        self.openSchemeOnMain(withParams: params, completion: completion)
      }
    }

    private func openSchemeOnMain(
      withParams params: OpenMethodParamModel,
      completion: @escaping PipeMethod.CompletionBlock
    ) {
      if params.interceptor != nil {
        completion(
          .notImplemented(
            message: "The interceptor parameter is not supported by LynxExplorer."),
          nil)
        return
      }

      guard let scheme = params.scheme, !scheme.isEmpty else {
        completion(.invalidParameter(message: "The scheme parameter must not be empty."), nil)
        return
      }

      if params.useSysBrowser {
        let status: MethodStatus =
          systemBrowserOpener(scheme)
          ? .succeeded()
          : .failed(message: "Failed to open the URL in the system browser.")
        completion(status, nil)
        return
      }

      let replacement: RouteReplacement?
      if params.replace {
        guard let policy = Self.replacementPolicy(for: params.replaceType) else {
          completion(
            .invalidParameter(
              message: "Unsupported replaceType: \(params.replaceType ?? "nil")."),
            nil)
          return
        }
        guard
          let callingViewController = callingViewControllerResolver(
            params.context?.pipeContainer)
        else {
          completion(.failed(message: "The calling container is unavailable."), nil)
          return
        }
        replacement = RouteReplacement(
          sourceViewController: callingViewController,
          policy: policy)
      } else {
        replacement = nil
      }

      guard let bridge = bridgeProvider() else {
        completion(.failed(message: "The route coordinator is unavailable."), nil)
        return
      }
      bridge.openSparklingRouterURL(
        scheme,
        // The pinned iOS model collapses an omitted `animated` option into
        // `false`, while Sparkling's official iOS host and Android default an
        // open to animated. Do not turn that lossy model value into a host
        // override; the parsed scheme remains the source of truth and defaults
        // to animated, while `animated=0` in the scheme can still opt out.
        animated: nil,
        extra: Self.extra(from: params.extra),
        replacement: replacement
      ) { payload in
        Self.complete(with: payload, completion: completion)
      }
    }

    static func closeTargetsCurrentContainer(
      requestedID: String?,
      currentID: String?
    ) -> Bool {
      guard let requestedID, !requestedID.isEmpty else {
        return true
      }
      return requestedID == currentID
    }

    private static func replacementPolicy(
      for rawValue: String?
    ) -> RouteReplacementPolicy? {
      switch rawValue {
      case nil, "":
        return .onlyCloseAfterOpenSucceed
      case "alwaysCloseBeforeOpen":
        return .alwaysCloseBeforeOpen
      case "alwaysCloseAfterOpen":
        return .alwaysCloseAfterOpen
      case "onlyCloseAfterOpenSucceed":
        return .onlyCloseAfterOpenSucceed
      default:
        return nil
      }
    }

    private static func extra(from dictionary: NSDictionary?) -> [String: AnyHashable] {
      guard let dictionary else {
        return [:]
      }

      var result: [String: AnyHashable] = [:]
      for (key, value) in dictionary {
        guard let key = key as? String, let value = value as? AnyHashable else {
          continue
        }
        result[key] = value
      }
      return result
    }

    private static func complete(
      with payload: Any,
      completion: @escaping PipeMethod.CompletionBlock
    ) {
      guard
        let result = payload as? [String: Any],
        result["success"] as? Bool == true
      else {
        let result = payload as? [String: Any]
        let message =
          result?["message"] as? String
          ?? result?["code"] as? String
          ?? "The route operation failed."
        completion(.failed(message: message), nil)
        return
      }
      completion(.succeeded(), nil)
    }

    private func performOnMain(_ operation: @escaping () -> Void) {
      if Thread.isMainThread {
        operation()
      } else {
        DispatchQueue.main.async {
          operation()
        }
      }
    }
  }
#endif
