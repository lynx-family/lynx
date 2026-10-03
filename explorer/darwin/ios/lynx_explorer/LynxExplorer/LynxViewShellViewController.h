// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

@interface LynxViewShellViewController : UIViewController
@property(nonatomic, copy) NSString* url;
@property(nonatomic, copy) NSData* data;
@property(nonatomic, copy) NSDictionary<NSString*, id>* params;
/// Semantic launch data supplied by `LaunchDescriptor`. A nil value preserves
/// the historical Explorer mock-data fallback.
@property(nonatomic, copy) NSDictionary<NSString*, id>* launchInitialData;
/// Global-prop precedence is common < legacy params < page. Host-owned stable
/// props are applied after all three layers.
@property(nonatomic, copy) NSDictionary<NSString*, id>* commonGlobalProps;
@property(nonatomic, copy) NSDictionary<NSString*, id>* pageGlobalProps;
@property(nonatomic, readwrite) BOOL hiddenNav;

- (instancetype)initWithLegacySourceURL:(NSString*)sourceURL
                             parameters:(NSDictionary<NSString*, id>*)parameters;
- (NSString*)getStorageItem:(NSString*)key;

@end
