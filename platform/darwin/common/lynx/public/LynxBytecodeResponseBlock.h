// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * block that represents a bytecode generate result callback.
 * @param errorMsg when success, errorMsg will be null, otherwise is error.
 * @param buffers when success, this will be bytecode result. Each key is sourceUrl, value is
 * bytecode result.Note: Please note that the current NSData is constructed using
 * dataWithBytesNoCopy. If you need to use it, please make a copy yourself and do not store this
 * object directly.
 */
typedef void (^LynxBytecodeResponseBlock)(NSString* _Nullable errorMsg,
                                          NSDictionary<NSString*, NSData*>* _Nullable buffers);

NS_ASSUME_NONNULL_END
