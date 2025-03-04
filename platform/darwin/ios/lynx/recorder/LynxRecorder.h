// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxView.h"

NS_ASSUME_NONNULL_BEGIN

/*!
 LynxRecorderDarwin  is used to record the necessary input for the OC code.
*/
@interface LynxRecorder : NSObject

+ (instancetype)sharedInstance;

- (void)recordUIEvent:(nullable UIEvent*)event withLynxView:(nullable LynxView*)lynxView;

@end

NS_ASSUME_NONNULL_END
