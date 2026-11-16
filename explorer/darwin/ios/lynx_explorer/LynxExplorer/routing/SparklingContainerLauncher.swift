// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Foundation
import UIKit

#if canImport(Lynx)
  import Lynx
#endif

#if canImport(Sparkling)
  import Sparkling
#endif

protocol SparklingCanonicalSchemeBuilding {
  func build(for descriptor: LaunchDescriptor) throws -> String
}

/// Adapts Explorer's semantic descriptor to Sparkling's public canonical
/// scheme builder. Sparkling remains the sole owner of canonical syntax.
struct SparklingCanonicalSchemeBuilder: SparklingCanonicalSchemeBuilding {
  private let validator: SparklingSchemeResolving

  init(validator: SparklingSchemeResolving = SparklingSchemeAdapter()) {
    self.validator = validator
  }

  func build(for descriptor: LaunchDescriptor) throws -> String {
    #if canImport(Sparkling)
    if case .recorder = descriptor.resource {
      throw RouteError.recorderUnsupportedInSparkling
    }

    if let canonicalScheme = descriptor.canonicalSparklingScheme {
      _ = try validator.resolve(canonicalScheme)
      return canonicalScheme
    }

    if descriptor.navigation.orientation != nil
      || descriptor.queryItems.contains(where: { $0.name == "orientation" })
      || descriptor.extras["orientation"] != nil
    {
      throw RouteError.sparklingOptionUnsupported("orientation")
    }

    var queryItems: [URLQueryItem] = []
    queryItems.append(
      contentsOf: descriptor.queryItems.compactMap { item in
        guard !Self.isContainerOwnedQueryName(item.name) else {
          return nil
        }
        return URLQueryItem(name: item.name, value: item.value)
      })
    queryItems.append(contentsOf: appearanceQueryItems(for: descriptor))
    let canonicalScheme = try SPKHybridSchemeParam.buildLynxPageScheme(
      resource: sparklingResource(for: descriptor.resource),
      queryItems: queryItems
    ).absoluteString
    _ = try validator.resolve(canonicalScheme)
    return canonicalScheme
    #else
      throw RouteError.sparklingUnavailable
    #endif
  }

  private static let containerOwnedQueryNames: Set<String> = [
    "url",
    "bundle",
    "hidden_nav",
    "fullscreen",
    "hide_nav_bar",
    "hide_status_bar",
    "title",
    "title_color",
    "bar_color",
    "nav_bar_color",
    "back_button_style",
    "orientation",
    "trans_status_bar",
    "container_bg_color",
  ]

  private static func isContainerOwnedQueryName(_ name: String) -> Bool {
    // Sparkling's public builder reserves resource targets
    // case-insensitively. The target is already represented by
    // descriptor.resource, so forwarding `URL`/`BUNDLE` again would turn an
    // otherwise valid raw resource URL into a reserved-query-item failure.
    if name.lowercased() == "url" || name.lowercased() == "bundle" {
      return true
    }
    return containerOwnedQueryNames.contains(name)
  }

  #if canImport(Sparkling)
  private func sparklingResource(for resource: LaunchResource) -> SPKHybridLynxResource {
    switch resource {
    case .localBundle(let path):
      return .bundle(path)
    case .remote(let url):
      return .url(url.absoluteString)
    case .recorder:
      // The caller rejects Recorder before requesting an SDK resource.
      return .bundle("")
    }
  }
  #endif

  private func appearanceQueryItems(for descriptor: LaunchDescriptor) -> [URLQueryItem] {
    var items = [
      URLQueryItem(
        name: "fullscreen",
        value: descriptor.navigation.fullScreen ? "1" : "0"),
      URLQueryItem(
        name: "hide_nav_bar",
        value: descriptor.navigation.navigationHidden ? "1" : "0"),
      URLQueryItem(
        name: "hide_status_bar",
        value: descriptor.appearance.statusBarHidden ? "1" : "0"),
      URLQueryItem(
        name: "trans_status_bar",
        value: descriptor.appearance.transparentStatusBar ? "1" : "0"),
    ]

    Self.append(&items, name: "title", value: descriptor.navigation.title)
    Self.append(&items, name: "title_color", value: descriptor.navigation.titleColor)
    Self.append(&items, name: "nav_bar_color", value: descriptor.navigation.barColor)
    Self.append(&items, name: "back_button_style", value: descriptor.navigation.backButtonStyle)
    Self.append(&items, name: "container_bg_color", value: descriptor.appearance.backgroundColor)
    return items
  }

  private static func append(
    _ items: inout [URLQueryItem],
    name: String,
    value: String?
  ) {
    if let value {
      items.append(URLQueryItem(name: name, value: value))
    }
  }
}

struct SparklingDisplayMetrics {
  let bounds: CGRect
  let scale: CGFloat
}

/// Resolves assets emitted by the embedded Sparkling Go bundle relative to
/// the extension's packaged resource root. Rspeedy emits image requests as
/// `asset:///static/...`, while Explorer keeps the extension isolated under
/// `Resource/extensions/sparkling-go`.
struct ExplorerSparklingAssetResolver {
  private static let extensionDirectory = "extensions/sparkling-go"

  private let extensionRoot: URL

  init(resourceRoot: URL) {
    extensionRoot = resourceRoot
      .appendingPathComponent(Self.extensionDirectory, isDirectory: true)
      .standardizedFileURL
  }

  func localURL(for url: URL) -> URL? {
    guard url.scheme?.lowercased() == "asset" else {
      return nil
    }

    let components = url.pathComponents.filter { $0 != "/" && $0 != "." }
    guard !components.isEmpty, !components.contains("..") else {
      return nil
    }

    let candidate = components.reduce(extensionRoot) { result, component in
      result.appendingPathComponent(component)
    }.standardizedFileURL
    guard candidate.path.hasPrefix(extensionRoot.path + "/") else {
      return nil
    }
    return candidate
  }
}

#if canImport(Sparkling) && canImport(Lynx)
  private final class ExplorerSparklingImageFetcher: NSObject, LynxImageFetcher {
    private let resolver: ExplorerSparklingAssetResolver?
    private let fallback = SPKLynxResourceProvider()

    override init() {
      resolver = Bundle.main.url(forResource: "Resource", withExtension: nil).map {
        ExplorerSparklingAssetResolver(resourceRoot: $0)
      }
      super.init()
    }

    func loadImage(
      with url: URL,
      size targetSize: CGSize,
      contextInfo: [AnyHashable: Any]?,
      completion completionBlock: @escaping LynxImageLoadCompletionBlock
    ) -> () -> Void {
      if let localURL = resolver?.localURL(for: url),
        let image = UIImage(contentsOfFile: localURL.path)
      {
        completionBlock(image, nil, url)
        return {}
      }

      return fallback.loadImage(
        with: url,
        size: targetSize,
        contextInfo: contextInfo,
        completion: completionBlock)
    }
  }

  /// Lynx's current image pipeline redirects URLs through the media fetcher
  /// before loading them. Map Sparkling Go's packaged `asset:///` URLs to
  /// local file URLs and preserve Explorer's existing redirects otherwise.
  private final class ExplorerSparklingMediaResourceFetcher: NSObject,
    LynxMediaResourceFetcher
  {
    private let resolver: ExplorerSparklingAssetResolver?
    private let fallback = DemoMediaResourceFetcher()

    override init() {
      resolver = Bundle.main.url(forResource: "Resource", withExtension: nil).map {
        ExplorerSparklingAssetResolver(resourceRoot: $0)
      }
      super.init()
    }

    func shouldRedirectUrl(_ request: LynxResourceRequest) -> String {
      if let url = URL(string: request.url),
        let localURL = resolver?.localURL(for: url),
        FileManager.default.fileExists(atPath: localURL.path)
      {
        return localURL.absoluteString
      }
      return fallback.shouldRedirectUrl(request)
    }
  }

  @MainActor
  protocol SparklingRouting: AnyObject {
    func create(
      withURL url: String,
      context: SPKContext,
      frame: CGRect
    ) throws -> UIViewController
  }

  @MainActor
  final class OfficialSparklingRouter: SparklingRouting {
    func create(
      withURL url: String,
      context: SPKContext,
      frame: CGRect
    ) throws -> UIViewController {
      SPKRouter.create(withURL: url, context: context, frame: frame)
    }
  }

  @MainActor
  protocol SparklingPageLifetimeConfiguring: AnyObject {
    var imageFetcher: LynxImageFetcher? { get }
    var napiModuleToken: Any? { get }
    func configure(builder: LynxViewBuilder, frame: CGRect)
  }

  extension SparklingPageLifetimeConfiguring {
    var imageFetcher: LynxImageFetcher? { nil }
    var napiModuleToken: Any? { nil }
  }

  /// The public Sparkling failed-view contract keeps asynchronous load errors
  /// inside the SDK-owned container lifecycle and reload path.
  @MainActor
  private final class ExplorerSparklingLoadErrorView: UIView, SPKLoadErrorViewProtocol {
    private let messageLabel = UILabel()
    private let retryButton = UIButton(type: .system)
    private var refreshBlock: SPKLoadErrorRefreshBlock?

    override init(frame: CGRect) {
      super.init(frame: frame)
      configureView()
    }

    required init?(coder: NSCoder) {
      super.init(coder: coder)
      configureView()
    }

    func register(refreshBlock: @escaping SPKLoadErrorRefreshBlock) {
      self.refreshBlock = refreshBlock
    }

    func container(
      _ container: SPKContainerProtocol,
      didReceiveError error: Error?
    ) {
      messageLabel.text = error?.localizedDescription ?? "Sparkling failed to load this page."
    }

    private func configureView() {
      if #available(iOS 13.0, *) {
        backgroundColor = .systemBackground
      } else {
        backgroundColor = .white
      }
      accessibilityIdentifier = "sparkling-load-error"

      let titleLabel = UILabel()
      titleLabel.font = .preferredFont(forTextStyle: .headline)
      titleLabel.text = "Unable to load this page"
      titleLabel.textAlignment = .center

      messageLabel.font = .preferredFont(forTextStyle: .body)
      messageLabel.numberOfLines = 0
      messageLabel.text = "Sparkling failed to load this page."
      messageLabel.textAlignment = .center

      retryButton.setTitle("Retry", for: .normal)
      retryButton.accessibilityIdentifier = "sparkling-load-error-retry"
      retryButton.addTarget(self, action: #selector(retry), for: .touchUpInside)

      let stack = UIStackView(arrangedSubviews: [titleLabel, messageLabel, retryButton])
      stack.axis = .vertical
      stack.alignment = .fill
      stack.spacing = 16
      stack.translatesAutoresizingMaskIntoConstraints = false
      addSubview(stack)

      NSLayoutConstraint.activate([
        stack.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: 24),
        stack.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -24),
        stack.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor),
      ])
    }

    @objc private func retry() {
      refreshBlock?()
    }
  }

  /// Keeps the NAPI runtime, its listener, and Explorer resource fetchers alive
  /// for exactly one Sparkling page.
  @MainActor
  final class ExplorerSparklingPageLifetime: SparklingPageLifetimeConfiguring {
    private let genericResourceFetcher = DemoGenericResourceFetcher()
    private let mediaResourceFetcher = ExplorerSparklingMediaResourceFetcher()
    private let templateResourceFetcher = DemoTemplateResourceFetcher()
    private let moduleToken = NSObject()
    private let pageImageFetcher = ExplorerSparklingImageFetcher()
    private let backgroundRuntime: LynxBackgroundRuntime?
    private let runtimeLifecycleListener: LynxNodeAPILifecycleListener?

    var napiModuleToken: Any? {
      moduleToken
    }

    var imageFetcher: LynxImageFetcher? {
      pageImageFetcher
    }

    init(enableNAPIAddon: Bool) {
      guard enableNAPIAddon else {
        backgroundRuntime = nil
        runtimeLifecycleListener = nil
        return
      }

      let options = LynxBackgroundRuntimeOptions()
      options.genericResourceFetcher = genericResourceFetcher
      options.mediaResourceFetcher = mediaResourceFetcher
      options.templateResourceFetcher = templateResourceFetcher

      let runtime = LynxBackgroundRuntime(options: options)
      guard let listener = LynxNodeAPILifecycleListener(token: moduleToken) else {
        backgroundRuntime = nil
        runtimeLifecycleListener = nil
        return
      }
      runtime.add(listener)
      backgroundRuntime = runtime
      runtimeLifecycleListener = listener
    }

    func configure(builder: LynxViewBuilder, frame: CGRect) {
      // XElement implementations use Lynx's global lazy registrations from
      // the linked local source pods. Only Explorer's custom element is
      // page-config-specific.
      builder.config?.registerUI(LynxExplorerInput.self, withName: "explorer-input")
      builder.screenSize = frame.size
      builder.fontScale = 1
      builder.debuggable = true
      builder.enableGenericResourceFetcher = .true
      builder.genericResourceFetcher = genericResourceFetcher
      builder.mediaResourceFetcher = mediaResourceFetcher
      // Sparkling owns top-level template resolution, including official
      // `.bundle` targets. Explorer only supplies subresource fetchers here.
      builder.lynxBackgroundRuntime = backgroundRuntime
      builder.setThreadStrategyForRender(LynxSettingManager.sharedDataHandler().threadStrategy)
    }
  }

  /// Bridges Sparkling's cancellable system gesture into the same serialized
  /// stack owner used by every other Explorer entry and back action. SPKContext
  /// keeps this delegate weak, so the process-wide instance owns its lifetime.
  @MainActor
  private final class ExplorerSparklingInteractivePopDelegate: NSObject,
    SPKInteractivePopGestureDelegate
  {
    static let shared = ExplorerSparklingInteractivePopDelegate()
    private weak var activeBridge: RouteCoordinatorBridge?

    func containerShouldBeginInteractivePop(_: SPKContainerProtocol) -> Bool {
      finishInteractivePop(completed: false)
      guard let coordinator = RouteCoordinatorBridge.currentBridge() else {
        return false
      }
      guard coordinator.beginInteractiveCloseFromHost() else {
        return false
      }
      activeBridge = coordinator
      return true
    }

    func container(
      _ container: SPKContainerProtocol,
      didEndInteractivePopCompleted completed: Bool
    ) {
      finishInteractivePop(completed: completed)
    }

    private func finishInteractivePop(completed: Bool) {
      let coordinator = activeBridge
      activeBridge = nil
      coordinator?.endInteractiveCloseFromHost(completed: completed)
    }
  }

#endif

/// Stateless launch adapter. All navigation remains owned by NavigationHost.
@MainActor
final class SparklingContainerLauncher: ContainerLaunching {
  private let schemeBuilder: SparklingCanonicalSchemeBuilding

  #if canImport(Sparkling) && canImport(Lynx)
    private let router: SparklingRouting
    private let displayMetrics: @MainActor () -> SparklingDisplayMetrics
    private let lifetimeFactory: @MainActor (Bool) -> SparklingPageLifetimeConfiguring

    init(
      router: SparklingRouting? = nil,
      schemeBuilder: SparklingCanonicalSchemeBuilding = SparklingCanonicalSchemeBuilder(),
      displayMetrics: (@MainActor () -> SparklingDisplayMetrics)? = nil,
      lifetimeFactory: (@MainActor (Bool) -> SparklingPageLifetimeConfiguring)? = nil
    ) {
      self.router = router ?? OfficialSparklingRouter()
      self.schemeBuilder = schemeBuilder
      self.displayMetrics =
        displayMetrics ?? {
          SparklingDisplayMetrics(bounds: UIScreen.main.bounds, scale: UIScreen.main.scale)
        }
      self.lifetimeFactory =
        lifetimeFactory ?? {
          ExplorerSparklingPageLifetime(enableNAPIAddon: $0)
        }
    }
  #else
    init(
      schemeBuilder: SparklingCanonicalSchemeBuilding = SparklingCanonicalSchemeBuilder()
    ) {
      self.schemeBuilder = schemeBuilder
    }
  #endif

  func makeViewController(for descriptor: LaunchDescriptor) throws -> UIViewController {
    guard descriptor.container == .sparkling else {
      throw RouteError.containerMismatch(expected: .sparkling, actual: descriptor.container)
    }

    #if canImport(Sparkling) && canImport(Lynx)
      let canonicalScheme = try schemeBuilder.build(for: descriptor)
      let metrics = displayMetrics()
      let frame = Self.frame(for: descriptor.viewport, metrics: metrics)
      let context = try makeContext(
        for: descriptor,
        canonicalScheme: canonicalScheme,
        frame: frame)
      return try router.create(withURL: canonicalScheme, context: context, frame: frame)
    #else
      throw RouteError.sparklingUnavailable
    #endif
  }

  #if canImport(Sparkling) && canImport(Lynx)
    private static let containerGlobalProps: [String: AnyHashable] = [
      "sparklingAvailable": true
    ]

    private func makeContext(
      for descriptor: LaunchDescriptor,
      canonicalScheme: String,
      frame: CGRect
    ) throws -> SPKContext {
      let context = SPKContext()
      context.navigationBarBackHandler = { _ in
        guard let coordinator = RouteCoordinatorBridge.currentBridge() else {
          // An Explorer-created context never delegates stack ownership back
          // to the SDK, including during composition-root teardown.
          return true
        }
        return coordinator.closeFromHostBack(animated: true)
      }
      if let backButtonStyle = descriptor.navigation.backButtonStyle {
        let imageName = backButtonStyle == "dark" ? "back_dark" : "back_light"
        context.leftNavigationBarButtonItemBuilder = { _ in
          let item = SPKNavigationBarButtonItem()
          item.icon = UIImage(named: imageName) ?? UIImage()
          return item
        }
      }
      context.interactivePopGestureDelegate = ExplorerSparklingInteractivePopDelegate.shared
      context.globalProps = GlobalPropsMerger.merge(
        common: descriptor.commonGlobalProps,
        container: Self.containerGlobalProps,
        page: descriptor.pageGlobalProps)
      context.extra = descriptor.extras
      context.queryItems = Self.queryItems(
        from: canonicalScheme,
        pageName: descriptor.pageName)
      context.initialData = try Self.initialData(from: descriptor.initialData)
      context.originURL = canonicalScheme
      context.templateProvider = LynxEnv.sharedInstance().config.templateProvider
      let loadingView = makeExplorerSparklingLoadingView()
      context.loadingView = loadingView
      context.containerLifecycleDelegate = loadingView
      context.failedViewBuilder = { _ in
        ExplorerSparklingLoadErrorView(frame: .zero)
      }
      let lifetime = lifetimeFactory(descriptor.debugOptions.enableNAPIAddon)
      context.imageFetcher = lifetime.imageFetcher
      context.containerBackgroundColor = Self.color(from: descriptor.appearance.backgroundColor)

      // The pinned SDK bridges widthMode/heightMode through NSNumber and then
      // casts back to LynxViewSizeMode, so the router frame is authoritative.
      // Remove this workaround when Sparkling exposes typed size modes.

      var lynxModules: [String: Any] = [
        NSStringFromClass(ExplorerModule.self): NSNumber(value: 0)
      ]
      if descriptor.debugOptions.enableNAPIAddon,
        let token = lifetime.napiModuleToken
      {
        lynxModules[NSStringFromClass(LynxNodeAPIModule.self)] = token
      }
      context.lynxModule = lynxModules
      context.rawViewBuilderBlock = { [lifetime] rawBuilder in
        guard let builder = rawBuilder as? LynxViewBuilder else {
          return
        }
        lifetime.configure(builder: builder, frame: frame)
      }
      return context
    }

    private static func frame(
      for viewport: ViewportOptions?,
      metrics: SparklingDisplayMetrics
    ) -> CGRect {
      guard let viewport else {
        return metrics.bounds
      }
      let scale = metrics.scale > 0 ? metrics.scale : 1
      return CGRect(
        x: 0,
        y: 0,
        width: CGFloat(viewport.widthInPixels) / scale,
        height: CGFloat(viewport.heightInPixels) / scale)
    }

    private static func queryItems(
      from canonicalScheme: String,
      pageName: String?
    ) -> [String: AnyHashable] {
      var values: [String: AnyHashable] = [:]
      for item in URLComponents(string: canonicalScheme)?.queryItems ?? [] {
        if let value = item.value {
          values[item.name] = value
        }
      }
      if let pageName {
        values["initial_page"] = pageName
      }
      return values
    }

    private static func initialData(from data: Data?) throws -> [AnyHashable: Any]? {
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
      return Dictionary(uniqueKeysWithValues: dictionary.map { ($0.key as AnyHashable, $0.value) })
    }

    private static func color(from value: String?) -> UIColor? {
      guard var value else {
        return nil
      }
      value = value.trimmingCharacters(in: .whitespacesAndNewlines)
      if value.hasPrefix("#") {
        value.removeFirst()
      }
      guard value.count == 6 || value.count == 8,
        let rawValue = UInt64(value, radix: 16)
      else {
        return nil
      }

      if value.count == 6 {
        return UIColor(
          red: CGFloat((rawValue >> 16) & 0xff) / 255,
          green: CGFloat((rawValue >> 8) & 0xff) / 255,
          blue: CGFloat(rawValue & 0xff) / 255,
          alpha: 1)
      }
      return UIColor(
        red: CGFloat((rawValue >> 24) & 0xff) / 255,
        green: CGFloat((rawValue >> 16) & 0xff) / 255,
        blue: CGFloat((rawValue >> 8) & 0xff) / 255,
        alpha: CGFloat(rawValue & 0xff) / 255)
    }
  #endif
}
