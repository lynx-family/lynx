// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <LynxDevtool/LynxTimingOverlayHelper.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxTimingFSPInfo;

@interface LynxTimingOverlayHelper () {
 @private
  LynxTimingFSPInfo *_fspInfo;
}

- (void)setNeedsUpdate;

- (void)log:(LynxLogLevel)level message:(NSString *)format, ...;

@end

NS_ASSUME_NONNULL_END
