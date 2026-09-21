// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/component_constants.h"

namespace clay {

namespace attr_value {
const char* kTrue = "true";
const char* kFalse = "false";
const char* kYes = "yes";
const char* kNo = "no";
const char* kModeScaleToFill = "scaleToFill";
const char* kModeAspectFit = "aspectFit";
const char* kModeAspectFill = "aspectFill";
const char* kModeCenter = "center";
const char* kListTypeWaterFall = "waterfall";
const char* kListTypeFlow = "flow";
const char* kListTypeSingle = "single";
const char* kListUpdateAnimationDefault = "default";
const char* kImageTransitionFadeIn = "fadeIn";

const char* kInputTypeNumber = "number";
const char* kInputTypeDigit = "digit";
const char* kInputTypePassword = "password";
const char* kInputTypeTel = "tel";
const char* kInputTypeEmail = "email";

const char* kInputActionSend = "send";
const char* kInputActionSearch = "search";
const char* kInputActionGo = "go";
const char* kInputActionDone = "done";
const char* kInputActionNext = "next";

const char* kVideoViewObjectFitContain = "contain";
const char* kVideoViewObjectFitCover = "cover";
const char* kVideoViewObjectFitFill = "fill";

const char* kAppRegionDrag = "drag";
const char* kAppRegionNoDrag = "no-drag";
}  // namespace attr_value

namespace event_attr {
const char* kEventFocus = "focus";
const char* kEventBlur = "blur";
const char* kEventTap = "tap";
const char* kEventLongPress = "longpress";
const char* kEventScroll = "scroll";
const char* kEventScrollToUpper = "scrolltoupper";
const char* kEventScrollToLower = "scrolltolower";
const char* kEventScrollToBounce = "scrolltobounce";
const char* kEventScrollStart = "scrollstart";
const char* kEventScrollEnd = "scrollend";
const char* kEventContentSizeChanged = "contentsizechanged";
const char* kEventScrollStateChange = "scrollstatechange";
const char* kEventNodeAppear = "nodeappear";
const char* kEventNodeDisappear = "nodedisappear";
const char* kEventLayoutComplete = "layoutcomplete";
const char* kEventStickyStart = "stickystart";
const char* kEventStickyEnd = "stickyend";
const char* kEventStickyTop = "stickytop";
const char* kEventStickyBottom = "stickybottom";
const char* kEventLayoutChange = "layoutchange";
const char* kEventMouseLongPress = "mouselongpress";

const char* kEventInput = "input";
const char* kEventSelectionChange = "selectionchange";
const char* kEventConfirm = "confirm";

const char* kEventLoad = "load";
const char* kEventError = "error";
const char* kEventStartPlay = "startplay";
const char* kEventCurrentLoopComplete = "currentloopcomplete";
const char* kEventFinalLoopComplete = "finalloopcomplete";
const char* kEventBgLoad = "bgload";
const char* kEventBgError = "bgerror";

const char* kEventStart = "start";
const char* kEventCompletion = "completion";
const char* kEventRepeat = "repeat";
const char* kEventCancel = "cancel";
const char* kEventReady = "ready";
const char* kEventUpdate = "update";
const char* kEventFps = "fps";
const char* kEventFirstFrame = "firstframe";

const char* kEventPlaybackStateChanged = "playbackstatechanged";
const char* kEventLoadingStateChanged = "loadingstatechanged";
const char* kEventSeek = "seek";
const char* kEventTimeUpdate = "timeupdate";
const char* kEventFinished = "finished";
const char* kEventRedirect = "redirect";
const char* kEventCanPlay = "canplay";

const char* kEventPlay = "play";
const char* kEventPause = "pause";
const char* kEventResume = "resume";
const char* kEventEnded = "ended";
const char* kEventStop = "stop";
const char* kEventBufferingChange = "bufferingchange";
const char* kEventPlaying = "playing";
const char* kEventWaiting = "waiting";
const char* kEventLoadStateChange = "loadstatechange";
const char* kEventLoadStateChanged = "loadstatechanged";
const char* kEventReload = "reload";
const char* kEventReadyToPlay = "readytoplay";
const char* kEventStalled = "stalled";
const char* kEventVideoInfos = "videoinfos";
const char* kEventCacheInfo = "cacheinfo";
const char* kEventBuffering = "buffering";
const char* kEventPrepare = "prepare";

const char* kEventFrame = "frame";

const char* kEventAnimationStart = "animationstart";
const char* kEventAnimationEnd = "animationend";
const char* kEventAnimationCancel = "animationcancel";
const char* kEventAnimationIteration = "animationiteration";
const char* kEventTransitionStart = "transitionstart";
const char* kEventTransitionEnd = "transitionend";

const char* kEventChange = "change";
const char* kEventWillChange = "willchange";
const char* kEventTransition = "transition";
const char* kEventOffsetChange = "offsetchange";

const char* kEventStartRefresh = "startrefresh";
const char* kEventRefreshStateChange = "refreshstatechange";
// "startloadmore", "headerreleased", "footerreleased" will be deprecated
const char* kEventStartLoadmore = "startloadmore";
const char* kEventHeaderReleased = "headerreleased";
const char* kEventFooterReleased = "footerreleased";

const char* kEventChanging = "changing";

const char* kEventLayout = "layout";
const char* kEventOffset = "offset";

const char* kEventFocusEscape = "focusescape";
const char* kEventFocusEnter = "focusenter";

const char* kEventDrawStart = "drawStart";
const char* kEventDrawEnd = "drawEnd";
const char* kEventOverflow = "overflow";
const char* kEventLink = "link";
const char* kEventImageTap = "imageTap";
const char* kEventParseEnd = "parseEnd";
const char* kEventAnimationStep = "animationStep";
const char* kEventTextClick = "textClick";

const char* kEventShowOverlay = "showoverlay";
const char* kEventDismissOverlay = "dismissoverlay";
const char* kEventRequestClose = "requestclose";
const char* kEventOverlayTouch = "overlaytouch";

}  // namespace event_attr

}  // namespace clay
