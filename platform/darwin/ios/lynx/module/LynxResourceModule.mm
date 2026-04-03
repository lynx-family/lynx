// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxResourceModule.h"

#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceImageProtocol.h>
#import <Lynx/LynxServiceResourceProtocol.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTraceEvent.h>
#import <Lynx/LynxTraceEventWrapper.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxView+Internal.h>
#import "LynxTraceEventDef.h"

@interface LynxResourceModule () <LynxFontFaceObserver>
@end

static NSString* kDataKey = @"data";
static NSString* kUriKey = @"uri";
static NSString* kTypeKey = @"type";
static NSString* kParamsKey = @"params";
static NSString* kCodeKey = @"code";
static NSString* kMsgKey = @"msg";
static NSString* kDetailKey = @"details";

static NSString* kImageType = @"image";
static NSString* kAudioType = @"audio";
static NSString* kVideoType = @"video";
static NSString* kFontType = @"font";

static NSInteger kDefaultMediaSize = 500 * 1024;

static NSString* const kParamErrorFixSuggestion =
    @"Please check the parameters passed to Lynx resource prefetch module.";

static NSString* const kResourceServiceErrorFixSuggestion =
    @"If it is a parameter error, please check the parameters passed in. If the Resource service "
    @"does not exist, it may be due to an error that occurred while creating the resource service "
    @"through reflection. Please contact the client RD for help.";

static NSString* const kAwaitTimeoutMsg =
    @"The prefetch task did not complete within the specified timeout.";

typedef void (^LynxPrefetchInternalCompletionBlock)(NSInteger code, NSString* _Nullable msg);

@interface LynxFontFacePrefetchObserver : NSObject <LynxFontFaceObserver>
@property(nonatomic, copy) LynxPrefetchInternalCompletionBlock completed;
@property(nonatomic, strong) LynxFontFaceContext* tempContext;
@end

@implementation LynxFontFacePrefetchObserver
- (void)onFontFaceLoad {
  if (self.completed) {
    self.completed(ECLynxSuccess, @"");
    self.completed = nil;
  }
}
@end

@implementation LynxResourceModule {
  __weak LynxContext* context_;
  NSMutableDictionary<NSString*, LynxFontFacePrefetchObserver*>* _fontFacePrefetchObserverMap;
}

+ (NSString*)name {
  return @"LynxResourceModule";
}

+ (NSDictionary<NSString*, NSString*>*)methodLookup {
  return @{
    @"requestResourcePrefetch" : NSStringFromSelector(@selector(requestResourcePrefetch:
                                                                               callback:config:)),
    @"cancelResourcePrefetch" : NSStringFromSelector(@selector(cancelResourcePrefetch:callback:)),
    @"requestResourcePrefetchImage" :
        NSStringFromSelector(@selector(requestResourcePrefetchImage:callback:)),
  };
}

- (instancetype)initWithLynxContext:(LynxContext*)context {
  self = [super init];
  if (self) {
    context_ = context;
    _fontFacePrefetchObserverMap = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)reportError:(NSInteger)code
            message:(NSString*)msg
      fixSuggestion:(NSString*)fixSuggestion
                uri:(nullable NSString*)uri
         actionType:(nullable NSString*)actionType {
  LynxError* error = [LynxError lynxErrorWithCode:code
                                          message:msg
                                    fixSuggestion:fixSuggestion
                                            level:LynxErrorLevelError];
  if (uri) {
    [error addCustomInfo:uri forKey:@"resourceUri"];
  }
  if (actionType) {
    [error addCustomInfo:actionType forKey:@"actionType"];
  }
  [context_ reportLynxError:error];
}

- (std::pair<NSInteger, NSString*>)resourcePrefetch:(NSDictionary*)prefetchData
                                           isCancel:(BOOL)isCancel
                                         allResults:(NSMutableDictionary*)allResults {
  NSInteger globalCode = ECLynxSuccess;
  NSString* globalMsg = @"";

  id data = [prefetchData objectForKey:kDataKey];

  if (data == nil || ![data isKindOfClass:[NSArray class]]) {
    globalCode = ECLynxResourceModuleParamsError;
    globalMsg =
        @"Parameters error in Lynx resource prefetch module! Value of 'data' should be an array.";
    [self reportError:globalCode
              message:globalMsg
        fixSuggestion:kParamErrorFixSuggestion
                  uri:nil
           actionType:(isCancel ? @"cancel" : @"request")];
  } else {
    NSArray* array = data;
    NSMutableArray* resultArray = [[NSMutableArray alloc] init];
    for (id obj in array) {
      NSInteger code = ECLynxSuccess;
      NSString* msg = @"";
      NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
      id uri = @"";
      if (![obj isKindOfClass:[NSDictionary class]]) {
        code = ECLynxResourceModuleParamsError;
        msg = @"Parameters error in Lynx resource prefetch module! The prefetch data should be a "
              @"map.";
      } else {
        uri = [obj objectForKey:kUriKey];
        id type = [obj objectForKey:kTypeKey];
        id params = [obj objectForKey:kParamsKey];
        if (uri == nil || type == nil) {
          code = ECLynxResourceModuleParamsError;
          msg = @"Parameters error in Lynx resource prefetch module! 'resource uri' or 'type' is "
                @"null.";
        } else {
          auto res = isCancel ? [self cancelResourcePrefetchInternal:uri type:type params:params]
                              : [self requestResourcePrefetchInternal:uri type:type params:params];
          code = res.first;
          msg = res.second;
          result[kUriKey] = uri;
          result[kTypeKey] = type;
        }
      }
      if (code != ECLynxSuccess) {
        [self reportError:code
                  message:msg
            fixSuggestion:kResourceServiceErrorFixSuggestion
                      uri:uri
               actionType:(isCancel ? @"cancel" : @"request")];
      }
      result[kCodeKey] = @(code);
      result[kMsgKey] = msg;
      [resultArray addObject:result];
    }
    allResults[kDetailKey] = resultArray;
  }
  return std::make_pair(globalCode, globalMsg);
}

- (void)resourcePrefetchAwaitComplete:(NSDictionary*)prefetchData
                             callback:(LynxCallbackBlock)callback
                              timeout:(long)awaitTimeout {
  NSMutableDictionary* allResults = [[NSMutableDictionary alloc] init];
  allResults[kCodeKey] = @(ECLynxSuccess);
  allResults[kMsgKey] = @"";

  id data = [prefetchData objectForKey:kDataKey];

  if (data == nil || ![data isKindOfClass:[NSArray class]]) {
    NSInteger globalCode = ECLynxResourceModuleParamsError;
    NSString* globalMsg =
        @"Parameters error in Lynx resource prefetch module! Value of 'data' should be an array.";
    [self reportError:globalCode
              message:globalMsg
        fixSuggestion:kParamErrorFixSuggestion
                  uri:nil
           actionType:@"request"];
    allResults[kCodeKey] = @(globalCode);
    allResults[kMsgKey] = globalMsg;
    if (callback) {
      callback(allResults);
    }
    return;
  }

  NSArray* array = data;
  NSUInteger totalCount = [array count];
  if (totalCount == 0) {
    if (callback) {
      callback(allResults);
    }
    return;
  }

  __block NSUInteger completedCount = 0;
  __block BOOL isCallbackInvoked = NO;
  NSMutableArray* resultArray = [[NSMutableArray alloc] init];
  allResults[kDetailKey] = resultArray;

  __weak typeof(self) weakSelf = self;
  __weak typeof(context_) weakContext = context_;

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(awaitTimeout * NSEC_PER_MSEC)),
                 dispatch_get_main_queue(), ^{
                   [weakContext runOnJSThread:^{
                     if (!isCallbackInvoked) {
                       isCallbackInvoked = YES;
                       if (callback) {
                         allResults[kCodeKey] = @(ECLynxResourceModuleAwaitTimeout);
                         allResults[kMsgKey] = kAwaitTimeoutMsg;
                         callback(allResults);
                       }
                     }
                   }];
                 });

  for (id obj in array) {
    NSString* uri = @"";
    NSString* type = @"";
    id params = nil;
    NSInteger initialCode = ECLynxSuccess;
    NSString* initialMsg = @"";

    if (![obj isKindOfClass:[NSDictionary class]]) {
      initialCode = ECLynxResourceModuleParamsError;
      initialMsg =
          @"Parameters error in Lynx resource prefetch module! The prefetch data should be a map.";
    } else {
      uri = [obj objectForKey:kUriKey];
      type = [obj objectForKey:kTypeKey];
      params = [obj objectForKey:kParamsKey];
      if (uri == nil || type == nil) {
        initialCode = ECLynxResourceModuleParamsError;
        initialMsg =
            @"Parameters error in Lynx resource prefetch module! 'resource uri' or 'type' is null.";
      }
    }

    LynxPrefetchInternalCompletionBlock internalCallback =
        ^(NSInteger code, NSString* _Nullable msg) {
          if (code != ECLynxSuccess) {
            [weakSelf reportError:code
                          message:msg
                    fixSuggestion:kResourceServiceErrorFixSuggestion
                              uri:uri
                       actionType:@"request"];
          }

          [weakContext runOnJSThread:^{
            if (isCallbackInvoked) {
              return;
            }
            NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
            result[kCodeKey] = @(code);
            result[kMsgKey] = msg ?: @"";
            result[kUriKey] = uri;
            result[kTypeKey] = type;

            [resultArray addObject:result];
            completedCount++;
            if (completedCount == totalCount && callback) {
              isCallbackInvoked = YES;
              callback(allResults);
            }
          }];
        };

    if (initialCode != ECLynxSuccess) {
      internalCallback(initialCode, initialMsg);
    } else {
      [self requestResourcePrefetchInternalAwaitComplete:uri
                                                    type:type
                                                  params:params
                                                callback:internalCallback];
    }
  }
}

- (void)requestResourcePrefetch:(NSDictionary*)prefetchData
                       callback:(LynxCallbackBlock)callback
                         config:(nullable NSDictionary*)config {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, RESOURCE_MODULE_REQUEST_PREFETCH);

  BOOL awaitComplete = NO;
  if (config && [config objectForKey:@"awaitComplete"]) {
    awaitComplete = [[config objectForKey:@"awaitComplete"] boolValue];
  }

  long awaitTimeout = 60000;
  if (config && [config objectForKey:@"awaitTimeout"]) {
    awaitTimeout = [[config objectForKey:@"awaitTimeout"] longValue];
  }

  if (awaitComplete) {
    [self resourcePrefetchAwaitComplete:prefetchData callback:callback timeout:awaitTimeout];
  } else {
    NSMutableDictionary* allResults = [[NSMutableDictionary alloc] init];
    auto res = [self resourcePrefetch:prefetchData isCancel:NO allResults:allResults];
    allResults[kCodeKey] = @(res.first);
    allResults[kMsgKey] = res.second;
    if (callback) {
      callback(allResults);
    }
  }

  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
}

- (void)cancelResourcePrefetch:(NSDictionary*)prefetchData callback:(LynxCallbackBlock)callback {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, RESOURCE_MODULE_CANCEL_PREFETCH);

  NSMutableDictionary* allResults = [[NSMutableDictionary alloc] init];

  auto res = [self resourcePrefetch:prefetchData isCancel:YES allResults:allResults];
  NSInteger globalCode = res.first;
  NSString* globalMsg = res.second;

  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
  allResults[kCodeKey] = @(globalCode);
  allResults[kMsgKey] = globalMsg;
  if (callback) {
    callback(allResults);
  }
}

- (void)requestResourcePrefetchInternalAwaitComplete:(NSString*)uri
                                                type:(NSString*)type
                                              params:(nullable NSDictionary*)params
                                            callback:(LynxPrefetchInternalCompletionBlock)callback {
  if ([type isEqualToString:kImageType]) {
    LynxURL* lynxUri = [[LynxURL alloc] init];
    lynxUri.url = [[NSURL alloc] initWithString:uri];
    id<LynxServiceImageProtocol> service = LynxService(LynxServiceImageProtocol);
    if (service) {
      [service prefetchImage:lynxUri
                      params:params
                   completed:^(UIImage* _Nullable image, NSError* _Nullable error,
                               NSURL* _Nullable imageURL) {
                     if (error) {
                       callback(error.code, [error description]);
                     } else {
                       callback(ECLynxSuccess, @"");
                     }
                   }];
    } else {
      callback(ECLynxResourceModuleImgPrefetchHelperNotExist,
               @"Image prefetch service do not exist!");
    }
  } else if ([type isEqualToString:kFontType]) {
    NSString* fontFamily = params[@"font-family"];
    if ([fontFamily length] == 0) {
      fontFamily = uri;
    }
    LynxUIOwner* uiOwner = context_.uiOwner;
    LynxFontFaceContext* fontFaceContext = uiOwner.fontFaceContext;
    if (fontFaceContext) {
      LynxFontFaceContext* tempContext = [[LynxFontFaceContext alloc] init];
      tempContext.rootView = fontFaceContext.rootView;
      tempContext.resourceFetcher = fontFaceContext.resourceFetcher;
      tempContext.resourceProvider = fontFaceContext.resourceProvider;
      tempContext.genericResourceServiceFetcher = fontFaceContext.genericResourceServiceFetcher;
      tempContext.builderRegistedAliasFontMap = fontFaceContext.builderRegistedAliasFontMap;

      NSString* src = [NSString stringWithFormat:@"url('%@')", uri];
      LynxFontFace* fontFace = [[LynxFontFace alloc] initWithFamilyName:fontFamily
                                                                 andSrc:src
                                                        withLynxContext:context_];
      [tempContext addFontFace:fontFace];
      LynxFontFacePrefetchObserver* observer = [[LynxFontFacePrefetchObserver alloc] init];
      observer.tempContext = tempContext;
      __weak typeof(self) weakSelf = self;
      __block NSString* observerKey = nil;
      observer.completed = ^(NSInteger code, NSString* _Nullable msg) {
        callback(code, msg);
        __strong typeof(self) strongSelf = weakSelf;
        if (strongSelf && observerKey != nil) {
          [strongSelf->_fontFacePrefetchObserverMap removeObjectForKey:observerKey];
        }
      };
      BOOL didRequestAsync = NO;
      [[LynxFontFaceManager sharedManager] generateFontWithSize:12
                                                         weight:UIFontWeightRegular
                                                          style:LynxFontStyleNormal
                                                 fontFamilyName:fontFamily
                                                fontFaceContext:tempContext
                                               fontFaceObserver:observer
                                                didRequestAsync:&didRequestAsync];
      if (didRequestAsync) {
        // Keep observer alive until onFontFaceLoad is called.
        // LynxFontFaceManager holds observer weakly in its notifierArray.
        observerKey = [NSString stringWithFormat:@"%p", observer];
        _fontFacePrefetchObserverMap[observerKey] = observer;
      } else {
        [observer onFontFaceLoad];
      }
    } else {
      callback(ECLynxResourceModuleParamsError, @"FontFaceContext is null");
    }
  } else if ([type isEqualToString:kAudioType] || [type isEqualToString:kVideoType]) {
    NSString* preloadKey = params[@"preloadKey"];
    NSString* videoID = params[@"videoID"];
    NSString* videoModelStr = params[@"videoModel"];
    NSUInteger resolution = [params[@"resolution"] unsignedIntegerValue];
    NSUInteger encodeType = [params[@"encodeType"] unsignedIntegerValue];
    NSString* apiString = params[@"apiString"];
    NSDictionary* videoModel = nil;
    if (videoModelStr) {
      NSData* data = [videoModelStr dataUsingEncoding:NSUTF8StringEncoding];
      if (data.length) {
        videoModel = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
      }
    }

    NSInteger size = kDefaultMediaSize;
    if (params[@"size"]) {
      size = [params[@"size"] integerValue];
    }

    id<LynxServiceResourceProtocol> service = LynxService(LynxServiceResourceProtocol);

    if (!preloadKey) {
      callback(ECLynxResourceModuleParamsError, @"missing preloadKey!");
    } else if (!service) {
      callback(ECLynxResourceModuleResourceServiceNotExist, @"Resource service do not exist!");
    } else {
      [service preloadMedia:uri
                   cacheKey:preloadKey
                    videoID:videoID
                 videoModel:videoModel
                 resolution:resolution
                 encodeType:encodeType
                  apiString:apiString
                       size:size
                  completed:callback];
    }
  } else {
    callback(ECLynxResourceModuleParamsError,
             [NSString stringWithFormat:@"%@%@", @"Parameters error! Unknown type :", type]);
  }
}

- (std::pair<NSInteger, NSString*>)cancelResourcePrefetchInternal:(NSString*)uri
                                                             type:(NSString*)type
                                                           params:(nullable NSDictionary*)params {
  NSInteger code = ECLynxSuccess;
  NSString* msg = @"";
  if ([type isEqualToString:kImageType]) {
    // TODO(wujintian): add image prefetch implementation
    code = ECLynxResourceModuleParamsError;
    msg = @"Image prefetch dose not been supported on iOS yet.";
  } else if ([type isEqualToString:kAudioType] || [type isEqualToString:kVideoType]) {
    NSString* preloadKey = params[@"preloadKey"];
    NSString* videoID = params[@"videoID"];
    BOOL videoModel = [params[@"videoModel"] boolValue];
    id<LynxServiceResourceProtocol> service = LynxService(LynxServiceResourceProtocol);
    if (!preloadKey) {
      code = ECLynxResourceModuleParamsError;
      msg = @"missing preloadKey!";
    } else if (!service) {
      code = ECLynxResourceModuleResourceServiceNotExist;
      msg = @"Resource service do not exist!";
    } else {
      [service cancelPreloadMedia:preloadKey videoID:videoID videoModel:videoModel];
    }
  } else {
    code = ECLynxResourceModuleParamsError;
    msg = [NSString stringWithFormat:@"%@%@", @"Parameters error! Unknown type :", type];
  }
  LOGI("LynxResourceModule requestResourcePrefetch uri: " << uri << " type: " << type);
  return std::make_pair(code, msg);
}

- (std::pair<NSInteger, NSString*>)requestResourcePrefetchInternal:(NSString*)uri
                                                              type:(NSString*)type
                                                            params:(nullable NSDictionary*)params {
  NSInteger code = ECLynxSuccess;
  NSString* msg = @"";
  if ([type isEqualToString:kImageType]) {
    LynxURL* lynxUri = [[LynxURL alloc] init];
    lynxUri.url = [[NSURL alloc] initWithString:uri];
    id<LynxServiceImageProtocol> service = LynxService(LynxServiceImageProtocol);
    if (service) {
      [service prefetchImage:lynxUri params:params];
    } else {
      code = ECLynxResourceModuleImgPrefetchHelperNotExist;
      msg = @"Image prefetch service do not exist!";
    }
  } else if ([type isEqualToString:kFontType]) {
    NSString* fontFamily = params[@"font-family"];
    if ([fontFamily length] == 0) {
      fontFamily = uri;
    }
    LynxUIOwner* uiOwner = context_.uiOwner;
    LynxFontFaceContext* fontFaceContext = uiOwner.fontFaceContext;
    if (fontFaceContext) {
      // Create a temporary context to avoid modifying the page's existing font context
      // and to avoid potential thread-safety issues.
      LynxFontFaceContext* tempContext = [[LynxFontFaceContext alloc] init];
      tempContext.rootView = fontFaceContext.rootView;
      tempContext.resourceFetcher = fontFaceContext.resourceFetcher;
      tempContext.resourceProvider = fontFaceContext.resourceProvider;
      tempContext.genericResourceServiceFetcher = fontFaceContext.genericResourceServiceFetcher;
      tempContext.builderRegistedAliasFontMap = fontFaceContext.builderRegistedAliasFontMap;

      NSString* src = [NSString stringWithFormat:@"url('%@')", uri];
      LynxFontFace* fontFace = [[LynxFontFace alloc] initWithFamilyName:fontFamily
                                                                 andSrc:src
                                                        withLynxContext:context_];
      [tempContext addFontFace:fontFace];
      [[LynxFontFaceManager sharedManager] generateFontWithSize:12
                                                         weight:UIFontWeightRegular
                                                          style:LynxFontStyleNormal
                                                 fontFamilyName:fontFamily
                                                fontFaceContext:tempContext
                                               fontFaceObserver:self];
    }
  } else if ([type isEqualToString:kAudioType] || [type isEqualToString:kVideoType]) {
    NSString* preloadKey = params[@"preloadKey"];
    NSString* videoID = params[@"videoID"];
    NSString* videoModelStr = params[@"videoModel"];
    NSUInteger resolution = [params[@"resolution"] unsignedIntegerValue];
    NSUInteger encodeType = [params[@"encodeType"] unsignedIntegerValue];
    NSString* apiString = params[@"apiString"];
    NSDictionary* videoModel = nil;
    if (videoModelStr) {
      NSData* data = [videoModelStr dataUsingEncoding:NSUTF8StringEncoding];
      if (data.length) {
        videoModel = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
      }
    }

    NSInteger size = kDefaultMediaSize;
    if (params[@"size"]) {
      size = [params[@"size"] integerValue];
    }

    id<LynxServiceResourceProtocol> service = LynxService(LynxServiceResourceProtocol);

    if (!preloadKey) {
      code = ECLynxResourceModuleParamsError;
      msg = @"missing preloadKey!";
    } else if (!service) {
      code = ECLynxResourceModuleResourceServiceNotExist;
      msg = @"Resource service do not exist!";
    } else {
      [service preloadMedia:uri
                   cacheKey:preloadKey
                    videoID:videoID
                 videoModel:videoModel
                 resolution:resolution
                 encodeType:encodeType
                  apiString:apiString
                       size:size
                  completed:nil];
    }
  } else {
    code = ECLynxResourceModuleParamsError;
    msg = [NSString stringWithFormat:@"%@%@", @"Parameters error! Unknown type :", type];
  }
  LOGI("LynxResourceModule requestResourcePrefetch uri: " << uri << " type: " << type);
  return std::make_pair(code, msg);
}

- (void)requestResourcePrefetchImage:(NSDictionary*)prefetchData
                            callback:(LynxCallbackBlock)callback {
  id uri = [prefetchData objectForKey:kUriKey];
  id params = [prefetchData objectForKey:kParamsKey];
  if (uri == nil) {
    NSInteger code = ECLynxResourceModuleParamsError;
    NSString* msg = @"Parameters error in Lynx resource prefetch module! 'uri' is null.";
    [self reportError:code
              message:msg
        fixSuggestion:kParamErrorFixSuggestion
                  uri:nil
           actionType:nil];
    if (callback) {
      NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
      result[kCodeKey] = @(code);
      result[kMsgKey] = msg;
      callback(result);
    }
    return;
  }
  id<LynxServiceImageProtocol> service = LynxService(LynxServiceImageProtocol);
  if (service == nil) {
    NSInteger code = ECLynxResourceModuleImgPrefetchHelperNotExist;
    NSString* msg = @"Image prefetch service do not exist!";
    [self reportError:code
              message:msg
        fixSuggestion:kResourceServiceErrorFixSuggestion
                  uri:nil
           actionType:nil];
    if (callback) {
      NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
      result[kCodeKey] = @(code);
      result[kMsgKey] = msg;
      callback(result);
    }
    return;
  }
  LynxURL* lynxUri = [[LynxURL alloc] init];
  lynxUri.url = [[NSURL alloc] initWithString:uri];
  LynxImageLoadCompletionBlock requestBlock =
      ^(UIImage* _Nullable image, NSError* _Nullable error, NSURL* _Nullable imageURL) {
        if (callback) {
          NSString* errorDetail = [NSString stringWithFormat:@"%@", [error description]];
          NSNumber* errorCode = error ? [NSNumber numberWithInteger:error.code] : @(0);
          NSInteger code = [errorCode integerValue];
          NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
          result[kCodeKey] = @(code);
          result[kMsgKey] = errorDetail ?: @"";
          callback(result);
        }
      };

  [service prefetchImage:lynxUri params:params completed:requestBlock];
}

- (void)onFontFaceLoad {
}

@end
