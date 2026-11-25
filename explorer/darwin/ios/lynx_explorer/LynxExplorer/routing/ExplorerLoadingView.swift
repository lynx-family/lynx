// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import UIKit

#if canImport(Sparkling)
  import Sparkling
#endif

private func explorerBackgroundColor() -> UIColor {
  if #available(iOS 13.0, *) {
    return .systemBackground
  }
  return .white
}

private func explorerPrimaryLabelColor() -> UIColor {
  if #available(iOS 13.0, *) {
    return .label
  }
  return .black
}

private func explorerSecondaryLabelColor() -> UIColor {
  if #available(iOS 13.0, *) {
    return .secondaryLabel
  }
  return .darkGray
}

private enum ExplorerLoadingBrand {
  case lynx
  case sparkling
}

/// A quiet, host-owned loading surface shared by Legacy Lynx and Sparkling.
/// It follows go-web's centered brand mark and three-dot motion while exposing
/// the two pieces of information that matter to developers: network transfer
/// and engine rendering.
private final class ExplorerLoadingView: UIView {
  private enum Stage {
    case downloading
    case rendering
    case failed(String)
  }

  private let brand: ExplorerLoadingBrand
  private let markContainer = UIView()
  private var markImageView: UIImageView?
  private let titleLabel = UILabel()
  private let stageLabel = UILabel()
  private let dotsStack = UIStackView()
  private var dots: [UIView] = []
  private var stage: Stage = .downloading

  init(brand: ExplorerLoadingBrand) {
    self.brand = brand
    super.init(frame: UIScreen.main.bounds)
    autoresizingMask = [.flexibleWidth, .flexibleHeight]
    configureView()
    apply(stage: .downloading, animated: false)
  }

  required init?(coder: NSCoder) {
    brand = .lynx
    super.init(coder: coder)
    configureView()
    apply(stage: .downloading, animated: false)
  }

  func showDownloading() {
    guard Thread.isMainThread else {
      DispatchQueue.main.async { [weak self] in self?.showDownloading() }
      return
    }
    apply(stage: .downloading, animated: stageLabel.text != nil)
    startDots()
  }

  func showRendering() {
    guard Thread.isMainThread else {
      DispatchQueue.main.async { [weak self] in self?.showRendering() }
      return
    }
    apply(stage: .rendering, animated: true)
    startDots()
  }

  func showError(_ message: String?) {
    guard Thread.isMainThread else {
      DispatchQueue.main.async { [weak self] in self?.showError(message) }
      return
    }
    apply(stage: .failed(message ?? "Unable to load bundle"), animated: true)
    stopDots()
  }

  func finish() {
    guard Thread.isMainThread else {
      DispatchQueue.main.async { [weak self] in self?.finish() }
      return
    }
    stopDots()
    UIView.animate(
      withDuration: UIAccessibility.isReduceMotionEnabled ? 0 : 0.18,
      animations: { self.alpha = 0 },
      completion: { _ in self.removeFromSuperview() })
  }

  override func didMoveToWindow() {
    super.didMoveToWindow()
    if window == nil {
      stopDots()
    } else if case .failed = stage {
      stopDots()
    } else {
      startDots()
    }
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    // Sparkling's pinned SDK does not currently constrain a custom loading
    // view correctly, so bind to the actual container once it is available.
    // Legacy supplies its content frame explicitly and must retain it.
    if brand == .sparkling, let superview {
      frame = superview.bounds
      autoresizingMask = [.flexibleWidth, .flexibleHeight]
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if #available(iOS 13.0, *),
      previousTraitCollection?.hasDifferentColorAppearance(comparedTo: traitCollection) == true
    {
      applyAppearance()
    }
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  private func configureView() {
    accessibilityIdentifier = "explorer-loading-view"
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(themePreferenceDidChange),
      name: Notification.Name("ExplorerThemePreferenceDidChange"),
      object: nil)

    markContainer.translatesAutoresizingMaskIntoConstraints = false
    markContainer.widthAnchor.constraint(equalToConstant: 52).isActive = true
    markContainer.heightAnchor.constraint(equalToConstant: 52).isActive = true

    let mark: UIView
    if let image = brand == .sparkling ? Self.sparklingImage() : Self.lynxImage(dark: isDark) {
      let imageView = UIImageView(image: image)
      imageView.contentMode = .scaleAspectFit
      imageView.layer.cornerRadius = brand == .sparkling ? 11 : 0
      imageView.clipsToBounds = brand == .sparkling
      markImageView = imageView
      mark = imageView
    } else if brand == .sparkling {
      let fallback = UIView()
      fallback.backgroundColor = UIColor(
        red: 225 / 255,
        green: 5 / 255,
        blue: 67 / 255,
        alpha: 1)
      fallback.layer.cornerRadius = 11
      fallback.accessibilityIdentifier = "sparkling-loading-mark-fallback"
      mark = fallback
    } else {
      mark = LynxLoadingMarkView()
    }
    mark.translatesAutoresizingMaskIntoConstraints = false
    markContainer.addSubview(mark)
    NSLayoutConstraint.activate([
      mark.leadingAnchor.constraint(equalTo: markContainer.leadingAnchor),
      mark.trailingAnchor.constraint(equalTo: markContainer.trailingAnchor),
      mark.topAnchor.constraint(equalTo: markContainer.topAnchor),
      mark.bottomAnchor.constraint(equalTo: markContainer.bottomAnchor),
    ])

    titleLabel.font = .preferredFont(forTextStyle: .headline)
    titleLabel.adjustsFontForContentSizeCategory = true
    titleLabel.textAlignment = .center
    titleLabel.text = brand == .sparkling ? "Sparkling" : "Lynx"

    // cspell:ignore subheadline
    stageLabel.font = .preferredFont(forTextStyle: .subheadline)
    stageLabel.adjustsFontForContentSizeCategory = true
    stageLabel.textAlignment = .center
    stageLabel.numberOfLines = 2

    dotsStack.axis = .horizontal
    dotsStack.alignment = .center
    dotsStack.spacing = 6
    for index in 0..<3 {
      let dot = UIView()
      dot.translatesAutoresizingMaskIntoConstraints = false
      dot.backgroundColor =
        brand == .sparkling
        ? UIColor(red: 225 / 255, green: 5 / 255, blue: 67 / 255, alpha: 1)
        : explorerSecondaryLabelColor()
      dot.layer.cornerRadius = 3
      dot.alpha = 0.32
      dot.accessibilityIdentifier = "explorer-loading-dot-\(index)"
      NSLayoutConstraint.activate([
        dot.widthAnchor.constraint(equalToConstant: 6),
        dot.heightAnchor.constraint(equalToConstant: 6),
      ])
      dots.append(dot)
      dotsStack.addArrangedSubview(dot)
    }

    let textStack = UIStackView(arrangedSubviews: [titleLabel, stageLabel])
    textStack.axis = .vertical
    textStack.alignment = .center
    textStack.spacing = 5

    let stack = UIStackView(arrangedSubviews: [markContainer, textStack, dotsStack])
    stack.axis = .vertical
    stack.alignment = .center
    stack.spacing = 16
    stack.setCustomSpacing(12, after: textStack)
    stack.translatesAutoresizingMaskIntoConstraints = false
    addSubview(stack)
    NSLayoutConstraint.activate([
      stack.centerXAnchor.constraint(equalTo: safeAreaLayoutGuide.centerXAnchor),
      stack.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor, constant: -12),
      stack.leadingAnchor.constraint(
        greaterThanOrEqualTo: safeAreaLayoutGuide.leadingAnchor, constant: 32),
      stack.trailingAnchor.constraint(
        lessThanOrEqualTo: safeAreaLayoutGuide.trailingAnchor, constant: -32),
    ])
    applyAppearance()
  }

  private var isDark: Bool {
    let preference = UserDefaults.standard.string(forKey: "preferredTheme")?.lowercased()
    if preference == "dark" { return true }
    if preference == "light" { return false }
    if #available(iOS 13.0, *) {
      return traitCollection.userInterfaceStyle == .dark
    }
    return false
  }

  @objc private func themePreferenceDidChange() {
    guard Thread.isMainThread else {
      DispatchQueue.main.async { [weak self] in self?.themePreferenceDidChange() }
      return
    }
    applyAppearance()
  }

  private func applyAppearance() {
    let dark = isDark
    backgroundColor = dark ? UIColor(red: 0.067, green: 0.078, blue: 0.098, alpha: 1) : .white
    titleLabel.textColor = dark ? .white : UIColor(red: 0.067, green: 0.067, blue: 0.075, alpha: 1)
    stageLabel.textColor = dark ? UIColor(white: 0.66, alpha: 1) : UIColor(white: 0.56, alpha: 1)
    if brand == .lynx {
      markImageView?.image = Self.lynxImage(dark: dark)
    }
    for dot in dots where brand == .lynx {
      dot.backgroundColor = dark ? UIColor(white: 0.72, alpha: 1) : UIColor(white: 0.58, alpha: 1)
    }
    if case .failed = stage {
      stageLabel.textColor = .systemRed
    }
  }

  private func apply(stage newStage: Stage, animated: Bool) {
    stage = newStage
    let update = {
      switch newStage {
      case .downloading:
        self.stageLabel.text = "Downloading bundle"
        self.dotsStack.isHidden = false
        self.accessibilityValue = "Downloading bundle"
      case .rendering:
        self.stageLabel.text = "Rendering"
        self.dotsStack.isHidden = false
        self.accessibilityValue = "Rendering"
      case .failed(let message):
        self.stageLabel.text = message
        self.stageLabel.textColor = .systemRed
        self.dotsStack.isHidden = true
        self.accessibilityValue = message
      }
    }
    if animated && !UIAccessibility.isReduceMotionEnabled {
      UIView.transition(
        with: stageLabel,
        duration: 0.16,
        options: [.transitionCrossDissolve, .allowAnimatedContent],
        animations: update)
    } else {
      update()
    }
    applyAppearance()
  }

  private func startDots() {
    guard !UIAccessibility.isReduceMotionEnabled else {
      for (index, dot) in dots.enumerated() {
        dot.alpha = index == 1 ? 0.72 : 0.32
      }
      return
    }
    for (index, dot) in dots.enumerated() {
      guard dot.layer.animation(forKey: "explorer.loading.pulse") == nil else { continue }
      let animation = CAKeyframeAnimation(keyPath: "transform.scale")
      animation.values = [0.78, 1.18, 0.78]
      animation.keyTimes = [0, 0.4, 1]
      animation.duration = 1.2
      animation.beginTime = CACurrentMediaTime() + Double(index) * 0.15
      animation.repeatCount = .infinity
      animation.timingFunctions = [
        CAMediaTimingFunction(name: .easeOut),
        CAMediaTimingFunction(name: .easeIn),
      ]
      dot.layer.add(animation, forKey: "explorer.loading.pulse")

      let opacity = CAKeyframeAnimation(keyPath: "opacity")
      opacity.values = [0.3, 1, 0.3]
      opacity.keyTimes = animation.keyTimes
      opacity.duration = animation.duration
      opacity.beginTime = animation.beginTime
      opacity.repeatCount = .infinity
      opacity.timingFunctions = animation.timingFunctions
      dot.layer.add(opacity, forKey: "explorer.loading.opacity")
    }
  }

  private func stopDots() {
    for dot in dots {
      dot.layer.removeAnimation(forKey: "explorer.loading.pulse")
      dot.layer.removeAnimation(forKey: "explorer.loading.opacity")
    }
  }

  private static func sparklingImage() -> UIImage? {
    UIImage(named: "sparkling_logo")
  }

  private static func lynxImage(dark: Bool) -> UIImage? {
    UIImage(named: dark ? "lynx_light_logo" : "lynx_dark_logo")
  }
}

/// Objective-C bridge kept deliberately free of Sparkling protocols so the
/// generated Explorer header stays usable in builds where Sparkling is absent.
@objcMembers
final class ExplorerLegacyLoadingView: NSObject {
  private let loadingView = ExplorerLoadingView(brand: .lynx)

  init(parentView: UIView, frame: CGRect) {
    super.init()
    loadingView.frame = frame
    loadingView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    parentView.addSubview(loadingView)
  }

  func updateFrame(_ frame: CGRect) {
    loadingView.frame = frame
  }

  func showDownloading() {
    loadingView.showDownloading()
  }

  func showRendering() {
    loadingView.showRendering()
  }

  func showError(_ message: String?) {
    loadingView.showError(message)
  }

  func finish() {
    loadingView.finish()
  }
}

/// Compact vector approximation of the Lynx Explorer orbit mark. Keeping it
/// programmatic makes the loading surface available before any bundle assets
/// have been resolved.
private final class LynxLoadingMarkView: UIView {
  override init(frame: CGRect) {
    super.init(frame: frame)
    backgroundColor = .clear
    isOpaque = false
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    backgroundColor = .clear
    isOpaque = false
  }

  override func draw(_ rect: CGRect) {
    let color = explorerPrimaryLabelColor()
    color.setStroke()
    let lineWidth = max(2.5, rect.width * 0.075)

    let orbit = UIBezierPath(ovalIn: rect.insetBy(dx: rect.width * 0.13, dy: rect.height * 0.24))
    orbit.lineWidth = lineWidth
    orbit.lineCapStyle = .round
    orbit.stroke()

    let slash = UIBezierPath()
    slash.move(to: CGPoint(x: rect.width * 0.12, y: rect.height * 0.72))
    slash.addCurve(
      to: CGPoint(x: rect.width * 0.88, y: rect.height * 0.30),
      controlPoint1: CGPoint(x: rect.width * 0.34, y: rect.height * 0.90),
      controlPoint2: CGPoint(x: rect.width * 0.69, y: rect.height * 0.15))
    slash.lineWidth = lineWidth
    slash.lineCapStyle = .round
    slash.stroke()

    let coreRect = CGRect(
      x: rect.width * 0.39, y: rect.height * 0.39,
      width: rect.width * 0.22, height: rect.height * 0.22)
    color.setFill()
    UIBezierPath(ovalIn: coreRect).fill()
  }
}

#if canImport(Sparkling)
  @MainActor
  func makeExplorerSparklingLoadingView() -> UIView & SPKLoadingViewProtocol
    & SPKContainerLifecycleProtocol
  {
    ExplorerLoadingView(brand: .sparkling)
  }

  extension ExplorerLoadingView: SPKLoadingViewProtocol {
    func startLoadingAnimation() {
      showDownloading()
    }

    func stopLoadingAnimation() {
      stopDots()
    }

    func update(loadingProgress progress: CGFloat) {
      if progress >= 0.1 {
        showRendering()
      }
    }
  }

  extension ExplorerLoadingView: SPKContainerLifecycleProtocol {
    func container(
      _ container: SPKContainerProtocol,
      didStartFetchResourceWithURL url: URL?
    ) {
      showDownloading()
    }

    func container(
      _ container: SPKContainerProtocol,
      didFetchedResource resource: SPKResourceProtocol?,
      error: Error?
    ) {
      if let error {
        showError(error.localizedDescription)
      } else {
        showRendering()
      }
    }
  }
#endif
