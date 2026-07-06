// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXELEMENT_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXELEMENT_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

// The Objective-C library marker macro shares this public class name. Keep it
// out before declaring or extending the class API.
#if defined(LynxElement)
#pragma push_macro("LynxElement")
#undef LynxElement
#define LYNX_RESTORE_LYNX_ELEMENT_HEADER 1
#endif

NS_ASSUME_NONNULL_BEGIN

@class LynxElement;

typedef NSString *LynxElementStatus NS_STRING_ENUM;
FOUNDATION_EXPORT LynxElementStatus const LynxElementStatusAlive;
FOUNDATION_EXPORT LynxElementStatus const LynxElementStatusDetached;

typedef void (^LynxElementResultCallback)(id _Nullable result);
typedef BOOL (^LynxElementFilter)(LynxElement *node);

@interface LynxElement : NSObject

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

- (void)findNodeById:(NSString *)idSelector
            callback:(nullable void (^)(LynxElement *_Nullable result))callback;
- (void)findNodeByFilter:(nullable LynxElementFilter)filter
                callback:(nullable void (^)(LynxElement *_Nullable result))callback;
- (void)findNodesByFilter:(nullable LynxElementFilter)filter
                 callback:(nullable void (^)(NSArray<LynxElement *> *result))callback;

- (void)getStatus:(nullable void (^)(LynxElementStatus result))callback;
- (void)getSign:(nullable void (^)(NSNumber *result))callback;
- (void)getTagName:(nullable void (^)(NSString *_Nullable result))callback;
- (void)getId:(nullable void (^)(NSString *_Nullable result))callback;

- (void)getParent:(nullable void (^)(LynxElement *_Nullable result))callback;
- (void)getChildren:(nullable void (^)(NSArray<LynxElement *> *result))callback;
- (void)getChildCount:(nullable void (^)(NSNumber *result))callback;
- (void)getRoot:(nullable void (^)(LynxElement *_Nullable result))callback;

- (void)getPositionInfo:(nullable void (^)(NSDictionary<NSString *, id> *_Nullable result))callback;
- (void)getDataset:(nullable void (^)(NSDictionary<NSString *, id> *result))callback;
- (void)getAttribute:(NSString *)name callback:(nullable void (^)(id _Nullable result))callback;
- (void)getAttributes:(nullable void (^)(NSDictionary<NSString *, id> *result))callback;

- (void)getPlatformView:(nullable void (^)(UIView *_Nullable result))callback;

/**
 * @apidoc
 * @brief Asynchronously serializes this LynxElement tree to a JSON string.
 * @param callback Receives the JSON string, or nil if the element is unavailable.
 */
- (void)toJSONString:(nullable void (^)(NSString *_Nullable result))callback;

@end

NS_ASSUME_NONNULL_END

#if defined(LYNX_RESTORE_LYNX_ELEMENT_HEADER)
#pragma pop_macro("LynxElement")
#undef LYNX_RESTORE_LYNX_ELEMENT_HEADER
#endif

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXELEMENT_H_
