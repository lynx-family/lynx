// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Foundation

enum RequestedContainer: Int, Equatable {
  case automatic
  case legacy
  case sparkling
}

enum ResolvedContainer: Equatable {
  case legacy
  case sparkling
}

enum LaunchResource: Equatable {
  case localBundle(String)
  case remote(URL)
  case recorder(String)
}

struct LaunchQueryItem: Equatable {
  let name: String
  let value: String?
}

enum RouteSource: Equatable {
  case manualInput
  case scanner
  case startup
  case externalURL
  case universalLink
  case debugBridge
  case nativeModule
  case sparklingRouter
  case recorder
}

struct ViewportOptions: Equatable {
  /// Exact physical pixels, converted to points only by the container launcher.
  let widthInPixels: Double
  let heightInPixels: Double
}

enum LaunchOrientation: Equatable {
  case portrait
  case landscape
}

struct AppearanceOptions: Equatable {
  let backgroundColor: String?
  let transparentStatusBar: Bool
  let statusBarHidden: Bool

  init(
    backgroundColor: String?,
    transparentStatusBar: Bool,
    statusBarHidden: Bool = false
  ) {
    self.backgroundColor = backgroundColor
    self.transparentStatusBar = transparentStatusBar
    self.statusBarHidden = statusBarHidden
  }
}

struct NavigationOptions: Equatable {
  let navigationHidden: Bool
  let fullScreen: Bool
  let title: String?
  let titleColor: String?
  let barColor: String?
  let backButtonStyle: String?
  let orientation: LaunchOrientation?
}

struct PresentationOptions: Equatable {
  let animated: Bool
}

struct DebugOptions: Equatable {
  let enableNAPIAddon: Bool
}

struct LaunchDescriptor: Equatable {
  let originalInput: String
  let resource: LaunchResource
  let initialData: Data?
  let commonGlobalProps: [String: AnyHashable]
  let pageGlobalProps: [String: AnyHashable]
  let pageName: String?
  let viewport: ViewportOptions?
  let appearance: AppearanceOptions
  let navigation: NavigationOptions
  let presentation: PresentationOptions
  let debugOptions: DebugOptions
  let queryItems: [LaunchQueryItem]
  let extras: [String: AnyHashable]
  let requestedContainer: RequestedContainer
  let container: ResolvedContainer
  let source: RouteSource
  let canonicalSparklingScheme: String?
}

extension LaunchDescriptor {
  /// Adds host-owned defaults without allowing them to overwrite explicit
  /// values parsed from the route.
  func addingCommonGlobalPropDefaults(
    _ defaults: [String: AnyHashable]
  ) -> LaunchDescriptor {
    replacing(
      commonGlobalProps: defaults.merging(commonGlobalProps) { _, descriptorValue in
        descriptorValue
      })
  }

  /// Adds method-level router metadata after URL parsing. Method parameters
  /// intentionally win over query parameters because they are the closest
  /// semantic input to `router.open`.
  func mergingExtras(_ values: [String: AnyHashable]) -> LaunchDescriptor {
    guard !values.isEmpty else {
      return self
    }
    return replacing(
      extras: extras.merging(values) { _, methodValue in methodValue })
  }

  private func replacing(
    commonGlobalProps: [String: AnyHashable]? = nil,
    extras: [String: AnyHashable]? = nil
  ) -> LaunchDescriptor {
    LaunchDescriptor(
      originalInput: originalInput,
      resource: resource,
      initialData: initialData,
      commonGlobalProps: commonGlobalProps ?? self.commonGlobalProps,
      pageGlobalProps: pageGlobalProps,
      pageName: pageName,
      viewport: viewport,
      appearance: appearance,
      navigation: navigation,
      presentation: presentation,
      debugOptions: debugOptions,
      queryItems: queryItems,
      extras: extras ?? self.extras,
      requestedContainer: requestedContainer,
      container: container,
      source: source,
      canonicalSparklingScheme: canonicalSparklingScheme)
  }
}

enum RouteError: Error, Equatable {
  case emptyInput
  case malformedURL(String)
  case invalidPercentEncoding(String)
  case unsupportedScheme(String)
  case unsupportedHybridHost(String)
  case missingTarget(String)
  case ambiguousTarget(String)
  case invalidParameter(name: String, value: String?)
  case wrapperDepthExceeded(Int)
  case recorderUnsupportedInSparkling
  case sparklingOptionUnsupported(String)
  case malformedSparklingScheme(String)
  case sparklingUnavailable
  case containerMismatch(expected: ResolvedContainer, actual: ResolvedContainer)
  case routeOwnerUnavailable
  case cannotCloseRoot
  case navigationBusy

  var code: String {
    switch self {
    case .emptyInput:
      return "empty_input"
    case .malformedURL:
      return "malformed_url"
    case .invalidPercentEncoding:
      return "invalid_percent_encoding"
    case .unsupportedScheme:
      return "unsupported_scheme"
    case .unsupportedHybridHost:
      return "unsupported_hybrid_host"
    case .missingTarget:
      return "missing_target"
    case .ambiguousTarget:
      return "ambiguous_target"
    case .invalidParameter:
      return "invalid_parameter"
    case .wrapperDepthExceeded:
      return "wrapper_depth_exceeded"
    case .recorderUnsupportedInSparkling:
      return "recorder_unsupported_in_sparkling"
    case .sparklingOptionUnsupported:
      return "sparkling_option_unsupported"
    case .malformedSparklingScheme:
      return "malformed_sparkling_scheme"
    case .sparklingUnavailable:
      return "sparkling_unavailable"
    case .containerMismatch:
      return "container_mismatch"
    case .routeOwnerUnavailable:
      return "route_owner_unavailable"
    case .cannotCloseRoot:
      return "cannot_close_root"
    case .navigationBusy:
      return "navigation_busy"
    }
  }
}

extension RouteError: LocalizedError {
  var errorDescription: String? {
    switch self {
    case .emptyInput:
      return "Enter a Lynx bundle URL or a Sparkling hybrid scheme."
    case .malformedURL(let input):
      return "The route is not a valid absolute URL: \(input)"
    case .invalidPercentEncoding(let input):
      return "The route contains an invalid percent escape: \(input)"
    case .unsupportedScheme(let scheme):
      return "The URL scheme '\(scheme)' is not supported by Lynx Explorer."
    case .unsupportedHybridHost(let host):
      return "The Sparkling hybrid host '\(host)' is not supported."
    case .missingTarget(let name):
      return "The route is missing its required '\(name)' target."
    case .ambiguousTarget(let name):
      return "The route contains more than one '\(name)' target. Keep exactly one."
    case .invalidParameter(let name, let value):
      let displayedValue = value ?? "<missing>"
      return "The route parameter '\(name)' has an invalid value '\(displayedValue)'."
    case .wrapperDepthExceeded(let maximumDepth):
      return "The route contains too many nested lynx://open wrappers (maximum \(maximumDepth))."
    case .recorderUnsupportedInSparkling:
      return "Recorder routes can only be opened with the Legacy container."
    case .sparklingOptionUnsupported(let option):
      return "The Sparkling container does not support the '\(option)' launch option."
    case .malformedSparklingScheme(let scheme):
      return "The Sparkling scheme is malformed or has an unsupported resource target: \(scheme)"
    case .sparklingUnavailable:
      return "Sparkling is not available in this Lynx Explorer build."
    case .containerMismatch(let expected, let actual):
      return "The \(expected) launcher cannot open a \(actual) route."
    case .routeOwnerUnavailable:
      return "The calling container is no longer owned by the Lynx Explorer navigation stack."
    case .cannotCloseRoot:
      return "The root Lynx Explorer page cannot be closed."
    case .navigationBusy:
      return "A Lynx Explorer navigation transition is already in progress."
    }
  }
}
