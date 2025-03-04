// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxError.h"

@protocol LUIErrorHandling <NSObject>

// deprecated: use didReceiveResourceError:withSource:type: instead
@optional
- (void)didReceiveResourceError:(NSError *_Nullable)error;

@required
- (void)didReceiveResourceError:(LynxError *_Nullable)error
                     withSource:(NSString *_Nullable)resourceUrl
                           type:(NSString *_Nullable)type;
@required
- (void)reportError:(nonnull NSError *)error;

@required
- (void)reportLynxError:(LynxError *_Nullable)error;

@end
