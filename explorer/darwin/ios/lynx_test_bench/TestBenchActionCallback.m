// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchActionCallback.h"
#import <Foundation/Foundation.h>

@interface TestBenchActionCallback ()
@end

@implementation TestBenchActionCallback
- (void)onLynxViewWillBuild:(TestBenchActionManager *)manager builder:(LynxViewBuilder *)builder {
}
- (void)onLynxViewDidBuild:(LynxView *)lynxView {
}
- (void)onError:(TestBenchReplayError)code message:(NSString *)message {
}
@end
