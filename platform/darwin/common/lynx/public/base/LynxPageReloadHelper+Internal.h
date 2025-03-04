// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxPageReloadHelper.h"

@interface LynxPageReloadHelper ()

/**
 * Load template.js content success from url
 */
- (void)onTemplateLoadSuccess:(nullable NSData*)tem;

/**
 * Get template.js content from offset to offset + size
 */
- (NSString* _Nullable)getTemplateJsInfo:(uint32_t)offset withSize:(uint32_t)size;

@end
