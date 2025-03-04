// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// if you don't need instance id, just use kUnknownInstanceId.
FOUNDATION_EXPORT int32_t const kUnknownInstanceId;
// Event name of lynx erorr.
FOUNDATION_EXPORT NSString *const kLynxSDKErrorEvent;

@protocol LynxEventReportObserverProtocol <NSObject>

/// Callback method to listen and handle Lynx reporting events.
/// Called in report thread.
/// - Parameters:
///   - eventName: name of event.
///   - instanceId: the unique id of template instance.
///   - props: all properties of event. Cannot be modified.
///   - extraData: Parameters set by method `LynxView.putParamsForReportingEvents()` of report
///   event. Cannot be modified.
- (void)onReportEvent:(NSString *)eventName
           instanceId:(NSInteger)instanceId
                props:(NSDictionary *_Nullable)props
            extraData:(NSDictionary *_Nullable)extraData;

@end

/// Report custom event.
/// All public methods can call on any thread.
@interface LynxEventReporter : NSObject

/// Report event.
/// Can call on any thread.
/// @param eventName name of event.
/// @param instanceId id of template instance.
/// @param props properties of the event.
+ (void)onEvent:(nonnull NSString *)eventName
     instanceId:(int32_t)instanceId
          props:(nullable NSDictionary<NSString *, NSObject *> *)props;

/// Report event.
/// Can call on any thread.
/// @param eventName name of event.
/// @param instanceId id of template instance.
/// @param propsBuilder build event props on report thread.
+ (void)onEvent:(nonnull NSString *)eventName
      instanceId:(int32_t)instanceId
    propsBuilder:(NSDictionary<NSString *, NSObject *> * (^)(void))propsBuilder;

/// Update generic info of template instance.
/// Can call on any thread.
/// @param value value of generic info.
/// @param key key of generic info.
/// @param instanceId id of template instance.
+ (void)updateGenericInfo:(nonnull NSObject *)value
                      key:(nonnull NSString *)key
               instanceId:(int32_t)instanceId;

/// Remove generic info of template instance by id.
/// Can call on any thread.
/// @param instanceId  id of template instance.
+ (void)removeGenericInfo:(int32_t)instanceId;
/// Put extra parameters for reporting events, overriding old values if the parameters already
/// exist.
/// @param params common parameters for report events.
/// @param instanceId id of template instance.
+ (void)putExtraParams:(NSDictionary<NSString *, NSObject *> *)params
         forInstanceId:(int32_t)instanceId;

/// Move extra parameters for reporting events from originInstance to targetInstanceId.
/// @param originInstanceId id of template instance.
/// @param targetInstanceId id of template instance.
+ (void)moveExtraParams:(int32_t)originInstanceId toInstanceId:(int32_t)targetInstanceId;

/// Clear cache directly mapped by instance ID.
/// @param instanceId The instance ID.
+ (void)clearCacheForInstanceId:(int32_t)instanceId;

/// Add event report observer.
+ (void)addEventReportObserver:(id<LynxEventReportObserverProtocol>)observer;

/// Remove event report observer.
+ (void)removeEventReportObserver:(id<LynxEventReportObserverProtocol>)observer;

@end

NS_ASSUME_NONNULL_END
