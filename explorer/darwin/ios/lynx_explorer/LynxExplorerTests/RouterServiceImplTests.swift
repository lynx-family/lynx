// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import XCTest
@testable import LynxExplorer

final class RouterServiceImplTests: XCTestCase {
    func testHybridURLStyleBecomesHTTPURLForLegacyDispatcher() {
        let scheme = "hybrid://lynxview_page?url=http%3A%2F%2Flocalhost%3A8765%2Fgp-container.lynx.bundle&title=Navigation%20Target&userId=42"

        let result = RouterServiceImpl.hybridToLegacy(scheme)

        XCTAssertEqual(
            result,
            "http://localhost:8765/gp-container.lynx.bundle?title=Navigation%20Target&userId=42"
        )
    }

    func testHybridBundleStyleStillBecomesLocalURLForLegacyDispatcher() {
        let scheme = "hybrid://lynxview_page?bundle=gp-container.lynx.bundle&userId=42"

        let result = RouterServiceImpl.hybridToLegacy(scheme)

        XCTAssertEqual(result, "file://lynx?local://gp-container.lynx.bundle?userId=42")
    }
}
