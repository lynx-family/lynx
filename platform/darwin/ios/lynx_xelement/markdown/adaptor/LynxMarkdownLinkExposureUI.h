// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI+Internal.h>

@interface LynxMarkdownLinkExposureUIV2 : LynxUI <UIView *>
- (instancetype)initWithRect:(CGRect)rect
                    uniqueID:(NSString *)uniqueID
                         url:(NSString *)url
                     content:(NSString *)content;
- (NSString *)getUniqueID;
- (NSDictionary *)getData;
- (NSDictionary *)getOption;
@end
