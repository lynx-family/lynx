// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxConverter.h"
#import "LynxConverter+UI.h"

@implementation LynxConverter

+ (BOOL)isBoolNumber:(NSNumber *)num {
  CFTypeID boolID = CFBooleanGetTypeID();                   // the type ID of CFBoolean
  CFTypeID numID = CFGetTypeID((__bridge CFTypeRef)(num));  // the type ID of num
  return numID == boolID;
}

+ (NSNumber *)toNSNumber:(id)value {
  if ([value isKindOfClass:[NSNumber class]]) {
    return value;
  }
  NSAssert(false, @"NSNumber convert error with value %@", value);
  return nil;
}

+ (NSNumber *)toid:(id)value {
  return value;
}

+ (NSString *)toNSString:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return @"";
  } else if ([value isKindOfClass:[NSString class]]) {
    return value;
  } else if ([value isKindOfClass:[NSNumber class]]) {
    // Fiber Compatible with the old version
    if ([self isBoolNumber:value]) {
      return [value boolValue] ? @"true" : @"false";
    } else {
      return [value stringValue];
    }
  }
  NSAssert(false, @"NSString convert error with value %@", value);
  return nil;
}

+ (CGFloat)toCGFloat:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return 0;
  }
  @try {
    return [value doubleValue];
  } @catch (__unused NSException *e) {
    NSAssert(false, @"CGFloat convert error with value %@", value);
    value = nil;
    return [value doubleValue];
  }
}

+ (NSInteger)toNSInteger:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return 0;
  }
  @try {
    return [value integerValue];
  } @catch (__unused NSException *e) {
    NSAssert(false, @"NSInteger convert error with value %@", value);
    value = nil;
    return [value integerValue];
  }
}

+ (int)toint:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return 0;
  }
  @try {
    return (int)[value integerValue];
  } @catch (__unused NSException *e) {
    NSAssert(false, @"NSInteger convert error with value %@", value);
    value = nil;
    return (int)[value integerValue];
  }
}

+ (NSUInteger)toNSUInteger:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return 0;
  }
  @try {
    return [value unsignedIntegerValue];
  } @catch (__unused NSException *e) {
    NSAssert(false, @"NSUInteger convert error with value %@", value);
    value = nil;
    return [value unsignedIntegerValue];
  }
}

+ (CGColorRef)toCGColorRef:(id)value {
  return [self toUIColor:value].CGColor;
}

+ (BOOL)toBOOL:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return NO;
  } else if ([value isKindOfClass:[NSNumber class]]) {
    return [value boolValue];
  } else if ([value isKindOfClass:[NSString class]]) {
    return [@"true" isEqualToString:value] || [@"YES" isEqualToString:value];
  }
  NSAssert(false, @"BOOL convert error with value %@", value);
  return NO;
}

+ (NSTimeInterval)toNSTimeInterval:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return 0;
  }
  return [value doubleValue] / 1000;
}

@end
