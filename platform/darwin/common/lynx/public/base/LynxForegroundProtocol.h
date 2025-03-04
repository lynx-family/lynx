// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LynxForegroundProtocol_h
#define LynxForegroundProtocol_h

#import <Foundation/Foundation.h>

@protocol LynxForegroundProtocol <NSObject>

@required
// when lynxview is foreground
- (void)onEnterForeground;
// when lynxview is background
- (void)onEnterBackground;

@end

#endif /* LynxForegroundProtocol_h */
