// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCEREQUEST_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCEREQUEST_H_

#import <Foundation/Foundation.h>
#import "LynxServiceResourceRequestParameters.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxResourceRequestAsyncMode) {
  EXACTLY_ASYNC,
  EXACTLY_SYNC,
  MOST_SYNC,
};

typedef NS_ENUM(NSInteger, LynxResourceRequestType) {
  LynxResourceTypeGeneric,
  LynxResourceTypeImage,
  LynxResourceTypeFont,
  LynxResourceTypeLottie,
  LynxResourceTypeVideo,
  LynxResourceTypeSVG,
  LynxResourceTypeTemplate,
  LynxResourceTypeLynxCoreJS,
  LynxResourceTypeDynamicComponent,
  LynxResourceTypeI18NText,
  LynxResourceTypeTheme,
  LynxResourceTypeExternalJS
};

@interface LynxResourceRequest : NSObject

@property(nonatomic, readonly, copy) NSString* url;
@property(nonatomic, readonly, assign) LynxResourceRequestType type;
@property(nonatomic, readwrite, strong) id requestParams;
@property(nonatomic, readwrite, assign) LynxResourceRequestAsyncMode mode;

- (instancetype)initWithUrl:(NSString*)url;
- (instancetype)initWithUrl:(NSString*)url type:(LynxResourceRequestType)type;
- (instancetype)initWithUrl:(NSString*)url andRequestParams:(id)requestParams;

// Only for LynxResourceFetcher use. Return the full request parameters for forest.
- (LynxServiceResourceRequestParameters* _Nullable)getLynxResourceServiceRequestParams;
@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCEREQUEST_H_
