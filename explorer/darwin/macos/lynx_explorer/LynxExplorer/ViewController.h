// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface ViewController : NSViewController

@property(nonatomic, strong) NSString *url;
@property(nonatomic, strong) NSData *data;

- (nonnull instancetype)initWithUrl:(nullable NSString *)url;
- (void)loadTemplateFromURL:(nonnull NSString *)url;
- (void)onRefresh;

@end

NS_ASSUME_NONNULL_END
