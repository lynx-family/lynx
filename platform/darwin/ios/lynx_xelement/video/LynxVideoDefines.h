// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxVideoObjectFit) {
  LynxVideoObjectFitContain = 0,
  LynxVideoObjectFitCover,
  LynxVideoObjectFitFill,
};

typedef NS_ENUM(NSInteger, LynxVideoErrorCode) {
  LynxVideoErrorInvalidSource = -1,
  LynxVideoErrorOperation = -2,
};

@protocol LynxVideoPlayable;

@protocol LynxVideoPlayableDelegate <NSObject>

- (void)playableDidLoadFirstFrame:(id<LynxVideoPlayable>)playable duration:(NSTimeInterval)duration;
- (void)playableDidStartPlaying:(id<LynxVideoPlayable>)playable;
- (void)playableDidPause:(id<LynxVideoPlayable>)playable;
- (void)playableDidStop:(id<LynxVideoPlayable>)playable;
- (void)playableDidEnd:(id<LynxVideoPlayable>)playable;
- (void)playableDidLoop:(id<LynxVideoPlayable>)playable;
- (void)playable:(id<LynxVideoPlayable>)playable
    didUpdateTime:(NSTimeInterval)current
         duration:(NSTimeInterval)duration;
- (void)playable:(id<LynxVideoPlayable>)playable didUpdateBuffering:(NSTimeInterval)buffering;
- (void)playable:(id<LynxVideoPlayable>)playable
    didFailWithCode:(NSInteger)errorCode
            message:(NSString *)errorMsg;

@end

@protocol LynxVideoPlayable <NSObject>

@property(nonatomic, weak, nullable) id<LynxVideoPlayableDelegate> delegate;
@property(nonatomic, copy, nullable) NSString *src;
@property(nonatomic, assign) BOOL loop;
@property(nonatomic, assign) CGFloat volume;
@property(nonatomic, assign) BOOL muted;
@property(nonatomic, assign) CGFloat speed;
@property(nonatomic, assign) LynxVideoObjectFit objectFit;
@property(nonatomic, assign) NSTimeInterval timeUpdateInterval;
@property(nonatomic, assign, readonly) NSTimeInterval duration;
@property(nonatomic, assign, readonly, getter=isPlaying) BOOL playing;

- (void)play;
- (void)pause;
- (void)stop;
- (void)seekTo:(NSTimeInterval)position
    completion:(void (^)(BOOL success, NSString *_Nullable errorMsg))completion;

@end

NS_ASSUME_NONNULL_END
