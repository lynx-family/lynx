// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxHttpRequest : NSObject

@property(nullable, copy) NSString *httpMethod;

@property(nullable, copy) NSString *url;

@property(nullable, copy) NSString *originUrl;

@property(nullable, copy) NSData *httpBody;

@property(nullable, copy) NSDictionary<NSString *, NSString *> *httpHeaders;

@property(nullable, copy) NSDictionary<NSString *, NSObject *> *customConfig;

@end

@interface LynxHttpResponse : NSObject

@property NSInteger statusCode;

@property(nullable, copy) NSString *statusText;

@property(nullable, copy) NSDictionary *httpHeaders;

@property(nullable, copy) NSString *url;

@property(nullable) NSData *httpBody;

@property(nullable, copy) NSDictionary<NSString *, NSObject *> *customInfo;

@end

NS_ASSUME_NONNULL_END
