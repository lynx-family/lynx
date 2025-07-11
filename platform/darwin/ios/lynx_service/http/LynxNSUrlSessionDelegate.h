// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <LynxService/LynxHttpService.h>

@interface LynxNSUrlSessionDelegate : NSObject <NSURLSessionDelegate>
- (instancetype)initWithDelegate:(LynxHttpStreamingDelegate *)httpDelegate
                    withCallback:(LynxHttpCallback)callback;
@end
