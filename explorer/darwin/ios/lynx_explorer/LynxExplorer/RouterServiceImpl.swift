// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Foundation

#if canImport(SparklingMethod)
import SparklingMethod
import Sparkling_Router

/// Routes sparkling-navigation open/close calls through LynxExplorer's
/// existing TasmDispatcher, bridging the hybrid:// scheme back to the
/// legacy file://lynx?local:// format that TasmDispatcher understands.
class RouterServiceImpl: RouterService {
    func closeContainer(withParams params: CloseMethodParamModel, completion: @escaping PipeMethod.CompletionBlock) {
        DispatchQueue.main.async {
            guard let nav = UIApplication.shared.keyWindow?.rootViewController as? UINavigationController,
                  nav.viewControllers.count > 1 else {
                completion(.failed(message: "Nothing to close"), nil)
                return
            }
            nav.popViewController(animated: true)
            completion(.succeeded(), nil)
        }
    }

    func openScheme(withParams params: OpenMethodParamModel, completion: @escaping PipeMethod.CompletionBlock) {
        let scheme = params.scheme ?? ""
        DispatchQueue.main.async {
            let url = Self.hybridToLegacy(scheme)
            TasmDispatcher.sharedInstance().openTargetUrl(url)
            completion(.succeeded(), nil)
        }
    }

    /// Converts `hybrid://lynxview_page?bundle=name.lynx.bundle&k=v`
    /// to `file://lynx?local://name.lynx.bundle?k=v`.
    private static func hybridToLegacy(_ scheme: String) -> String {
        guard let components = URLComponents(string: scheme),
              components.scheme == "hybrid",
              let queryItems = components.queryItems,
              let bundle = queryItems.first(where: { $0.name == "bundle" })?.value else {
            return scheme
        }
        let remaining = queryItems.filter { $0.name != "bundle" }
        if remaining.isEmpty {
            return "file://lynx?local://\(bundle)"
        }
        let qs = remaining.map { "\($0.name)=\($0.value ?? "")" }.joined(separator: "&")
        return "file://lynx?local://\(bundle)?\(qs)"
    }
}
#endif
