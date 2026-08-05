// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Foundation

struct LaunchDescriptorParser {
  private let recorderURLPrefix: String
  private let maximumWrapperDepth: Int
  private let sparklingSchemeResolver: SparklingSchemeResolving

  init(
    recorderURLPrefix: String,
    maximumWrapperDepth: Int = 2,
    sparklingSchemeResolver: SparklingSchemeResolving = SparklingSchemeAdapter()
  ) {
    self.recorderURLPrefix = recorderURLPrefix
    self.maximumWrapperDepth = max(0, maximumWrapperDepth)
    self.sparklingSchemeResolver = sparklingSchemeResolver
  }

  func parse(
    _ input: String,
    requestedContainer: RequestedContainer,
    source: RouteSource
  ) throws -> LaunchDescriptor {
    try parse(
      input,
      originalInput: input,
      requestedContainer: requestedContainer,
      source: source,
      wrapperDepth: 0
    )
  }

  private func parse(
    _ input: String,
    originalInput: String,
    requestedContainer: RequestedContainer,
    source: RouteSource,
    wrapperDepth: Int
  ) throws -> LaunchDescriptor {
    let route = input.trimmingCharacters(in: .whitespacesAndNewlines)
    guard !route.isEmpty else {
      throw RouteError.emptyInput
    }
    guard Self.hasValidPercentEncoding(route) else {
      throw RouteError.invalidPercentEncoding(route)
    }
    guard let components = URLComponents(string: route),
      let scheme = components.scheme?.lowercased(),
      !scheme.isEmpty
    else {
      throw RouteError.malformedURL(route)
    }

    if scheme == "lynx" {
      return try unwrap(
        route,
        components: components,
        originalInput: originalInput,
        requestedContainer: requestedContainer,
        source: source,
        wrapperDepth: wrapperDepth
      )
    }

    if !recorderURLPrefix.isEmpty && route.hasPrefix(recorderURLPrefix) {
      guard requestedContainer != .sparkling else {
        throw RouteError.recorderUnsupportedInSparkling
      }
      return try makeDescriptor(
        originalInput: originalInput,
        resource: .recorder(route),
        requestedContainer: requestedContainer,
        resolvedContainer: .legacy,
        source: source,
        canonicalSparklingScheme: nil,
        queryItems: Self.queryItems(from: components)
      )
    }

    switch scheme {
    case "http", "https":
      return try parseRemote(
        route,
        components: components,
        originalInput: originalInput,
        requestedContainer: requestedContainer,
        source: source
      )
    case "file":
      return try parseLocal(
        route,
        components: components,
        originalInput: originalInput,
        requestedContainer: requestedContainer,
        source: source
      )
    case "hybrid":
      return try parseCanonicalSparkling(
        route,
        originalInput: originalInput,
        requestedContainer: requestedContainer,
        source: source
      )
    default:
      throw RouteError.unsupportedScheme(scheme)
    }
  }

  private func unwrap(
    _ route: String,
    components: URLComponents,
    originalInput: String,
    requestedContainer: RequestedContainer,
    source: RouteSource,
    wrapperDepth: Int
  ) throws -> LaunchDescriptor {
    guard components.host?.lowercased() == "open" else {
      throw RouteError.unsupportedScheme(components.scheme?.lowercased() ?? "lynx")
    }
    let target: String
    if let legacyTarget = Self.unescapedLegacyWrapperTarget(from: components) {
      target = legacyTarget
    } else {
      let targets = Self.queryItems(from: components).filter { $0.name == "url" }
      guard targets.count <= 1 else {
        throw RouteError.ambiguousTarget("url")
      }
      guard let encodedTarget = targets.first?.value, !encodedTarget.isEmpty else {
        throw RouteError.missingTarget("url")
      }
      target = encodedTarget
    }
    guard wrapperDepth < maximumWrapperDepth else {
      throw RouteError.wrapperDepthExceeded(maximumWrapperDepth)
    }

    return try parse(
      target,
      originalInput: originalInput,
      requestedContainer: requestedContainer,
      source: source,
      wrapperDepth: wrapperDepth + 1
    )
  }

  private func parseRemote(
    _ route: String,
    components: URLComponents,
    originalInput: String,
    requestedContainer: RequestedContainer,
    source: RouteSource
  ) throws -> LaunchDescriptor {
    guard let host = components.host, !host.isEmpty, let url = URL(string: route) else {
      throw RouteError.malformedURL(route)
    }
    return try makeDescriptor(
      originalInput: originalInput,
      resource: .remote(url),
      requestedContainer: requestedContainer,
      resolvedContainer: requestedContainer == .sparkling ? .sparkling : .legacy,
      source: source,
      canonicalSparklingScheme: nil,
      queryItems: Self.queryItems(from: components)
    )
  }

  private func parseLocal(
    _ route: String,
    components: URLComponents,
    originalInput: String,
    requestedContainer: RequestedContainer,
    source: RouteSource
  ) throws -> LaunchDescriptor {
    guard components.host?.lowercased() == "lynx",
      let nestedRoute = components.percentEncodedQuery,
      !nestedRoute.isEmpty,
      Self.hasValidPercentEncoding(nestedRoute),
      let nestedComponents = URLComponents(string: nestedRoute),
      nestedComponents.scheme?.lowercased() == "local",
      let nestedHost = nestedComponents.host,
      !nestedHost.isEmpty,
      let decodedPath = nestedComponents.percentEncodedPath.removingPercentEncoding
    else {
      throw RouteError.malformedURL(route)
    }

    return try makeDescriptor(
      originalInput: originalInput,
      resource: .localBundle(nestedHost + decodedPath),
      requestedContainer: requestedContainer,
      resolvedContainer: requestedContainer == .sparkling ? .sparkling : .legacy,
      source: source,
      canonicalSparklingScheme: nil,
      queryItems: Self.queryItems(from: nestedComponents)
    )
  }

  private func parseCanonicalSparkling(
    _ route: String,
    originalInput: String,
    requestedContainer: RequestedContainer,
    source: RouteSource
  ) throws -> LaunchDescriptor {
    let resolution = try sparklingSchemeResolver.resolve(route)

    return try makeDescriptor(
      originalInput: originalInput,
      resource: resolution.resource,
      requestedContainer: requestedContainer,
      resolvedContainer: .sparkling,
      source: source,
      canonicalSparklingScheme: resolution.canonicalScheme,
      queryItems: resolution.outerQueryItems,
      normalizedExtras: resolution.normalizedExtras
    )
  }

  private func makeDescriptor(
    originalInput: String,
    resource: LaunchResource,
    requestedContainer: RequestedContainer,
    resolvedContainer: ResolvedContainer,
    source: RouteSource,
    canonicalSparklingScheme: String?,
    queryItems: [LaunchQueryItem],
    normalizedExtras: [String: String] = [:]
  ) throws -> LaunchDescriptor {
    let normalizedItems = normalizedExtras.sorted { $0.key < $1.key }.map {
      LaunchQueryItem(name: $0.key, value: $0.value)
    }
    let semanticItems = normalizedItems + queryItems
    let animated = Self.booleanValue(named: "animated", in: semanticItems) ?? true
    let fullScreen = Self.booleanValue(named: "fullscreen", in: semanticItems) ?? false
    let explicitlyHidden =
      Self.booleanValue(named: "hide_nav_bar", in: semanticItems)
      ?? Self.booleanValue(named: "hidden_nav", in: semanticItems)
      ?? false
    let statusBarHidden =
      Self.booleanValue(named: "hide_status_bar", in: semanticItems) ?? false
    let transparentStatusBar =
      Self.booleanValue(named: "trans_status_bar", in: semanticItems) ?? false
    let viewport = Self.viewport(from: semanticItems)
    let orientation = Self.orientation(from: semanticItems)

    var extras = normalizedExtras
    var pageGlobalProps: [String: AnyHashable] = [:]
    for (name, value) in normalizedExtras {
      pageGlobalProps[Self.globalPropName(for: name)] = value
    }
    for item in queryItems {
      guard let value = item.value else {
        continue
      }
      extras[item.name] = value
      pageGlobalProps[Self.globalPropName(for: item.name)] = value
    }

    return LaunchDescriptor(
      originalInput: originalInput,
      resource: resource,
      initialData: nil,
      commonGlobalProps: [:],
      pageGlobalProps: pageGlobalProps,
      pageName: Self.stringValue(named: "initial_page", in: semanticItems),
      viewport: viewport,
      appearance: AppearanceOptions(
        backgroundColor: Self.stringValue(named: "container_bg_color", in: semanticItems),
        transparentStatusBar: fullScreen || transparentStatusBar,
        statusBarHidden: fullScreen || statusBarHidden
      ),
      navigation: NavigationOptions(
        navigationHidden: fullScreen || explicitlyHidden,
        fullScreen: fullScreen,
        title: Self.stringValue(named: "title", in: semanticItems),
        titleColor: Self.stringValue(named: "title_color", in: semanticItems),
        barColor: Self.stringValue(named: "nav_bar_color", in: semanticItems)
          ?? Self.stringValue(named: "bar_color", in: semanticItems),
        backButtonStyle: Self.stringValue(named: "back_button_style", in: semanticItems),
        orientation: orientation
      ),
      presentation: PresentationOptions(animated: animated),
      cachePolicy: .useProtocolCachePolicy,
      queryItems: queryItems,
      extras: extras.mapValues(AnyHashable.init),
      requestedContainer: requestedContainer,
      container: resolvedContainer,
      source: source,
      canonicalSparklingScheme: canonicalSparklingScheme
    )
  }

  private static func queryItems(from components: URLComponents) -> [LaunchQueryItem] {
    (components.queryItems ?? []).map { LaunchQueryItem(name: $0.name, value: $0.value) }
  }

  /// The historical UIApplication entry treated everything after an
  /// unescaped `url=https://...&...` as the nested URL. Preserve that form
  /// while encoded wrappers continue through URLComponents, including its
  /// duplicate-target validation.
  private static func unescapedLegacyWrapperTarget(
    from components: URLComponents
  ) -> String? {
    guard let query = components.percentEncodedQuery else {
      return nil
    }

    var componentStart = query.startIndex
    while componentStart < query.endIndex {
      let componentEnd = query[componentStart...].firstIndex(of: "&") ?? query.endIndex
      let component = query[componentStart..<componentEnd]
      if component.hasPrefix("url=") {
        let valueStart = query.index(componentStart, offsetBy: 4)
        let rawTail = query[valueStart...]
        guard let separator = rawTail.firstIndex(of: "&") else {
          return nil
        }
        let firstValue = rawTail[..<separator]
        return firstValue.contains("://") ? String(rawTail) : nil
      }
      guard componentEnd < query.endIndex else {
        return nil
      }
      componentStart = query.index(after: componentEnd)
    }
    return nil
  }

  private static func stringValue(
    named name: String,
    in queryItems: [LaunchQueryItem]
  ) -> String? {
    queryItems.last(where: { $0.name == name && $0.value != nil })?.value
  }

  private static func booleanValue(
    named name: String,
    in queryItems: [LaunchQueryItem]
  ) -> Bool? {
    guard let rawValue = stringValue(named: name, in: queryItems) else {
      return nil
    }
    return (rawValue as NSString).boolValue
  }

  private static func viewport(from queryItems: [LaunchQueryItem]) -> ViewportOptions? {
    guard let widthValue = stringValue(named: "width", in: queryItems),
      let heightValue = stringValue(named: "height", in: queryItems),
      // The Legacy shell has always interpreted physical pixels with
      // NSString.intValue. Model that contract here so an explicit
      // Sparkling launch does not reinterpret inputs such as `1e3` as 1000
      // while the same Legacy launch renders at one physical pixel.
      case let width = (widthValue as NSString).intValue, width > 0,
      case let height = (heightValue as NSString).intValue, height > 0
    else {
      return nil
    }
    return ViewportOptions(
      widthInPixels: Double(width),
      heightInPixels: Double(height))
  }

  private static func orientation(from queryItems: [LaunchQueryItem]) -> LaunchOrientation? {
    guard let value = stringValue(named: "orientation", in: queryItems) else {
      return nil
    }
    // Preserve the Legacy shell's exact, lower-case-only contract.
    switch value {
    case "portrait":
      return .portrait
    case "landscape":
      return .landscape
    default:
      return nil
    }
  }

  private static func globalPropName(for queryName: String) -> String {
    let trimmedName = queryName.drop(while: { $0 == "_" })
    guard !trimmedName.isEmpty else {
      return queryName
    }

    let parts = trimmedName.split(separator: "_", omittingEmptySubsequences: true)
    guard let first = parts.first else {
      return queryName
    }
    return parts.dropFirst().reduce(String(first)) { result, part in
      guard let firstCharacter = part.first else {
        return result
      }
      return result + String(firstCharacter).uppercased() + part.dropFirst()
    }
  }

  private static func hasValidPercentEncoding(_ input: String) -> Bool {
    let bytes = Array(input.utf8)
    var index = 0
    while index < bytes.count {
      if bytes[index] == Character("%").asciiValue {
        guard index + 2 < bytes.count,
          isHexDigit(bytes[index + 1]),
          isHexDigit(bytes[index + 2])
        else {
          return false
        }
        index += 3
      } else {
        index += 1
      }
    }
    // Syntactically valid escapes can still encode invalid UTF-8 (for
    // example `%FF`). Foundation would silently turn those query values into
    // nil, which is not a lossless semantic descriptor.
    return input.removingPercentEncoding != nil
  }

  private static func isHexDigit(_ byte: UInt8) -> Bool {
    (byte >= Character("0").asciiValue! && byte <= Character("9").asciiValue!)
      || (byte >= Character("A").asciiValue! && byte <= Character("F").asciiValue!)
      || (byte >= Character("a").asciiValue! && byte <= Character("f").asciiValue!)
  }
}
