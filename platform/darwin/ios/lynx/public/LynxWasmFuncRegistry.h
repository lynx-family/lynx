// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@interface LynxWasmFuncRegistry : NSObject

// Install the JavaScriptCore WebAssembly registration function pointer.
+ (void)registerWasmFunc:(void*)func;

// Install a non-null QuickJS WebAssembly registration function pointer before
// the first QuickJS context is created. A null pointer leaves the current
// registration unchanged. Callers must check respondsToSelector when supporting
// older Registry implementations without this entry.
+ (void)registerQjsWasmFunc:(void*)func;

@end
