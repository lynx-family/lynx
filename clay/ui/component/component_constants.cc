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
const char* kTextOverflowClip = "clip";
const char* kListTypeWaterFall = "waterfall";
const char* kListTypeFlow = "flow";
const char* kListTypeSingle = "single";
const char* kListUpdateAnimationDefault = "default";
const char* kImageTransitionFadeIn = "fadeIn";

const char* kInputTypeText = "text";
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
const char* kEventDetail = "detail";
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
const char* kEventListLayoutComplete = "layoutcomplete";
const char* kEventListStickyStart = "stickystart";
const char* kEventListStickyEnd = "stickyend";
const char* kEventListStickyTop = "stickytop";
const char* kEventListStickyBottom = "stickybottom";
const char* kEventLayoutChange = "layoutchange";
const char* kEventMouseLongPress = "mouselongpress";

const char* kEventEditInput = "input";
const char* kEventEditSelectionChange = "selectionchange";
const char* kEventEditConfirm = "confirm";

const char* kEventImageLoadSuccess = "load";
const char* kEventImageLoadError = "error";
const char* kEventImageStartPlay = "startplay";
const char* kEventImageCurrentLoopComplete = "currentloopcomplete";
const char* kEventImageFinalLoopComplete = "finalloopcomplete";

const char* kEventLottieStart = "start";
const char* kEventLottieComplete = "completion";
const char* kEventLottieRepeat = "repeat";
const char* kEventLottieCancel = "cancel";
const char* kEventLottieReady = "ready";
const char* kEventLottieUpdate = "update";
const char* kEventLottieFPS = "fps";
const char* kEventLottieFirstFrame = "firstframe";
const char* kEventLottieError = "error";

const char* kEventIntersection = "intersection";

const char* kEventVideoPlay = "play";
const char* kEventVideoPause = "pause";
const char* kEventVideoEnded = "ended";
const char* kEventVideoError = "error";
const char* kEventVideoTimeUpdate = "timeupdate";
const char* kEventVideoBufferingChange = "bufferingchange";
const char* kEventVideoSeek = "seek";
const char* kEventVideoCanPlay = "canplay";
const char* kEventVideoLoadedMetadata = "loadedmetadata";
const char* kEventVideoPlaying = "playing";
const char* kEventVideoWaiting = "waiting";
const char* kEventVideoLoadStateChange = "loadstatechange";
const char* kEventVideoPlaybackStateChange = "playbackstatechange";
const char* kEventVideoAbort = "abort";
const char* kEventVideoCanplayThrough = "canplaythrough";
const char* kEventVideoDurationChange = "durationchange";
const char* kEventVideoEmptied = "emptied";
const char* kEventVideoLoadeddata = "loadeddata";
const char* kEventVideoLoadStart = "loadstart";
const char* kEventVideoProgress = "progress";
const char* kEventVideoRateChange = "ratechange";
const char* kEventVideoSeeked = "seeked";
const char* kEventVideoSeeking = "seeking";
const char* kEventVideoStalled = "stalled";
const char* kEventVideoSuspend = "suspend";
const char* kEventVideoVolumeChange = "volumechange";
const char* kEventVideoFirstFrame = "firstframe";
const char* kEventVideoVideoInfos = "videoinfos";
const char* kEventVideoCacheInfo = "cacheinfo";
const char* kEventVideoBuffering = "buffering";
const char* kEventVideoStart = "start";
const char* kEventVideoPrepare = "prepare";
const char* kEventVideoReady = "ready";
const char* kEventVideoCompletion = "completion";
const char* kEventVideoUpdate = "update";

const char* kEventCameraFrame = "frame";
const char* kEventCameraError = "error";
const char* kEventCameraReady = "ready";
const char* kEventCameraStop = "stop";

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

const char* kEventRefreshStartRefresh = "startrefresh";
const char* kEventRefreshHeaderOffset = "headeroffset";
const char* kEventRefreshStateChange = "refreshstatechange";
// "startloadmore", "headerreleased", "footerreleased" will be deprecated
const char* kEventRefreshStartLoadmore = "startloadmore";
const char* kEventRefreshHeaderReleased = "headerreleased";
const char* kEventRefreshFooterReleased = "footerreleased";

const char* kEventSliderChange = "change";
const char* kEventSliderChanging = "changing";

const char* kEventLayout = "layout";
const char* kEventExit = "exit";
const char* kEventOffset = "offset";

const char* kEventFocusEscape = "focusescape";
const char* kEventFocusEnter = "focusenter";

const char* kEventMarkdownDrawStart = "drawStart";
const char* kEventMarkdownDrawEnd = "drawEnd";
const char* kEventMarkdownOverflow = "overflow";
const char* kEventMarkdownLink = "link";

const char* kEventShowOverlay = "showoverlay";
const char* kEventDismissOverlay = "dismissoverlay";
const char* kEventRequestClose = "requestclose";
const char* kEventOverlayTouch = "overlaytouch";

}  // namespace event_attr

namespace video_res {
const char* kPlayBtn =
    "data:image/"
    "png;base64,"
    "iVBORw0KGgoAAAANSUhEUgAAAE4AAABWCAYAAABhL6DrAAAACXBIWXMAABCcAAAQnAEmzTo0AA"
    "AAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAVASURBVHgB7ZzNSyN3GMefmOgodivuBmts"
    "xYNuC6JQYunqQbqn1oN72ku92FP/"
    "JI9ePTQgXRDxUCGlLbQGjboh664plFobKUZjopL39HlwfrNjNpmdTOZ9fh8IGRNB88nzfH8vkw"
    "wARxM+Nb9Ur9cFvBvD2wPxoQzdfD5fHjxKQOlJFDZSrVZn8DDU4vk03r1GgW/"
    "AYzStOKqwSqUyFwgEPgV15IvF4l5vb+9r8AjviENp1I5f4+0RtI9nBDYT9xy0SZND2bfr5ha+"
    "J65QKHwmCMJXoB+"
    "UgVE3DiL3xGG1LcHbkVNPqPJ23SRQGlVzuVwQjJFG0CATwjcmgfISeFwHhyNVHL6oabybA+"
    "NxRf51sQOcfhhVbY3Q33mKb9RTcQR3JPKKewYtJroG48j8YxWnaullEJR/"
    "izSig4NgwnxYcd+CcYODWqjqfsbq+xdsjpRxuCa1suoY9MYtOiH/"
    "usCeUPsulUqlL46PjwWwIVKr4qj6nd/v7wH7Ycv1rzzjvgd7Q/"
    "m3YZfR166t2gzKvCW75J+TxDEo/55T/"
    "oGFBMCZ9HR3d4ex8h6DRcs3J1acHLZ8e2Z2+"
    "zpdHIOWiqbmn1NbtRXy7auXYCBuqTg5VHFztCmLN7Unm9rGjeIY0vZVJpP5EHTGSRPgjsCVUQJ"
    "Pd77UawLt5oq7B0qbgrsNBF3a1zPiRFj7Uv6NQAd4TRyj4+"
    "0rr4pjSNtX0CZum8dpQsvyzesVJ4fl3zdq2peLexf6HOB725eLa4HYvkutqo+"
    "LU4Ztns40PsHFqWNGbF3pTCAXpxJqXZQnVR4X1wYk7/"
    "b29mM65uLapK+"
    "vL4x3Pi6ufUK5XO4RF6cBQRBGuDgNVKvVB1ycBnCQ6OHiNFAul0tcnAZqtdo1F6eBs7OzNBfXJ"
    "jgwXE9MTJxzcW2SzWbjeFfn4toAJ777wWCQPuDIxamFpA0MDOziYY1+"
    "5uJUgO25I0qrsse4OAVoIEgkEi8GBwcPQSaN4OJakMlkDjY3N3+cnp7+"
    "D8T2lMNPDzZQLBbPotHorwsLC1loIozBxYlQW6bT6Z3R0dG/"
    "4K4tFb8a6vlWRWElasu1tbUXKO1PfKgCKr5P6+mKu7m5OYnH47/"
    "Pz89fgUJbNsOT4ijHksnkQTgc/gcaRku1eEoctWU+n9/"
    "H6QV9vf29OaaEZzJOzLEISjsAlTmmhOsrjtoyFov9oiXHlHCtOJpeHB4e/"
    "tZJjinhOnGUYxcXFwdDQ0P0PYeOckwJV2Xc5eXlK8oxlKZLjinhioqjHEulUvtTU1PUlrrlmBK"
    "OFifLsVMwsC2b4chWlS+"
    "TUNrfYHBbNsNxFVcoFFKRSOSP5eXlW7DwGk2OESdbJp2ASTmmhCSOyt+OV4Gg/"
    "+v09DQ2NjZGJ0lMzTEl5BlXAhshy7EISkuCBTmmhFRxWG22+"
    "aeMWibpia0yDqvs4ujoaAfnY2x6YVvkGXeDVWfJ9Tz03O4xCynj6vV6ESwAhSX13O4xC1Zx9XK"
    "5fB0ImNe5as8m2RX54GDKNYsalkkVcCiSONyKSQ8PD4NRUI5ls9lXwWCQfZTA0VdmlTIuFApl6"
    "MWBAbDtHpQWAwflmBLyUKvhOjDe39//"
    "BHTCiu0es2i8vGNXpVJZxLzrqGet3O4xi8Ztpdrq6upP9MJBA2KO7eDo/"
    "INV2z1m4W98YGNjo4ytlRofH+9HAQ9BBSQMcyyxvr4enZ2dtf2sXw+UrsTq39vb+"
    "2RycvJzQRA+avYLlGG4ED/Z3t5+Y/X+mN0gsYGtra2HuBf2+Pz8/"
    "MnV1dWX6XR6cmVl5QO4G1zscBlc0/kffo+5rAUmuPwAAAAASUVORK5CYII=";
const char* kPauseBtn =
    "data:image/"
    "png;base64,"
    "iVBORw0KGgoAAAANSUhEUgAAAEYAAABICAYAAABLJIP0AAAACXBIWXMAABCcAAAQnAEmzTo0AA"
    "AAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAHuSURBVHgB7dzNTsJAFAXgW0pwBZu68R10"
    "QXRlwkvoc2oiLnTpjhW4QQlPIC5s/"
    "EsITes4g60hxLMw3JmkzfmSRpCknRzunWGSggj9KaoeGGOSoihO4zg+"
    "ED1P9riLouhDlIQaZ1RerGsvdm4v1hF9mT0uNMJx47R/zuyxJ/"
    "qyNE0vkyR5d09a5T+PPYXidPI8PxIF9jwn4icUp9Pr9Q6rJ1UwXfHIVsu+"
    "bLTtDufxOs52u51Uj1tSHzsH+5/"
    "rhAzGSI0ECcbOXy6UUO+4iiDB2BXPhcKKaYKQwbCVALZSEzAYIORyXSshl+"
    "taYSsBDAZgMACDARgMwGAA7pUA7pUAthLAVgJC7pXYStu4V2oQBgMwGICrEsAPeABbCWAwAIMB"
    "OPkCnHwBVgzAigE4+"
    "QIMBuAcA7BiAAYDcFUCOMcArBiAcwzAO8MB3hkOsJUABgMwGIDBAAwG4CdfgBUDcEsAsGIAfvs"
    "E4B1VADeRADeRACdfgK0EsJUAthKwDma5XKbikTFG46eYTJZlL+"
    "JRnuefUlb2Opj5fP5gyz0TD9x5p9PpvSgYj8ePoca5Dqbf77+NRqMr++"
    "JCFK1Wq8VkMrl15xcFg8HgdTgcXtvKeRZFbpyz2exmc5zbK0UsuvPOV3loTrxuzC3RHWchP+"
    "P89Q1luasMCocSlwAAAABJRU5ErkJggg==";
}  // namespace video_res

}  // namespace clay
