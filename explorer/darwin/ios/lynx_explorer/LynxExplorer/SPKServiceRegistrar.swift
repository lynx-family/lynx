// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Foundation

#if canImport(SparklingMethod)
import Lynx
import SparklingMethod
import Sparkling_Router

private final class ExplorerAppearancePreferenceParamModel: SPKMethodModel {
    @objc var preference: String?

    @objc override class func requiredKeyPaths() -> Set<String>? {
        return ["preference"]
    }

    override class func jsonKeyPathsByPropertyKey() -> [AnyHashable: Any] {
        return ["preference": "preference"]
    }
}

/// Lets embedded Sparkling pages update Explorer's app-wide appearance.
/// Standalone Sparkling does not register this host-owned method.
private final class ExplorerAppearanceSetPreferenceMethod: PipeMethod {
    override var methodName: String { Self.methodName() }
    override class func methodName() -> String { "explorer.appearance.setPreference" }
    override var paramsModelClass: AnyClass { ExplorerAppearancePreferenceParamModel.self }
    override var resultModelClass: AnyClass { EmptyMethodModelClass.self }

    override func call(
        withParamModel paramModel: Any,
        completionHandler: PipeMethod.CompletionHandlerProtocol
    ) {
        guard let params = paramModel as? ExplorerAppearancePreferenceParamModel,
              let rawPreference = params.preference else {
            completionHandler.handleCompletion(
                status: .invalidParameter(message: "A preference is required"), result: nil)
            return
        }

        let preference: String
        switch rawPreference.lowercased() {
        case "auto": preference = "Auto"
        case "light": preference = "Light"
        case "dark": preference = "Dark"
        default:
            completionHandler.handleCompletion(
                status: .invalidParameter(message: "Preference must be Auto, Light, or Dark"),
                result: nil)
            return
        }

        UserDefaults.standard.set(preference, forKey: "preferredTheme")
        DispatchQueue.main.async {
            NotificationCenter.default.post(
                name: Notification.Name("ExplorerThemePreferenceDidChange"),
                object: nil,
                userInfo: ["preference": preference])
        }
        completionHandler.handleCompletion(status: .succeeded(), result: nil)
    }
}

@objc public class SPKServiceRegistrar: NSObject {
    @objc public static func registerAll() {
        DefaultDIContainerProvider.inject()

        DIProviderRegistry.provider.pipeShared().register(RouterService.self) {
            return RouterServiceImpl()
        }

        MethodRegistry.autoRegisterGlobalMethods()
        MethodRegistry.global.register(methodType: OpenMethod.self)
        MethodRegistry.global.register(methodType: CloseMethod.self)
        MethodRegistry.global.register(methodType: ExplorerAppearanceSetPreferenceMethod.self)
    }

    /// Register spkPipe module on a LynxConfig with the given containerID.
    /// Must be called inside the builder block before the LynxView is created.
    @objc public static func setupLynxPipe(config: LynxConfig, containerID: String) {
        config.spk_containerID = containerID
        MethodPipe.setupLynxPipe(config: config)
    }

    /// Associated-object key for retaining the MethodPipe alongside the LynxView.
    /// LynxPipeEngine holds the executor as a weak reference, so without an
    /// explicit owner the MethodPipe is deallocated immediately after creation
    /// and every subsequent JS->native `spkPipe.call` silently no-ops.
    private static var pipeKey: UInt8 = 0

    /// Connect the MethodPipe execution engine to an existing LynxView and
    /// retain it for the LynxView's lifetime. Idempotent — calling twice on
    /// the same LynxView is a no-op, which avoids replacing the entry in
    /// LynxPipeEnginePool out from under any in-flight callbacks.
    @objc public static func connectPipe(to lynxView: LynxView) {
        if objc_getAssociatedObject(lynxView, &pipeKey) as? MethodPipe != nil {
            return
        }
        let pipe = MethodPipe(withLynxView: lynxView)
        objc_setAssociatedObject(lynxView, &pipeKey, pipe, .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
    }
}
#endif
