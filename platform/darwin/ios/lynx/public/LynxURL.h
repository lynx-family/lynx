// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxResourceRequest.h>

typedef NS_ENUM(NSInteger, LynxImageRequestType) {
  LynxImageRequestUndefined,
  LynxImageRequestSrc,
  LynxImageRequestPlaceholder,
};

@interface LynxURL : NSObject

@property(nonatomic) NSURL* url;
@property(nonatomic) NSURL* redirectedURL;
@property(nonatomic) BOOL initiallyLoaded;
@property(nonatomic) LynxImageRequestType type;
@property(nonatomic) NSURL* lastRequestUrl;
@property(nonatomic) NSURL* preUrl;
@property(nonatomic) BOOL fromMemoryCache;
@property(nonatomic) LynxResourceRequest* request;

// Image status info
/*
 "FetchTime" represents the time interval between sending a request and receiving a callback, which
 essentially encompasses both the loading time and decode time.
 */
@property(nonatomic) NSTimeInterval fetchTime;
/*
 "CompleteTime" is the sum of "FetchTime" and Lynx's internal rendering duration.
 */
@property(nonatomic) NSTimeInterval completeTime;
@property(nonatomic) CGFloat memoryCost;
@property(nonatomic) NSInteger isSuccess;
@property(nonatomic) NSError* error;
@property(nonatomic) CGSize imageSize;

/*
 resourceInfo get from imageService.
 */
@property(nonatomic) NSMutableDictionary* resourceInfo;

/*
 A dictionary used to pass all image info to load and monitor.
 */
@property(nonatomic) NSMutableDictionary* reportInfo;

- (void)updatePreviousUrl;
- (BOOL)isPreviousUrl;
- (void)initResourceInformation;
- (void)updateTimeStamp:(NSDate*)getImageTime startRequestTime:(NSDate*)startRequestTime;
@end  // LynxURL
