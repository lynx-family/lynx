// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSInteger, LynxGestureHandlerOption) {
  LynxGestureHandlerOptionPan = 1,
  LynxGestureHandlerOptionFling = 1 << 1,
  LynxGestureHandlerOptionDefault = 1 << 2,
  LynxGestureHandlerOptionTap = 1 << 3,
  LynxGestureHandlerOptionLongPress = 1 << 4,
  LynxGestureHandlerOptionRotation = 1 << 5,
  LynxGestureHandlerOptionPinch = 1 << 6,
  LynxGestureHandlerOptionAll = ~(1 << 7),

};

static NSString *const _Nonnull ON_TOUCHES_DOWN = @"onTouchesDown";
static NSString *const _Nonnull ON_TOUCHES_MOVE = @"onTouchesMove";
static NSString *const _Nonnull ON_TOUCHES_UP = @"onTouchesUp";
static NSString *const _Nonnull ON_TOUCHES_CANCEL = @"onTouchesCancel";
static NSString *const _Nonnull ON_BEGIN = @"onBegin";
static NSString *const _Nonnull ON_START = @"onStart";
static NSString *const _Nonnull ON_UPDATE = @"onUpdate";
static NSString *const _Nonnull ON_END = @"onEnd";

@protocol LynxGestureArenaMember;
@class LynxUIContext;
@class LynxGestureDetectorDarwin;
@class LynxTouchEvent;

@interface LynxBaseGestureHandler : NSObject

@property(nonatomic, strong, readonly) LynxGestureDetectorDarwin *gestureDetector;
@property(nonatomic, weak, readonly) id<LynxGestureArenaMember> gestureMember;
@property(nonatomic, assign, readonly) NSInteger sign;
@property(nonatomic, weak, readonly) LynxUIContext *context;

/**
 Convert gesture detectors to gesture handlers and returns a map of gesture handlers.
 @param sign              The LynxUI key.
 @param lynxContext       The Lynx context.
 @param member            The gesture arena member.
 @param gestureDetectors  A map of gesture detectors.
 @return                  A map of gesture handlers.
 */
+ (NSDictionary<NSNumber *, LynxBaseGestureHandler *> *)
    convertToGestureHandler:(NSInteger)sign
                    context:(LynxUIContext *)lynxContext
                     member:(id<LynxGestureArenaMember>)member
           gestureDetectors:
               (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors;

- (instancetype)initWithSign:(NSInteger)sign
                     context:(LynxUIContext *)lynxContext
                      member:(id<LynxGestureArenaMember>)member
                    detector:(LynxGestureDetectorDarwin *)detector;

/**
 Check if the gesture type is matched with the provided type.
 @param typeMask  The type of the gesture to check.
 @return {@code true} if the gesture type is matched, {@code false} otherwise.
*/
- (BOOL)isGestureTypeMatched:(NSUInteger)typeMask;

/**
 Check if the current gesture can be activated with the provided delta values.
 @param deltaPoint  The change in coordinate.
 @return {@code YES} if the gesture can be activated, {@code NO} otherwise.
 */
- (BOOL)canActiveWithCurrentGesture:(CGPoint)deltaPoint;

/**
 * Checks if the current gesture is called end.
 * @return if end or not
 */
- (BOOL)isCurrentGestureEnd;

/**
 Activate the gesture to activate status
 */
- (void)activate;

/**
 Reset the gesture handler to its active state.
 */
- (void)reset;

/**
 Fail the gesture handler, deactivating it and triggering the "onEnd" callback with the given
 coordinates and touch event.
 */
- (void)fail;

/**
  End the gesture handler, deactivating it and triggering the "onEnd" callback, end current
  gesture, make winner to null.
 */
- (void)end;

/**
 * make the gesture state to ignore
 */
- (void)ignore;

/**
 * make the gesture state to begin
 */
- (void)begin;

/**
 Begin the gesture with the specified type mask, coordinates, and touch event.
 @param typeMask The type mask of the gesture.
 @param point The coordinate at the beginning of the gesture.
 @param touches The UITouches associated with the event.
 @param event The UIEvent to dispatch.
 */
- (void)begin:(NSUInteger)typeMask
         point:(CGPoint)point
       touches:(NSSet<UITouch *> *_Nullable)touches
         event:(UIEvent *_Nullable)event
    touchEvent:(LynxTouchEvent *_Nullable)touchEvent;

/**
 Update the gesture with the given type mask, delta values, and touch event.
 If the gesture type does not match the provided type mask, the method returns.
 @param typeMask The type mask of the gesture.
 @param point The coordinate at the beginning of the gesture.
 @param touches The UITouches associated with the event.
 @param event The UIEvent to dispatch.
 */
- (void)update:(NSUInteger)typeMask
         point:(CGPoint)point
       touches:(NSSet<UITouch *> *_Nullable)touches
         event:(UIEvent *_Nullable)event
    touchEvent:(LynxTouchEvent *_Nullable)touchEvent;

/**
 End the gesture with the given type mask, end coordinates, and touch event.
 If the gesture type does not match the provided type mask, the method returns.
 @param typeMask The type mask of the gesture.
 @param point The coordinate at the beginning of the gesture.
 @param touchEvent The touchEvent of lynx to dispatch.
 */
- (void)end:(NSUInteger)typeMask
         point:(CGPoint)point
       touches:(NSSet<UITouch *> *_Nullable)touches
         event:(UIEvent *_Nullable)event
    touchEvent:(LynxTouchEvent *_Nullable)touchEvent;
/**
 Called when the gesture begins at the given coordinates.
 @param point The coordinate at the beginning of the gesture.
 @param touchEvent The touchEvent of lynx to dispatch.
 */
- (void)onBegin:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent;
/**
 Called when the gesture is updated with the given delta values.
 @param point The coordinate at the beginning of the gesture.
 @param touchEvent The touchEvent of lynx to dispatch.
 */
- (void)onUpdate:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent;

/**
 Called when the gesture is started with the given delta values.
 @param point The coordinate at the beginning of the gesture.
 @param touchEvent The touchEvent of lynx to dispatch.
 */
- (void)onStart:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent;

/**
 Called when the gesture ends at the given coordinates.
 @param point The coordinate at the beginning of the gesture.
 @param touchEvent The touchEvent of lynx to dispatch.
 */
- (void)onEnd:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent;

- (BOOL)onBeginEnabled;

- (BOOL)onUpdateEnabled;

- (BOOL)onStartEnabled;

- (BOOL)onEndEnabled;

- (void)handleUIEvent:(NSString *const)touchType
              touches:(NSSet<UITouch *> *)touches
                event:(UIEvent *_Nullable)event
           touchEvent:(LynxTouchEvent *_Nullable)touchEvent
           flingPoint:(CGPoint)flingPoint;

/**
    handle ui event and determine whether active or not
   */
- (void)onHandle:(NSString *const)touchType
         touches:(NSSet<UITouch *> *)touches
           event:(UIEvent *_Nullable)event
      touchEvent:(LynxTouchEvent *_Nullable)touchEvent
      flingPoint:(CGPoint)flingPoint;

- (bool)isEnd;

- (bool)isActive;

/**
 return the status of gesture
 */
- (int)status;

/**
 Handle the "onTouchesDown" event by sending the gesture event if enabled.
 @param touchEvent The touch event associated with the "onTouchesDown" event.
 */
- (void)onTouchesDown:(LynxTouchEvent *)touchEvent;
/**
 Handle the "onTouchesMove" event by sending the gesture event if enabled.
 @param touchEvent The touch event associated with the "onTouchesMove" event.
 */
- (void)onTouchesMove:(LynxTouchEvent *)touchEvent;

/**
 Handle the "onTouchesUp" event by sending the gesture event if enabled.
 @param touchEvent The touch event associated with the "onTouchesUp" event.
 */
- (void)onTouchesUp:(LynxTouchEvent *)touchEvent;

/**
 Handle the "onTouchesCancel" event by sending the gesture event if enabled.
 @param touchEvent The touch event associated with the "onTouchesCancel" event.
 */
- (void)onTouchesCancel:(LynxTouchEvent *)touchEvent;

/**
 Send a gesture event with the specified event name and parameters.
 @param eventName   The name of the gesture event.
 @param eventParams The parameters associated with the gesture event.
 */
- (void)sendGestureEvent:(NSString *)eventName params:(NSDictionary *)eventParams;

/**
 handle gesture config in specification gesture detector
 @param config The custom config map of gesture
 */
- (void)handleConfigMap:(nullable NSMutableDictionary *)config;

/**
 Retrieve the event parameters from the given touch event.
 @param touchEvent The touch event to extract the parameters from.
 @return A NSDictionary containing the extracted event parameters.
 */
- (NSDictionary *)eventParamsFromTouchEvent:(LynxTouchEvent *)touchEvent;

@end

NS_ASSUME_NONNULL_END
