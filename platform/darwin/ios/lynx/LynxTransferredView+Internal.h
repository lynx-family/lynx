// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTransferredView.h>

@interface LynxTransferredView (Internal)
- (instancetype)initWithId:(NSString*)transferId
                    client:(id<LynxTransferClient>)client
                  dispatch:(void (^)(NSString*, NSDictionary*))dispatch
                unregister:(dispatch_block_t)unregisterCard;
- (void)startComponent:(NSString*)componentName initialProps:(NSDictionary*)props;
- (void)bindContent:(UIView*)content;
- (void)detachContent:(UIView*)content;
- (void)dispose:(LynxTransferReleaseReason)reason;
@end
