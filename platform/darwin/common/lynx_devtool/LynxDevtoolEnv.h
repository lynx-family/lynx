// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxConfig.h>

@interface LynxDevtoolEnv : NSObject

+ (instancetype)sharedInstance;
- (void)set:(BOOL)value forKey:(NSString *)key;
- (BOOL)get:(NSString *)key withDefaultValue:(BOOL)value;

- (void)set:(NSSet *)newGroupValues forGroup:(NSString *)groupKey;
- (NSSet *)getGroup:(NSString *)groupKey;

- (void)setSwitchMask:(BOOL)value forKey:(NSString *)key;
- (BOOL)getSwitchMask:(NSString *)key;

- (BOOL)isErrorTypeIgnored:(NSInteger)errCode;

- (BOOL)getDefaultValue:(NSString *)key;

// support iOS platform now
- (void)prepareConfig:(LynxConfig *)config;

@property(nonatomic, readwrite) BOOL showDevtoolBadge
    __attribute__((deprecated("Deprecated after Lynx2.9")));
@property(nonatomic, readwrite) BOOL v8Enabled
    __attribute__((deprecated("Deprecated after Lynx3.1")));
@property(nonatomic, readwrite) BOOL domTreeEnabled;
@property(nonatomic, readwrite) BOOL quickjsDebugEnabled;

// swithches below only support iOS platform now
@property(nonatomic, readwrite) BOOL longPressMenuEnabled;
@property(nonatomic, readonly) BOOL previewScreenshotEnabled;
@property(nonatomic, readwrite) BOOL perfMetricsEnabled;

@end
