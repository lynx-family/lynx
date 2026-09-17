// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_PUBLIC_LYNX_RESOURCE_HANDLE_H_
#define DARWIN_COMMON_LYNX_PUBLIC_LYNX_RESOURCE_HANDLE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * @apidoc
 * @brief Describes one reusable immutable resource snapshot that can be consumed by Lynx.
 * Invalidating this object releases its ownership without interrupting native consumers that
 * already retained the snapshot.
 */
@interface LynxResourceHandle : NSObject

/**
 * @apidoc
 * @brief Creates a reusable resource handle for an absolute local file path.
 * @param filePath An absolute local filesystem path. URI-form and relative paths are not
 * supported.
 * @return A resource handle that synchronously caches the file bytes or read failure during
 * initialization. Create a new handle to retry a failed read. Returns `nil` when `filePath` is
 * empty, relative, or URI-form.
 */
- (nullable instancetype)initWithFilePath:(NSString*)filePath NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

/**
 * @apidoc
 * @brief Returns the absolute local file path described by this handle.
 */
@property(nonatomic, readonly, copy) NSString* filePath;

/**
 * @apidoc
 * @brief Returns the immutable snapshot size in bytes.
 * @return The byte length, or `-1` when the data is unavailable or this handle has been
 * invalidated.
 */
@property(nonatomic, readonly) NSInteger size;

/**
 * @apidoc
 * @brief Returns whether this handle still owns a native resource description.
 */
@property(nonatomic, readonly, getter=isValid) BOOL valid;

/**
 * @apidoc
 * @brief Invalidates this handle's native ownership. Calling this method more than once is safe.
 */
- (void)invalidate;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_PUBLIC_LYNX_RESOURCE_HANDLE_H_
