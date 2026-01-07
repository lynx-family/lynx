// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_COMPONENT_CONSTANTS_H_
#define CLAY_UI_COMPONENT_COMPONENT_CONSTANTS_H_

#include <memory>
#include <string>

namespace clay {

namespace attr_value {

extern const char* kTrue;
extern const char* kFalse;
extern const char* kYes;
extern const char* kNo;
extern const char* kModeScaleToFill;
extern const char* kModeAspectFit;
extern const char* kModeAspectFill;
extern const char* kModeCenter;
extern const char* kTextOverflowClip;
extern const char* kListTypeWaterFall;
extern const char* kListTypeFlow;
extern const char* kListTypeSingle;
extern const char* kListUpdateAnimationDefault;
extern const char* kImageTransitionFadeIn;

// for input
extern const char* kInputTypeText;
extern const char* kInputTypeNumber;
extern const char* kInputTypeDigit;
extern const char* kInputTypePassword;
extern const char* kInputTypeTel;
extern const char* kInputTypeEmail;
extern const char* kInputActionSend;
extern const char* kInputActionSearch;
extern const char* kInputActionGo;
extern const char* kInputActionDone;
extern const char* kInputActionNext;

// video - objectfit
extern const char* kVideoViewObjectFitContain;
extern const char* kVideoViewObjectFitCover;
extern const char* kVideoViewObjectFitFill;

// app region
extern const char* kAppRegionDrag;
extern const char* kAppRegionNoDrag;
}  // namespace attr_value

namespace event_attr {
extern const char* kEventDetail;

extern const char* kEventFocus;
extern const char* kEventBlur;
extern const char* kEventTap;
extern const char* kEventLongPress;
extern const char* kEventScroll;
extern const char* kEventScrollToUpper;
extern const char* kEventScrollToLower;
extern const char* kEventScrollToBounce;
extern const char* kEventScrollStart;
extern const char* kEventScrollEnd;
extern const char* kEventContentSizeChanged;
extern const char* kEventScrollStateChange;
extern const char* kEventNodeAppear;
extern const char* kEventNodeDisappear;
extern const char* kEventListLayoutComplete;
extern const char* kEventListStickyStart;
extern const char* kEventListStickyEnd;
extern const char* kEventListStickyTop;
extern const char* kEventListStickyBottom;
extern const char* kEventLayoutChange;
extern const char* kEventMouseLongPress;

extern const char* kEventEditInput;
extern const char* kEventEditSelectionChange;
extern const char* kEventEditConfirm;

extern const char* kEventImageLoadSuccess;
extern const char* kEventImageLoadError;
extern const char* kEventImageStartPlay;
extern const char* kEventImageCurrentLoopComplete;
extern const char* kEventImageFinalLoopComplete;

extern const char* kEventLottieStart;
extern const char* kEventLottieComplete;
extern const char* kEventLottieRepeat;
extern const char* kEventLottieCancel;
extern const char* kEventLottieReady;
extern const char* kEventLottieUpdate;
extern const char* kEventLottieFPS;
extern const char* kEventLottieFirstFrame;
extern const char* kEventLottieError;

extern const char* kEventIntersection;

// video
extern const char* kEventVideoPlay;
extern const char* kEventVideoPause;
extern const char* kEventVideoEnded;
extern const char* kEventVideoError;
extern const char* kEventVideoTimeUpdate;
extern const char* kEventVideoBufferingChange;
extern const char* kEventVideoSeek;
extern const char* kEventVideoCanPlay;
extern const char* kEventVideoLoadedMetadata;
extern const char* kEventVideoPlaying;
extern const char* kEventVideoWaiting;
extern const char* kEventVideoLoadStateChange;
extern const char* kEventVideoPlaybackStateChange;
extern const char* kEventVideoAbort;
extern const char* kEventVideoCanplayThrough;
extern const char* kEventVideoDurationChange;
extern const char* kEventVideoEmptied;
extern const char* kEventVideoLoadeddata;
extern const char* kEventVideoLoadStart;
extern const char* kEventVideoProgress;
extern const char* kEventVideoRateChange;
extern const char* kEventVideoSeeked;
extern const char* kEventVideoSeeking;
extern const char* kEventVideoStalled;
extern const char* kEventVideoSuspend;
extern const char* kEventVideoVolumeChange;
extern const char* kEventVideoFirstFrame;
extern const char* kEventVideoVideoInfos;
extern const char* kEventVideoCacheInfo;
extern const char* kEventVideoBuffering;
extern const char* kEventVideoStart;
extern const char* kEventVideoPrepare;
extern const char* kEventVideoReady;
extern const char* kEventVideoCompletion;
extern const char* kEventVideoUpdate;

// camera
extern const char* kEventCameraFrame;
extern const char* kEventCameraError;
extern const char* kEventCameraReady;
extern const char* kEventCameraStop;

extern const char* kEventAnimationStart;
extern const char* kEventAnimationEnd;
extern const char* kEventAnimationCancel;
extern const char* kEventAnimationIteration;
extern const char* kEventTransitionStart;
extern const char* kEventTransitionEnd;

extern const char* kEventChange;
extern const char* kEventWillChange;
extern const char* kEventTransition;
extern const char* kEventOffsetChange;

extern const char* kEventRefreshStartRefresh;
extern const char* kEventRefreshHeaderOffset;
extern const char* kEventRefreshStateChange;
// "startloadmore", "headerreleased", "footerreleased" will be deprecated
extern const char* kEventRefreshStartLoadmore;
extern const char* kEventRefreshHeaderReleased;
extern const char* kEventRefreshFooterReleased;

extern const char* kEventSliderChange;
extern const char* kEventSliderChanging;

extern const char* kEventLayout;
extern const char* kEventExit;
extern const char* kEventOffset;

extern const char* kEventFocusEscape;
extern const char* kEventFocusEnter;

extern const char* kEventMarkdownDrawStart;
extern const char* kEventMarkdownDrawEnd;
extern const char* kEventMarkdownOverflow;
extern const char* kEventMarkdownLink;
extern const char* kEventMarkdownImageTap;
extern const char* kEventMarkdownParseEnd;
extern const char* kEventMarkdownAnimationStep;
extern const char* kEventMarkdownTextClick;

// x-overlay-ng events
extern const char* kEventShowOverlay;
extern const char* kEventDismissOverlay;
extern const char* kEventRequestClose;
extern const char* kEventOverlayTouch;
}  // namespace event_attr

namespace color_value {
constexpr uint32_t kColorWhite = 0xffffffff;
constexpr uint32_t kColorGreen = 0xff00ff00;
}  // namespace color_value

namespace num_value {
constexpr int kFocusRingThickness = 10;
}

namespace app_region_value {
constexpr int kAppRegionDrag = 1;
constexpr int kAppRegionNoDrag = 2;
}  // namespace app_region_value

namespace video_res {
extern const char* kPlayBtn;
extern const char* kPauseBtn;
}  // namespace video_res

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_COMPONENT_CONSTANTS_H_
