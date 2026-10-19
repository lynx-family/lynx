// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Verify that QuickJS registration preserves the existing JavaScriptCore slot
// and that an unavailable callback cannot replace a valid registration.
#import <XCTest/XCTest.h>

#import <Lynx/LynxWasmFuncRegistry.h>

#include "core/runtime/js/jsi/jsc/jsc_context_wrapper.h"
#include "core/runtime/js/jsi/quickjs/quickjs_context_wrapper.h"

namespace {

void SentinelQJS(void*, void*) {}

}  // namespace

@interface LynxWasmFuncRegistryWasmContractUnitTest : XCTestCase {
 @public
  decltype(lynx::runtime::js::JSCContextWrapper::register_wasm_func_) savedJSC;
  decltype(lynx::runtime::js::QuickjsContextWrapper::register_wasm_func_) savedQJS;
}
@end

@implementation LynxWasmFuncRegistryWasmContractUnitTest

- (void)setUp {
  [super setUp];
  // Snapshot the process-global slots so tearDown restores the exact prior
  // values instead of overwriting the host installation with a sentinel.
  savedJSC = lynx::runtime::js::JSCContextWrapper::register_wasm_func_;
  savedQJS = lynx::runtime::js::QuickjsContextWrapper::register_wasm_func_;
}

- (void)tearDown {
  lynx::runtime::js::JSCContextWrapper::register_wasm_func_ = savedJSC;
  lynx::runtime::js::QuickjsContextWrapper::register_wasm_func_ = savedQJS;
  [super tearDown];
}

- (void)testQjsRegistrationPreservesJsc {
  [LynxWasmFuncRegistry registerQjsWasmFunc:(void*)SentinelQJS];

  XCTAssertTrue(lynx::runtime::js::QuickjsContextWrapper::register_wasm_func_ == SentinelQJS);
  XCTAssertTrue(lynx::runtime::js::JSCContextWrapper::register_wasm_func_ == savedJSC);
}

- (void)testNullQjsCallbackPreservesRegistration {
  [LynxWasmFuncRegistry registerQjsWasmFunc:nullptr];
  XCTAssertTrue(lynx::runtime::js::QuickjsContextWrapper::register_wasm_func_ == savedQJS);

  [LynxWasmFuncRegistry registerQjsWasmFunc:(void*)SentinelQJS];
  [LynxWasmFuncRegistry registerQjsWasmFunc:nullptr];

  XCTAssertTrue(lynx::runtime::js::JSCContextWrapper::register_wasm_func_ == savedJSC);
  XCTAssertTrue(lynx::runtime::js::QuickjsContextWrapper::register_wasm_func_ == SentinelQJS);
}

// The production Registry exposes the selectors the caller is expected to guard
// with respondsToSelector.
- (void)testRegistryRespondsToSelectors {
  Class registry = [LynxWasmFuncRegistry class];
  XCTAssertTrue([registry respondsToSelector:@selector(registerWasmFunc:)]);
  XCTAssertTrue([registry respondsToSelector:@selector(registerQjsWasmFunc:)]);
}

@end
