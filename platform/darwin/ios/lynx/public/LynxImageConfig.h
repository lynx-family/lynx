// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@interface LynxImageConfig : NSObject

/**
 * Additional custom parameters passed to the image library.
 * Useful for business or library-specific options.
 * Can be overridden by the component's `additional-custom-info` property.
 */
@property(nonatomic, strong, nullable) NSDictionary* additionalCustomInfo;

/**
 * Default metadata for the preview placeholder hash.
 * Used as a fallback when `placeholder-hash-config` (type "preview") does not specify `metaData`.
 */
@property(nonatomic, strong, nullable) NSString* previewHashMetaData;

@end
