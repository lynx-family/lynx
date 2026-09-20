// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxKeyframeAnimator.h>
#import <Lynx/LynxKeyframeManager.h>
#import <Lynx/LynxTransformRaw.h>
#import <Lynx/LynxTransitionAnimationManager.h>
#import <Lynx/LynxUI.h>

NS_ASSUME_NONNULL_BEGIN

// Internal typed construction; raw CSS arrays continue to use initWithArray:.
@interface LynxTransformRaw (PlatformAnimation)
- (instancetype)initWithTranslationX:(LynxPlatformLength*)x
                                   y:(LynxPlatformLength*)y
                                   z:(LynxPlatformLength*)z;
@end

typedef LynxKeyframeParsedData* _Nullable (^LynxTypedKeyframesProvider)(LynxUI* ui);

// The blocks bridge the logical animation without exposing C++ to .m files.
@interface LynxPlatformAnimationPlayback : NSObject
@property(nonatomic, copy) NSTimeInterval (^currentTime)(void);
@property(nonatomic, copy) BOOL (^isCurrentExecutor)(void);
@property(nonatomic, copy) BOOL (^claimEvent)(NSString* event);
@end

@interface LynxKeyframeAnimator (TypedKeyframes)
@property(nonatomic, strong, nullable) LynxPlatformAnimationPlayback* platformPlayback;
- (void)detachPlatformAnimation;
- (void)applyAnimationInfo:(LynxAnimationInfo*)info
         keyframesProvider:(LynxTypedKeyframesProvider)provider
            reuseKeyframes:(BOOL)reuseKeyframes
                generation:(uint32_t)generation;
- (void)reapply;
+ (nullable LynxKeyframeParsedData*)buildParsedTransformKeyframes:
                                        (NSArray<NSArray<LynxTransformRaw*>*>*)keyframes
                                                            times:(NSArray<NSNumber*>*)times
                                                               ui:(LynxUI*)ui;
@end

@interface LynxKeyframeManager (TypedKeyframes)
- (void)applyAnimationInfo:(nullable LynxAnimationInfo*)info
         keyframesProvider:(nullable LynxTypedKeyframesProvider)provider
            reuseKeyframes:(BOOL)reuseKeyframes
               animationID:(uint64_t)animationID
                generation:(uint32_t)generation
                    cancel:(BOOL)cancel
                    detach:(BOOL)detach
                  playback:(nullable LynxPlatformAnimationPlayback*)playback;
@end

@interface LynxUI (TypedTransitions)
@property(nonatomic, strong, nullable) LynxTransitionAnimationManager* transitionAnimationManager;
- (void)prepareTransitionAnimationManager;
@end

@interface LynxTransitionAnimationManager (TypedTransitions)
- (void)applyPlatformOpacityTransition:(nullable LynxAnimationInfo*)info
                           fromOpacity:(CGFloat)fromOpacity
                             toOpacity:(CGFloat)toOpacity
                           animationID:(uint64_t)animationID
                            generation:(uint32_t)generation
                                cancel:(BOOL)cancel;
- (void)applyPlatformTransformTransition:(nullable LynxAnimationInfo*)info
                        fromTransformRaw:(nullable NSArray<LynxTransformRaw*>*)fromTransformRaw
                          toTransformRaw:(nullable NSArray<LynxTransformRaw*>*)toTransformRaw
                             animationID:(uint64_t)animationID
                              generation:(uint32_t)generation
                                  cancel:(BOOL)cancel;
@end

NS_ASSUME_NONNULL_END
