// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Foundation

#if canImport(Sparkling)
  import Sparkling
#endif

struct SparklingSchemeResolution: Equatable {
  let resource: LaunchResource
  let canonicalScheme: String
  let outerQueryItems: [LaunchQueryItem]
  let normalizedExtras: [String: String]
}

protocol SparklingSchemeResolving {
  func resolve(_ scheme: String) throws -> SparklingSchemeResolution
}

enum SparklingSchemeEngineKind: Equatable {
  case lynx
  case web
  case unknown
}

enum SparklingSchemeContainerKind: Equatable {
  case page
  case card
  case unknown
}

struct SparklingSchemeParameters: Equatable {
  let canResolve: Bool
  let engine: SparklingSchemeEngineKind
  let container: SparklingSchemeContainerKind
  let url: String?
  let extras: [String: String]
}

struct SparklingSchemeAdapter: SparklingSchemeResolving {
  typealias ParameterResolver = (URL, [String: String]) throws -> SparklingSchemeParameters

  private let parameterResolver: ParameterResolver
  private let isSparklingUnavailable: Bool

  init() {
    parameterResolver = Self.resolveWithOfficialAPI
    #if canImport(Sparkling)
      isSparklingUnavailable = false
    #else
      isSparklingUnavailable = true
    #endif
  }

  init(_ parameterResolver: @escaping ParameterResolver) {
    self.parameterResolver = parameterResolver
    isSparklingUnavailable = false
  }

  func resolve(_ scheme: String) throws -> SparklingSchemeResolution {
    guard let components = URLComponents(string: scheme),
      components.scheme?.lowercased() == "hybrid",
      let url = components.url
    else {
      throw RouteError.malformedSparklingScheme(scheme)
    }
    guard !isSparklingUnavailable else {
      throw RouteError.sparklingUnavailable
    }

    let outerQueryItems = (components.queryItems ?? []).map {
      LaunchQueryItem(name: $0.name, value: $0.value)
    }
    let urlTargets = outerQueryItems.filter { $0.name == "url" }
    let bundleTargets = outerQueryItems.filter { $0.name == "bundle" }
    guard urlTargets.count <= 1 else {
      throw RouteError.ambiguousTarget("url")
    }
    guard bundleTargets.count <= 1 else {
      throw RouteError.ambiguousTarget("bundle")
    }
    guard urlTargets.isEmpty || bundleTargets.isEmpty else {
      throw RouteError.ambiguousTarget("bundle or url")
    }
    if let target = urlTargets.first {
      guard let value = target.value, !value.isEmpty else {
        throw RouteError.missingTarget("url")
      }
    } else if let target = bundleTargets.first {
      guard let value = target.value, !value.isEmpty else {
        throw RouteError.missingTarget("bundle")
      }
    } else {
      throw RouteError.missingTarget("bundle or url")
    }

    var outerValues: [String: String] = [:]
    for item in outerQueryItems {
      if let value = item.value {
        outerValues[item.name] = value
      }
    }

    let parameters = try parameterResolver(url, outerValues)
    guard parameters.canResolve else {
      throw RouteError.malformedSparklingScheme(scheme)
    }
    guard parameters.engine == .lynx, parameters.container == .page else {
      throw RouteError.unsupportedHybridHost(components.host?.lowercased() ?? "")
    }

    let resource: LaunchResource
    if !urlTargets.isEmpty {
      guard let target = parameters.url ?? parameters.extras["url"],
        let targetURL = URL(string: target),
        let targetComponents = URLComponents(url: targetURL, resolvingAgainstBaseURL: false),
        let targetScheme = targetComponents.scheme?.lowercased(),
        ["http", "https"].contains(targetScheme),
        let targetHost = targetComponents.host,
        !targetHost.isEmpty
      else {
        throw RouteError.malformedSparklingScheme(scheme)
      }
      resource = .remote(targetURL)
    } else {
      guard let bundle = parameters.extras["bundle"], !bundle.isEmpty else {
        throw RouteError.malformedSparklingScheme(scheme)
      }
      resource = .localBundle(bundle)
    }

    var normalizedExtras = parameters.extras
    for item in outerQueryItems {
      if let value = item.value {
        normalizedExtras[item.name] = value
      }
    }

    return SparklingSchemeResolution(
      resource: resource,
      canonicalScheme: scheme,
      outerQueryItems: outerQueryItems,
      normalizedExtras: normalizedExtras)
  }

  private static func resolveWithOfficialAPI(
    _ url: URL,
    outerValues: [String: String]
  ) throws -> SparklingSchemeParameters {
    #if canImport(Sparkling)
      let canResolve = SPKHybridSchemeParam.canResolve(url: url)
      let officialEngine = SPKHybridSchemeParam.engineType(withURL: url)
      guard canResolve else {
        return SparklingSchemeParameters(
          canResolve: false,
          engine: .unknown,
          container: .unknown,
          url: nil,
          extras: [:])
      }

      let normalizedURL: URL?
      if outerValues["url"] != nil {
        normalizedURL = SPKHybridSchemeParam.resolveURLStyle(
          toHybridScheme: url,
          queries: outerValues)
      } else {
        normalizedURL = SPKHybridSchemeParam.resolveBundleStyle(
          toHybridScheme: url,
          queries: outerValues)
      }
      guard normalizedURL?.scheme?.lowercased() == "hybrid" else {
        throw RouteError.malformedSparklingScheme(url.absoluteString)
      }

      let context = SPKHybridContext()
      let parameters = SPKHybridSchemeParam.resolver(withScheme: url, context: context)
      let engine: SparklingSchemeEngineKind
      switch officialEngine {
      case .SPKHybridEngineTypeLynx:
        engine = .lynx
      case .SPKHybridEngineTypeWeb:
        engine = .web
      case .SPKHybridEngineTypeUnknown:
        engine = .unknown
      @unknown default:
        engine = .unknown
      }

      let container: SparklingSchemeContainerKind
      switch parameters.containerType {
      case .SPKHybridContainerTypePage:
        container = .page
      case .SPKHybridContainerTypeCard:
        container = .card
      case .SPKHybridContainerTypeUnknown:
        container = .unknown
      @unknown default:
        container = .unknown
      }

      return SparklingSchemeParameters(
        canResolve: true,
        engine: engine,
        container: container,
        url: parameters.url,
        extras: parameters.extra.compactMapValues { $0 as? String })
    #else
      throw RouteError.sparklingUnavailable
    #endif
  }
}
