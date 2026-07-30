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

    /// Connect the MethodPipe engine to an existing LynxView.
    /// Must be called AFTER the LynxView is created.
    @objc public static func connectPipe(to lynxView: LynxView) {
        let _ = MethodPipe(withLynxView: lynxView)
    }
}
#endif
