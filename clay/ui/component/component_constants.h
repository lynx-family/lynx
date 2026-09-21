// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_COMPONENT_CONSTANTS_H_
#define CLAY_UI_COMPONENT_COMPONENT_CONSTANTS_H_

#include <cstdint>

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
extern const char* kListTypeWaterFall;
extern const char* kListTypeFlow;
extern const char* kListTypeSingle;
extern const char* kListUpdateAnimationDefault;
extern const char* kImageTransitionFadeIn;

// for input
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
extern const char* kEventLayoutComplete;
extern const char* kEventStickyStart;
extern const char* kEventStickyEnd;
extern const char* kEventStickyTop;
extern const char* kEventStickyBottom;
extern const char* kEventLayoutChange;
extern const char* kEventMouseLongPress;

extern const char* kEventInput;
extern const char* kEventSelectionChange;
extern const char* kEventConfirm;

extern const char* kEventLoad;
extern const char* kEventError;
extern const char* kEventStartPlay;
extern const char* kEventCurrentLoopComplete;
extern const char* kEventFinalLoopComplete;
extern const char* kEventBgLoad;
extern const char* kEventBgError;

extern const char* kEventStart;
extern const char* kEventCompletion;
extern const char* kEventRepeat;
extern const char* kEventCancel;
extern const char* kEventReady;
extern const char* kEventUpdate;
extern const char* kEventFps;
extern const char* kEventFirstFrame;

// audio
extern const char* kEventPlaybackStateChanged;
extern const char* kEventLoadingStateChanged;
extern const char* kEventSeek;
extern const char* kEventTimeUpdate;
extern const char* kEventFinished;
extern const char* kEventRedirect;
extern const char* kEventCanPlay;

// video
extern const char* kEventPlay;
extern const char* kEventPause;
extern const char* kEventResume;
extern const char* kEventEnded;
extern const char* kEventStop;
extern const char* kEventBufferingChange;
extern const char* kEventPlaying;
extern const char* kEventWaiting;
extern const char* kEventLoadStateChange;
extern const char* kEventLoadStateChanged;
extern const char* kEventReload;
extern const char* kEventReadyToPlay;
extern const char* kEventStalled;
extern const char* kEventVideoInfos;
extern const char* kEventCacheInfo;
extern const char* kEventBuffering;
extern const char* kEventPrepare;

// camera
extern const char* kEventFrame;

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

extern const char* kEventStartRefresh;
extern const char* kEventRefreshStateChange;
// "startloadmore", "headerreleased", "footerreleased" will be deprecated
extern const char* kEventStartLoadmore;
extern const char* kEventHeaderReleased;
extern const char* kEventFooterReleased;

extern const char* kEventChanging;

extern const char* kEventLayout;
extern const char* kEventOffset;

extern const char* kEventFocusEscape;
extern const char* kEventFocusEnter;

extern const char* kEventDrawStart;
extern const char* kEventDrawEnd;
extern const char* kEventOverflow;
extern const char* kEventLink;
extern const char* kEventImageTap;
extern const char* kEventParseEnd;
extern const char* kEventAnimationStep;
extern const char* kEventTextClick;

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

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_COMPONENT_CONSTANTS_H_
