// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore containerid containertype explorersupportsexplicitrouteownership
// cspell:ignore explorersupportssparklingcontainer sparklingavailable
// cspell:ignore sparklingnavigation spkcontainerid spkpipe

import UIKit

/// Stateless adapter from the semantic launch model to the existing Legacy
/// view controllers. Navigation remains the sole responsibility of
/// `NavigationHosting`.
@MainActor
final class LegacyContainerLauncher: ContainerLaunching {
  func makeViewController(for descriptor: LaunchDescriptor) throws -> UIViewController {
    guard descriptor.container == .legacy else {
      throw RouteError.containerMismatch(expected: .legacy, actual: descriptor.container)
    }

    switch descriptor.resource {
    case .recorder(let url):
      let viewController = LynxRecorderViewController()
      viewController.url = url
      viewController.register(LynxRecorderDefaultActionCallback())
      return viewController

    case .localBundle(let path):
      return try Self.makeShellViewController(
        legacySourceURL: try Self.localSourceURL(for: path),
        descriptor: descriptor)

    case .remote(let url):
      return try Self.makeShellViewController(
        legacySourceURL: url.absoluteString,
        descriptor: descriptor)
    }
  }

  private static func makeShellViewController(
    legacySourceURL: String,
    descriptor: LaunchDescriptor
  ) throws -> UIViewController {
    let launchInitialData = try initialData(from: descriptor.initialData)
    let viewController = LynxViewShellViewController(
      legacySourceURL: legacySourceURL,
      parameters: parameters(for: descriptor))!
    viewController.launchInitialData = launchInitialData
    viewController.commonGlobalProps = globalProps(from: descriptor.commonGlobalProps)
    viewController.pageGlobalProps = globalProps(from: descriptor.pageGlobalProps)
    return viewController
  }

  private static func localSourceURL(for bundlePath: String) throws -> String {
    let pieces = bundlePath.split(separator: "/", maxSplits: 1, omittingEmptySubsequences: false)
    guard let host = pieces.first, !host.isEmpty else {
      throw RouteError.malformedURL(bundlePath)
    }

    var components = URLComponents()
    components.scheme = "local"
    components.host = String(host)
    if pieces.count == 2 {
      components.path = "/" + pieces[1]
    }
    guard let nestedURL = components.string else {
      throw RouteError.malformedURL(bundlePath)
    }
    return "file://lynx?\(nestedURL)"
  }

  /// Keep the Legacy controller's stringly-typed input at the edge. Values
  /// already preserved by the parser win; typed fields only fill gaps for
  /// descriptors constructed by non-URL entry points.
  private static func parameters(for descriptor: LaunchDescriptor) -> [String: Any] {
    var values: [String: Any] = descriptor.extras.filter {
      !ExplorerReservedKeys.isReservedCapabilityKey($0.key)
    }

    if values["animated"] == nil, !descriptor.presentation.animated {
      values["animated"] = false
    }
    if let viewport = descriptor.viewport {
      if values["width"] == nil {
        values["width"] = viewport.widthInPixels
      }
      if values["height"] == nil {
        values["height"] = viewport.heightInPixels
      }
    }
    if values["hidden_nav"] == nil, descriptor.navigation.navigationHidden {
      values["hidden_nav"] = true
    }
    if values["fullscreen"] == nil, descriptor.navigation.fullScreen {
      values["fullscreen"] = true
    }
    Self.fill(&values, key: "title", value: descriptor.navigation.title)
    Self.fill(&values, key: "title_color", value: descriptor.navigation.titleColor)
    Self.fill(&values, key: "bar_color", value: descriptor.navigation.barColor)
    Self.fill(&values, key: "back_button_style", value: descriptor.navigation.backButtonStyle)
    Self.fill(&values, key: "container_bg_color", value: descriptor.appearance.backgroundColor)
    if values["trans_status_bar"] == nil, descriptor.appearance.transparentStatusBar {
      values["trans_status_bar"] = true
    }
    if values["orientation"] == nil, let orientation = descriptor.navigation.orientation {
      switch orientation {
      case .portrait:
        values["orientation"] = "portrait"
      case .landscape:
        values["orientation"] = "landscape"
      }
    }
    if values["initial_page"] == nil, let pageName = descriptor.pageName {
      values["initial_page"] = pageName
    }

    return values
  }

  private static func initialData(from data: Data?) throws -> [String: Any]? {
    guard let data else {
      return nil
    }
    let object: Any
    do {
      object = try JSONSerialization.jsonObject(with: data)
    } catch {
      throw RouteError.invalidParameter(name: "initial_data", value: "invalid JSON")
    }
    guard let dictionary = object as? [String: Any] else {
      throw RouteError.invalidParameter(name: "initial_data", value: "expected JSON object")
    }
    return dictionary
  }

  private static func globalProps(
    from values: [String: AnyHashable]
  ) -> [String: Any] {
    values.reduce(into: [:]) { result, item in
      // Query params are converted to camel-case global props by the Legacy
      // controller. Reserve container-owned capabilities at this earlier
      // boundary so input cannot impersonate a Sparkling page -- via the shared
      // ExplorerReservedKeys so every boundary stays in lockstep.
      if !ExplorerReservedKeys.isReservedCapabilityKey(item.key) {
        result[item.key] = item.value.base
      }
    }
  }

  private static func fill(
    _ values: inout [String: Any],
    key: String,
    value: String?
  ) {
    if values[key] == nil, let value {
      values[key] = value
    }
  }
}
