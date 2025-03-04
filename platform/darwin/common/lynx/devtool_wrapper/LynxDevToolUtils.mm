// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDevToolUtils.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceDevToolProtocol.h>
#import <objc/message.h>

@implementation LynxDevToolUtils

+ (void)setDevtoolEnv:(BOOL)value forKey:(NSString *)key {
  [LynxService(LynxServiceDevToolProtocol) devtoolEnvSetValue:value forKey:key];
}

+ (BOOL)getDevtoolEnv:(NSString *)key withDefaultValue:(BOOL)value {
  return [LynxService(LynxServiceDevToolProtocol) devtoolEnvGetValue:key withDefaultValue:value];
}

+ (void)setDevtoolEnv:(NSSet *)newGroupValues forGroup:(NSString *)groupKey {
  [LynxService(LynxServiceDevToolProtocol) devtoolEnvSet:newGroupValues forGroup:groupKey];
}

+ (NSSet *)getDevtoolEnvWithGroupName:(NSString *)groupKey {
  return [LynxService(LynxServiceDevToolProtocol) devtoolEnvGetGroup:groupKey];
}

@end
