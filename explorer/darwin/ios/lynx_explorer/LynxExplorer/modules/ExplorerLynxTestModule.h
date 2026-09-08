// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContextModule.h>

NS_ASSUME_NONNULL_BEGIN

/// Page-scoped native operations exposed as NativeModules.LynxTestModule.
@interface ExplorerLynxTestModule : NSObject <LynxContextModule>

- (void)eventTest:(NSString *)value;
- (void)valueTest:(NSString *)value;
- (void)back;
- (void)reload;
- (void)call:(NSString *)name params:(NSDictionary *)params callback:(LynxCallbackBlock)callback;
- (void)invokeParams:(NSDictionary *)params callback:(LynxCallbackBlock)callback;
- (NSDictionary *)call:(NSString *)name params:(NSDictionary *)params;
- (void)updateData:(NSDictionary *)data;
- (void)resetData:(NSDictionary *)data;
- (void)updateGlobalProps:(NSDictionary *)props;
- (void)reloadTemplate:(NSDictionary *)data props:(NSDictionary *)props;
- (void)getPageDataByKey:(NSArray *)keys callback:(LynxCallbackBlock)callback;
- (void)updateScreenMatrix:(NSDictionary *)matrix;
- (void)addButton:(NSDictionary *)info;
- (void)setDefaultValueForSetting:(NSDictionary *)info;

@end

NS_ASSUME_NONNULL_END
