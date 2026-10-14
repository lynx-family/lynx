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

/**
 * block consulted once per decoded js file right before its bytecode would be generated.
 * It is invoked on a background thread.
 * @param jsName the name of the decoded js file, the same as its key in the buffers of
 * LynxBytecodeResponseBlock.
 * @param md5 the md5 of the js file's source content.
 * @return YES to continue generating bytecode for this file; NO to skip it, e.g. when the caller
 * already holds the bytecode for this md5.
 */
typedef BOOL (^LynxBytecodeContinueBlock)(NSString* jsName, NSString* md5);

NS_ASSUME_NONNULL_END
