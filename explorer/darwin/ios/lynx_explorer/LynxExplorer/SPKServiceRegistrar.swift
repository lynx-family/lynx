// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Foundation

#if canImport(SparklingMethod)
import Lynx
import SparklingMethod
import Sparkling_Router

@objc public class SPKServiceRegistrar: NSObject {
    @objc public static func registerAll() {
        DefaultDIContainerProvider.inject()

        DIProviderRegistry.provider.pipeShared().register(RouterService.self) {
            return RouterServiceImpl()
        }

        MethodRegistry.autoRegisterGlobalMethods()
        MethodRegistry.global.register(methodType: OpenMethod.self)
        MethodRegistry.global.register(methodType: CloseMethod.self)
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
