function __IsArray(a) {
  if (a) {
    if (a.push === [].push) {
      return true;
    }
  }
  return false;
}
let getCssPropertyIDObj;
let $selectorStyleIndexMap;
let $dataProcessorMap = {};
function registerDataProcessor(func, alias) {
  if (typeof alias === "string") {
    $dataProcessorMap[alias] = func;
  } else {
    $dataProcessorMap["default"] = func;
  }
}
function processData(data, processorName) {
  let func = $dataProcessorMap["default"];
  if (processorName) {
    func = $dataProcessorMap[processorName];
  }
  if (typeof func === "function") {
    data = func(data);
  }
  return data;
}
function __GetElementByUniqueID(a) {}
let lynx = {};
let __globalProps = undefined;
let SystemInfo = undefined;
let __pow = Math.pow;
function getGlobalProps() {
  let globalProps;
  if (typeof lynx !== "undefined") {
    globalProps = lynx.__globalProps;
  } else {
    globalProps = __globalProps;
  }
  return globalProps || {};
}
let getGlobalProps_default = getGlobalProps;
function isAndroid() {
  let _getGlobalProps_defau = getGlobalProps_default(),
      os = _getGlobalProps_defau.os;
  return os === "android" || os === "Android";
}
function isIOS() {
  let _getGlobalProps_defau2 = getGlobalProps_default(),
      os = _getGlobalProps_defau2.os;
  return os === "ios" || os === "iOS" || os === "iphone";
}
function version2Number(versionStr) {
  if (!versionStr) {
    return 0;
  }
  if (!isNaN(parseInt(versionStr, 10))) {
    return parseInt(versionStr, 10);
  }
  function str2Num(version) {
    if (isNaN(parseInt(version, 10))) {
      return 0;
    }
    return parseInt(version, 10);
  }
  let mainVersionString = versionStr.replace(/(\d+)\.(\d+)\.(\d+)(.+)/, "$1.$2.$3");
  if (mainVersionString) {
    let _mainVersionString$sp = mainVersionString.split("."),
        mainVersionStr = _mainVersionString$sp[0],
        midVersionStr = _mainVersionString$sp[1],
        lastVersionStr = _mainVersionString$sp[2];
    let mainVersion = str2Num(mainVersionStr);
    let midVersion = str2Num(midVersionStr);
    let lastVersion = str2Num(lastVersionStr);
    let version = mainVersion * __pow(10, 4) + midVersion * __pow(10, 2) + lastVersion;
    return version;
  }
  return 0;
}
let _appVersion = 0;
function getAppVersion() {
  if (!_appVersion) {
    let props = getGlobalProps_default();
    let version = props.appVersion;
    _appVersion = version2Number(version);
  }
  return _appVersion;
}
function getTestNum() {
  return 0;
}
function isZyTest() {
  let test_num = getTestNum();
  return test_num === 1000;
}
function isLegou() {
  let test_num = getTestNum();
  return test_num === 2000;
}
function isDylite() {
  let test_num = getTestNum();
  return test_num === 3000;
}
function getScaleFontClassName(fontSizePref) {
  if (fontSizePref === 1) {
    return "mall-font-mini";
  } else if (fontSizePref === 2) {
    return "mall-font-large";
  } else if (fontSizePref === 3 || fontSizePref === 4) {
    return "mall-font-larger";
  }
  return "mall-font-medium";
}
function normalizeZYFontSizePref(fontSizePref) {
  if (fontSizePref === 1) {
    return 0;
  } else if (fontSizePref === 2 || fontSizePref === 3 || fontSizePref === 4) {
    return 2;
  }
  return 0;
}
function getFontScaleParams() {
  let _a;
  let _ref = ((_a = getGlobalProps_default()) == null ? undefined : _a.ec_extra) || {},
      fontSizePref = _ref.font_size_pref,
      bigFontEnabled = _ref.big_font_enabled;
  let finalFontSizePref = fontSizePref;
  if (isZyTest() || isDylite() || isLegou()) {
    finalFontSizePref = normalizeZYFontSizePref(fontSizePref != null ? fontSizePref : 0);
  }
  return {
    fontSizePref: finalFontSizePref != null ? finalFontSizePref : 0,
    bigFontEnabled: bigFontEnabled
  };
}
function getClassName(className) {
  let _getFontScaleParams = getFontScaleParams(),
      fontSizePref = _getFontScaleParams.fontSizePref,
      bigFontEnabled = _getFontScaleParams.bigFontEnabled;
  if (!bigFontEnabled) {
    return className;
  }
  return className + " " + getScaleFontClassName(fontSizePref != null ? fontSizePref : 0);
}
function getFontScale() {
  let _getFontScaleParams2 = getFontScaleParams(),
      fontSizePref = _getFontScaleParams2.fontSizePref,
      bigFontEnabled = _getFontScaleParams2.bigFontEnabled;
  if (!bigFontEnabled) {
    return 1;
  }
  if (fontSizePref === 1) {
    return 0.9;
  } else if (fontSizePref === 2) {
    return 1.15;
  } else if (fontSizePref === 3 || fontSizePref === 4) {
    return 1.3;
  }
  return 1;
}
function showBorder(showBorderTime) {
  if (!showBorderTime) {
    return false;
  }
  if (showBorderTime && Date.now() - showBorderTime >= 4e3) {
    return false;
  }
  return true;
}
function getVideoAnimationImage(video_animation) {
  return {
    image_2: video_animation == null ? undefined : video_animation.video_cover_url
  };
}
function getSilenceImageStyle(show) {
  if (show) {
    return "opacity: 1;";
  }
  return "opacity: 0;";
}
function getVideoAnimationStyle(loading) {
  if (loading) {
    return "background-color: rgba(250, 250, 250, 1);";
  }
  return "background-color: rgba(250, 250, 250, 0);";
}
function getGlobalProps2() {
  let globalProps;
  if (typeof lynx !== "undefined") {
    globalProps = lynx.__globalProps;
  } else {
    globalProps = __globalProps;
  }
  return globalProps || {};
}
function isAndroid2() {
  let _getGlobalProps = getGlobalProps2(),
      os = _getGlobalProps.os;
  return os === "android" || os === "Android";
}
function version2Number2(versionStr) {
  if (!versionStr) {
    return 0;
  }
  if (!isNaN(parseInt(versionStr, 10))) {
    return parseInt(versionStr, 10);
  }
  function str2Num(version) {
    if (isNaN(parseInt(version, 10))) {
      return 0;
    }
    return parseInt(version, 10);
  }
  let mainVersionString = versionStr.replace(/(\d+)\.(\d+)\.(\d+)(.+)/, "$1.$2.$3");
  if (mainVersionString) {
    let _mainVersionString$sp2 = mainVersionString.split("."),
        mainVersionStr = _mainVersionString$sp2[0],
        midVersionStr = _mainVersionString$sp2[1],
        lastVersionStr = _mainVersionString$sp2[2];
    let mainVersion = str2Num(mainVersionStr);
    let midVersion = str2Num(midVersionStr);
    let lastVersion = str2Num(lastVersionStr);
    let version = mainVersion * __pow(10, 4) + midVersion * __pow(10, 2) + lastVersion;
    return version;
  }
  return 0;
}
function getAppVersion2() {
  let props = getGlobalProps2();
  let version = props.appVersion;
  return version2Number2(version);
}
function lowerThanVersionCheck(version) {
  let currentVersionNum = getAppVersion2();
  return currentVersionNum < version;
}
function getMaskPosition(aspectRatio) {
  let _lepusTmp = __globalProps;
  let screenWidth = _lepusTmp.screenWidth;
  let col = 2;
  let ratio = parseInt(aspectRatio, 10);
  if (ratio === undefined || ratio === null) {
    ratio = 1;
  }
  let maskRatio = ratio === 1 ? 0.63 : 0.52;
  let width = (screenWidth - 10 - 10 - (col - 1) * 9) / col;
  return "width: " + width + "px;height: " + width / maskRatio + "px";
}
function getSKUBubbleStyle(item) {
  let _a, _b;
  let left = ((_a = item == null ? undefined : item.position) == null ? undefined : _a[0]) || 0;
  let top = ((_b = item == null ? undefined : item.position) == null ? undefined : _b[1]) || 0;
  if (top < 0.13) {
    top = 0.13;
  } else if (top > 0.87) {
    top = 0.87;
  }
  return "left: calc(" + left * 100 + "% - 6rpx); top: calc(" + top * 100 + "% - 6rpx);";
}
function isLynx() {
  return typeof lynx !== "undefined" && typeof globalThis.document === "undefined";
}
function getPageOneItems(items, limit) {
  return items.slice(0, limit);
}
function getPageTwoItems(items, limit) {
  return items.slice(limit);
}
function getPageTwoItemClass(items, limit, index) {
  let name = "gyl_feedback-neg--item gyl_feedback-neg-pagetwo-item";
  if (index === items.length - limit - 1) {
    name = name + " gyl_feedback-neg--item-last";
  }
  return name;
}
function hasMore(items, limit) {
  return (items == null ? undefined : items.length) > limit;
}
function getPageOneItemClass(items, limit, index) {
  let name = "gyl_feedback-neg--item";
  if (index + 1 === limit && !hasMore(items, limit)) {
    name = name + " gyl_feedback-neg--item-last";
  }
  return name;
}
function getMoreText(items, limit) {
  let moreText = items.slice(limit, limit + 2).map(function (item, index) {
    let text = "";
    let abbr = "";
    if (item) {
      text = item.text || "";
      abbr = item.abbr || "";
    }
    if (index === 0) {
      return text;
    }
    return abbr;
  }).join(", ");
  return moreText;
}
let FeedbackClickArea = {
  FeedbackItem: "feedback_item",
  CloseButton: "close_button",
  MoreButton: "more_button",
  SimilarButton: "similar_button",
  BackButton: "back_button",
  Mask: "mask"
};
function getFeedbackMaskName() {
  return FeedbackClickArea.Mask;
}
function getFeedbackCloseButtonName() {
  return FeedbackClickArea.CloseButton;
}
function getFeedbackMoreButtonName() {
  return FeedbackClickArea.MoreButton;
}
function getFeedbackSimilarButtonName() {
  return FeedbackClickArea.SimilarButton;
}
function getFeedbackBackButtonName() {
  return FeedbackClickArea.BackButton;
}
function getFeedbackItemName() {
  return FeedbackClickArea.FeedbackItem;
}
let A11yRoleMap = {
  Button: "button",
  Image: "img",
  Text: "article"
};
let A11yRoleTextMap = {
  button: "button",
  img: "image",
  article: ""
};
function getA11yLabel(label, role) {
  if (typeof globalThis.window !== "undefined") {
    return label;
  }
  if (role && A11yRoleTextMap[role]) {
    return label + ", " + A11yRoleTextMap[role];
  }
  return label;
}
function getA11yTraits(role) {
  if (typeof globalThis.window !== "undefined") {
    if (role && A11yRoleMap[role]) {
      return A11yRoleMap[role];
    }
    return "button";
  }
  return "none";
}
function getButtonA11yTraits() {
  return getA11yTraits(A11yRoleMap.Button);
}
function getButtonA11yLabel(label) {
  return getA11yLabel(label, A11yRoleMap.Button);
}
function getVideoEngineStyle() {
  if (isAndroid2()) {
    return "bottom: -1.5rpx; height: unset;";
  }
  return "";
}
function canUseMediaWrapper() {
  if (isIOS()) {
    return true;
  }
  return getAppVersion() >= 300700;
}
function getGlobalProps3() {
  if (typeof lynx !== "undefined" && lynx.__globalProps) {
    return lynx.__globalProps;
  }
  if (typeof __globalProps !== "undefined") {
    return __globalProps || {};
  }
  return {};
}
function isAndroid3() {
  let _getGlobalProps2 = getGlobalProps3(),
      os = _getGlobalProps2.os;
  return os === "android" || os === "Android";
}
function isIOS3() {
  let _getGlobalProps3 = getGlobalProps3(),
      os = _getGlobalProps3.os;
  return os === "ios" || os === "iOS";
}
function isLynx2() {
  return typeof lynx !== "undefined" && typeof globalThis.document === "undefined";
}
function isLynxAndroid() {
  return isLynx2() && isAndroid3();
}
function isLynxIOS() {
  return isLynx2() && isIOS3();
}
function isH5() {
  return typeof globalThis.document !== "undefined";
}
function getTagSdkData(rawData) {
  return JSON.parse(rawData);
}
function getScrollPriceNumListStyle(scrollList) {
  let _a;
  let fontSize = ((_a = scrollList == null ? undefined : scrollList[0]) == null ? undefined : _a.font_size) || 12;
  return "width:" + 1.1 * fontSize + "rpx; height:" + 2 * fontSize + "rpx;";
}
function getScrollPriceNumListWrapperStyle(item, length) {
  let style = "";
  if (length > 0) {
    let fontSize = (item == null ? undefined : item.font_size) || 12;
    style = "height:" + 2 * fontSize * length + "rpx;";
  }
  return style;
}
function getScrollPriceNumItemStyle(item) {
  let fontSize = (item == null ? undefined : item.font_size) || 12;
  return "height:" + 2 * fontSize + "rpx;";
}
function getSlashImage() {
  return "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAeCAYAAAAYa/93AAAACXBIWXMAACE4AAAhOAFFljFgAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAELSURBVHgBlZJBDoIwEEVnkJ0bj+AZ1IXLJmriTm+g3sCreARuwNJEF+w1EW/AEUiMCRGkFgIGajuMXbShncefwgP4cyC3UIr1AJJk4HIBeDxvCrs7nFo5mQs1D0GizwJU4UbNMeR9JgAg1HV9DP248w5yPFuX7SB4xTMjoWgHI7yeg05ATpdDtagECOo9OuH1FlXVgQeg3ABiiJdTWG+5ZDtpJiCHXXPfnpBl23LNewEPKH4WQoDhMeoEvirk6OlnjvXtlQo8oKGCfvDzlXQVGAltFUjApAKdYFCBBgwq6MNttWNQwZ6Qpvty1VSwA+CsTCoYAUoFcwKhghkgVNCHK0eLLaWCJcGugj4+YX5vrQn5RcAAAAAASUVORK5CYII=";
}
function isH52() {
  return typeof globalThis.document !== "undefined";
}
function isAndroid4() {
  return isAndroid3();
}
function getLeftIgnoreRoundedCorner(style) {
  if (typeof style === "string") {
    if (/border\-radius:\s?0*[1-9]+/.test(style)) {
      return false;
    }
  }
  return true;
}
function isAndroid5() {
  return isAndroid3();
}
function getCouponBarTitle(data, isCouponBarCountDownCompleted, isCouponBarCountDownUpdated) {
  let _a, _b, _c, _d, _e, _f;
  let title = ((_b = (_a = data == null ? undefined : data.coupon_bar) == null ? undefined : _a.content) == null ? undefined : _b.content) || "";
  let titleSuffix = (_d = (_c = data == null ? undefined : data.coupon_bar) == null ? undefined : _c.extra) == null ? undefined : _d.title_suffix;
  if ((_f = (_e = data == null ? undefined : data.coupon_bar) == null ? undefined : _e.extra) == null ? undefined : _f.count_down) {
    if (isCouponBarCountDownCompleted) {
      return title + "  expired";
    }
    if (isCouponBarCountDownUpdated) {
      return title + (titleSuffix ? titleSuffix : " ");
    }
    return title;
  }
  if (titleSuffix) {
    return title + titleSuffix;
  }
  return title;
}
function isAndroid6() {
  return isAndroid3();
}
function isH53() {
  return typeof globalThis.document !== "undefined";
}
function isAndroid7() {
  return isAndroid3();
}
let separators = "()[]{}"":?!,.";
let separatorArr = separators.split("");
function isTitleSeparator(content) {
  return content.length === 1 && String.indexOf(separators, content) >= 0;
}
function getTitleSegments(title) {
  let len = separatorArr.length;
  console.log("liuli titlaarr: ", separatorArr);
  console.log("liuli arry len: ", len);
  let segments = [];
  let content = title || "";
  let length = content.length;
  console.log("liuli content length: ", length);
  if (!length) {
    return segments;
  }
  let segmentStart = 0;
  let index = 0;
  while (index < length) {
    let _char = content.charAt(index);
    if (isTitleSeparator(_char)) {
      if (segmentStart < index) {
        segments.push(content.substring(segmentStart, index));
      }
      segments.push(_char);
      segmentStart = index + 1;
    }
    index += 1;
  }
  let current = segmentStart < length ? content.substring(segmentStart, length) : "";
  let curlen = current.length;
  console.log("liuli curent len: ", curlen);
  console.log("liuli current: ", current);
  if (current.length > 0) {
    segments.push(current);
  }
  return segments;
}
function isNarrowChar(content) {
  return isTitleSeparator(content);
}
function getTitleTextSegClass(baseClassName, content) {
  let className = baseClassName;
  if (isNarrowChar(content)) {
    className = className + " " + baseClassName + "-narrow";
  }
  return className;
}
function canUseVideoEngine() {
  let _lepusTmp = ((SystemInfo == null ? undefined : SystemInfo.lynxSdkVersion) === undefined || (SystemInfo == null ? undefined : SystemInfo.lynxSdkVersion) === null ? "" : SystemInfo == null ? undefined : SystemInfo.lynxSdkVersion).split(".");
  let major = _lepusTmp[0];
  let minor = _lepusTmp[1];
  console.log("canUseVideoEngine", SystemInfo == null ? undefined : SystemInfo.lynxSdkVersion, "major: " + major + ", minor: " + minor);
  return parseInt(major, 10) > 2 || parseInt(major, 10) === 2 && parseInt(minor) >= 18;
}
let VideoEngineState = {
  INIT: "init",
  // initial state
  WAIT_CAN_PLAY: "wait_can_play",
  // waiting for CAN_PLAY
  WAIT_PLAY_CMD: "wait_play_cmd",
  // waiting for play command
  READY_TO_PLAY: "ready_to_play",
  // about to play
  PLAYING: "playing",
  // playing
  PAUSED: "paused",
  // paused
  COMPLETED: "completed",
  // finished
  FAILED: "failed" // error occurred
};
function getOpacity(status) {
  return "opacity: " + (status === VideoEngineState.PLAYING ? 1 : 0) + ";";
}
function isAndroid8() {
  return getGlobalProps_default().os === "android";
}
function getTestNum2() {
  return 0;
}
function getAppVersionNumber() {
  let _a, _b;
  let appVersion = 0;
  let vers = (_b = (_a = getGlobalProps_default().appVersion) == null ? undefined : _a.split(".")) != null ? _b : "";
  appVersion = parseInt(vers[0] + "0" + vers[1] + "00", 10);
  return appVersion;
}
function getGlobals() {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p;
  let globalCompat = getGlobalProps_default();
  return {
    query: {
      enter_from: (_b = (_a = globalCompat.ec_extra) == null ? undefined : _a.enter_from) != null ? _b : "",
      page_name: (_d = (_c = globalCompat.ec_extra) == null ? undefined : _c.page_name) != null ? _d : "",
      previous_page: (_f = (_e = globalCompat.ec_extra) == null ? undefined : _e.enter_from) != null ? _f : "",
      topOffset: (_h = (_g = globalCompat.ec_extra) == null ? undefined : _g.topOffset) != null ? _h : 0,
      height_percent: (_j = (_i = globalCompat.ec_extra) == null ? undefined : _i.height_percent) != null ? _j : "",
      initPageTime: ((_k = globalCompat.ec_extra) == null ? undefined : _k.initPageTime) || 0
    },
    ab: {},
    entrance_info: (_l = globalCompat.ec_extra) == null ? undefined : _l.entrance_info,
    testNum: getTestNum2(),
    appVersion: getAppVersionNumber(),
    // @ts-ignore os
    os: globalCompat.os,
    isAndroid: ((_m = globalCompat.os) == null ? undefined : _m.toLowerCase()) === "android",
    theme: globalCompat.appTheme,
    screenWidth: globalCompat.screenWidth,
    screenHeight: globalCompat.screenHeight,
    deviceID: (_n = globalCompat.deviceID) != null ? _n : "",
    ecExtra: (_o = globalCompat.ec_extra) != null ? _o : {},
    containerID: (_p = globalCompat.containerID) != null ? _p : "",
    topHeight: globalCompat.topHeight,
    statusBarHeight: globalCompat.statusBarHeight,
    enterFrom: globalCompat.enter_from
  };
}
function isSameHeight(useSameHeight) {
  return ["1", "2", "3", "4"].includes(useSameHeight);
}
function getPreferCardBg(data) {
  let _a, _b, _c;
  let preferCardBg = "";
  let windVaneData = data;
  preferCardBg = ((_a = windVaneData.response) == null ? undefined : _a.wind_bg_url) === undefined || ((_b = windVaneData.response) == null ? undefined : _b.wind_bg_url) === null ? "" : (_c = windVaneData.response) == null ? undefined : _c.wind_bg_url;
  return "background-image: url('" + preferCardBg + "');";
  return preferCardBg;
}
function getPreferCardMoreIconText(data) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i;
  let trendingCardMoreTitle = "";
  let windVaneData = data;
  trendingCardMoreTitle = ((_c = (_b = (_a = windVaneData.response) == null ? undefined : _a.extra) == null ? undefined : _b.extend_info) == null ? undefined : _c.trending_card_icon_text) === undefined || ((_f = (_e = (_d = windVaneData.response) == null ? undefined : _d.extra) == null ? undefined : _e.extend_info) == null ? undefined : _f.trending_card_icon_text) === null ? "" : (_i = (_h = (_g = windVaneData.response) == null ? undefined : _g.extra) == null ? undefined : _h.extend_info) == null ? undefined : _i.trending_card_icon_text;
  return trendingCardMoreTitle;
}
function getTitle(data) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p;
  let title = "";
  let windVaneData = data;
  if ((_b = (_a = windVaneData.response) == null ? undefined : _a.extra_v2) == null ? undefined : _b.title) {
    title = ((_d = (_c = windVaneData.response) == null ? undefined : _c.extra_v2) == null ? undefined : _d.title) === undefined || ((_f = (_e = windVaneData.response) == null ? undefined : _e.extra_v2) == null ? undefined : _f.title) === null ? "" : (_h = (_g = windVaneData.response) == null ? undefined : _g.extra_v2) == null ? undefined : _h.title;
  } else if ((_j = (_i = windVaneData.response) == null ? undefined : _i.extra) == null ? undefined : _j.title) {
    title = ((_l = (_k = windVaneData.response) == null ? undefined : _k.extra) == null ? undefined : _l.title) === undefined || ((_n = (_m = windVaneData.response) == null ? undefined : _m.extra) == null ? undefined : _n.title) === null ? "" : (_p = (_o = windVaneData.response) == null ? undefined : _o.extra) == null ? undefined : _p.title;
  }
  if (title && title.slice(title.length - 1) === ":") {
    title = title.slice(0, -1);
  }
  return title;
}
function getItemList(data) {
  let _a, _b, _c;
  let itemList = [];
  let windVaneData = data;
  itemList = ((_a = windVaneData.response) == null ? undefined : _a.items) === undefined || ((_b = windVaneData.response) == null ? undefined : _b.items) === null ? [] : (_c = windVaneData.response) == null ? undefined : _c.items;
  return itemList;
}
function getId(data) {
  let result = "";
  let item = data;
  result = item.product_id === undefined || item.product_id === null ? "" : item.product_id;
  return result;
}
function getKeyword(data) {
  let result = "";
  let item = data;
  result = item.keyword === undefined || item.keyword === null ? "" : item.keyword;
  return result;
}
function getTagBgUrl(data) {
  let _a, _b, _c;
  let result = "";
  let item = data;
  result = ((_a = item.key_word_tag) == null ? undefined : _a.tag_bg_url) === undefined || ((_b = item.key_word_tag) == null ? undefined : _b.tag_bg_url) === null ? "" : (_c = item.key_word_tag) == null ? undefined : _c.tag_bg_url;
  return result;
}
function getCoverUrl(data) {
  let _a, _b, _c, _d, _e, _f;
  let result = "";
  let item = data;
  result = ((_b = (_a = item.cover) == null ? undefined : _a.url_list) == null ? undefined : _b[0]) === undefined || ((_d = (_c = item.cover) == null ? undefined : _c.url_list) == null ? undefined : _d[0]) === null ? "" : (_f = (_e = item.cover) == null ? undefined : _e.url_list) == null ? undefined : _f[0];
  return result;
}
function getLimitLenKeyword(data) {
  let result = "";
  let item = data;
  result = item.keyword === undefined || item.keyword === null ? "" : item.keyword;
  return result;
}
function getMarketingBgUrl(data) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i;
  let marketingBgUrl = "";
  let windVaneData = data;
  marketingBgUrl = ((_c = (_b = (_a = windVaneData.response) == null ? undefined : _a.marketing_items) == null ? undefined : _b[0]) == null ? undefined : _c.marketing_bg_url) === undefined || ((_f = (_e = (_d = windVaneData.response) == null ? undefined : _d.marketing_items) == null ? undefined : _e[0]) == null ? undefined : _f.marketing_bg_url) === null ? "" : (_i = (_h = (_g = windVaneData.response) == null ? undefined : _g.marketing_items) == null ? undefined : _h[0]) == null ? undefined : _i.marketing_bg_url;
  return marketingBgUrl;
}
function getMarketingPrice(data) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i;
  let marketingPrice = 0;
  let windVaneData = data;
  marketingPrice = ((_c = (_b = (_a = windVaneData.response) == null ? undefined : _a.marketing_items) == null ? undefined : _b[0]) == null ? undefined : _c.marketing_subsidy_price) === undefined || ((_f = (_e = (_d = windVaneData.response) == null ? undefined : _d.marketing_items) == null ? undefined : _e[0]) == null ? undefined : _f.marketing_subsidy_price) === null ? 0 : (_i = (_h = (_g = windVaneData.response) == null ? undefined : _g.marketing_items) == null ? undefined : _h[0]) == null ? undefined : _i.marketing_subsidy_price;
  marketingPrice = marketingPrice * 0.01;
  return marketingPrice;
}
function getMarketingText(data) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i;
  let marketingText = "";
  let windVaneData = data;
  marketingText = ((_c = (_b = (_a = windVaneData.response) == null ? undefined : _a.marketing_items) == null ? undefined : _b[0]) == null ? undefined : _c.marketing_text) === undefined || ((_f = (_e = (_d = windVaneData.response) == null ? undefined : _d.marketing_items) == null ? undefined : _e[0]) == null ? undefined : _f.marketing_text) === null ? "" : (_i = (_h = (_g = windVaneData.response) == null ? undefined : _g.marketing_items) == null ? undefined : _h[0]) == null ? undefined : _i.marketing_text;
  return marketingText;
}
function getMarketingButtonText(data) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
  let marketingButtonText = "";
  let windVaneData = data;
  let marketingButtonApply = ((_d = (_c = (_b = (_a = windVaneData.response) == null ? undefined : _a.marketing_items) == null ? undefined : _b[0]) == null ? undefined : _c.button_info) == null ? undefined : _d.is_apply) === undefined || ((_h = (_g = (_f = (_e = windVaneData.response) == null ? undefined : _e.marketing_items) == null ? undefined : _f[0]) == null ? undefined : _g.button_info) == null ? undefined : _h.is_apply) === null ? false : (_l = (_k = (_j = (_i = windVaneData.response) == null ? undefined : _i.marketing_items) == null ? undefined : _j[0]) == null ? undefined : _k.button_info) == null ? undefined : _l.is_apply;
  if (marketingButtonApply === true) {
    marketingButtonText = "Use";
  } else {
    marketingButtonText = "Claim";
  }
  return marketingButtonText;
}
function getMarketingItemCardStyle(data) {
  return "display:relative; background-image: url('" + getMarketingBgUrl(data) + "');";
}
function hasMarketingItemCard(data) {
  let _a, _b, _c, _d;
  let marketingItemCard = false;
  let windVaneData = data;
  if (((_b = (_a = windVaneData.response) == null ? undefined : _a.marketing_items) == null ? undefined : _b[0]) !== null && ((_d = (_c = windVaneData.response) == null ? undefined : _c.marketing_items) == null ? undefined : _d[0]) !== undefined) {
    marketingItemCard = true;
  }
  return marketingItemCard;
}
function getPreferCardTitleStyle(data, abValues) {
  let _lepusTmp = abValues || {};
  let use_small_font_size = _lepusTmp.use_small_font_size;
  let use_same_height = _lepusTmp.use_same_height;
  if (use_same_height === undefined || use_same_height === null) {
    use_same_height = "";
  }
  let use_new_grid = _lepusTmp.use_new_grid;
  let marginBottom = -2;
  if (hasMarketingItemCard(data) === true) {
    marginBottom = 0;
  }
  if (use_small_font_size || isSameHeight(use_same_height) || use_new_grid) {
    marginBottom = 0;
  }
  return "margin-bottom: " + marginBottom * 2 + "rpx;";
}
function getPreferCardTitleTextStyle(data, abValues) {
  if (hasMarketingItemCard(data) === true) {
    return "color: #ff1c49;";
  }
  let color = "rgba(22, 24, 35, 0.45)";
  let use_design_v2025 = (abValues || {}).use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.enable) {
    color = "#8A8B91";
  }
  return "color: " + color + ";";
}
function getPreferCardTitleCouponStyle() {
  let scale = getFontScale();
  if (scale !== null && scale !== undefined && scale === 1 && isIOS()) {
    return "margin-top: 2rpx;";
  }
  return "";
}
function getPreferCardTitleIcon(data, abValues) {
  let img = "network_address";
  let _lepusTmp3 = abValues || {};
  let high_value = _lepusTmp3.high_value;
  let use_design_v2025 = _lepusTmp3.use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  if (high_value) {
    img = "network_address";
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.enable) {
    img = "network_address";
  }
  return img;
}
function getTextMaxWidth(data) {
  if (getTagBgUrl(data) !== "") {
    return "max-width: 113rpx;";
  } else {
    return "max-width: 137rpx;";
  }
}
function getContainerClassName(abValues) {
  let _lepusTmp4 = abValues || {};
  let new_padding = _lepusTmp4.new_padding;
  let high_value = _lepusTmp4.high_value;
  let use_small_font_size = _lepusTmp4.use_small_font_size;
  let use_design_x_version = _lepusTmp4.use_design_x_version;
  let use_same_height = _lepusTmp4.use_same_height;
  if (use_same_height === undefined || use_same_height === null) {
    use_same_height = "";
  }
  let lego_same_height = _lepusTmp4.lego_same_height;
  let lego_magnify_style = _lepusTmp4.lego_magnify_style;
  let use_new_grid = _lepusTmp4.use_new_grid;
  let use_design_v2025 = _lepusTmp4.use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  let card_border_radius_adjust = _lepusTmp4.card_border_radius_adjust;
  let className = "prefer-card";
  if (new_padding) {
    className += " prefer-card-newpadding";
  }
  if (high_value) {
    className += " prefer-card-highvalue";
  }
  if (use_small_font_size) {
    className += " prefer-card-smallfs";
  }
  if (use_design_x_version === "1" || use_design_x_version === "2") {
    className += " prefer-card-design-x";
  }
  if (isSameHeight(use_same_height) || lego_same_height) {
    className += " prefer-card-sameheight";
  }
  if (lego_magnify_style) {
    className += " prefer-card-magnifystyle";
  }
  if (use_new_grid) {
    className += " prefer-card-use-new-grid";
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.enable) {
    className += " prefer-card-design-v2025";
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.borderless) {
    className += " prefer-card-design-v2025-borderless";
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.legou_live_same_height) {
    className += " prefer-card-design-v2025-sameheight";
  }
  if (card_border_radius_adjust === 1) {
    className += " prefer-card-border-adjust-1";
  } else if (card_border_radius_adjust === 2) {
    className += " prefer-card-border-adjust-2";
  }
  return getClassName(className);
}
function getBorderStyle(abValues) {
  let _lepusTmp5 = abValues || {};
  let use_design_x_version = _lepusTmp5.use_design_x_version;
  let use_new_grid = _lepusTmp5.use_new_grid;
  let use_design_v2025 = _lepusTmp5.use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  let card_border_radius_adjust = _lepusTmp5.card_border_radius_adjust;
  let radius = 0;
  if (use_design_x_version === "1" || use_design_x_version === "2") {
    radius = 10;
  }
  if (use_new_grid) {
    radius = 6;
  }
  if (use_design_v2025.enable) {
    radius = 8;
  }
  if (card_border_radius_adjust === 1) {
    radius = 8;
  } else if (card_border_radius_adjust === 2) {
    radius = 4;
  }
  if (radius > 0) {
    return "border-radius: " + radius * 2 + "rpx;";
  }
  return "";
}
function appendDesignV2025ClassName(className, classPrefix, abValues) {
  let use_design_v2025 = (abValues || {}).use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.enable) {
    className = className + " " + classPrefix + "-design-v2025";
  }
  return className;
}
function getCategoryNameUI(ab_values) {
  let className = "item-category-name";
  if (getGlobals().isAndroid) {
    className = className + " item-category-name--" + getGlobals().os;
  }
  className = appendDesignV2025ClassName(className, "item-category-name", ab_values);
  return className;
}
function getItemTitleClassName(classPrefix, abValues) {
  let className = classPrefix;
  className = appendDesignV2025ClassName(className, classPrefix, abValues);
  return className;
}
function getCategoryIconUrl(abValues) {
  let url = "network_address";
  let use_design_v2025 = (abValues || {}).use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.enable) {
    url = "network_address";
  }
  return url;
}
function getItemStyle(abValues, index) {
  let _lepusTmp8 = abValues || {};
  let use_new_grid = _lepusTmp8.use_new_grid;
  _lepusTmp8.use_same_height;
  let use_design_v2025 = _lepusTmp8.use_design_v2025;
  let style = "";
  console.log("abValues", abValues);
  if ((use_design_v2025 == null ? undefined : use_design_v2025.enable) && (use_design_v2025 == null ? undefined : use_design_v2025.legou_live_same_height)) {
    if (index >= 2) {
      style += "margin-top: 26rpx";
    } else {
      style += "margin-top: 20rpx";
    }
  } else if ((use_design_v2025 == null ? undefined : use_design_v2025.enable) && !(use_design_v2025 == null ? undefined : use_design_v2025.borderless) && index >= 2) {
    style += "margin-top: 12rpx";
  } else if (use_new_grid) {
    style += "margin-top: 16rpx";
  } else if (isSameHeight((abValues == null ? undefined : abValues.use_same_height) || "")) {
    style += "margin-top: 24rpx";
  }
  return style;
}
function getWindVaneCardHeight(abValues) {
  let _lepusTmp9 = abValues || {};
  let use_new_grid = _lepusTmp9.use_new_grid;
  let use_design_v2025 = _lepusTmp9.use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  let _lepusTmp10 = __globalProps;
  let screenWidth = _lepusTmp10.screenWidth;
  let str = "";
  if ((use_design_v2025 == null ? undefined : use_design_v2025.enable) && (use_design_v2025 == null ? undefined : use_design_v2025.legou_live_same_height)) {
    str = "height: 510rpx;";
  } else if (use_new_grid) {
    str = "height: " + ((375 - 15 * 375 / screenWidth) * 236 / 180 + 10) + "rpx;";
  }
  return str;
}
function getCardSameHeight(abValues) {
  let _ref2 = abValues || {},
      use_same_height = _ref2.use_same_height;
  console.log("getCardSameHeight", use_same_height);
  let str = "";
  if (["1", "2", "3", "4"].includes(use_same_height)) {
    str = "height: 504rpx";
  }
  return str;
}
function getCardHeight(abValues) {
  return getCardSameHeight(abValues);
}
function card_width() {
  return "width: 100%;";
}
function getRecReasonsWithReplacement(rec_reason_common, rec_reason_replacement_tag) {
  let _ref3 = rec_reason_common || {},
      _ref3$rec_reasons = _ref3.rec_reasons,
      rec_reasons = _ref3$rec_reasons === undefined ? [] : _ref3$rec_reasons;
  let recReasons = rec_reasons;
  if (__IsArray(rec_reasons) && rec_reasons.length > 0 && rec_reason_replacement_tag && Object.keys(rec_reason_replacement_tag).length > 0) {
    recReasons = rec_reasons.map(function (item) {
      if (item == null ? undefined : item.replacement) {
        let _ref4 = item.replacement || {},
            rec_reason = _ref4.rec_reason,
            replace_when = _ref4.replace_when;
        if (replace_when && (rec_reason_replacement_tag == null ? undefined : rec_reason_replacement_tag[replace_when])) {
          return rec_reason;
        }
      }
      return item;
    }).filter(function (item) {
      return !!item;
    });
  }
  return Object.assign({}, rec_reason_common, {
    // replaced recReasons, use original rec_reasons when empty
    rec_reasons: recReasons.length > 0 ? recReasons : rec_reasons
  });
}
function findCountdown(rec_reason_common) {
  let _a;
  let _ref5 = rec_reason_common || {},
      _ref5$rec_reasons = _ref5.rec_reasons,
      rec_reasons = _ref5$rec_reasons === undefined ? [] : _ref5$rec_reasons,
      outerCountdown = _ref5.countdown;
  return ((_a = rec_reasons == null ? undefined : rec_reasons[0]) == null ? undefined : _a.countdown) || outerCountdown;
}
function getCountdown(countdown, startCountTs) {
  let time = 0;
  let showTime = false;
  if (countdown == null ? undefined : countdown.time) {
    time = parseInt(countdown.time, 10) * 1e3 - Date.now();
    showTime = true;
  }
  if (typeof startCountTs === "number" && startCountTs > 0 && (countdown == null ? undefined : countdown.remaining_time) && parseInt(countdown == null ? undefined : countdown.remaining_time, 10) > 0) {
    time = parseInt(countdown.remaining_time, 10) * 1e3 - (Date.now() - startCountTs);
    showTime = true;
  }
  let textStyle = {
    style: "font-size: " + (countdown.font_size ? parseInt(countdown.font_size, 10) * 2 : 22) + "rpx;line-height: " + (countdown.line_height ? parseInt(countdown.line_height, 10) * 2 : 26) + "rpx;color: " + ((countdown == null ? undefined : countdown.font_color) || "#FF3344;") + ";"
  };
  return {
    showTime: showTime,
    time: time,
    prefixText: (countdown == null ? undefined : countdown.prefix_text) || "",
    recText: (countdown == null ? undefined : countdown.rec_text) || "",
    expiredText: (countdown == null ? undefined : countdown.expired_text) || "",
    textStyle: textStyle
  };
}
function getRecReasonCountdown(rec_reason_common, recReasonReplacementTags, startCountTs) {
  let countdown = findCountdown(getRecReasonsWithReplacement(rec_reason_common, recReasonReplacementTags));
  let res = getCountdown(countdown, startCountTs);
  return res;
}
function getGlobalProps5() {
  let globalProps;
  if (typeof lynx !== "undefined") {
    globalProps = lynx.__globalProps;
  } else {
    globalProps = __globalProps;
  }
  return globalProps || {};
}
function isHarmony2() {
  let _getGlobalProps4 = getGlobalProps5(),
      os = _getGlobalProps4.os;
  return os === "openHarmony";
}
let getRecReasonCountdown2 = getRecReasonCountdown;
let getRecReasonsWithReplacement2 = getRecReasonsWithReplacement;
let isHarmony3 = isHarmony2;
function getProductInfoClassName(abValues) {
  let _ref6 = abValues || {},
      use_small_font_size = _ref6.use_small_font_size,
      _ref6$use_design_v = _ref6.use_design_v2025,
      use_design_v2025 = _ref6$use_design_v === undefined ? {} : _ref6$use_design_v;
  let className = "product-info";
  if (use_small_font_size) {
    className += " product-info-smallfs";
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.borderless) {
    className += " product-info-borderless";
  }
  return className;
}
function showWindVane(windVaneData, instantRecommendData, abValues) {
  let _a, _b, _c, _d, _e, _f, _g, _h;
  if (!windVaneData || !(windVaneData == null ? undefined : windVaneData.lynx_data)) {
    return false;
  }
  if ((abValues == null ? undefined : abValues.mix_wind_vane) === "2") {
    if (!instantRecommendData) {
      return true;
    }
    let instantRecommendId = "";
    let itemData = JSON.parse((_b = (_a = instantRecommendData == null ? undefined : instantRecommendData.items) == null ? undefined : _a[0]) == null ? undefined : _b.item_data);
    instantRecommendId = (_c = itemData == null ? undefined : itemData.product) == null ? undefined : _c.product_id;
    let windVaneResponse = null;
    windVaneResponse = (_e = (_d = JSON.parse(windVaneData == null ? undefined : windVaneData.lynx_data)) == null ? undefined : _d.data) == null ? undefined : _e.response;
    let length = ((_f = windVaneResponse == null ? undefined : windVaneResponse.items) == null ? undefined : _f.length) || 0;
    for (let i = 0; i < length; i++) {
      let windVaneId = (_h = (_g = windVaneResponse == null ? undefined : windVaneResponse.items) == null ? undefined : _g[i]) == null ? undefined : _h.product_id;
      if (instantRecommendId && (instantRecommendId == null ? undefined : instantRecommendId.length) > 0 && windVaneId && (windVaneId == null ? undefined : windVaneId.length) > 0) {
        if (instantRecommendId === windVaneId) {
          return false;
        }
      }
    }
  }
  return true;
}
function getDynamicInfo(extra, coverDynamicInfo) {
  let dynamicInfo = {};
  let source = coverDynamicInfo || {};
  let keys = Object.keys(source);
  for (let i = 0; i < keys.length; i++) {
    let key = keys[i];
    dynamicInfo[key] = source[key];
  }
  dynamicInfo.usePiercedSrc = (extra == null ? undefined : extra.use_chip_view) === 1;
  return dynamicInfo;
}
function getCardSameHeight2(abValues) {
  let _ref7 = abValues || {},
      use_same_height = _ref7.use_same_height;
  console.log("getCardSameHeight", use_same_height);
  let str = "";
  if (["1", "2", "3", "4"].includes(use_same_height)) {
    str = "height: 504rpx";
  }
  return str;
}
function getCardHeight2(abValues) {
  return getCardSameHeight2(abValues);
}
function baseSceneTag2(pageName, type) {
  let prefix = "";
  if (pageName === "order_list_page") {
    prefix = "ec_na_order_list";
  } else if (pageName === "product_cart_page") {
    prefix = "ec_na_cart";
  } else if (pageName === "product_detail") {
    prefix = "ec_na_product_detail";
  }
  return prefix + "_" + type;
}
function consturctSceneTag2(sceneSuffix, type) {
  let _a;
  let pageName = ((_a = __globalProps == null ? undefined : __globalProps.ec_extra) == null ? undefined : _a.page_name) || "";
  if (sceneSuffix === undefined || sceneSuffix.length === 0 || ["xtab_homepage", "order_homepage", ""].includes(pageName)) {
    return {
      sceneTag: ""
    };
  }
  let base = baseSceneTag2(pageName, type);
  return {
    sceneTag: base + sceneSuffix
  };
}
function getWindVaneMarginTopRpx(abValues) {
  let _lepusTmp = abValues || {};
  let use_design_x_version = _lepusTmp.use_design_x_version;
  let use_new_grid = _lepusTmp.use_new_grid;
  let use_design_v2025 = _lepusTmp.use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.enable) {
    return "8";
  }
  if (use_new_grid) {
    return "10";
  }
  if (use_design_x_version === "1" || use_design_x_version === "2") {
    return "14";
  }
  return "18";
}
function getBorderStyle2(abValues) {
  let _lepusTmp = abValues || {};
  let use_design_x_version = _lepusTmp.use_design_x_version;
  let use_new_grid = _lepusTmp.use_new_grid;
  let use_design_v2025 = _lepusTmp.use_design_v2025;
  if (use_design_v2025 === undefined || use_design_v2025 === null) {
    use_design_v2025 = {};
  }
  let card_border_radius_adjust = _lepusTmp.card_border_radius_adjust;
  if (card_border_radius_adjust === 1) {
    return "border-radius: 24rpx;";
  } else if (card_border_radius_adjust === 2) {
    return "border-radius: 16rpx;";
  }
  if (use_design_v2025 == null ? undefined : use_design_v2025.enable) {
    return "border-radius: 32rpx;";
  }
  if (use_new_grid) {
    return "border-radius: 16rpx;";
  }
  if (use_design_x_version === "1" || use_design_x_version === "2") {
    return "border-radius: 24rpx;";
  }
  return "";
}
function getScreenMetricsOverride(metrics) {
  let width = metrics.width;
  let globalProps = __globalProps;
  if (!globalProps && typeof lynx !== "undefined") {
    globalProps = lynx.__globalProps;
  }
  globalProps = globalProps || {};
  let mallContainerRealWidth = globalProps.mallContainerRealWidth;
  let mallDeviceAdapterInfo = globalProps.mallDeviceAdapterInfo || {};
  let mallMetricsWidth = mallDeviceAdapterInfo.mallMetricsWidth;
  let cardMetricWidth = 0;
  if (mallMetricsWidth > 0) {
    cardMetricWidth = mallMetricsWidth;
  } else {
    cardMetricWidth = mallContainerRealWidth;
  }
  if (typeof cardMetricWidth === "number" && cardMetricWidth > 0) {
    let pixelRatio = SystemInfo && SystemInfo.pixelRatio;
    if (!pixelRatio && typeof lynx !== "undefined" && lynx.SystemInfo) {
      pixelRatio = lynx.SystemInfo.pixelRatio;
    }
    let cardMetricWidthPX = cardMetricWidth * pixelRatio;
    if (typeof cardMetricWidthPX === "number" && cardMetricWidthPX > 0 && typeof width === "number" && width > 0 && cardMetricWidthPX < width) {
      metrics.width = cardMetricWidthPX;
    }
  }
  return metrics;
}
function copyObject(source) {
  let target = {};
  if (!source || typeof source !== "object") {
    return target;
  }
  let keys = Object.keys(source);
  for (let i = 0; i < keys.length; i++) {
    let key = keys[i];
    target[key] = source[key];
  }
  return target;
}
function galaXDataProdcessor(rawData) {
  let _a, _b, _c, _d, _e;
  let finalData = copyObject(rawData);
  if (rawData && typeof rawData.ec_lynx_props_extra === "object") {
    let bff_ecom_scene_id = ((_a = rawData == null ? undefined : rawData.ec_lynx_props_extra) == null ? undefined : _a.ecom_scene_id) || (rawData == null ? undefined : rawData.bff_ecom_scene_id);
    let ab_values = ((_b = rawData == null ? undefined : rawData.ec_lynx_props_extra) == null ? undefined : _b.ab_values) || ((_c = rawData == null ? undefined : rawData.extra) == null ? undefined : _c.ab_values) || (rawData == null ? undefined : rawData.ab_values);
    let recommend_info = ((_d = rawData == null ? undefined : rawData.ec_lynx_props_extra) == null ? undefined : _d.recommend_info) || ((_e = rawData == null ? undefined : rawData.extra) == null ? undefined : _e.recommend_info);
    finalData.bff_ecom_scene_id = bff_ecom_scene_id;
    finalData.ab_values = ab_values;
    if (typeof rawData.extra === "object") {
      finalData.extra = copyObject(rawData.extra);
      finalData.extra.recommend_info = recommend_info;
      finalData.extra.ab_values = ab_values;
    }
  }
  return finalData;
}
function dateProcessor(rawData) {
  return galaXDataProdcessor(rawData);
}
registerDataProcessor(getScreenMetricsOverride, "getScreenMetricsOverride");
registerDataProcessor(dateProcessor, undefined);
let $currentComponentId = 10;
let $lepusElementLepusIdMap = {};
let $cardInstance;
let $page;
let $cardOptions;
let $initDataMap;
let $airFirstScreen = false;
let $update = false;
let $initAppService = false;
let $outPre = false;
let $lepusGetElementRefByLepusID;
let $lepusStoreElementRefByLepusID;
let sequenceExpression = null;
sequenceExpression = function (b, a) {
  return a;
};
function $getDataType(data) {
  let type = typeof data;
  if (type !== "object") return type;
  if (__IsArray(data)) return "array";
  if (data == null) return "null";
  return "object";
}
function $deepClone(src) {
  let type = $getDataType(src);
  if (type === "array") {
    let array = [];
    src.forEach(function (item) {
      array.push(item);
    });
    return array;
  } else if (type === "object") {
    let keys = Object.keys(src);
    let dic = {};
    keys.forEach(function (key) {
      dic[key] = src[key];
    });
    return dic;
  } else {
    return src;
  }
}
let cssPropertyReverseMap = {
  "top": 1,
  "left": 2,
  "right": 3,
  "bottom": 4,
  "position": 5,
  "box-sizing": 6,
  "background-color": 7,
  "border-left-color": 8,
  "border-right-color": 9,
  "border-top-color": 10,
  "border-bottom-color": 11,
  "border-radius": 12,
  "border-top-left-radius": 13,
  "border-bottom-left-radius": 14,
  "border-top-right-radius": 15,
  "border-bottom-right-radius": 16,
  "border-width": 17,
  "border-left-width": 18,
  "border-right-width": 19,
  "border-top-width": 20,
  "border-bottom-width": 21,
  "color": 22,
  "opacity": 23,
  "display": 24,
  "overflow": 25,
  "height": 26,
  "width": 27,
  "max-width": 28,
  "min-width": 29,
  "max-height": 30,
  "min-height": 31,
  "padding": 32,
  "padding-left": 33,
  "padding-right": 34,
  "padding-top": 35,
  "padding-bottom": 36,
  "margin": 37,
  "margin-left": 38,
  "margin-right": 39,
  "margin-top": 40,
  "margin-bottom": 41,
  "white-space": 42,
  "letter-spacing": 43,
  "text-align": 44,
  "line-height": 45,
  "text-overflow": 46,
  "font-size": 47,
  "font-weight": 48,
  "flex": 49,
  "flex-grow": 50,
  "flex-shrink": 51,
  "flex-basis": 52,
  "flex-direction": 53,
  "flex-wrap": 54,
  "align-items": 55,
  "align-self": 56,
  "align-content": 57,
  "justify-content": 58,
  "background": 59,
  "border-color": 60,
  "font-family": 61,
  "font-style": 62,
  "transform": 63,
  "animation": 64,
  "animation-name": 65,
  "animation-duration": 66,
  "animation-timing-function": 67,
  "animation-delay": 68,
  "animation-iteration-count": 69,
  "animation-direction": 70,
  "animation-fill-mode": 71,
  "animation-play-state": 72,
  "line-spacing": 73,
  "border-style": 74,
  "order": 75,
  "box-shadow": 76,
  "transform-origin": 77,
  "linear-orientation": 78,
  "linear-weight-sum": 79,
  "linear-weight": 80,
  "linear-gravity": 81,
  "linear-layout-gravity": 82,
  "layout-animation-create-duration": 83,
  "layout-animation-create-timing-function": 84,
  "layout-animation-create-delay": 85,
  "layout-animation-create-property": 86,
  "layout-animation-delete-duration": 87,
  "layout-animation-delete-timing-function": 88,
  "layout-animation-delete-delay": 89,
  "layout-animation-delete-property": 90,
  "layout-animation-update-duration": 91,
  "layout-animation-update-timing-function": 92,
  "layout-animation-update-delay": 93,
  "adapt-font-size": 94,
  "aspect-ratio": 95,
  "text-decoration": 96,
  "text-shadow": 97,
  "background-image": 98,
  "background-position": 99,
  "background-origin": 100,
  "background-repeat": 101,
  "background-size": 102,
  "border": 103,
  "visibility": 104,
  "border-right": 105,
  "border-left": 106,
  "border-top": 107,
  "border-bottom": 108,
  "transition": 109,
  "transition-property": 110,
  "transition-duration": 111,
  "transition-delay": 112,
  "transition-timing-function": 113,
  "content": 114,
  "border-left-style": 115,
  "border-right-style": 116,
  "border-top-style": 117,
  "border-bottom-style": 118,
  "implicit-animation": 119,
  "overflow-x": 120,
  "overflow-y": 121,
  "word-break": 122,
  "background-clip": 123,
  "outline": 124,
  "outline-color": 125,
  "outline-style": 126,
  "outline-width": 127,
  "vertical-align": 128,
  "caret-color": 129,
  "direction": 130,
  "relative-id": 131,
  "relative-align-top": 132,
  "relative-align-right": 133,
  "relative-align-bottom": 134,
  "relative-align-left": 135,
  "relative-top-of": 136,
  "relative-right-of": 137,
  "relative-bottom-of": 138,
  "relative-left-of": 139,
  "relative-layout-once": 140,
  "relative-center": 141,
  "enter-transition-name": 142,
  "exit-transition-name": 143,
  "pause-transition-name": 144,
  "resume-transition-name": 145,
  "flex-flow": 146,
  "z-index": 147,
  "text-decoration-color": 148,
  "linear-cross-gravity": 149,
  "margin-inline-start": 150,
  "margin-inline-end": 151,
  "padding-inline-start": 152,
  "padding-inline-end": 153,
  "border-inline-start-color": 154,
  "border-inline-end-color": 155,
  "border-inline-start-width": 156,
  "border-inline-end-width": 157,
  "border-inline-start-style": 158,
  "border-inline-end-style": 159,
  "border-start-start-radius": 160,
  "border-end-start-radius": 161,
  "border-start-end-radius": 162,
  "border-end-end-radius": 163,
  "relative-align-inline-start": 164,
  "relative-align-inline-end": 165,
  "relative-inline-start-of": 166,
  "relative-inline-end-of": 167,
  "inset-inline-start": 168,
  "inset-inline-end": 169,
  "mask-image": 170,
  "grid-template-columns": 171,
  "grid-template-rows": 172,
  "grid-auto-columns": 173,
  "grid-auto-rows": 174,
  "grid-column-span": 175,
  "grid-row-span": 176,
  "grid-column-start": 177,
  "grid-column-end": 178,
  "grid-row-start": 179,
  "grid-row-end": 180,
  "grid-column-gap": 181,
  "grid-row-gap": 182,
  "justify-items": 183,
  "justify-self": 184,
  "grid-auto-flow": 185,
  "filter": 186,
  "list-main-axis-gap": 187,
  "list-cross-axis-gap": 188,
  "linear-direction": 189,
  "perspective": 190,
  "cursor": 191,
  "text-indent": 192,
  "clip-path": 193,
  "text-stroke": 194,
  "text-stroke-width": 195,
  "text-stroke-color": 196,
  "-x-auto-font-size": 197,
  "-x-auto-font-size-preset-sizes": 198,
  "mask": 199,
  "mask-repeat": 200,
  "mask-position": 201,
  "mask-clip": 202,
  "mask-origin": 203,
  "mask-size": 204,
  "gap": 205,
  "column-gap": 206,
  "row-gap": 207,
  "image-rendering": 208,
  "hyphens": 209,
  "-x-app-region": 210,
  "-x-animation-color-interpolation": 211,
  "-x-handle-color": 212,
  "-x-handle-size": 213
};
function parseStyleStringToObject(styleStr) {
  if (!styleStr) {
    return {};
  }
  if (typeof styleStr === "object") {
    return getCssPropertyIDObj(styleStr);
  }
  let finalObj = {};
  let style = typeof styleStr === "string" ? styleStr : styleStr + "";
  let length = style.length;
  let start = 0;
  while (start < length) {
    let end = start;
    let separatorIndex = -1;
    while (end < length) {
      let ch = style.charAt(end);
      if (ch === ":" && separatorIndex < 0) {
        separatorIndex = end;
      } else if (ch === ";") {
        break;
      }
      end++;
    }
    if (separatorIndex >= 0) {
      let keyStart = start;
      let keyEnd = separatorIndex;
      while (keyStart < keyEnd) {
        let _ch = style.charAt(keyStart);
        if (_ch !== " " && _ch !== "\n" && _ch !== "	" && _ch !== "\r" && _ch !== "\f") {
          break;
        }
        keyStart++;
      }
      while (keyEnd > keyStart) {
        let _ch2 = style.charAt(keyEnd - 1);
        if (_ch2 !== " " && _ch2 !== "\n" && _ch2 !== "	" && _ch2 !== "\r" && _ch2 !== "\f") {
          break;
        }
        keyEnd--;
      }
      let id = cssPropertyReverseMap[style.slice(keyStart, keyEnd)];
      if (id !== undefined) {
        let valueStart = separatorIndex + 1;
        let valueEnd = end;
        while (valueStart < valueEnd) {
          let _ch3 = style.charAt(valueStart);
          if (_ch3 !== " " && _ch3 !== "\n" && _ch3 !== "	" && _ch3 !== "\r" && _ch3 !== "\f") {
            break;
          }
          valueStart++;
        }
        while (valueEnd > valueStart) {
          let _ch4 = style.charAt(valueEnd - 1);
          if (_ch4 !== " " && _ch4 !== "\n" && _ch4 !== "	" && _ch4 !== "\r" && _ch4 !== "\f") {
            break;
          }
          valueEnd--;
        }
        finalObj[id] = style.slice(valueStart, valueEnd);
      }
    }
    start = end + 1;
  }
  return finalObj;
}
getCssPropertyIDObj = function (obj) {
  let finalObj = {};
  let keys = Object.keys(obj);
  for (let i = 0; i < keys.length; i++) {
    let item = keys[i];
    let id = cssPropertyReverseMap[item];
    if (id) {
      finalObj[id] = obj[item];
    }
  }
  return finalObj;
};
function getClassStyleIndex(classStr, comId) {
  let classes = classStr.split(" ");
  let result = [];
  for (let index = 0; index < classes.length; ++index) {
    let selector = classes[index] + "_" + comId;
    let array = $selectorStyleIndexMap[selector];
    if (array) {
      result.push(array);
    }
  }
  return result;
}
let $createdElementKey0 = 0;
let $createdElementKey1 = 0;
function $setKeyForCreatedElement(lepusId) {
  let key = lepusId;
  let uniqueKey = lepusId;
  let cardInstance = $cardInstance;
  let forElement = cardInstance._currentForElement;
  let templateElement = cardInstance._currentTemplateElement;
  let compElement = cardInstance._currentComponentElement;
  let templateElementId = templateElement ? templateElement._templateId : -1;
  let forElementId = forElement ? forElement._uniqueId : -1;
  let compElementId = compElement ? compElement._uniqueId : -1;
  if (templateElementId >= forElementId && templateElementId >= compElementId) {
    if (templateElementId !== -1) {
      key = templateElementId;
      uniqueKey = templateElementId;
    }
  } else if (compElementId >= forElementId) {
    if (compElementId !== -1) {
      key = compElementId;
      uniqueKey = compElementId;
    }
  } else if (forElementId !== -1) {
    uniqueKey = (forElementId ^ forElement.activeIndex) * 31;
  }
  if (forElementId > 0) {
    key = (forElement._lepusId ^ forElement.activeIndex) * 31;
  }
  $createdElementKey0 = key;
  $createdElementKey1 = uniqueKey;
}
$lepusGetElementRefByLepusID = function (tag, lepusId) {
  $setKeyForCreatedElement(lepusId);
  let elementId = $lepusElementLepusIdMap[$createdElementKey1 * 65536 | lepusId];
  if (elementId) {
    return __GetElementByUniqueID(elementId);
  }
  return null;
};
function $cardConstructor(componentId, path) {
  let _a;
  $cardOptions = $cardOptions != null ? $cardOptions : {};
  $cardOptions.data = (_a = $initDataMap[path].data) != null ? _a : {};
  $cardOptions._componentId = componentId;
  $cardOptions._uniqueId = componentId;
  $cardOptions._data = {};
  $cardOptions.forCache = {};
  $cardOptions.ownerCache = {};
  $cardOptions.slots = {};
  $cardOptions._currentForElement = undefined;
  $cardOptions._currentComponentElement = undefined;
  $cardOptions._currentTemplateElement = undefined;
  $cardOptions._currentOwner = undefined;
  $cardOptions.fiberComponentId = 10;
  $cardInstance = $cardOptions;
  return $cardInstance;
}
function $lepusPushFiberForNode(elementRef, lepusId, uniqueId) {
  let forElement = elementRef;
  if (forElement) {
    if (!forElement._uniqueId) {
      forElement = $cardInstance["forCache"][uniqueId];
      if (!forElement) {
        forElement = {
          _lepusId: lepusId,
          _uniqueId: uniqueId,
          activeIndex: 0,
          _lastLength: 0
        };
        $cardInstance["forCache"][uniqueId] = forElement;
      }
    }
    let lastForElement = $cardInstance._currentForElement;
    $cardInstance._currentForElement = forElement;
    return [forElement, lastForElement];
  } else {
    $cardInstance._currentForElement = undefined;
    return [undefined, undefined];
  }
}
function $lepusPushFiberComponentNode(compElement) {
  if (compElement) {
    let uniqueId = compElement._componentId;
    let lastComponent = $cardInstance._currentComponentElement;
    $cardInstance._currentComponentElement = compElement;
    globalThis.$currentComponentId = uniqueId;
    return lastComponent;
  } else {
    $cardInstance._currentComponentElement = undefined;
    globalThis.$currentComponentId = $cardInstance._componentId;
    return undefined;
  }
}
function $lepusUpdateFiberForNodeIndex(index) {
  $cardInstance._currentForElement.activeIndex = index;
}
function __GetElementUniqueID(a) {}
$lepusStoreElementRefByLepusID = function (elementRef, lepusId) {
  $setKeyForCreatedElement(lepusId);
  let uniqueId = __GetElementUniqueID(elementRef);
  $lepusElementLepusIdMap[$createdElementKey1 * 65536 | lepusId] = uniqueId;
  return [uniqueId, $createdElementKey0];
};
let $renderComponents = {};
let $componentInfo = {};
let $comUpdatePropsSet = [];
let $createdIds = [];
let $updatedIds = [];
let $deletedOwnerIds = [];
function $componentConstructor(componentId, componentRef, componentPath, lepusId) {
  let _a, _b;
  let component = {
    _parentComponentId: (_b = (_a = $cardInstance._currentComponentElement) == null ? undefined : _a._componentId) != null ? _b : "card",
    _componentId: componentId + "",
    _lepusId: lepusId,
    _uniqueId: componentId,
    _componentRef: componentRef,
    $path: componentPath,
    properties: {},
    _data: {},
    data: {},
    slots: {},
    methods: {}
  };
  let initData = $initDataMap[componentPath];
  if (initData) {
    let rawData = initData.data;
    if (rawData) {
      let rawDataKeys = Object.keys(rawData);
      for (let index = 0; index < rawDataKeys.length; index++) {
        let key = rawDataKeys[index];
        component.data[key] = rawData[key];
      }
    }
    let rawProperties = initData.properties;
    if (rawProperties) {
      let rawPropertyKeys = Object.keys(rawProperties);
      for (let _index = 0; _index < rawPropertyKeys.length; _index++) {
        let _key = rawPropertyKeys[_index];
        let value = rawProperties[_key];
        component.properties[_key] = value;
        component.data[_key] = value;
      }
    }
  }
  component._setProp = function (key, value, update) {
    let data = component.data;
    if (!update || value !== data[key]) {
      $comUpdatePropsSet == null ? undefined : $comUpdatePropsSet.push(key);
      component._data[key] = data[key];
      data[key] = value;
      component.properties[key] = value;
      return true;
    }
    return false;
  };
  $componentInfo[componentId] = component;
  $cardInstance.fiberComponentId = $cardInstance.fiberComponentId + 1;
  return component;
}
function $lepusPushOwner(id) {
  let owner = $cardInstance["ownerCache"][id];
  if (!owner) {
    owner = {
      childOwners: [],
      componentIds: []
    };
    $cardInstance["ownerCache"][id] = owner;
  }
  let lastOwner = $cardInstance._currentOwner;
  if (lastOwner) {
    lastOwner.childOwners.push(id);
  }
  $cardInstance._currentOwner = owner;
  return [owner, lastOwner];
}
function $lepusPopOwner(owner) {
  $cardInstance._currentOwner = owner;
}
function __OnLifecycleEvent(a) {}
function $componentsCreateLifeCycle() {
  let len = $createdIds.length;
  for (let index = 0; index < len; ++index) {
    let componentId = $createdIds[index];
    let instance = $componentInfo[componentId];
    let params = {
      path: instance.$path,
      parentId: instance._parentComponentId,
      initData: $deepClone(instance.data)
    };
    __OnLifecycleEvent([componentId, ["created", "attached", "ready"], params]);
  }
  $createdIds = [];
}
function $componentsUpdateLifeCycle() {
  let len = $updatedIds.length;
  for (let i = 0; i < len; ++i) {
    let componentId = $updatedIds[i];
    let instance = $componentInfo[componentId];
    __OnLifecycleEvent([componentId, "propertiesChanged", {
      props: $deepClone(instance.properties)
    }]);
  }
  $updatedIds = [];
}
function $componentsDeleteLifeCycle() {
  let len = $deletedOwnerIds.length;
  let componentIds = [];
  let ownerIds = [];
  let ownerCache = $cardInstance["ownerCache"] || {};
  function pushUnique(list, item) {
    if (!item) {
      return;
    }
    if (!list.includes(item)) {
      list.push(item);
    }
  }
  function collectOwner(ownerId) {
    if (!ownerId || ownerIds.includes(ownerId)) {
      return;
    }
    pushUnique(ownerIds, ownerId);
    let owner = ownerCache[ownerId];
    if (!owner) {
      return;
    }
    let ownerComponentIds = owner.componentIds || [];
    for (let i = 0; i < ownerComponentIds.length; ++i) {
      pushUnique(componentIds, ownerComponentIds[i]);
    }
    let childOwners = owner.childOwners || [];
    for (let _i2 = 0; _i2 < childOwners.length; ++_i2) {
      collectOwner(childOwners[_i2]);
    }
  }
  for (let i = 0; i < len; ++i) {
    collectOwner($deletedOwnerIds[i]);
  }
  for (let _i3 = 0; _i3 < ownerIds.length; ++_i3) {
    ownerCache[ownerIds[_i3]] = undefined;
  }
  for (let _i4 = 0; _i4 < componentIds.length; ++_i4) {
    let componentId = componentIds[_i4];
    __OnLifecycleEvent([componentId, "detached", {}]);
    $componentInfo[componentId] = undefined;
  }
  $deletedOwnerIds = [];
}
let renderPage = null;
let updatePage = null;
let $cardVariables = [];
let $varUpdateState = [];
$selectorStyleIndexMap = {
  "card_27472000": [0, 1, 2, 3],
  "card-animation_27472000": [4],
  "product_27472000": [5, 2, 0, 1, 6, 3],
  "use-base-background_27472000": [7],
  "product-info_27472000": [7, 8, 6, 3, 9],
  "product-info-borderless_27472000": [10],
  "product-info-smallfs_27472000": [11],
  "product-info-price-atmosphere_27472000": [12],
  "price_27472000": [0, 13, 14],
  "ad-common_27472000": [15, 5, 16, 17, 18, 3],
  "feedback-wp_27472000": [15, 17, 18, 19, 5, 16],
  "similar_27472000": [15, 5, 20, 18],
  "belt_common_27472000": [21, 22, 23],
  "page_21316000": [],
  "mall-font-mini_21316000": [],
  "mall-font-medium_21316000": [],
  "mall-font-large_21316000": [],
  "mall-font-larger_21316000": [],
  "container_21316000": [15, 5, 16, 17, 18],
  "product_with_live_tag_21316000": [15, 24, 25, 26, 27, 0],
  "product_with_live_tag-icon_21316000": [28, 29, 30, 31],
  "product_with_live_tag-text_21316000": [32, 33, 34, 35, 36, 37, 38, 38, 39],
  "corner-mark_21316000": [15, 40, 41, 29, 42, 43],
  "corner-mark-text_21316000": [44, 45, 46, 47, 48, 49, 35, 50, 51, 34],
  "product-cover-border-image-container_21316000": [5, 52, 15, 17, 18, 53],
  "product-cover-border-image_21316000": [5, 52, 15, 17, 18, 54],
  "just_see_see_21316000": [15, 17, 18],
  "just_see_see-icon_21316000": [55, 56],
  "just_see_see-text_21316000": [15, 35, 57, 25, 34],
  "just_see_see_logo_21316000": [15, 24, 25, 58, 59, 60],
  "decoration_21316000": [15, 29, 0, 61, 40, 41, 27, 26],
  "decoration-collocate_21316000": [24, 25],
  "decoration-icon_21316000": [28, 29, 26],
  "decoration-icon-collocate_21316000": [62, 63, 32],
  "product-video-tag_21316000": [15, 40, 41],
  "product-video-tag-icon_21316000": [30, 31],
  "decoration-text_21316000": [29, 37, 64, 35, 34],
  "decoration-text-collocate_21316000": [65],
  "ad-tag_21316000": [15, 28, 66, 67, 68, 0, 69, 70],
  "ad-tag-text_21316000": [71, 72, 5, 73, 50],
  "ad-pot_21316000": [74, 75, 15, 76, 20, 77],
  "border_21316000": [15, 18, 17, 5, 16, 78, 79, 80, 81, 82, 83, 84, 19],
  "product-coupon_21316000": [15, 85, 86, 87, 3, 17, 88, 89, 22, 90, 91, 14],
  "product-coupon-container_21316000": [92, 93, 94, 14, 0, 23],
  "product-coupon-container-legou_21316000": [92, 95, 96, 14, 0, 23],
  "product-coupon-symbol_21316000": [34, 97, 98, 99, 100],
  "product-coupon-integer_21316000": [34, 101, 98, 100, 102],
  "product-redPacket_21316000": [15, 103, 104, 87, 3, 105, 22, 90, 91, 106, 107, 108, 109, 110, 111, 112, 113],
  "product-redPacket-wrapper_21316000": [1, 114, 115, 61, 23],
  "product-redPacket-container_21316000": [14, 0, 23],
  "product-redPacket-num_21316000": [34, 116, 36, 117],
  "product-redPacket-symbol_21316000": [34, 118, 119, 99, 120],
  "product-redPacket-subtitle_21316000": [121, 97, 122, 36, 123],
  "dislike-entry_21316000": [16, 5, 61, 124, 125],
  "mask-animation-show_21316000": [126, 127],
  "mask-animation-hide_21316000": [128, 129],
  "title-show_21316000": [130],
  "title-hide_21316000": [131],
  "dislike-lottie_21316000": [132, 133],
  "dislike-title_21316000": [134],
  "dislike-subtitle_21316000": [135, 0],
  "dislike-entry-text_21316000": [34, 50, 136, 137, 138],
  "text-yellow_21316000": [139],
  "container_12169000": [15, 5, 16, 17, 18, 3],
  "animation_12169000": [140, 141, 15, 3, 142, 143],
  "animation-lottie_12169000": [5, 16],
  "mask_12169000": [15, 88, 17, 5, 16, 79, 3],
  "mask-image_12169000": [5, 16, 79],
  "gyl-belt_32391000": [0, 61, 5, 16, 144],
  "gyl-belt__main_32391000": [0, 61],
  "gyl-belt__action_32391000": [0],
  "gyl-belt__main-icon_left_32391000": [6],
  "gyl-belt__main-text_32391000": [0, 61],
  "gyl-countdown_20523000": [5, 145, 61, 13, 0, 23],
  "gyl-countdown__num_20523000": [146, 35, 36, 147],
  "gyl-countdown__split-text_20523000": [146, 35, 36, 147],
  "gyl-cover__container_49914000": [5, 6],
  "gyl-cover__wrapper_49914000": [5, 0, 16],
  "gyl-cover__absolute_49914000": [15],
  "gyl-cover__cover_49914000": [5, 148, 149],
  "gyl-cover__mask_49914000": [5, 148, 149, 15, 15, 18, 88, 17, 20],
  "gyl-cover__video_49914000": [5, 16, 15, 17, 18, 148],
  "gyl-cover__swiper_49914000": [5, 16, 15, 17, 18, 150, 148],
  "gyl-cover__swiper-item_49914000": [5, 16],
  "gyl-cover__swiper-item-image_49914000": [5, 16, 151],
  "gyl-cover__tag_49914000": [15, 25, 152, 153, 154, 149, 155, 156, 0, 61, 33],
  "gyl-cover__tag-image_49914000": [62, 63, 155, 157, 158],
  "gyl-cover__tag-left_49914000": [159, 35, 122, 36, 160, 149],
  "gyl-cover__tag-icon_49914000": [161, 66, 162, 163, 32, 164],
  "gyl-cover__tag-right_49914000": [159, 35, 122, 36, 160],
  "gyl-cover__bubble_49914000": [15],
  "gyl-cover__horizontal_skus_49914000": [5],
  "feedback_54878000": [5, 16, 165, 3],
  "feedback-container_54878000": [166, 5, 16, 79, 0, 1, 167, 168, 6, 169, 3],
  "gyl_feedback_39746000": [15, 17, 18, 5, 16, 0, 1, 167, 168],
  "gyl_feedback-neg_39746000": [5, 170, 0, 1, 23],
  "gyl_feedback-neg--item_39746000": [171, 0, 172, 173, 174, 175, 61, 176, 177],
  "gyl_feedback-neg--item-more_39746000": [178],
  "gyl_feedback-neg-pagetwo-item_39746000": [23],
  "gyl_feedback-neg--item-last_39746000": [178],
  "gyl_feedback-neg-pagetwo-item-icon_39746000": [179],
  "gyl_feedback-neg--item-icon_39746000": [180, 181, 33],
  "gyl_feedback-neg--item-icon-more_39746000": [182, 183, 184, 185],
  "gyl_feedback-neg--item-text_39746000": [34, 118, 119],
  "gyl_feedback-neg--item-text-more_39746000": [34, 118, 119],
  "gyl_feedback-neg--item-text-more-small_39746000": [34, 186, 119],
  "gyl_feedback-similar_39746000": [5, 187, 43, 0, 188, 61, 23, 189, 190],
  "gyl_feedback-similar-arrow-icon_39746000": [182, 183, 184],
  "gyl_feedback-similar-text_39746000": [119, 191, 192, 34],
  "gyl_feedback-header_39746000": [15, 5, 193, 17, 18, 194],
  "gyl_feedback-header-back_39746000": [0, 195, 196, 15, 18],
  "gyl_feedback-header-back-icon_39746000": [183, 182, 197, 198],
  "gyl_feedback-header-back-text_39746000": [34, 118, 36, 199],
  "gyl_feedback-header-close_39746000": [196, 200, 0, 15, 88],
  "gyl_feedback-header-close-icon_39746000": [183, 182, 201],
  "gyl-find_similar_26698000": [5, 202, 203],
  "gyl-find_similar-container_26698000": [0, 61, 204, 5, 202, 205, 206, 22, 21],
  "gyl-find_similar-cover_26698000": [79, 207, 208, 209],
  "gyl-find_similar-mask_26698000": [15, 18, 17, 207, 208, 210, 79],
  "gyl-find_similar-text_26698000": [211, 35, 119, 48, 212],
  "gyl-find_similar-arrow_26698000": [182, 183, 120],
  "gyl-cover__horizontal-skus__container_17257000": [0, 61, 144, 213, 7, 5],
  "gyl-cover__horizontal-skus__short-card-wrapper_17257000": [5, 0, 61, 144],
  "gyl-cover__horizontal-skus__short-card_17257000": [149, 0, 61, 214, 215, 216, 217, 79, 9],
  "gyl-cover__horizontal-skus__short-card:first-child_17257000": [157],
  "gyl-cover__horizontal-skus__short-card-image_17257000": [149, 218, 219],
  "gyl-cover__horizontal-skus__short-card-price-container_17257000": [0, 220],
  "gyl-cover__horizontal-skus__short-card-price_17257000": [149, 3, 221],
  "gyl-cover__horizontal-skus__long-card_17257000": [149, 0, 61, 144, 222, 216, 5, 79],
  "gyl-cover__horizontal-skus__long-card-image-wrapper_17257000": [149, 0, 61, 214],
  "gyl-cover__horizontal-skus__long-card-image_17257000": [218, 219, 223],
  "gyl-cover__horizontal-skus__long-card-price_17257000": [0, 149, 220],
  "gyl-cover__horizontal-skus__long-card-price-suffix_17257000": [224, 225, 36, 226, 227],
  "container_20117000": [5, 6],
  "swiper-item_20117000": [5, 16],
  "swiper-item-image_20117000": [5, 16, 151],
  "live_wrapper_20117000": [5, 16, 15, 17, 18],
  "live_20117000": [5, 16, 126],
  "live_show_20117000": [128],
  "feed_back_button_20117000": [15, 17, 88],
  "feed_back_button_image_20117000": [228, 59],
  "video_engine_20117000": [15, 17, 18, 20, 88, 3],
  "scroll_price_container_51983000": [0, 13, 14, 229],
  "scroll_price_num_list_51983000": [0, 1, 14, 23, 3, 230],
  "translateY4_51983000": [231],
  "scroll_price_num_list_wrapper_51983000": [0, 1, 61],
  "scroll_price_num_list_item_51983000": [0, 232, 149, 107, 233, 5],
  "scroll_price_num_list_item_text_51983000": [50, 117, 146, 221],
  "scroll_price_point_51983000": [118, 146, 117, 234],
  "gyl-price_51983000": [0, 13, 220, 170],
  "gyl-price--lynx-android_51983000": [],
  "gyl-price__symbol_51983000": [235],
  "gyl-price__number_51983000": [221, 236],
  "gyl-price__slash_51983000": [237, 66],
  "gyl-price__text_51983000": [221, 238, 239, 240],
  "gyl-price__coin-wrapper_51983000": [0, 13, 61, 155, 241, 154, 160, 215, 242],
  "gyl-price__coin-text_51983000": [35, 160, 243, 119],
  "gyl-price__coin-img_51983000": [62, 63, 244],
  "gyl-price__tag--h5_51983000": [245],
  "gyl-price__scroll_51983000": [239],
  "gyl-price__scroll--integer_51983000": [246, 14, 234],
  "gyl-price__scroll--decimal_51983000": [246, 14, 231],
  "gyl-price__scroll--point_51983000": [118, 146, 117, 234],
  "gyl-price__space--ml-1_51983000": [120],
  "gyl-price__space--ml-4_51983000": [247],
  "gyl-price__space--mx-1-5_51983000": [248, 249],
  "gyl-price__space--ml-2_51983000": [227],
  "gyl-recommend-reason__container_47842000": [0, 149, 13, 61],
  "gyl-recommend-reason__header_47842000": [136],
  "gyl-recommend-reason__header--android_47842000": [250],
  "gyl-recommend-reason__icon-left--android_47842000": [251],
  "gyl-recommend-reason__text_47842000": [147],
  "gyl-recommend-reason__countdown_47842000": [146, 36],
  "gyl-scroll-to-number__container_369000": [0, 1, 14, 23, 3],
  "gyl-scroll-to-number__wrap_369000": [15, 17, 18, 0, 1, 14, 5, 252, 253],
  "gyl-scroll-to-number__item_369000": [0, 232, 149, 107, 233, 5],
  "gyl-scroll-to-number__number_369000": [5, 16, 50, 117, 146, 221],
  "gyl_buy_together-main_pic-bubble-pic_38422000": [254, 255],
  "gyl_buy_together-main_pic-bubble-stroke_38422000": [15, 256, 257, 57, 258],
  "gyl_buy_together-main_pic-bubble-container_38422000": [259, 172, 54, 260, 144, 261, 15, 262],
  "gyl_buy_together-main_pic-bubble-title_38422000": [263],
  "gyl_buy_together-main_pic-bubble-title-text_38422000": [264, 97, 36, 160],
  "gyl_buy_together-main_pic-bubble-price-integer_38422000": [265, 117, 122, 36, 35, 48, 266],
  "gyl_buy_together-main_pic-bubble-price-label_38422000": [265, 97, 122, 36, 48],
  "gyl-sku_pics_35645000": [0, 61],
  "gyl-sku_pics-cover_35645000": [79, 207, 208, 209],
  "gyl-sku_pics-mask_35645000": [15, 18, 17, 207, 208, 210, 79],
  "gyl-cover__cover-blur_35681000": [267, 3, 6, 239, 268],
  "gyl-cover__cover-mask_35681000": [15, 269, 270, 17, 18, 271],
  "gyl-cover__cover-blur-img_35681000": [6, 272, 269, 270, 273, 274, 275],
  "gyl-cover__cover-blur-container_35681000": [15, 17, 18, 5, 16, 0, 1, 61, 144, 276],
  "gyl-cover__cover-blur-sku_products_35681000": [0, 1, 61],
  "gyl-cover__cover-blur-sku_products-image_35681000": [277, 172],
  "gyl-cover__cover-blur-sku_products-price_35681000": [226, 117, 186, 36, 278],
  "gyl-cover__cover-blur-sku_products-price-container_35681000": [0, 220],
  "gyl-suspend__btn_27959000": [0, 61, 23, 279, 164, 219, 79, 280],
  "gyl-suspend__line_27959000": [281, 257, 282, 283],
  "gyl-suspend__topic_27959000": [0, 13, 61],
  "gyl-suspend__topic-text_27959000": [9],
  "gyl-suspend__background_27959000": [284],
  "gyl-suspend__coin_27959000": [0, 23, 61],
  "gyl-suspend__coin-text_27959000": [50],
  "gyl-suspend--store_27959000": [0, 61, 181, 285, 26, 286, 287],
  "gyl-suspend--store-text_27959000": [50, 288, 3, 289],
  "gyl-suspend__guid_27959000": [0, 13, 61],
  "gyl-suspend__guide_new_27959000": [0, 13, 61],
  "gyl-suspend__guide-icon_right_27959000": [290],
  "gyl-suspend__coupon-bar_27959000": [0, 13, 61, 154, 291, 21, 22],
  "gyl-suspend__coupon-bar--countdown-completed_27959000": [292, 293],
  "gyl-suspend__coupon-bar--anim-box_27959000": [170, 16, 3],
  "gyl-suspend__coupon-bar--anim-box-offset_27959000": [5, 15, 18, 88, 17, 294],
  "gyl-suspend__coupon-bar--anim-box-loading_27959000": [295],
  "gyl-suspend__coupon-bar-anim-box-item_27959000": [0, 61, 5, 16, 296, 3],
  "gyl-suspend__coupon-bar-text_27959000": [146, 35, 297, 36, 147, 149, 117],
  "gyl-suspend__coupon-bar-text-coudan_27959000": [146, 35, 297, 119, 147, 149],
  "gyl-suspend__coupon-bar-countdown_27959000": [149],
  "gyl-suspend__guide-text_27959000": [149],
  "gyl-suspend__guide_new-text_27959000": [149],
  "gyl-suspend__buy_together_27959000": [298, 0, 61],
  "gyl-suspend__buy_together-wrapper_27959000": [0, 61],
  "gyl-suspend__buy_together-cover_27959000": [299, 300, 79],
  "gyl-suspend__buy_together-price_27959000": [32],
  "gyl-suspend__buy_together-mask_27959000": [15, 18, 17, 299, 300, 210, 79],
  "gyl-suspend__coupon-bar-legou_27959000": [0, 13, 61, 154, 301, 21, 22],
  "gyl-suspend__coupon-bar-legou-box-item_27959000": [144],
  "gyl-suspend__coupon-bar-legou-box-item_iOS_27959000": [251],
  "gyl-suspend__coupon-bar-legou-header-text_27959000": [302, 136, 35, 122, 303, 225, 304, 149],
  "gyl-suspend__coupon-bar-legou-divider_27959000": [305, 306, 247, 157, 307],
  "container_19453000": [171, 308, 262, 0, 61, 3],
  "gyl-title_9017000": [288, 3, 289, 309, 51],
  "gyl-title__seg_9017000": [288, 3, 289, 309, 310],
  "gyl-title__seg-narrow_9017000": [311, 310],
  "gyl-title__tag_9017000": [239, 51, 69, 157, 312],
  "gyl-title__tag--android_9017000": [313],
  "gyl-title__tag--h5_9017000": [314],
  "gyl-video-engine_50448000": [5, 16],
  "page_57778000": [],
  "mall-font-mini_57778000": [],
  "mall-font-medium_57778000": [],
  "mall-font-large_57778000": [],
  "mall-font-larger_57778000": [],
  "prefer-card_57778000": [5, 0, 1, 22, 315, 316, 7, 317, 54, 318, 319],
  "prefer-card-sameheight_57778000": [320, 321],
  "prefer-card--title-coupon_57778000": [247, 322, 35],
  "prefer-card-magnifystyle_57778000": [323, 46],
  "prefer-card-smallfs_57778000": [324, 325],
  "prefer-card-smallfs .prefer-card--products_57778000": [326, 327],
  "prefer-card-smallfs .prefer-card--products-item_57778000": [278, 328, 329],
  "prefer-card--title_57778000": [0, 13, 61, 304],
  "prefer-card-design-x_57778000": [330],
  "prefer-card-use-new-grid_57778000": [54, 324],
  "prefer-card-design-v2025_57778000": [331, 325],
  "prefer-card-design-v2025-borderless_57778000": [332, 316, 331],
  "prefer-card-design-v2025-sameheight_57778000": [54, 318],
  "prefer-card-border-adjust-1_57778000": [330],
  "prefer-card-border-adjust-2_57778000": [54],
  "prefer-card--title-icon_57778000": [333, 334],
  "prefer-card--title-text_57778000": [122, 36, 247, 128, 335, 336, 336, 337],
  "marketing_item_card_57778000": [187, 13, 3, 22, 315, 338, 317, 298, 339, 340, 341],
  "marketing_item_card-price_57778000": [122, 98, 342, 343, 34, 344, 199],
  "marketing_item_card-price_unit_57778000": [122, 119, 97, 34, 297],
  "marketing_item_card-text_57778000": [122, 36, 97, 247, 34, 345, 297],
  "marketing_item_card-button_bg_57778000": [346, 29, 150, 347, 348, 349],
  "marketing_item_card-button_text_57778000": [122, 119, 97, 350, 297],
  "prefer-card--products_57778000": [0, 13, 351, 5, 352, 353],
  "prefer-card--products-item_57778000": [0, 1, 354, 355, 356, 279],
  "item-bg_57778000": [5, 52, 79],
  "item-image_57778000": [5, 52, 54],
  "item-masking_57778000": [5, 52, 54, 357, 358],
  "item-top-layer-masking-gradient_57778000": [5, 52, 54, 359],
  "item-top-layer-masking_57778000": [5, 52, 54, 357, 358],
  "item-title_57778000": [5, 298, 63],
  "item-title-design-v2025_57778000": [283],
  "item-category_57778000": [0, 106, 188, 63, 3],
  "item-category-name_57778000": [360, 36, 361, 288, 38],
  "item-category-name--android_57778000": [251],
  "item-category-name-design-v2025_57778000": [226],
  "item-category-icon_57778000": [362, 363],
  "item-category-logo_57778000": [182, 183]
};
let $conditionNodeIndex = {};
$cardOptions = {
  data: {}
};
let $componentUpdate = null;
$initDataMap = {
  "src/cards/guessyoulike/product_card_v2/index": {
    "data": {
      "extra": {
        "ab_values": {
          "wind_vane_freq": "2",
          "new_recommend_position": true,
          "is_ui_optimize": true,
          "mix_wind_vane": "3",
          "live_card_enlarge_play_area": true,
          "feedback_educate_page_limit": "xtab_homepage",
          "customer_new_style": "1",
          "use_design_v2025": {
            "enable": true,
            "borderless": false,
            "revert_444": false
          },
          "ai_guide_type": "",
          "new_padding": true,
          "hide_video_time": true,
          "product_with_x": true,
          "activity_hot_zone": 1,
          "is_ui_compact": true,
          "new_font_family": true,
          "use_design_x_version": "2",
          "selling_point_update": 1,
          "use_new_grid": "1",
          "show_interaction_trigger": "deep_feed",
          "use_feed_back_button": "",
          "card_border_radius_adjust": 2,
          "info_font_adjust": 4
        },
        "animation_control": null,
        "room_id": "",
        "with_live_open_type": "",
        "auto_apply_coupon_info": undefined,
        "is_ad": false,
        "product_id": "3815523199945408710",
        "accessibility_label": "button font test c test",
        "need_animation": false,
        "use_chip_view": 1,
        "card_animation_type": "",
        "image_scene_tag_suffix": "_first",
        "tab_id": 0,
        "need_v_page_num": false,
        "jump_guide_area_for_apply": "",
        "jump_guide_area_type": "",
        "jump_guide_area_extra_info": "",
        "rec_unique_id": "20260507110832F4AC18941C37D2B1F3F2_3815523199945408710",
        "is_new_product_live": false,
        "is_ai_guide_card": false,
        "recommend_info": '{"uid":110893068884,"feeds_user_group_label":"churn","online_predict_model":"ecom_center_rank_models_31123_v2247_r18423414_0","gid":3815523199945408710,"ghs5":"wtw6h","chr":0,"bat":0,"har":0,"ohr":0,"adtp":"poi_address","lbsn":0,"astn":0,"stn":0,"is_seckill":0,"is_eff_pass":10,"is_dis_pass":10,"prd":{"gcp":1,"style_id_v2":301737,"pnum":1,"pname":"xtab_homepage","rsn":"vk_discover:75:0.306956","new":0,"thr_new":0,"lv4s":2,"ppsv3":0.8642403584261071,"pptv3":2,"ppsv2":0.6351975210988682,"ppsv4":0.0,"pptv4s2":0,"pptv4s7":0,"pptv4s2n":0,"pptv4s7n":0,"seedsaab":-1.0,"seedsababc":-1.0,"seedosaab":-1.0,"seedosababc":-1.0,"seedcs":-1.0,"epad":0,"bfn":0,"goods_id":"3815523199945408710","product_pic_simid_v5":"3815523199945408710","product_spu_sim_id_v2":"3775542601562849411","okr_brand_id":"83898716","g_goods_pred_leaf_ctg":"22229","simprd_simhash_1":"1700862118627455534","simprd_simhash_2":"2324749316526887555","simprd_simhash_3":"3717563022327799083","simprd_simhash_4":"1941967124953143618","mcsgb":0,"isagg":0,"shop1_gb":9,"shop2_gb":0,"shop3_gb":0,"p_cvr_thre_m":"1.0","p_cvr_thre":"1.0","prd_price_avg":0.0,"idx":37,"rf_idx":0,"nfs_1h":"1:0:0:0:0:0","nfs_24h":"3:0:0:0:0:0","nfs_all":"4:0:0:0:0:0","pr_idx":98,"rp_idx":248,"rp_score":9.826852209609,"rp_cascade_gmv":0.05166995897889137,"rp_cascade_ctcvr":0.015614282339811325,"rp_cascade_ctr":0.35396620631217957,"rp_cascade":0.3888445794582367,"rp_order":0.0013659077230840921,"rp_buy":0.1478581726551056,"rp_click":0.06761271506547928,"impr":0,"act":0,"14d_lctg":4,"in_180d_lctg":1,"in_all_lctg":1,"in_180d_ctg1":1,"qversion":961623798681960745,"vk_discover":"0.306956","simid":3815523199945408710,"spu_simid":0,"white_simid":"b1cdce32ce3238c9","lctg":22229,"ctg1":20000,"f_lctg":1000007175,"ai_ctg":20000,"gfprice":399,"dprice":39900,"aprice":44285,"sprice":0.0,"fprice":39900,"market_price":39900,"ai_lctg":22229,"pr_cmp_v3_rt":0,"brand":83898716,"brand_pl":"","trig_clk":"51,18,28,20018_14,22229_6,38946_34600_906203","trig_buy":"1,1,1,20018_1,29193_1,20018_29193_5314110","ei_basic_fea":"google Pixel 6_1_WiFi_0.93_1","u_exit_prob":"-1.0","u_exit_ts":"0","pred_sc":"2042.945134","ctr_term":"55.591416","ctcvr_term":"2.054541","gmv_term":"3.882158","pctr_rank_idx_score":"0.000000","pcvr2_rank_idx_score":"0.000000","price_rank_idx_score":"0.000000","intent_rank_idx_score":"0.000000","cascade_rank_idx_score":"0.000000","pctr_beta":"4720.000000","pcvr2_beta":"1400000.000000","ori_pctr":"0.011778","ori_pcvr2":"0.000091","ctcvr_term1":"1.604833","price_beta":"6630.000000","price_alhpa":"0.000000","card_price":"399.000000","finepred_price":"399.000000","gmv_term1":"0.007600","ori_ctr":"0.011778","ori_cvr":"0.000091","fatigue":"0.000000","trg_related":"0.000000","ori_gcvr":"0.000000","explosive_subsidy_subscribe_flag":0,"remove_high_dislike_product_user_tag":0,"ori_cu":"2042.945134","ccr_mean":"0.015664","ccr_std":"0.025638","pccr_mean":"0.018747","pqccr_mean":"0.011699","is_cascade_topk":0,"rp_rank_idx":43,"cascade_rough_predict_score":133.5287942260038,"is_active_refresh":false,"rctr":"0.004829","rcvr":"0.000933","rcascade_ctr":"0.038002","rcascade_ctcvr":"0.000580","rough_cas_impression":"0.015261","kgplv":7,"buy_cnt_14d":0,"gover_labels_new":"0","app_punish_1128_mask_part1":"0","app_punish_1128_mask_part2":"0","app_punish_zhili_ready":true,"is_exchange":"0","expid":"6","cs_expid":"1","exchange_status":0,"rt_exchange_status":0,"is_in_assign":"false","asset_template_id":"","mixed_coupon_discount_value":100,"coupon_uplift":0.0,"delta_cu":"0.0000000000","longtime_cu":"0.0000000000","exchange_std_price":0.0,"exchange_sku_price":0.0,"rt_exchange_sku_price":0.0,"pv_coef":"5.0000000000000000","if_quit":false,"cid":0,"ecom_center_boost_bitmap":131537,"ecom_center_exchange_7d_pv":3,"ecom_center_exchange_7d_buy":0,"rprice":"399.000000","rscore":"459.7598140001788920","rscore_ori":"0.0000000000","rscore_ab":"0.0000000000","rbst":"1.0000000000000000","merge_pos":213,"is_pc_ept":0,"rgh_cas_idx":-1,"rgh_mtl_idx":-1,"rgh_cas_miss10":0,"rgh_cas_miss50":20,"rgh_mtl_miss10":0,"rgh_mtl_miss50":0,"feature_price_for_discount":39900,"mixed_coupon_meta_id":"","goods_roi2_ecpm_score":"2015.220744","goods_roi2_origin_ecpm":"5038.051859","fine_sort_index":2,"index":1,"tscore1":"0.512334","prd_clk":"0.011778","ord_sub":"0.000091","cvr_split2":"0.000091","pccr":"0.006564","pqccr":"0.004944","pbcmt":"0.960361","pfeedback":"0.010570","dcvr":"0.000419","p_rebuy":"0.003185","p_rebuy_gmv":"61.906250","p_aft_rfd":"0.026964","p_ctg1_rfd":"0.000380","order_high":"0.009596","order_good":"0.010902","bst":1.0,"be_bst":1.0,"price_alpha":"0.000000"},"is_allowance":0,"pure_impr_cnt":0,"pure_impr_win":1,"pred":{"clevel_x_pay":"L5_0"},"cs_s":0,"is_cs":0,"mcs_s":0,"baopin_status":1,"shop_cs":{"cs_s":0,"cs_val":0.0},"chnid":1111868,"chnid_bak":0,"ent":"guess_u_like","appid":1128,"cs_force_insert":{"u_cs_ctr":0.0,"u_cs_cvr":0.0,"u_cs_ctcvr":0.0,"u_cs_cu":0.0},"rt":1,"traffic_from":"homepage_top_tab","tfs":"","req_id":"20260507110832F4AC18941C37D2B1F3F2","loc":310110,"tab":0,"sub_req_id":"B6FCB","sess":"0ce5e21a4e0db2aecdcc8894f7c69cc6","zerovk":0,"tsv_num":"11","tsv_ctg1":"38946","tsv_rk":"2","is_discovery_product":true,"discovery_recall":true,"is_discovery_rough_force":true,"pv_14d":52623,"mt_cs":0,"his":{"gul":1048576,"gul_prd":0,"feed":2031616,"search":1245184,"all":2031616},"lctg_buy_interval":99999999999,"ctg_buy_interval":99999999999,"is_roa_user":0,"is_bypass":1,"is_refresh":0,"in_rebuy_time":0,"spw":{"enable":1,"pv":0.0,"ctr":0.0,"ctcvr":0.0,"gpm":0.0,"peo":1,"rough":8.625829119306566e-116},"impr":{"size":0,"d_size":0,"fg_size":0},"llt":1777475839,"lctg_refund_interval":99999999999,"ctg_refund_interval":99999999999,"gover_labels_enable":"843950873081776","gover_labels_enable_new":"0","gover_labels_enable_v2":"17873387485379544","gover_labels_enable_v2_new":"0","ctxp":"{\\"source\\":\\"homepage_top_tab\\",\\"enter_from\\":\\"homepage_top_tab\\"}","pctx":{"sid":0},"ecpp_pool_tag":0,"item_sub_type":1,"quota":{"predict_reason":2,"rough_predict_reason":2,"strong_rough_predict_reason":1,"strong_rough_predict":0,"recall":0,"rough":958,"predict":150},"ecom_center_traffic_identity":"zytest:1111868:zytest_ecom_center"}'
      },
      "ab_values": {
        "wind_vane_freq": "2",
        "new_recommend_position": true,
        "is_ui_optimize": true,
        "mix_wind_vane": "3",
        "live_card_enlarge_play_area": true,
        "feedback_educate_page_limit": "xtab_homepage",
        "customer_new_style": "1",
        "use_design_v2025": {
          "enable": true,
          "borderless": false,
          "revert_444": false
        },
        "ai_guide_type": "",
        "new_padding": true,
        "hide_video_time": true,
        "product_with_x": true,
        "activity_hot_zone": 1,
        "is_ui_compact": true,
        "new_font_family": true,
        "use_design_x_version": "2",
        "selling_point_update": 1,
        "use_new_grid": "1",
        "show_interaction_trigger": "deep_feed",
        "use_feed_back_button": "",
        "card_border_radius_adjust": 2,
        "info_font_adjust": 4
      },
      "log_data": [],
      "log_meta_set": {
        "meta_list": [{
          "event_name": "ecom_card_auto_play",
          "params_from_client": [{
            "client_key": "index",
            "report_key": "outflow_order"
          }, {
            "client_key": "page_name",
            "report_key": "page_name"
          }, {
            "client_key": "enter_from",
            "report_key": "previous_page"
          }, {
            "client_key": "recommend_info",
            "report_key": "recommend_info"
          }]
        }, {
          "event_name": "close_ecom_dislike",
          "params_from_client": [{
            "client_key": "index",
            "report_key": "outflow_order"
          }, {
            "client_key": "page_name",
            "report_key": "page_name"
          }, {
            "client_key": "enter_from",
            "report_key": "previous_page"
          }, {
            "client_key": "recommend_info",
            "report_key": "recommend_info"
          }, {
            "client_key": "close_type",
            "report_key": "close_type"
          }]
        }, {
          "event_name": "click_ecom_card",
          "params_from_client": [{
            "client_key": "index",
            "report_key": "display_rank"
          }, {
            "client_key": "index",
            "report_key": "outflow_order"
          }, {
            "client_key": "page_name",
            "report_key": "page_name"
          }, {
            "client_key": "enter_from",
            "report_key": "previous_page"
          }, {
            "client_key": "page_name",
            "report_key": "source_page"
          }, {
            "client_key": "click_area",
            "report_key": "click_area"
          }, {
            "client_key": "recommend_info",
            "report_key": "recommend_info"
          }, {
            "client_key": "interaction_feed_state",
            "report_key": "interaction_feed_state"
          }]
        }, {
          "event_name": "click_product",
          "params_from_client": [{
            "client_key": "index",
            "report_key": "display_rank"
          }, {
            "client_key": "index",
            "report_key": "outflow_order"
          }, {
            "client_key": "page_name",
            "report_key": "page_name"
          }, {
            "client_key": "enter_from",
            "report_key": "previous_page"
          }, {
            "client_key": "page_name",
            "report_key": "source_page"
          }, {
            "client_key": "click_area",
            "report_key": "click_area"
          }, {
            "client_key": "recommend_info",
            "report_key": "recommend_info"
          }]
        }]
      },
      "data": {
        "absolute_common": null,
        "ad_common": null,
        "suspend_area_common": null,
        "title_common": {
          "content": {
            "content": "[Brand] iPhoneAir/iPhone17ProMax Ultra-thin Magnetic Phone Case",
            "style": "color:#161823;font-size:26rpx;font-weight:500;line-height:36rpx;max-height:36rpx;"
          },
          "tag": {
            "url": "network_address",
            "style": "width:68rpx;height:26rpx;margin-right:8rpx;margin-bottom:4rpx;border-radius:4rpx 4rpx 4rpx 4rpx;"
          },
          "max_line": 3,
          "narrow_char": {
            "style": "color:#161823;font-size:28rpx;font-weight:500;line-height:36rpx;max-height:36rpx;"
          },
          "style": "margin-top:-3rpx;margin-bottom:4rpx;order:-2;flex-shrink:0;"
        },
        "cover_common": {
          "cover": {
            "src": "network_address",
            "pierced_src": "network_address",
            "style": "background-color:rgba(22, 24, 35, 0.08);aspect-ratio:1;"
          }
        },
        "price_common": {
          "price": {
            "symbol": {
              "style": "color:#ff003c;font-size:26rpx;font-weight:400;font-family:ZYNumberABC-Medium;margin-left:2rpx;"
            },
            "integers": {
              "content": "499",
              "style": "color:#ff003c;font-size:36rpx;font-weight:500;font-family:ZYNumberABC-Bold;"
            }
          },
          "sales_count": {
            "content": "246 sold",
            "style": "color:rgba(22, 24, 35, 0.5);font-size:24rpx;font-weight:400;margin-left:8rpx;vertical-align:2rpx;"
          },
          "style": "height:36rpx;margin-top:1rpx;"
        },
        "rec_reason_common": {
          "rec_reasons": [{
            "content": {
              "content": "10k good reviews",
              "style": "color:#b48352;font-size:24rpx;font-weight:400;line-height:26rpx;"
            },
            "style": "margin-left:0rpx;",
            "enhance": {}
          }, {
            "content": {
              "content": "Next day delivery",
              "style": "color:#00BC35;font-size:24rpx;font-weight:400;line-height:26rpx;"
            },
            "style": "margin-left:12rpx;",
            "enhance": {}
          }],
          "schema_key": "card_click",
          "log_keys": ["click_ecom_card", "click_product", "selling_pt_click", "click_store_entrance"],
          "type": "",
          "countdown": {},
          "animation": {},
          "style": "height:32rpx;margin-top:3rpx;margin-bottom:4rpx;order:-1;display:flex;flex-direction:row;flex-shrink:0;"
        },
        "feedback_common": {
          "dislike_infos": {
            "8": {
              "name": "This type of product"
            }
          },
          "associated_link": "network_address",
          "product_id": "3815523199945408710",
          "ecom_type": "product",
          "cover_params": {
            "is_aigc_pic": "0",
            "bigsale_border_template_id": "",
            "uri": "ecom-shop-material/png_m_e81bc22e0b1ddc9b0e2604ce2196ab0b_sx_928954_www800-800",
            "page_num": "1",
            "is_long_pic": "0",
            "is_smart_pic": "0",
            "prod_pic_type": "11_pic",
            "prod_smart_pic_type": "",
            "cover_recommend_uri": "",
            "is_first_detail_pic": "1"
          },
          "business_type": "mall",
          "show_associated_link": true,
          "style_sub_key": "2"
        },
        "insert_common": {
          "product_id": "3815523199945408710",
          "is_other_channel": "",
          "trigger_ecom_type": "product",
          "wind_vane_params": '{"id":"3815523199945408710","id_type":2,"section_id":"favorite_section","item_type":51007,"item_id":"3815523199945408710","is_ad":false,"ab_value":1,"is_live_request":false,"ecom_scene_id":"1099,1031","word_reason":"product","from_gid":"3815523199945408710","trigger_ecom_type":"product","image_url":"network_address","product_id":"3815523199945408710","recommend_info":"","mix_wind_vane":"3","enable_insert_instant":true,"instant_enhanced_params":{"insert_delay":{"android":"0.2","ios":"0"},"insert_below":true},"use_same_height":"","use_design_x_version":"2","lego_same_height":false,"lego_magnify_style":false,"use_new_grid":"1","instant_retail_rec":"0","rec_unique_id":"20260507110832F4AC18941C37D2B1F3F2_3815523199945408710","enable_insert_wind_vane":false,"use_small_font_size":false,"new_padding":true,"high_value":false,"feed_ui_style":0,"home_ui_style":0,"use_design_v2025":{"enable":true,"borderless":false,"revert_444":false}}',
          "disable_vane": false,
          "product_use_new_wind_vane": true,
          "bff_ecom_scene_id": "1099,1031",
          "type": 2
        },
        "media_wrapper": {
          "enable": true,
          "media_type": "product",
          "media_id": "product-media-wrapper3815523199945408710",
          "transition_element_id": "trainsition-view3815523199945408710",
          "transition_item_id": "3815523199945408710",
          "media_name": "product-media-wrapper"
        }
      },
      "coverDynamicInfo": {
        "isVideoPlaying": false,
        "isSwiperPlaying": false,
        "isMediaReported": false,
        "usePiercedSrc": true
      },
      "absoluteDynamicInfo": {
        "show_border_time": 0
      },
      "adDynamicInfo": {
        "showMask": false,
        "showAnimation": false,
        "isMaskShowed": false,
        "animateStage": 0
      },
      "ec_lynx_props_extra": {
        "is_first_show": 1,
        "ab_values": {
          "new_recommend_position": true,
          "is_ui_optimize": true,
          "mix_wind_vane": "3",
          "live_card_enlarge_play_area": true,
          "feedback_educate_page_limit": "xtab_homepage",
          "customer_new_style": "1",
          "use_design_v2025": {
            "enable": true,
            "borderless": false,
            "revert_444": false
          },
          "ai_guide_type": "",
          "new_padding": true,
          "hide_video_time": true,
          "product_with_x": true,
          "activity_hot_zone": 1,
          "is_ui_compact": true,
          "new_font_family": true,
          "use_design_x_version": "2",
          "selling_point_update": 1,
          "use_new_grid": "1",
          "show_interaction_trigger": "deep_feed",
          "use_feed_back_button": "",
          "card_border_radius_adjust": 2,
          "info_font_adjust": 4
        },
        "track_data_key": "eb9549b2-4fde-429a-8f21-531f861ed985",
        "track_common_data": {
          "tab_order": "0",
          "tab_id": "0",
          "video_guide_mall": '"chnid":"homepage_hot","content_recommend_behavior_level":"0","xtab_nn_version":"aggr_v3_ecom_guide_to_mall_lib_1.0_on_1_raise_on_99_99","is_from_ad":0,"ad_type":0,"ecom_act_after_video_version":"19493","product_potential_behavior_level":"0","ecom_content_type":"other_video","group_id":"7636411698845024625","pitaya_analyze_interest_on_good_version":"41123","tomall_source":"0","request_id":"20260506200229E285D69C0ED3151E5321","cid":""',
          "guess_favorite_impression_flag": 1,
          "product_id": "3815523199945408710",
          "page_name": "xtab_homepage",
          "tab_name": "tab_recommend",
          "request_id": "20260507110832F4AC18941C37D2B1F3F2"
        },
        "recommend_info": '{"uid":110893068884,"feeds_user_group_label":"churn","online_predict_model":"ecom_center_rank_models_31123_v2247_r18423414_0","gid":3815523199945408710,"ghs5":"wtw6h","chr":0,"bat":0,"har":0,"ohr":0,"adtp":"poi_address","lbsn":0,"astn":0,"stn":0,"is_seckill":0,"is_eff_pass":10,"is_dis_pass":10,"prd":{"gcp":1,"style_id_v2":301737,"pnum":1,"pname":"xtab_homepage","rsn":"vk_discover:75:0.306956","new":0,"thr_new":0,"lv4s":2,"ppsv3":0.8642403584261071,"pptv3":2,"ppsv2":0.6351975210988682,"ppsv4":0.0,"pptv4s2":0,"pptv4s7":0,"pptv4s2n":0,"pptv4s7n":0,"seedsaab":-1.0,"seedsababc":-1.0,"seedosaab":-1.0,"seedosababc":-1.0,"seedcs":-1.0,"epad":0,"bfn":0,"goods_id":"3815523199945408710","product_pic_simid_v5":"3815523199945408710","product_spu_sim_id_v2":"3775542601562849411","okr_brand_id":"83898716","g_goods_pred_leaf_ctg":"22229","simprd_simhash_1":"1700862118627455534","simprd_simhash_2":"2324749316526887555","simprd_simhash_3":"3717563022327799083","simprd_simhash_4":"1941967124953143618","mcsgb":0,"isagg":0,"shop1_gb":9,"shop2_gb":0,"shop3_gb":0,"p_cvr_thre_m":"1.0","p_cvr_thre":"1.0","prd_price_avg":0.0,"idx":37,"rf_idx":0,"nfs_1h":"1:0:0:0:0:0","nfs_24h":"3:0:0:0:0:0","nfs_all":"4:0:0:0:0:0","pr_idx":98,"rp_idx":248,"rp_score":9.826852209609,"rp_cascade_gmv":0.05166995897889137,"rp_cascade_ctcvr":0.015614282339811325,"rp_cascade_ctr":0.35396620631217957,"rp_cascade":0.3888445794582367,"rp_order":0.0013659077230840921,"rp_buy":0.1478581726551056,"rp_click":0.06761271506547928,"impr":0,"act":0,"14d_lctg":4,"in_180d_lctg":1,"in_all_lctg":1,"in_180d_ctg1":1,"qversion":961623798681960745,"vk_discover":"0.306956","simid":3815523199945408710,"spu_simid":0,"white_simid":"b1cdce32ce3238c9","lctg":22229,"ctg1":20000,"f_lctg":1000007175,"ai_ctg":20000,"gfprice":399,"dprice":39900,"aprice":44285,"sprice":0.0,"fprice":39900,"market_price":39900,"ai_lctg":22229,"pr_cmp_v3_rt":0,"brand":83898716,"brand_pl":"","trig_clk":"51,18,28,20018_14,22229_6,38946_34600_906203","trig_buy":"1,1,1,20018_1,29193_1,20018_29193_5314110","ei_basic_fea":"google Pixel 6_1_WiFi_0.93_1","u_exit_prob":"-1.0","u_exit_ts":"0","pred_sc":"2042.945134","ctr_term":"55.591416","ctcvr_term":"2.054541","gmv_term":"3.882158","pctr_rank_idx_score":"0.000000","pcvr2_rank_idx_score":"0.000000","price_rank_idx_score":"0.000000","intent_rank_idx_score":"0.000000","cascade_rank_idx_score":"0.000000","pctr_beta":"4720.000000","pcvr2_beta":"1400000.000000","ori_pctr":"0.011778","ori_pcvr2":"0.000091","ctcvr_term1":"1.604833","price_beta":"6630.000000","price_alhpa":"0.000000","card_price":"399.000000","finepred_price":"399.000000","gmv_term1":"0.007600","ori_ctr":"0.011778","ori_cvr":"0.000091","fatigue":"0.000000","trg_related":"0.000000","ori_gcvr":"0.000000","explosive_subsidy_subscribe_flag":0,"remove_high_dislike_product_user_tag":0,"ori_cu":"2042.945134","ccr_mean":"0.015664","ccr_std":"0.025638","pccr_mean":"0.018747","pqccr_mean":"0.011699","is_cascade_topk":0,"rp_rank_idx":43,"cascade_rough_predict_score":133.5287942260038,"is_active_refresh":false,"rctr":"0.004829","rcvr":"0.000933","rcascade_ctr":"0.038002","rcascade_ctcvr":"0.000580","rough_cas_impression":"0.015261","kgplv":7,"buy_cnt_14d":0,"gover_labels_new":"0","app_punish_1128_mask_part1":"0","app_punish_1128_mask_part2":"0","app_punish_zhili_ready":true,"is_exchange":"0","expid":"6","cs_expid":"1","exchange_status":0,"rt_exchange_status":0,"is_in_assign":"false","asset_template_id":"","mixed_coupon_discount_value":100,"coupon_uplift":0.0,"delta_cu":"0.0000000000","longtime_cu":"0.0000000000","exchange_std_price":0.0,"exchange_sku_price":0.0,"rt_exchange_sku_price":0.0,"pv_coef":"5.0000000000000000","if_quit":false,"cid":0,"ecom_center_boost_bitmap":131537,"ecom_center_exchange_7d_pv":3,"ecom_center_exchange_7d_buy":0,"rprice":"399.000000","rscore":"459.7598140001788920","rscore_ori":"0.0000000000","rscore_ab":"0.0000000000","rbst":"1.0000000000000000","merge_pos":213,"is_pc_ept":0,"rgh_cas_idx":-1,"rgh_mtl_idx":-1,"rgh_cas_miss10":0,"rgh_cas_miss50":20,"rgh_mtl_miss10":0,"rgh_mtl_miss50":0,"feature_price_for_discount":39900,"mixed_coupon_meta_id":"","goods_roi2_ecpm_score":"2015.220744","goods_roi2_origin_ecpm":"5038.051859","fine_sort_index":2,"index":1,"tscore1":"0.512334","prd_clk":"0.011778","ord_sub":"0.000091","cvr_split2":"0.000091","pccr":"0.006564","pqccr":"0.004944","pbcmt":"0.960361","pfeedback":"0.010570","dcvr":"0.000419","p_rebuy":"0.003185","p_rebuy_gmv":"61.906250","p_aft_rfd":"0.026964","p_ctg1_rfd":"0.000380","order_high":"0.009596","order_good":"0.010902","bst":1.0,"be_bst":1.0,"price_alpha":"0.000000"},"is_allowance":0,"pure_impr_cnt":0,"pure_impr_win":1,"pred":{"clevel_x_pay":"L5_0"},"cs_s":0,"is_cs":0,"mcs_s":0,"baopin_status":1,"shop_cs":{"cs_s":0,"cs_val":0.0},"chnid":1111868,"chnid_bak":0,"ent":"guess_u_like","appid":1128,"cs_force_insert":{"u_cs_ctr":0.0,"u_cs_cvr":0.0,"u_cs_ctcvr":0.0,"u_cs_cu":0.0},"rt":1,"traffic_from":"homepage_top_tab","tfs":"","req_id":"20260507110832F4AC18941C37D2B1F3F2","loc":310110,"tab":0,"sub_req_id":"B6FCB","sess":"0ce5e21a4e0db2aecdcc8894f7c69cc6","zerovk":0,"tsv_num":"11","tsv_ctg1":"38946","tsv_rk":"2","is_discovery_product":true,"discovery_recall":true,"is_discovery_rough_force":true,"pv_14d":52623,"mt_cs":0,"his":{"gul":1048576,"gul_prd":0,"feed":2031616,"search":1245184,"all":2031616},"lctg_buy_interval":99999999999,"ctg_buy_interval":99999999999,"is_roa_user":0,"is_bypass":1,"is_refresh":0,"in_rebuy_time":0,"spw":{"enable":1,"pv":0.0,"ctr":0.0,"ctcvr":0.0,"gpm":0.0,"peo":1,"rough":8.625829119306566e-116},"impr":{"size":0,"d_size":0,"fg_size":0},"llt":1777475839,"lctg_refund_interval":99999999999,"ctg_refund_interval":99999999999,"gover_labels_enable":"843950873081776","gover_labels_enable_new":"0","gover_labels_enable_v2":"17873387485379544","gover_labels_enable_v2_new":"0","ctxp":"{\\"source\\":\\"homepage_top_tab\\",\\"enter_from\\":\\"homepage_top_tab\\"}","pctx":{"sid":0},"ecpp_pool_tag":0,"item_sub_type":1,"quota":{"predict_reason":2,"rough_predict_reason":2,"strong_rough_predict_reason":1,"strong_rough_predict":0,"recall":0,"rough":958,"predict":150},"ecom_center_traffic_identity":"zytest:1111868:zytest_ecom_center"}',
        "ecom_scene_id": "1099,1031"
      },
      "feedbackVisible": false,
      "feedbackData": [{
        "text": "Not interested",
        "icon": "network_address",
        "type": 1
      }, {
        "text": "Block this type",
        "icon": "network_address",
        "type": 2
      }, {
        "text": "Already bought",
        "icon": "network_address",
        "type": 4
      }, {
        "text": "Image discomfort",
        "icon": "network_address",
        "type": 3
      }, {
        "text": "Unreasonable price",
        "icon": "network_address",
        "type": 6,
        "abbr": "Unreasonable price"
      }, {
        "text": "Privacy concern",
        "icon": "network_address",
        "type": 5,
        "abbr": "Privacy"
      }, {
        "text": "Suspected counterfeit",
        "icon": "network_address",
        "type": 7,
        "abbr": "Counterfeit"
      }],
      "feedbackLimit": 4,
      "need_show_ad_tag_ui": false,
      "is_instant_rec_card": "0",
      "instantRecommendListNum": 0,
      "need_animation": false,
      "interactionFeedState": 0,
      "bff_ecom_scene_id": "1099,1031",
      "isOneOfMultiInstant": false,
      "wind_vane_data": {},
      "schema": [{
        "key": "card_click",
        "link": "network_address",
        "btm": "c9582.d6555",
        "bcm": {
          "type": "product",
          "bst_group_type": "product",
          "channel_id": "1111868",
          "product_id": "3815523199945408710",
          "request_id": "20260507110832F4AC18941C37D2B1F3F2",
          "summary_track_id": "202605071108332566A003000000000003b36f46662468e0",
          "user_id": "110893068884",
          "sub_request_id": "B6FCB",
          "group_id": "3815523199945408710"
        },
        "extra": {
          "context_provider": {
            "isAD": 0
          },
          "container_id": "page >>> #product-media-wrapper3815523199945408710",
          "scene": "go_product_detail"
        }
      }],
      "isCacheData": 0,
      "autoOpenDetailAlreadyDone": 0,
      "canBackToGylTopAfterAutoOpenDetail": 0,
      "couponBarLoading": false,
      "isPriceScrolledV2": false,
      "hideSimilar": false,
      "dislikeEntryVisible": false,
      "latestEnterDeepFeed": false,
      "cardHeight": 0,
      "cardIndex": 0,
      "canBackToGylAfterNativeOpenDetail": true,
      "canReportClickWhenOpenDetailInNaitve": true,
      "recReasonReplacementTags": {},
      "recReasonCountStartTs": 0,
      "find_similar": {},
      "readOnly": true
    }
  },
  "components/sku_product/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/sku_bubble/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/horizontal_skus/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/cover_common/index": {
    "data": {},
    "properties": {
      "data": {},
      "coverId": "",
      "usePiercedSrc": true,
      "isPlaying": false,
      "isCacheData": 0,
      "imageMonitorTag": {},
      "isNA": false
    }
  },
  "components/video_engine/index": {
    "data": {
      "videoEngineStatus": "init"
    },
    "properties": {
      "playing": false,
      "src": "",
      "id": "guessyoulike-x-video-engine-1778507280588",
      "tag": "mall",
      "subTag": "mall_native_hevc",
      "startTime": 0,
      "endTime": 0
    }
  },
  "components/mall_cover_common/index": {
    "data": {},
    "properties": {
      "isCacheData": 0,
      "dynamicInfo": {},
      "info": {},
      "logData": [],
      "ecLynxPropsExtra": {},
      "imageMonitorTag": {}
    }
  },
  "components/absolute_common/index": {
    "data": {
      "isDataReady": false,
      "showMask": true,
      "zytest1Loading": true,
      "isLegou": false
    },
    "properties": {
      "isCacheData": 0,
      "info": {},
      "dynamicInfo": {},
      "dislikeEntryVisible": false,
      "videoAnimation": {}
    }
  },
  "components/ad_common/index": {
    "data": {},
    "properties": {
      "isCacheData": 0,
      "dynamicInfo": {},
      "info": {}
    }
  },
  "components/feedback_ui_common/index": {
    "data": {
      "onMoreList": false
    },
    "properties": {
      "limit": 4,
      "items": [],
      "showAssociated": false
    }
  },
  "components/feedback_common/index": {
    "data": {
      "index": -1
    },
    "properties": {
      "limit": 0,
      "items": [],
      "item_id": "",
      "item_type": 0,
      "showAssociated": false,
      "associatedLink": "",
      "ecomType": "",
      "logId": "",
      "recommendInfo": "",
      "ad": {},
      "isAD": false,
      "abValues": {},
      "bffEcomSceneId": "",
      "customRemoveSelf": false,
      "isLife": false,
      "coverTrack": {},
      "commonTrack": {},
      "businessType": "mall",
      "showMore": true
    }
  },
  "components/wind_vane/wind_vane": {
    "data": {
      "animationStyle": ""
    },
    "properties": {
      "data": {},
      "abValues": {},
      "scrollToTopData": {}
    }
  },
  "components/countdown/index": {
    "data": {
      "hour": "00",
      "minute": "00",
      "second": "00",
      "isReady": false,
      "isCompleted": false
    },
    "properties": {
      "duration": -1,
      "textStyle": {},
      "useAdaptiveScheduler": false,
      "isPageVisible": true
    }
  },
  "components/recommend_reason_common/index": {
    "data": {
      "visible": true,
      "countdown": {},
      "countdownUpdate": false,
      "countdownComplete": false
    },
    "properties": {
      "isNA": false,
      "data": {}
    }
  },
  "components/media_wrapper/index": {
    "data": {},
    "properties": {
      "enable": false,
      "mediatype": "",
      "name": "",
      "mediaName": "",
      "mediaid": "",
      "transition_element_id": "",
      "transition_item_id": ""
    }
  },
  "components/image/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/splitor/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/text/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/tag/index": {
    "data": {
      "newData": {},
      "ui_items": {},
      "backgroundStyle": ""
    },
    "properties": {
      "data": {},
      "height": 0,
      "customClassName": "",
      "ratio": 1,
      "isRpx": false
    }
  },
  "components/scroll_to_number/index": {
    "data": {
      "isDataReady": false,
      "showAni": true
    },
    "properties": {
      "num": {
        "start_num": 0,
        "end_num": 9,
        "num_list": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
        "duration": 300,
        "delay": 0,
        "font_size": 18,
        "style": ""
      },
      "scrollShown": false
    }
  },
  "components/price_common/index": {
    "data": {},
    "properties": {
      "data": {},
      "isPriceScrolledV2": false,
      "isCacheData": 0
    }
  },
  "components/suspend_common/index": {
    "data": {
      "isCouponBarCountDownUpdated": false,
      "isCouponBarCountDownCompleted": false
    },
    "properties": {
      "data": {},
      "type": 0,
      "couponBarLoading": false
    }
  },
  "components/title_common/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/sku_pics/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/find_similar/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  },
  "components/belt_common/index": {
    "data": {},
    "properties": {
      "data": {}
    }
  }
};
function __CreateView(a){}
function __SetAttribute(a,b,c) {}
function __AppendElement(a,b) {}
function __CreateIf(a) {}
function __UpdateIfNodeIndex(a,b){}
function __CreateText(a) {}
function __CreateElement(a,b) {}
function __CreateImage(a) {}
function __AddEvent(a,b,c,d) {}
function __SetID(a,b) {}
function __CreateFor(a) {}
function _GetLength(a) {}
function __UpdateForChildCount(a,b) {}
function __AddDataset(a,b,c) {}
$renderComponents["components/absolute_common/index"] = {
  variables: ["info", "isCacheData", "dynamicInfo", "isLegou", "isDataReady", "dislikeEntryVisible", "showMask", "zytest1Loading", "videoAnimation"],
  varUpdateState: [],
  update_318dca8_33: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.corner_mark) == null ? undefined : _b.text) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n34 = $update2 ? $lepusGetElementRefByLepusID("view", 34) : null;
            let $temp2 = $update2;
            if (!$n34) {
              $update2 = false;
              $n34 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n34, 34, "view");
              __SetStyleObject($n34, [getClassStyleIndex("corner-mark", "21316000")]);
              __AppendElement($parent, $n34);
            }
            {
              let $n35 = $update2 ? $lepusGetElementRefByLepusID("text", 35) : null;
              let $temp3 = $update2;
              if (!$n35) {
                $update2 = false;
                $n35 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n35, 35, "text");
                __SetStyleObject($n35, [getClassStyleIndex("corner-mark-text", "21316000")]);
                __AppendElement($n34, $n35);
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let $value = (_d = (_c = $data.info) == null ? undefined : _c.corner_mark) == null ? undefined : _d.text;
                  if (!$update2 || $value !== ((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.corner_mark) == null ? undefined : _f.text)) {
                    __SetAttribute($n35, "text", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_37: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.just_see) == null ? undefined : _b.logo) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n38 = $update2 ? $lepusGetElementRefByLepusID("view", 38) : null;
            let $temp2 = $update2;
            if (!$n38) {
              $update2 = false;
              $n38 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n38, 38, "view");
              __AppendElement($parent, $n38);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n38, [getClassStyleIndex("just_see_see_logo", "21316000"), parseStyleStringToObject((_e = (_d = (_c = $data.info) == null ? undefined : _c.just_see) == null ? undefined : _d.logo) == null ? undefined : _e.style)]);
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $template_update = $update2;
            let $n39 = $update2 ? $lepusGetElementRefByLepusID("if", 39) : null;
            if (!$n39) {
              $update2 = false;
              $n39 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n39, 39, "if");
              __AppendElement($parent, $n39);
            }
            $renderComponents[$path].update_318dca8_39($lepusComponent, $n39, $data, $props, $update2, $slotUpdate);
            $update2 = $template_update;
          }
          $update2 = _$temp;
        }
      }
    }
  },
  update_318dca8_39: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.info) == null ? undefined : _a.just_see) == null ? undefined : _b.text) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n40 = $update2 ? $lepusGetElementRefByLepusID("view", 40) : null;
          let $temp2 = $update2;
          if (!$n40) {
            $update2 = false;
            $n40 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n40, 40, "view");
            __SetStyleObject($n40, [getClassStyleIndex("just_see_see", "21316000")]);
            __AppendElement($parent, $n40);
          }
          {
            let $n41 = $update2 ? $lepusGetElementRefByLepusID("image", 41) : null;
            let $temp3 = $update2;
            if (!$n41) {
              $update2 = false;
              $n41 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n41, 41, "image");
              __SetStyleObject($n41, [getClassStyleIndex("just_see_see-icon", "21316000")]);
              __SetAttribute($n41, "src", "network_address");
              __SetAttribute($n41, "skip-redirection", "true");
              __AppendElement($n40, $n41);
            }
            $update2 = $temp3;
          }
          {
            let $n42 = $update2 ? $lepusGetElementRefByLepusID("text", 42) : null;
            let _$temp2 = $update2;
            if (!$n42) {
              $update2 = false;
              $n42 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n42, 42, "text");
              __SetStyleObject($n42, [getClassStyleIndex("just_see_see-text", "21316000")]);
              __AppendElement($n40, $n42);
            }
            {
              if (!$update2 || $renderComponents["components/absolute_common/index"].varUpdateState[0]) {
                let $value = (_d = (_c = $data.info) == null ? undefined : _c.just_see) == null ? undefined : _d.text;
                if (!$update2 || $value !== ((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.just_see) == null ? undefined : _f.text)) {
                  __SetAttribute($n42, "text", $value);
                }
              }
            }
            $update2 = _$temp2;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_318dca8_44: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.decoration) == null ? undefined : _b.text) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n45 = $update2 ? $lepusGetElementRefByLepusID("view", 45) : null;
            let $temp2 = $update2;
            if (!$n45) {
              $update2 = false;
              $n45 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n45, 45, "view");
              __AppendElement($parent, $n45);
            }
            {
              let $value = "decoration " + (((_d = (_c = $data.info) == null ? undefined : _c.decoration) == null ? undefined : _d.type) === "collocate" ? "decoration-collocate" : "");
              if (!$update2 || $value !== "decoration " + (((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.decoration) == null ? undefined : _f.type) === "collocate" ? "decoration-collocate" : "")) {
                __SetStyleObject($n45, [getClassStyleIndex("decoration " + (((_h = (_g = $data.info) == null ? undefined : _g.decoration) == null ? undefined : _h.type) === "collocate" ? "decoration-collocate" : ""), "21316000")]);
              }
            }
            {
              let $template_update = $update2;
              let $n46 = $update2 ? $lepusGetElementRefByLepusID("if", 46) : null;
              if (!$n46) {
                $update2 = false;
                $n46 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n46, 46, "if");
                __AppendElement($n45, $n46);
              }
              $renderComponents[$path].update_318dca8_46($lepusComponent, $n46, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let $n48 = $update2 ? $lepusGetElementRefByLepusID("text", 48) : null;
              let $temp3 = $update2;
              if (!$n48) {
                $update2 = false;
                $n48 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n48, 48, "text");
                __AppendElement($n45, $n48);
              }
              {
                let _$value = "decoration-text " + (((_j = (_i = $data.info) == null ? undefined : _i.decoration) == null ? undefined : _j.type) === "collocate" ? "decoration-text-collocate" : "");
                if (!$update2 || _$value !== "decoration-text " + (((_l = (_k = $lepusComponent._data.info) == null ? undefined : _k.decoration) == null ? undefined : _l.type) === "collocate" ? "decoration-text-collocate" : "")) {
                  __SetStyleObject($n48, [getClassStyleIndex("decoration-text " + (((_n = (_m = $data.info) == null ? undefined : _m.decoration) == null ? undefined : _n.type) === "collocate" ? "decoration-text-collocate" : ""), "21316000")]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let _$value2 = (_p = (_o = $data.info) == null ? undefined : _o.decoration) == null ? undefined : _p.text;
                  if (!$update2 || _$value2 !== ((_r = (_q = $lepusComponent._data.info) == null ? undefined : _q.decoration) == null ? undefined : _r.text)) {
                    __SetAttribute($n48, "text", _$value2);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_46: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.info) == null ? undefined : _a.decoration) == null ? undefined : _b.icon) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n47 = $update2 ? $lepusGetElementRefByLepusID("image", 47) : null;
          let $temp2 = $update2;
          if (!$n47) {
            $update2 = false;
            $n47 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n47, 47, "image");
            __SetAttribute($n47, "skip-redirection", true);
            __SetAttribute($n47, "mode", "aspectFit");
            __SetAttribute($n47, "flatten", true);
            __AppendElement($parent, $n47);
          }
          if (!$update2 || $renderComponents["components/absolute_common/index"].varUpdateState[0]) {
            {
              let $value = "decoration-icon " + (((_d = (_c = $data.info) == null ? undefined : _c.decoration) == null ? undefined : _d.type) === "collocate" ? "decoration-icon-collocate" : "");
              if (!$update2 || $value !== "decoration-icon " + (((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.decoration) == null ? undefined : _f.type) === "collocate" ? "decoration-icon-collocate" : "")) {
                __SetStyleObject($n47, [getClassStyleIndex("decoration-icon " + (((_h = (_g = $data.info) == null ? undefined : _g.decoration) == null ? undefined : _h.type) === "collocate" ? "decoration-icon-collocate" : ""), "21316000")]);
              }
            }
            {
              let _$value3 = (_j = (_i = $data.info) == null ? undefined : _i.decoration) == null ? undefined : _j.icon;
              if (!$update2 || _$value3 !== ((_l = (_k = $lepusComponent._data.info) == null ? undefined : _k.decoration) == null ? undefined : _l.icon)) {
                __SetAttribute($n47, "src", _$value3);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_318dca8_56: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_c = (_b = (_a = $data.info) == null ? undefined : _a.ad) == null ? undefined : _b.tag) == null ? undefined : _c.text) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n57 = $update2 ? $lepusGetElementRefByLepusID("view", 57) : null;
            let $temp2 = $update2;
            if (!$n57) {
              $update2 = false;
              $n57 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n57, 57, "view");
              __SetStyleObject($n57, [getClassStyleIndex("ad-tag", "21316000")]);
              __AppendElement($parent, $n57);
            }
            {
              let $n58 = $update2 ? $lepusGetElementRefByLepusID("text", 58) : null;
              let $temp3 = $update2;
              if (!$n58) {
                $update2 = false;
                $n58 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n58, 58, "text");
                __SetStyleObject($n58, [getClassStyleIndex("ad-tag-text", "21316000")]);
                __AppendElement($n57, $n58);
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let $value = (_f = (_e = (_d = $data.info) == null ? undefined : _d.ad) == null ? undefined : _e.tag) == null ? undefined : _f.text;
                  if (!$update2 || $value !== ((_i = (_h = (_g = $lepusComponent._data.info) == null ? undefined : _g.ad) == null ? undefined : _h.tag) == null ? undefined : _i.text)) {
                    __SetAttribute($n58, "text", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_60: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/absolute_common/index"].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.ad) == null ? undefined : _b.show_pot) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n61 = $update2 ? $lepusGetElementRefByLepusID("view", 61) : null;
            let $temp2 = $update2;
            if (!$n61) {
              $update2 = false;
              $n61 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n61, 61, "view");
              __SetStyleObject($n61, [getClassStyleIndex("ad-pot", "21316000")]);
              __AppendElement($parent, $n61);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_64: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.video_tag) == null ? undefined : _b.icon) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n65 = $update2 ? $lepusGetElementRefByLepusID("view", 65) : null;
            let $temp2 = $update2;
            if (!$n65) {
              $update2 = false;
              $n65 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n65, 65, "view");
              __SetStyleObject($n65, [getClassStyleIndex("product-video-tag", "21316000")]);
              __AppendElement($parent, $n65);
            }
            {
              let $n66 = $update2 ? $lepusGetElementRefByLepusID("image", 66) : null;
              let $temp3 = $update2;
              if (!$n66) {
                $update2 = false;
                $n66 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n66, 66, "image");
                __SetStyleObject($n66, [getClassStyleIndex("product-video-tag-icon", "21316000")]);
                __SetAttribute($n66, "skip-redirection", true);
                __SetAttribute($n66, "mode", "aspectFit");
                __SetAttribute($n66, "accessibility-element", "false");
                __SetAttribute($n66, "flatten", true);
                __AppendElement($n65, $n66);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  let $value = (_d = (_c = $data.info) == null ? undefined : _c.video_tag) == null ? undefined : _d.icon;
                  if (!$update2 || $value !== ((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.video_tag) == null ? undefined : _f.icon)) {
                    __SetAttribute($n66, "src", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_67: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.cover_border_image) == null ? undefined : _b.image[0]) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n68 = $update2 ? $lepusGetElementRefByLepusID("view", 68) : null;
            let $temp2 = $update2;
            if (!$n68) {
              $update2 = false;
              $n68 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n68, 68, "view");
              __SetStyleObject($n68, [getClassStyleIndex("product-cover-border-image-container", "21316000")]);
              __AppendElement($parent, $n68);
            }
            {
              let $n69 = $update2 ? $lepusGetElementRefByLepusID("image", 69) : null;
              let $temp3 = $update2;
              if (!$n69) {
                $update2 = false;
                $n69 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n69, 69, "image");
                __SetStyleObject($n69, [getClassStyleIndex("product-cover-border-image", "21316000")]);
                __SetAttribute($n69, "skip-redirection", true);
                __SetAttribute($n69, "mode", "aspectFit");
                __SetAttribute($n69, "flatten", true);
                __AppendElement($n68, $n69);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  let $value = (_d = (_c = $data.info) == null ? undefined : _c.cover_border_image) == null ? undefined : _d.image[0];
                  if (!$update2 || $value !== ((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.cover_border_image) == null ? undefined : _f.image[0])) {
                    __SetAttribute($n69, "src", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_77: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.live_tag) == null ? undefined : _b.text) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n78 = $update2 ? $lepusGetElementRefByLepusID("view", 78) : null;
            let $temp2 = $update2;
            if (!$n78) {
              $update2 = false;
              $n78 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n78, 78, "view");
              __SetStyleObject($n78, [getClassStyleIndex("product_with_live_tag", "21316000")]);
              __AppendElement($parent, $n78);
            }
            {
              let $n79 = $update2 ? $lepusGetElementRefByLepusID("image", 79) : null;
              let $temp3 = $update2;
              if (!$n79) {
                $update2 = false;
                $n79 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n79, 79, "image");
                __SetStyleObject($n79, [getClassStyleIndex("product_with_live_tag-icon", "21316000")]);
                __SetAttribute($n79, "skip-redirection", "true");
                __AppendElement($n78, $n79);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  let $value = $data.info.live_tag.icon;
                  if (!$update2 || $value !== $lepusComponent._data.info.live_tag.icon) {
                    __SetAttribute($n79, "src", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            {
              let $n80 = $update2 ? $lepusGetElementRefByLepusID("text", 80) : null;
              let _$temp3 = $update2;
              if (!$n80) {
                $update2 = false;
                $n80 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n80, 80, "text");
                __SetStyleObject($n80, [getClassStyleIndex("product_with_live_tag-text", "21316000")]);
                __AppendElement($n78, $n80);
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let _$value4 = $data.info.live_tag.text;
                  if (!$update2 || _$value4 !== $lepusComponent._data.info.live_tag.text) {
                    __SetAttribute($n80, "text", _$value4);
                  }
                }
              }
              $update2 = _$temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_111: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_a = $data.info.card_mask_lottie) == null ? undefined : _a.url) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n112 = $update2 ? $lepusGetElementRefByLepusID("zytest1-view", 112) : null;
            let $temp2 = $update2;
            if (!$n112) {
              $update2 = false;
              $n112 = __CreateElement("zytest1-view", $currentComponentId);
              $lepusStoreElementRefByLepusID($n112, 112, "zytest1-view");
              __SetStyleObject($n112, [15, 17, 18, 5, 16]);
              __SetAttribute($n112, "autoplay", true);
              __SetAttribute($n112, "loop", false);
              __SetAttribute($n112, "objectfit", "cover");
              __SetAttribute($n112, "ignore-attach-status", true);
              __SetAttribute($n112, "ignore-lynx-lifecycle", true);
              __SetAttribute($n112, "user-interaction-enabled", false);
              __AppendElement($parent, $n112);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                let $value = (_b = $data.info.card_mask_lottie) == null ? undefined : _b.url;
                if (!$update2 || $value !== ((_c = $lepusComponent._data.info.card_mask_lottie) == null ? undefined : _c.url)) {
                  __SetAttribute($n112, "src", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_3dbf210_52: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_b = (_a = $data.info) == null ? undefined : _a.lottie_tag) == null ? undefined : _b.url) && !((_c = $data.dynamicInfo) == null ? undefined : _c.lottieHasPlayed)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n53 = $update2 ? $lepusGetElementRefByLepusID("lottie-view", 53) : null;
          let $temp2 = $update2;
          if (!$n53) {
            $update2 = false;
            $n53 = __CreateElement("lottie-view", $currentComponentId);
            $lepusStoreElementRefByLepusID($n53, 53, "lottie-view");
            __SetAttribute($n53, "autoplay", true);
            __AddEvent($n53, "bindEvent", "completion", "handleLottieTagCompletion");
            __AppendElement($parent, $n53);
          }
          if (!$update2 || $renderComponents["components/absolute_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n53, [parseStyleStringToObject((_e = (_d = $data.info) == null ? undefined : _d.lottie_tag) == null ? undefined : _e.style)]);
            }
            {
              let $value = (_g = (_f = $data.info) == null ? undefined : _f.lottie_tag) == null ? undefined : _g.url;
              if (!$update2 || $value !== ((_i = (_h = $lepusComponent._data.info) == null ? undefined : _h.lottie_tag) == null ? undefined : _i.url)) {
                __SetAttribute($n53, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_3dbf210_54: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    let $path = "components/absolute_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.info) == null ? undefined : _a.lottie_tag) == null ? undefined : _b.static_image) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n55 = $update2 ? $lepusGetElementRefByLepusID("image", 55) : null;
          let $temp2 = $update2;
          if (!$n55) {
            $update2 = false;
            $n55 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n55, 55, "image");
            __SetAttribute($n55, "skip-redirection", true);
            __SetAttribute($n55, "flatten", true);
            __AppendElement($parent, $n55);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[2]) {
            {
              __SetStyleObject($n55, [15, ((_d = (_c = $data.info) == null ? undefined : _c.lottie_tag) == null ? undefined : _d.url) && !((_e = $data.dynamicInfo) == null ? undefined : _e.lottieHasPlayed) ? [126] : [], parseStyleStringToObject((_g = (_f = $data.info) == null ? undefined : _f.lottie_tag) == null ? undefined : _g.style)]);
            }
            {
              let $value = (_i = (_h = $data.info) == null ? undefined : _h.lottie_tag) == null ? undefined : _i.static_image;
              if (!$update2 || $value !== ((_k = (_j = $lepusComponent._data.info) == null ? undefined : _j.lottie_tag) == null ? undefined : _k.static_image)) {
                __SetAttribute($n55, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_280618_50: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!$data.isCacheData && ((_a = $data.info) == null ? undefined : _a.lottie_tag) && ((_c = (_b = $data.info) == null ? undefined : _b.lottie_tag) == null ? undefined : _c.style)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n51 = $update2 ? $lepusGetElementRefByLepusID("view", 51) : null;
            let $temp2 = $update2;
            if (!$n51) {
              $update2 = false;
              $n51 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n51, 51, "view");
              __SetStyleObject($n51, [5, 16]);
              __AppendElement($parent, $n51);
            }
            {
              let $template_update = $update2;
              let $n52 = $update2 ? $lepusGetElementRefByLepusID("if", 52) : null;
              if (!$n52) {
                $update2 = false;
                $n52 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n52, 52, "if");
                __AppendElement($n51, $n52);
              }
              $renderComponents[$path].update_3dbf210_52($lepusComponent, $n52, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let _$template_update = $update2;
              let $n54 = $update2 ? $lepusGetElementRefByLepusID("if", 54) : null;
              if (!$n54) {
                $update2 = false;
                $n54 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n54, 54, "if");
                __AppendElement($n51, $n54);
              }
              $renderComponents[$path].update_3dbf210_54($lepusComponent, $n54, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_280618_62: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.info) == null ? undefined : _a.border) == null ? undefined : _b.status) === 1 && showBorder((_c = $data.dynamicInfo) == null ? undefined : _c.show_border_time) && !$data.isCacheData) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n63 = $update2 ? $lepusGetElementRefByLepusID("view", 63) : null;
            let $temp2 = $update2;
            if (!$n63) {
              $update2 = false;
              $n63 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n63, 63, "view");
              __SetAttribute($n63, "user-interaction-enabled", false);
              __AppendElement($parent, $n63);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n63, [getClassStyleIndex("border", "21316000"), parseStyleStringToObject((_e = (_d = $data.info) == null ? undefined : _d.border) == null ? undefined : _e.style)]);
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_1f5adb0_70: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s, _t, _u, _v;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[3]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.coupon) == null ? undefined : _b.integers) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n71 = $update2 ? $lepusGetElementRefByLepusID("view", 71) : null;
            let $temp2 = $update2;
            if (!$n71) {
              $update2 = false;
              $n71 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n71, 71, "view");
              __SetAttribute($n71, "clip-radius", "true");
              __SetAttribute($n71, "user-interaction-enabled", false);
              __AppendElement($parent, $n71);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n71, [getClassStyleIndex("product-coupon", "21316000"), parseStyleStringToObject((_d = (_c = $data.info) == null ? undefined : _c.coupon) == null ? undefined : _d.style)]);
              }
            }
            {
              let $n72 = $update2 ? $lepusGetElementRefByLepusID("view", 72) : null;
              let $temp3 = $update2;
              if (!$n72) {
                $update2 = false;
                $n72 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n72, 72, "view");
                __SetAttribute($n72, "clip-radius", "true");
                __AppendElement($n71, $n72);
              }
              {
                let $value = $data.isLegou ? "product-coupon-container-legou" : "product-coupon-container";
                if (!$update2 || $value !== ($lepusComponent._data.isLegou ? "product-coupon-container-legou" : "product-coupon-container")) {
                  __SetStyleObject($n72, [getClassStyleIndex($data.isLegou ? "product-coupon-container-legou" : "product-coupon-container", "21316000")]);
                }
              }
              {
                let $n73 = $update2 ? $lepusGetElementRefByLepusID("text", 73) : null;
                let $temp4 = $update2;
                if (!$n73) {
                  $update2 = false;
                  $n73 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n73, 73, "text");
                  __AppendElement($n72, $n73);
                }
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  {
                    __SetStyleObject($n73, [getClassStyleIndex("product-coupon-symbol", "21316000"), parseStyleStringToObject((_g = (_f = (_e = $data.info) == null ? undefined : _e.coupon) == null ? undefined : _f.symbol) == null ? undefined : _g.style)]);
                  }
                }
                {
                  if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                    let _$value5 = (_j = (_i = (_h = $data.info) == null ? undefined : _h.coupon) == null ? undefined : _i.symbol) == null ? undefined : _j.content;
                    if (!$update2 || _$value5 !== ((_m = (_l = (_k = $lepusComponent._data.info) == null ? undefined : _k.coupon) == null ? undefined : _l.symbol) == null ? undefined : _m.content)) {
                      __SetAttribute($n73, "text", _$value5);
                    }
                  }
                }
                $update2 = $temp4;
              }
              {
                let $n75 = $update2 ? $lepusGetElementRefByLepusID("text", 75) : null;
                let _$temp4 = $update2;
                if (!$n75) {
                  $update2 = false;
                  $n75 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n75, 75, "text");
                  __AppendElement($n72, $n75);
                }
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  {
                    __SetStyleObject($n75, [getClassStyleIndex("product-coupon-integer", "21316000"), parseStyleStringToObject((_p = (_o = (_n = $data.info) == null ? undefined : _n.coupon) == null ? undefined : _o.integers) == null ? undefined : _p.style)]);
                  }
                }
                {
                  if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                    let _$value6 = (_s = (_r = (_q = $data.info) == null ? undefined : _q.coupon) == null ? undefined : _r.integers) == null ? undefined : _s.content;
                    if (!$update2 || _$value6 !== ((_v = (_u = (_t = $lepusComponent._data.info) == null ? undefined : _t.coupon) == null ? undefined : _u.integers) == null ? undefined : _v.content)) {
                      __SetAttribute($n75, "text", _$value6);
                    }
                  }
                }
                $update2 = _$temp4;
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_1280d10_82: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s, _t, _u, _v, _w, _x, _y, _z, _A, _B, _C, _D, _E, _F;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[4]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_c = (_b = (_a = $data.info) == null ? undefined : _a.red_packet) == null ? undefined : _b.integers) == null ? undefined : _c.content) && $data.isDataReady) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n83 = $update2 ? $lepusGetElementRefByLepusID("view", 83) : null;
            let $temp2 = $update2;
            if (!$n83) {
              $update2 = false;
              $n83 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n83, 83, "view");
              __SetAttribute($n83, "clip-radius", "true");
              __SetAttribute($n83, "user-interaction-enabled", false);
              __SetAttribute($n83, "overlap", false);
              __AppendElement($parent, $n83);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n83, [getClassStyleIndex("product-redPacket", "21316000"), 367, 368, parseStyleStringToObject((_e = (_d = $data.info) == null ? undefined : _d.red_packet) == null ? undefined : _e.style)]);
              }
            }
            {
              let $n84 = $update2 ? $lepusGetElementRefByLepusID("view", 84) : null;
              let $temp3 = $update2;
              if (!$n84) {
                $update2 = false;
                $n84 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n84, 84, "view");
                __SetStyleObject($n84, [getClassStyleIndex("product-redPacket-wrapper", "21316000")]);
                __SetAttribute($n84, "clip-radius", "true");
                __AppendElement($n83, $n84);
              }
              {
                let $n85 = $update2 ? $lepusGetElementRefByLepusID("view", 85) : null;
                let $temp4 = $update2;
                if (!$n85) {
                  $update2 = false;
                  $n85 = __CreateView($currentComponentId);
                  $lepusStoreElementRefByLepusID($n85, 85, "view");
                  __SetStyleObject($n85, [getClassStyleIndex("product-redPacket-container", "21316000")]);
                  __SetAttribute($n85, "clip-radius", "true");
                  __AppendElement($n84, $n85);
                }
                {
                  let $n86 = $update2 ? $lepusGetElementRefByLepusID("text", 86) : null;
                  let $temp5 = $update2;
                  if (!$n86) {
                    $update2 = false;
                    $n86 = __CreateText($currentComponentId);
                    $lepusStoreElementRefByLepusID($n86, 86, "text");
                    __AppendElement($n85, $n86);
                  }
                  if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                    {
                      __SetStyleObject($n86, [getClassStyleIndex("product-redPacket-num", "21316000"), parseStyleStringToObject((_h = (_g = (_f = $data.info) == null ? undefined : _f.red_packet) == null ? undefined : _g.integers) == null ? undefined : _h.style)]);
                    }
                  }
                  {
                    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                      let $value = (_k = (_j = (_i = $data.info) == null ? undefined : _i.red_packet) == null ? undefined : _j.integers) == null ? undefined : _k.content;
                      if (!$update2 || $value !== ((_n = (_m = (_l = $lepusComponent._data.info) == null ? undefined : _l.red_packet) == null ? undefined : _m.integers) == null ? undefined : _n.content)) {
                        __SetAttribute($n86, "text", $value);
                      }
                    }
                  }
                  $update2 = $temp5;
                }
                {
                  let $n88 = $update2 ? $lepusGetElementRefByLepusID("text", 88) : null;
                  let _$temp5 = $update2;
                  if (!$n88) {
                    $update2 = false;
                    $n88 = __CreateText($currentComponentId);
                    $lepusStoreElementRefByLepusID($n88, 88, "text");
                    __AppendElement($n85, $n88);
                  }
                  if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                    {
                      __SetStyleObject($n88, [getClassStyleIndex("product-redPacket-symbol", "21316000"), parseStyleStringToObject((_q = (_p = (_o = $data.info) == null ? undefined : _o.red_packet) == null ? undefined : _p.symbol) == null ? undefined : _q.style)]);
                    }
                  }
                  {
                    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                      let _$value7 = (_t = (_s = (_r = $data.info) == null ? undefined : _r.red_packet) == null ? undefined : _s.symbol) == null ? undefined : _t.content;
                      if (!$update2 || _$value7 !== ((_w = (_v = (_u = $lepusComponent._data.info) == null ? undefined : _u.red_packet) == null ? undefined : _v.symbol) == null ? undefined : _w.content)) {
                        __SetAttribute($n88, "text", _$value7);
                      }
                    }
                  }
                  $update2 = _$temp5;
                }
                $update2 = $temp4;
              }
              {
                let $n90 = $update2 ? $lepusGetElementRefByLepusID("text", 90) : null;
                let _$temp6 = $update2;
                if (!$n90) {
                  $update2 = false;
                  $n90 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n90, 90, "text");
                  __AppendElement($n84, $n90);
                }
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  {
                    __SetStyleObject($n90, [getClassStyleIndex("product-redPacket-subtitle", "21316000"), parseStyleStringToObject((_z = (_y = (_x = $data.info) == null ? undefined : _x.red_packet) == null ? undefined : _y.subtitle) == null ? undefined : _z.style)]);
                  }
                }
                {
                  if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                    let _$value8 = (_C = (_B = (_A = $data.info) == null ? undefined : _A.red_packet) == null ? undefined : _B.subtitle) == null ? undefined : _C.content;
                    if (!$update2 || _$value8 !== ((_F = (_E = (_D = $lepusComponent._data.info) == null ? undefined : _D.red_packet) == null ? undefined : _E.subtitle) == null ? undefined : _F.content)) {
                      __SetAttribute($n90, "text", _$value8);
                    }
                  }
                }
                $update2 = _$temp6;
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_464060_92: function ($lepusComponent, $parent, $data, $props, $update2) {
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[5] || $renderComponents[$path].varUpdateState[6]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.dislikeEntryVisible) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n93 = $update2 ? $lepusGetElementRefByLepusID("view", 93) : null;
            let $temp2 = $update2;
            if (!$n93) {
              $update2 = false;
              $n93 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n93, 93, "view");
              __SetAttribute($n93, "user-interaction-enabled", false);
              __AppendElement($parent, $n93);
            }
            {
              let $value = "dislike-entry " + ($data.showMask ? "mask-animation-show" : "mask-animation-hide");
              if (!$update2 || $value !== "dislike-entry " + ($lepusComponent._data.showMask ? "mask-animation-show" : "mask-animation-hide")) {
                __SetStyleObject($n93, [getClassStyleIndex("dislike-entry " + ($data.showMask ? "mask-animation-show" : "mask-animation-hide"), "21316000")]);
              }
            }
            {
              let $n94 = $update2 ? $lepusGetElementRefByLepusID("view", 94) : null;
              let $temp3 = $update2;
              if (!$n94) {
                $update2 = false;
                $n94 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n94, 94, "view");
                __AppendElement($n93, $n94);
              }
              {
                let _$value9 = "dislike-title " + ($data.showMask ? "title-show" : "title-hide");
                if (!$update2 || _$value9 !== "dislike-title " + ($lepusComponent._data.showMask ? "title-show" : "title-hide")) {
                  __SetStyleObject($n94, [getClassStyleIndex("dislike-title " + ($data.showMask ? "title-show" : "title-hide"), "21316000")]);
                }
              }
              {
                let $n95 = $update2 ? $lepusGetElementRefByLepusID("text", 95) : null;
                let $temp4 = $update2;
                if (!$n95) {
                  $update2 = false;
                  $n95 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n95, 95, "text");
                  __SetStyleObject($n95, [getClassStyleIndex("dislike-entry-text", "21316000")]);
                  __AppendElement($n94, $n95);
                }
                __SetAttribute($n95, "text", "If you dislike the product,");
                $update2 = $temp4;
              }
              $update2 = $temp3;
            }
            {
              let $n97 = $update2 ? $lepusGetElementRefByLepusID("view", 97) : null;
              let _$temp7 = $update2;
              if (!$n97) {
                $update2 = false;
                $n97 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n97, 97, "view");
                __AppendElement($n93, $n97);
              }
              {
                let _$value10 = "dislike-subtitle " + ($data.showMask ? "title-show" : "title-hide");
                if (!$update2 || _$value10 !== "dislike-subtitle " + ($lepusComponent._data.showMask ? "title-show" : "title-hide")) {
                  __SetStyleObject($n97, [getClassStyleIndex("dislike-subtitle " + ($data.showMask ? "title-show" : "title-hide"), "21316000")]);
                }
              }
              {
                let $n98 = $update2 ? $lepusGetElementRefByLepusID("text", 98) : null;
                let _$temp8 = $update2;
                if (!$n98) {
                  $update2 = false;
                  $n98 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n98, 98, "text");
                  __SetStyleObject($n98, [getClassStyleIndex("dislike-entry-text", "21316000")]);
                  __AppendElement($n97, $n98);
                }
                __SetAttribute($n98, "text", "you can");
                $update2 = _$temp8;
              }
              {
                let $n100 = $update2 ? $lepusGetElementRefByLepusID("text", 100) : null;
                let _$temp9 = $update2;
                if (!$n100) {
                  $update2 = false;
                  $n100 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n100, 100, "text");
                  __SetStyleObject($n100, [getClassStyleIndex("dislike-entry-text text-yellow", "21316000")]);
                  __AppendElement($n97, $n100);
                }
                __SetAttribute($n100, "text", "long press");
                $update2 = _$temp9;
              }
              {
                let $n102 = $update2 ? $lepusGetElementRefByLepusID("text", 102) : null;
                let _$temp10 = $update2;
                if (!$n102) {
                  $update2 = false;
                  $n102 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n102, 102, "text");
                  __SetStyleObject($n102, [getClassStyleIndex("dislike-entry-text text", "21316000")]);
                  __AppendElement($n97, $n102);
                }
                __SetAttribute($n102, "text", "to give feedback");
                $update2 = _$temp10;
              }
              $update2 = _$temp7;
            }
            {
              let $n104 = $update2 ? $lepusGetElementRefByLepusID("lottie-view", 104) : null;
              let _$temp11 = $update2;
              if (!$n104) {
                $update2 = false;
                $n104 = __CreateElement("lottie-view", $currentComponentId);
                $lepusStoreElementRefByLepusID($n104, 104, "lottie-view");
                __SetStyleObject($n104, [getClassStyleIndex("dislike-lottie", "21316000")]);
                __SetAttribute($n104, "src", "network_address");
                __SetAttribute($n104, "autoplay", false);
                __SetAttribute($n104, "objectfit", "cover");
                __SetID($n104, "dislike-lottie");
                __AddEvent($n104, "bindEvent", "ready", "handleLottieReady");
                __AddEvent($n104, "bindEvent", "completion", "handleLottieCompletion");
                __AddEvent($n104, "bindEvent", "error", "handleLottieError");
                __AddEvent($n104, "bindEvent", "cancel", "handleLottieError");
                __AppendElement($n93, $n104);
              }
              $update2 = _$temp11;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_364e580_109: function ($lepusComponent, $parent, $data, $props, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.videoAnimation.silenceImage) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n110 = $update2 ? $lepusGetElementRefByLepusID("image", 110) : null;
          let $temp2 = $update2;
          if (!$n110) {
            $update2 = false;
            $n110 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n110, 110, "image");
            __SetAttribute($n110, "skip-redirection", true);
            __SetAttribute($n110, "user-interaction-enabled", false);
            __AppendElement($parent, $n110);
          }
          if (!$update2 || $renderComponents["components/absolute_common/index"].varUpdateState[8]) {
            {
              __SetStyleObject($n110, [15, 24, 25, 371, 59, parseStyleStringToObject(getSilenceImageStyle(!$data.videoAnimation.id))]);
            }
            {
              let $value = $data.videoAnimation.silenceImage;
              if (!$update2 || $value !== $lepusComponent._data.videoAnimation.silenceImage) {
                __SetAttribute($n110, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_321a6a8_107: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let $path = "components/absolute_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.videoAnimation.id) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n108 = $update2 ? $lepusGetElementRefByLepusID("zytest1-view", 108) : null;
          let $temp2 = $update2;
          if (!$n108) {
            $update2 = false;
            $n108 = __CreateElement("zytest1-view", $currentComponentId);
            $lepusStoreElementRefByLepusID($n108, 108, "zytest1-view");
            __SetStyleObject($n108, [15, 17, 18, 5, 16]);
            __SetAttribute($n108, "autoplay", false);
            __SetAttribute($n108, "loop", false);
            __SetAttribute($n108, "objectfit", "contain");
            __SetAttribute($n108, "ignore-attach-status", true);
            __SetAttribute($n108, "ignore-lynx-lifecycle", true);
            __SetAttribute($n108, "user-interaction-enabled", false);
            __AddEvent($n108, "bindEvent", "ready", "handleVideoAnimationReady");
            __AddEvent($n108, "bindEvent", "error", "handleVideoAnimationError");
            __AppendElement($parent, $n108);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[8]) {
            {
              let $value = getVideoAnimationImage($data.info.video_animation);
              if (!$update2 || $value !== undefined) {
                __SetAttribute($n108, "src-polyfill", $value);
              }
            }
            {
              let _$value11 = $data.videoAnimation.lottie;
              if (!$update2 || _$value11 !== $lepusComponent._data.videoAnimation.lottie) {
                __SetAttribute($n108, "src-format", _$value11);
              }
            }
            {
              let _$value12 = $data.videoAnimation.id;
              if (!$update2 || _$value12 !== $lepusComponent._data.videoAnimation.id) {
                __SetID($n108, _$value12);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp12 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n109 = $update2 ? $lepusGetElementRefByLepusID("if", 109) : null;
          if (!$n109) {
            $update2 = false;
            $n109 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n109, 109, "if");
            __AppendElement($parent, $n109);
          }
          $renderComponents[$path].update_364e580_109($lepusComponent, $n109, $data, $props, $update2, $slotUpdate);
          $update2 = $template_update;
        }
        $update2 = _$temp12;
      }
    }
  },
  update_ad66a0_105: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a;
    let $path = "components/absolute_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[7] || $renderComponents[$path].varUpdateState[8]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_a = $data.info) == null ? undefined : _a.video_animation) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n106 = $update2 ? $lepusGetElementRefByLepusID("view", 106) : null;
            let $temp2 = $update2;
            if (!$n106) {
              $update2 = false;
              $n106 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n106, 106, "view");
              __SetAttribute($n106, "user-interaction-enabled", false);
              __AppendElement($parent, $n106);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[7] || $renderComponents[$path].varUpdateState[8]) {
              {
                __SetStyleObject($n106, [5, 369, 370, parseStyleStringToObject(getVideoAnimationStyle($data.zytest1Loading && $data.videoAnimation.id))]);
              }
            }
            {
              let $template_update = $update2;
              let $n107 = $update2 ? $lepusGetElementRefByLepusID("if", 107) : null;
              if (!$n107) {
                $update2 = false;
                $n107 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n107, 107, "if");
                __AppendElement($n106, $n107);
              }
              $renderComponents[$path].update_321a6a8_107($lepusComponent, $n107, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/absolute_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 32);
    let $n33 = $lepusGetElementRefByLepusID("if", 33);
    $renderComponents[$path].update_318dca8_33($lepusComponent, $n33, $data, $props, $update2, $slotUpdate);
    let $n37 = $lepusGetElementRefByLepusID("if", 37);
    $renderComponents[$path].update_318dca8_37($lepusComponent, $n37, $data, $props, $update2, $slotUpdate);
    let $n44 = $lepusGetElementRefByLepusID("if", 44);
    $renderComponents[$path].update_318dca8_44($lepusComponent, $n44, $data, $props, $update2, $slotUpdate);
    let $n50 = $lepusGetElementRefByLepusID("if", 50);
    $renderComponents[$path].update_280618_50($lepusComponent, $n50, $data, $props, $update2, $slotUpdate);
    let $n56 = $lepusGetElementRefByLepusID("if", 56);
    $renderComponents[$path].update_318dca8_56($lepusComponent, $n56, $data, $props, $update2, $slotUpdate);
    let $n60 = $lepusGetElementRefByLepusID("if", 60);
    $renderComponents[$path].update_318dca8_60($lepusComponent, $n60, $data, $props, $update2, $slotUpdate);
    let $n62 = $lepusGetElementRefByLepusID("if", 62);
    $renderComponents[$path].update_280618_62($lepusComponent, $n62, $data, $props, $update2, $slotUpdate);
    let $n64 = $lepusGetElementRefByLepusID("if", 64);
    $renderComponents[$path].update_318dca8_64($lepusComponent, $n64, $data, $props, $update2, $slotUpdate);
    let $n67 = $lepusGetElementRefByLepusID("if", 67);
    $renderComponents[$path].update_318dca8_67($lepusComponent, $n67, $data, $props, $update2, $slotUpdate);
    let $n70 = $lepusGetElementRefByLepusID("if", 70);
    $renderComponents[$path].update_1f5adb0_70($lepusComponent, $n70, $data, $props, $update2, $slotUpdate);
    let $n77 = $lepusGetElementRefByLepusID("if", 77);
    $renderComponents[$path].update_318dca8_77($lepusComponent, $n77, $data, $props, $update2, $slotUpdate);
    let $n82 = $lepusGetElementRefByLepusID("if", 82);
    $renderComponents[$path].update_1280d10_82($lepusComponent, $n82, $data, $props, $update2, $slotUpdate);
    let $n92 = $lepusGetElementRefByLepusID("if", 92);
    $renderComponents[$path].update_464060_92($lepusComponent, $n92, $data, $props, $update2, $slotUpdate);
    let $n105 = $lepusGetElementRefByLepusID("if", 105);
    $renderComponents[$path].update_ad66a0_105($lepusComponent, $n105, $data, $props, $update2, $slotUpdate);
    let $n111 = $lepusGetElementRefByLepusID("if", 111);
    $renderComponents[$path].update_318dca8_111($lepusComponent, $n111, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $path = "components/absolute_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n32 = __CreateView($currentComponentId);
      __SetStyleObject($n32, [getClassStyleIndex(getClassName("container"), "21316000")]);
      __SetAttribute($n32, "user-interaction-enabled", false);
      __AppendElement($component, $n32);
      let $n33 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n33, 33, "if");
      __AppendElement($n32, $n33);
      $renderComponents[$path].update_318dca8_33($lepusComponent, $n33, $data, $props, $update2, $slotUpdate);
      let $n37 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n37, 37, "if");
      __AppendElement($n32, $n37);
      $renderComponents[$path].update_318dca8_37($lepusComponent, $n37, $data, $props, $update2, $slotUpdate);
      let $n44 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n44, 44, "if");
      __AppendElement($n32, $n44);
      $renderComponents[$path].update_318dca8_44($lepusComponent, $n44, $data, $props, $update2, $slotUpdate);
      let $n50 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n50, 50, "if");
      __AppendElement($n32, $n50);
      $renderComponents[$path].update_280618_50($lepusComponent, $n50, $data, $props, $update2, $slotUpdate);
      let $n56 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n56, 56, "if");
      __AppendElement($n32, $n56);
      $renderComponents[$path].update_318dca8_56($lepusComponent, $n56, $data, $props, $update2, $slotUpdate);
      let $n60 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n60, 60, "if");
      __AppendElement($n32, $n60);
      $renderComponents[$path].update_318dca8_60($lepusComponent, $n60, $data, $props, $update2, $slotUpdate);
      let $n62 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n62, 62, "if");
      __AppendElement($n32, $n62);
      $renderComponents[$path].update_280618_62($lepusComponent, $n62, $data, $props, $update2, $slotUpdate);
      let $n64 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n64, 64, "if");
      __AppendElement($n32, $n64);
      $renderComponents[$path].update_318dca8_64($lepusComponent, $n64, $data, $props, $update2, $slotUpdate);
      let $n67 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n67, 67, "if");
      __AppendElement($n32, $n67);
      $renderComponents[$path].update_318dca8_67($lepusComponent, $n67, $data, $props, $update2, $slotUpdate);
      let $n70 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n70, 70, "if");
      __AppendElement($n32, $n70);
      $renderComponents[$path].update_1f5adb0_70($lepusComponent, $n70, $data, $props, $update2, $slotUpdate);
      let $n77 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n77, 77, "if");
      __AppendElement($n32, $n77);
      $renderComponents[$path].update_318dca8_77($lepusComponent, $n77, $data, $props, $update2, $slotUpdate);
      let $n82 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n82, 82, "if");
      __AppendElement($n32, $n82);
      $renderComponents[$path].update_1280d10_82($lepusComponent, $n82, $data, $props, $update2, $slotUpdate);
      let $n92 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n92, 92, "if");
      __AppendElement($n32, $n92);
      $renderComponents[$path].update_464060_92($lepusComponent, $n92, $data, $props, $update2, $slotUpdate);
      let $n105 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n105, 105, "if");
      __AppendElement($n32, $n105);
      $renderComponents[$path].update_ad66a0_105($lepusComponent, $n105, $data, $props, $update2, $slotUpdate);
      let $n111 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n111, 111, "if");
      __AppendElement($n32, $n111);
      $renderComponents[$path].update_318dca8_111($lepusComponent, $n111, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/ad_common/index"] = {
  variables: ["info", "dynamicInfo"],
  varUpdateState: [],
  update_3dbf210_114: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i;
    let $path = "components/ad_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.info) == null ? undefined : _a.animation) == null ? undefined : _b.src) && ((_c = $data.dynamicInfo) == null ? undefined : _c.showAnimation)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n115 = $update2 ? $lepusGetElementRefByLepusID("view", 115) : null;
            let $temp2 = $update2;
            if (!$n115) {
              $update2 = false;
              $n115 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n115, 115, "view");
              __AppendElement($parent, $n115);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n115, [getClassStyleIndex("animation", "12169000"), parseStyleStringToObject((_e = (_d = $data.info) == null ? undefined : _d.animation) == null ? undefined : _e.style)]);
              }
            }
            {
              let $n116 = $update2 ? $lepusGetElementRefByLepusID("zytest1-view", 116) : null;
              let $temp3 = $update2;
              if (!$n116) {
                $update2 = false;
                $n116 = __CreateElement("zytest1-view", $currentComponentId);
                $lepusStoreElementRefByLepusID($n116, 116, "zytest1-view");
                __SetStyleObject($n116, [getClassStyleIndex("animation-lottie", "12169000")]);
                __SetAttribute($n116, "start-frame", 0);
                __SetAttribute($n116, "end-frame", 57);
                __SetAttribute($n116, "loop", false);
                __SetAttribute($n116, "flatten", false);
                __SetAttribute($n116, "autoplay", true);
                __SetID($n116, "coupon_lottie");
                __AddEvent($n116, "bindEvent", "completion", "handleLottieCompletion");
                __AppendElement($n115, $n116);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  let $value = (_g = (_f = $data.info) == null ? undefined : _f.animation) == null ? undefined : _g.src;
                  if (!$update2 || $value !== ((_i = (_h = $lepusComponent._data.info) == null ? undefined : _h.animation) == null ? undefined : _i.src)) {
                    __SetAttribute($n116, "src", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_3dbf210_117: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c;
    let $path = "components/ad_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.info) == null ? undefined : _a.mask) == null ? undefined : _b.src) && ((_c = $data.dynamicInfo) == null ? undefined : _c.showMask)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n118 = $update2 ? $lepusGetElementRefByLepusID("view", 118) : null;
            let $temp2 = $update2;
            if (!$n118) {
              $update2 = false;
              $n118 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n118, 118, "view");
              __SetStyleObject($n118, [getClassStyleIndex("mask", "12169000")]);
              __AppendElement($parent, $n118);
            }
            {
              let $template_update = $update2;
              let $n119 = $update2 ? $lepusGetElementRefByLepusID("if", 119) : null;
              if (!$n119) {
                $update2 = false;
                $n119 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n119, 119, "if");
                __AppendElement($n118, $n119);
              }
              $renderComponents[$path].update_318dca8_119($lepusComponent, $n119, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let _$template_update2 = $update2;
              let $n121 = $update2 ? $lepusGetElementRefByLepusID("if", 121) : null;
              if (!$n121) {
                $update2 = false;
                $n121 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n121, 121, "if");
                __AppendElement($n118, $n121);
              }
              $renderComponents[$path].update_318dca8_121($lepusComponent, $n121, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update2;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_3dbf210_123: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    let $path = "components/ad_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!lowerThanVersionCheck(280900) && ((_b = (_a = $data.info) == null ? undefined : _a.mask) == null ? undefined : _b.brand_ad_mask_style) === 2) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n124 = $update2 ? $lepusGetElementRefByLepusID("view", 124) : null;
            let $temp2 = $update2;
            if (!$n124) {
              $update2 = false;
              $n124 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n124, 124, "view");
              __SetStyleObject($n124, [getClassStyleIndex("mask", "12169000")]);
              __AppendElement($parent, $n124);
            }
            {
              let $n125 = $update2 ? $lepusGetElementRefByLepusID("origin-image", 125) : null;
              let $temp3 = $update2;
              if (!$n125) {
                $update2 = false;
                $n125 = __CreateElement("origin-image", $currentComponentId);
                $lepusStoreElementRefByLepusID($n125, 125, "origin-image");
                __SetAttribute($n125, "loop-count", "1");
                __SetAttribute($n125, "autoplay", true);
                __SetAttribute($n125, "skip-redirection", true);
                __SetAttribute($n125, "flatten", "false");
                __SetID($n125, "olympics_mask");
                __AddEvent($n125, "bindEvent", "finalloopcomplete", "handleOlympicEffectCompled");
                __AppendElement($n124, $n125);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1]) {
                {
                  __SetStyleObject($n125, [getClassStyleIndex("mask-image", "12169000"), parseStyleStringToObject(getMaskPosition((_d = (_c = $data.info) == null ? undefined : _c.mask) == null ? undefined : _d.cover_aspect_ratio))]);
                }
                {
                  let $value = (_h = (_f = (_e = $data.info) == null ? undefined : _e.mask) == null ? undefined : _f.olympic_src_list) == null ? undefined : _h[(_g = $data.dynamicInfo) == null ? undefined : _g.animateStage];
                  if (!$update2 || $value !== ((_l = (_j = (_i = $lepusComponent._data.info) == null ? undefined : _i.mask) == null ? undefined : _j.olympic_src_list) == null ? undefined : _l[(_k = $lepusComponent._data.dynamicInfo) == null ? undefined : _k.animateStage])) {
                    __SetAttribute($n125, "src", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_119: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (lowerThanVersionCheck(280900) && ((_b = (_a = $data.info) == null ? undefined : _a.mask) == null ? undefined : _b.brand_ad_mask_style) !== 2) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n120 = $update2 ? $lepusGetElementRefByLepusID("image", 120) : null;
          let $temp2 = $update2;
          if (!$n120) {
            $update2 = false;
            $n120 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n120, 120, "image");
            __SetStyleObject($n120, [getClassStyleIndex("mask-image", "12169000")]);
            __SetAttribute($n120, "loop-count", "1");
            __SetAttribute($n120, "skip-redirection", true);
            __SetAttribute($n120, "mode", "aspectFill");
            __AppendElement($parent, $n120);
          }
          if (!$update2 || $renderComponents["components/ad_common/index"].varUpdateState[0]) {
            {
              let $value = (_d = (_c = $data.info) == null ? undefined : _c.mask) == null ? undefined : _d.src;
              if (!$update2 || $value !== ((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.mask) == null ? undefined : _f.src)) {
                __SetAttribute($n120, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_318dca8_121: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!lowerThanVersionCheck(280900) && ((_b = (_a = $data.info) == null ? undefined : _a.mask) == null ? undefined : _b.brand_ad_mask_style) !== 2) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n122 = $update2 ? $lepusGetElementRefByLepusID("origin-image", 122) : null;
          let $temp2 = $update2;
          if (!$n122) {
            $update2 = false;
            $n122 = __CreateElement("origin-image", $currentComponentId);
            $lepusStoreElementRefByLepusID($n122, 122, "origin-image");
            __SetStyleObject($n122, [getClassStyleIndex("mask-image", "12169000")]);
            __SetAttribute($n122, "loop-count", "1");
            __SetAttribute($n122, "autoplay", true);
            __SetAttribute($n122, "skip-redirection", true);
            __SetAttribute($n122, "mode", "aspectFill");
            __SetAttribute($n122, "flatten", "false");
            __AddEvent($n122, "bindEvent", "startplay", "handleEffectStart");
            __AddEvent($n122, "bindEvent", "finalloopcomplete", "handleEffectCompled");
            __AppendElement($parent, $n122);
          }
          if (!$update2 || $renderComponents["components/ad_common/index"].varUpdateState[0]) {
            {
              let $value = (_d = (_c = $data.info) == null ? undefined : _c.mask) == null ? undefined : _d.src;
              if (!$update2 || $value !== ((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.mask) == null ? undefined : _f.src)) {
                __SetAttribute($n122, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/ad_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 113);
    let $n114 = $lepusGetElementRefByLepusID("if", 114);
    $renderComponents[$path].update_3dbf210_114($lepusComponent, $n114, $data, $props, $update2, $slotUpdate);
    let $n117 = $lepusGetElementRefByLepusID("if", 117);
    $renderComponents[$path].update_3dbf210_117($lepusComponent, $n117, $data, $props, $update2, $slotUpdate);
    let $n123 = $lepusGetElementRefByLepusID("if", 123);
    $renderComponents[$path].update_3dbf210_123($lepusComponent, $n123, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $path = "components/ad_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n113 = __CreateView($currentComponentId);
      __SetStyleObject($n113, [getClassStyleIndex("container", "12169000")]);
      __SetAttribute($n113, "clip-radius", "true");
      __AddEvent($n113, "catchEvent", "tap", "handleContainerClick");
      __AppendElement($component, $n113);
      let $n114 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n114, 114, "if");
      __AppendElement($n113, $n114);
      $renderComponents[$path].update_3dbf210_114($lepusComponent, $n114, $data, $props, $update2, $slotUpdate);
      let $n117 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n117, 117, "if");
      __AppendElement($n113, $n117);
      $renderComponents[$path].update_3dbf210_117($lepusComponent, $n117, $data, $props, $update2, $slotUpdate);
      let $n123 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n123, 123, "if");
      __AppendElement($n113, $n123);
      $renderComponents[$path].update_3dbf210_123($lepusComponent, $n123, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/belt_common/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update_22898d8_129: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/belt_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo = $lepusPushFiberForNode($parent, 129, uniqueId),
          $forLepus = _$lepusPushFiberForNo[0],
          $lastForLepus = _$lepusPushFiberForNo[1];
      let $object = (_a = $data.data) == null ? undefined : _a.icon_left_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.icon_left_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n130 = $update2 ? $lepusGetElementRefByLepusID("image", 130) : null;
          let $temp2 = $update2;
          if (!$n130) {
            $update2 = false;
            $n130 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n130, 130, "image");
            __SetAttribute($n130, "mode", "aspectFill");
            __SetAttribute($n130, "skip-redirection", true);
            __SetAttribute($n130, "accessibility-element", false);
            __AppendElement($parent, $n130);
          }
          __SetStyleObject($n130, [parseStyleStringToObject(item == null ? undefined : item.style)]);
          __SetAttribute($n130, "src", item == null ? undefined : item.url);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_131: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/belt_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo2 = $lepusPushFiberForNode($parent, 131, uniqueId),
          $forLepus = _$lepusPushFiberForNo2[0],
          $lastForLepus = _$lepusPushFiberForNo2[1];
      let $object = (_a = $data.data) == null ? undefined : _a.title_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.title_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n132 = $update2 ? $lepusGetElementRefByLepusID("view", 132) : null;
          let $temp2 = $update2;
          if (!$n132) {
            $update2 = false;
            $n132 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n132, 132, "view");
            __SetStyleObject($n132, [getClassStyleIndex("gyl-belt__main-text", "32391000")]);
            __AppendElement($parent, $n132);
          }
          {
            let $n133 = $update2 ? $lepusGetElementRefByLepusID("text", 133) : null;
            let $temp3 = $update2;
            if (!$n133) {
              $update2 = false;
              $n133 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n133, 133, "text");
              __SetAttribute($n133, "text-maxline", "1");
              __SetAttribute($n133, "accessibility-element", false);
              __AppendElement($n132, $n133);
            }
            __SetStyleObject($n133, [parseStyleStringToObject(item == null ? undefined : item.style)]);
            __SetAttribute($n133, "text", item == null ? undefined : item.content);
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_136: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/belt_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo3 = $lepusPushFiberForNode($parent, 136, uniqueId),
          $forLepus = _$lepusPushFiberForNo3[0],
          $lastForLepus = _$lepusPushFiberForNo3[1];
      let $object = (_a = $data.data) == null ? undefined : _a.action_text_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.action_text_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n137 = $update2 ? $lepusGetElementRefByLepusID("view", 137) : null;
          let $temp2 = $update2;
          if (!$n137) {
            $update2 = false;
            $n137 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n137, 137, "view");
            __SetStyleObject($n137, [getClassStyleIndex("gyl-belt__action-text", "32391000")]);
            __AppendElement($parent, $n137);
          }
          {
            let $n138 = $update2 ? $lepusGetElementRefByLepusID("text", 138) : null;
            let $temp3 = $update2;
            if (!$n138) {
              $update2 = false;
              $n138 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n138, 138, "text");
              __SetAttribute($n138, "skip-redirection", true);
              __SetAttribute($n138, "accessibility-element", "false");
              __AppendElement($n137, $n138);
            }
            __SetStyleObject($n138, [parseStyleStringToObject(item == null ? undefined : item.style)]);
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_140: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/belt_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo4 = $lepusPushFiberForNode($parent, 140, uniqueId),
          $forLepus = _$lepusPushFiberForNo4[0],
          $lastForLepus = _$lepusPushFiberForNo4[1];
      let $object = (_a = $data.data) == null ? undefined : _a.icon_right_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.icon_right_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n141 = $update2 ? $lepusGetElementRefByLepusID("image", 141) : null;
          let $temp2 = $update2;
          if (!$n141) {
            $update2 = false;
            $n141 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n141, 141, "image");
            __SetAttribute($n141, "mode", "aspectFill");
            __SetAttribute($n141, "skip-redirection", true);
            __SetAttribute($n141, "accessibility-element", false);
            __AppendElement($parent, $n141);
          }
          __SetStyleObject($n141, [parseStyleStringToObject(item == null ? undefined : item.style)]);
          __SetAttribute($n141, "src", item == null ? undefined : item.url);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/belt_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 126);
    $lepusGetElementRefByLepusID("view", 127);
    $lepusGetElementRefByLepusID("view", 128);
    let $n129 = $lepusGetElementRefByLepusID("for", 129);
    $renderComponents[$path].update_22898d8_129($lepusComponent, $n129, $data, $props, $update2, $slotUpdate);
    let $n131 = $lepusGetElementRefByLepusID("for", 131);
    $renderComponents[$path].update_22898d8_131($lepusComponent, $n131, $data, $props, $update2, $slotUpdate);
    $lepusGetElementRefByLepusID("view", 135);
    let $n136 = $lepusGetElementRefByLepusID("for", 136);
    $renderComponents[$path].update_22898d8_136($lepusComponent, $n136, $data, $props, $update2, $slotUpdate);
    $lepusGetElementRefByLepusID("view", 139);
    let $n140 = $lepusGetElementRefByLepusID("for", 140);
    $renderComponents[$path].update_22898d8_140($lepusComponent, $n140, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $path = "components/belt_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n126 = __CreateView($currentComponentId);
      __SetStyleObject($n126, [getClassStyleIndex("gyl-belt", "32391000")]);
      __AppendElement($component, $n126);
      let $n127 = __CreateView($currentComponentId);
      __SetStyleObject($n127, [getClassStyleIndex("gyl-belt__main", "32391000")]);
      __AppendElement($n126, $n127);
      let $n128 = __CreateView($currentComponentId);
      __SetStyleObject($n128, [getClassStyleIndex("gyl-belt__main-icon_left", "32391000")]);
      __AppendElement($n127, $n128);
      let $n129 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n129, 129, "for");
      __AppendElement($n128, $n129);
      $renderComponents[$path].update_22898d8_129($lepusComponent, $n129, $data, $props, $update2, $slotUpdate);
      let $n131 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n131, 131, "for");
      __AppendElement($n127, $n131);
      $renderComponents[$path].update_22898d8_131($lepusComponent, $n131, $data, $props, $update2, $slotUpdate);
      let $n135 = __CreateView($currentComponentId);
      __SetStyleObject($n135, [getClassStyleIndex("gyl-belt__action", "32391000")]);
      __AppendElement($n126, $n135);
      let $n136 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n136, 136, "for");
      __AppendElement($n135, $n136);
      $renderComponents[$path].update_22898d8_136($lepusComponent, $n136, $data, $props, $update2, $slotUpdate);
      let $n139 = __CreateView($currentComponentId);
      __SetStyleObject($n139, [getClassStyleIndex("gyl-belt__action-icon_right", "32391000")]);
      __AppendElement($n135, $n139);
      let $n140 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n140, 140, "for");
      __AppendElement($n139, $n140);
      $renderComponents[$path].update_22898d8_140($lepusComponent, $n140, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/countdown/index"] = {
  variables: ["isReady", "textStyle", "hour", "minute", "second"],
  varUpdateState: [],
  update_72ab78_142: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e;
    let $path = "components/countdown/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3] || $renderComponents[$path].varUpdateState[4]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.isReady) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n143 = $update2 ? $lepusGetElementRefByLepusID("view", 143) : null;
            let $temp2 = $update2;
            if (!$n143) {
              $update2 = false;
              $n143 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n143, 143, "view");
              __SetStyleObject($n143, [getClassStyleIndex("gyl-countdown", "20523000")]);
              __AppendElement($parent, $n143);
            }
            {
              let $n144 = $update2 ? $lepusGetElementRefByLepusID("text", 144) : null;
              let $temp3 = $update2;
              if (!$n144) {
                $update2 = false;
                $n144 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n144, 144, "text");
                __SetAttribute($n144, "text-maxline", "1");
                __AppendElement($n143, $n144);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[1]) {
                {
                  __SetStyleObject($n144, [getClassStyleIndex("gyl-countdown__num", "20523000"), parseStyleStringToObject((_a = $data.textStyle) == null ? undefined : _a.style)]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[2]) {
                  let $value = $data.hour;
                  if (!$update2 || $value !== $lepusComponent._data.hour) {
                    __SetAttribute($n144, "text", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            {
              let $n146 = $update2 ? $lepusGetElementRefByLepusID("text", 146) : null;
              let _$temp13 = $update2;
              if (!$n146) {
                $update2 = false;
                $n146 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n146, 146, "text");
                __AppendElement($n143, $n146);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[1]) {
                {
                  __SetStyleObject($n146, [getClassStyleIndex("gyl-countdown__split-text", "20523000"), parseStyleStringToObject((_b = $data.textStyle) == null ? undefined : _b.style)]);
                }
              }
              __SetAttribute($n146, "text", ":");
              $update2 = _$temp13;
            }
            {
              let $n148 = $update2 ? $lepusGetElementRefByLepusID("text", 148) : null;
              let _$temp14 = $update2;
              if (!$n148) {
                $update2 = false;
                $n148 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n148, 148, "text");
                __SetAttribute($n148, "text-maxline", "1");
                __AppendElement($n143, $n148);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[1]) {
                {
                  __SetStyleObject($n148, [getClassStyleIndex("gyl-countdown__num", "20523000"), parseStyleStringToObject((_c = $data.textStyle) == null ? undefined : _c.style)]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
                  let _$value13 = $data.minute;
                  if (!$update2 || _$value13 !== $lepusComponent._data.minute) {
                    __SetAttribute($n148, "text", _$value13);
                  }
                }
              }
              $update2 = _$temp14;
            }
            {
              let $n150 = $update2 ? $lepusGetElementRefByLepusID("text", 150) : null;
              let _$temp15 = $update2;
              if (!$n150) {
                $update2 = false;
                $n150 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n150, 150, "text");
                __AppendElement($n143, $n150);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[1]) {
                {
                  __SetStyleObject($n150, [getClassStyleIndex("gyl-countdown__split-text", "20523000"), parseStyleStringToObject((_d = $data.textStyle) == null ? undefined : _d.style)]);
                }
              }
              __SetAttribute($n150, "text", ":");
              $update2 = _$temp15;
            }
            {
              let $n152 = $update2 ? $lepusGetElementRefByLepusID("text", 152) : null;
              let _$temp16 = $update2;
              if (!$n152) {
                $update2 = false;
                $n152 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n152, 152, "text");
                __SetAttribute($n152, "text-maxline", "1");
                __AppendElement($n143, $n152);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[1]) {
                {
                  __SetStyleObject($n152, [getClassStyleIndex("gyl-countdown__num", "20523000"), parseStyleStringToObject((_e = $data.textStyle) == null ? undefined : _e.style)]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[4]) {
                  let _$value14 = $data.second;
                  if (!$update2 || _$value14 !== $lepusComponent._data.second) {
                    __SetAttribute($n152, "text", _$value14);
                  }
                }
              }
              $update2 = _$temp16;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/countdown/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    let $n142 = $lepusGetElementRefByLepusID("if", 142);
    $renderComponents[$path].update_72ab78_142($lepusComponent, $n142, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n142 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n142, 142, "if");
      __AppendElement($component, $n142);
      $renderComponents["components/countdown/index"].update_72ab78_142($lepusComponent, $n142, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
function __CreateComponent(a,b,c,d,e) {}
$renderComponents["components/cover_common/index"] = {
  variables: ["data", "isNA", "usePiercedSrc", "imageMonitorTag", "coverId", "isPlaying", "isCacheData"],
  varUpdateState: [],
  update_22898d8_157: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/cover_common/index"].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.sku_list) == null ? undefined : _b.length) > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let _$lepusPushOwner = $lepusPushOwner(uniqueId + "_0"),
              owner = _$lepusPushOwner[0],
              lastOwner = _$lepusPushOwner[1];
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n158 = $update2 ? $lepusGetElementRefByLepusID("component", 158) : null;
            let $compCreated = true;
            let $childLepusComponent = null;
            let $componentId = null;
            if (!$n158) {
              $compCreated = false;
              $n158 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 20, "", "sku-product", "components/sku_product/index", {});
              let $nid158 = $lepusStoreElementRefByLepusID($n158, 158, "sku-product");
              $componentId = $nid158[0];
              $childLepusComponent = $componentConstructor($componentId, $n158, "components/sku_product/index", 158);
              $createdIds.push($componentId + "");
              $cardInstance._currentOwner.componentIds.push($componentId);
              __AppendElement($parent, $n158);
            } else {
              $componentId = __GetElementUniqueID($n158);
              $childLepusComponent = $componentInfo[$componentId];
            }
            $comUpdatePropsSet = [];
            $childLepusComponent._setProp("data", $data.data, $update2 && $compCreated);
            if ($compCreated) {
              let $update_keys = $comUpdatePropsSet;
              let $childSlotUpdate = false;
              if ($update_keys.length > 0 || $childSlotUpdate) {
                $updatedIds.push($componentId + "");
                $renderComponents["components/sku_product/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
              }
            } else {
              $renderComponents["components/sku_product/index"].entry($n158, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
            }
          }
          $update2 = $temp;
          $lepusPopOwner(lastOwner);
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_159: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b;
    let $path = "components/cover_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.anchor) == null ? undefined : _b.length) > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let _$lepusPushOwner2 = $lepusPushOwner(uniqueId + "_0"),
              owner = _$lepusPushOwner2[0],
              lastOwner = _$lepusPushOwner2[1];
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            let $n160 = $update2 ? $lepusGetElementRefByLepusID("for", 160) : null;
            if (!$n160) {
              $n160 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n160, 160, "for");
              __AppendElement($parent, $n160);
            }
            $renderComponents[$path].update_22898d8_160($lepusComponent, $n160, $data, $props, $update2, $slotUpdate);
          }
          $update2 = $temp;
          $lepusPopOwner(lastOwner);
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_160: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/cover_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo5 = $lepusPushFiberForNode($parent, 160, uniqueId),
          $forLepus = _$lepusPushFiberForNo5[0],
          $lastForLepus = _$lepusPushFiberForNo5[1];
      let $object = (_a = $data.data) == null ? undefined : _a.anchor;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.anchor;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let _$lepusPushOwner3 = $lepusPushOwner(uniqueId + "_" + index),
            owner = _$lepusPushOwner3[0],
            lastOwner = _$lepusPushOwner3[1];
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n161 = $update2 ? $lepusGetElementRefByLepusID("view", 161) : null;
          let $temp2 = $update2;
          if (!$n161) {
            $update2 = false;
            $n161 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n161, 161, "view");
            __AppendElement($parent, $n161);
          }
          __SetStyleObject($n161, [getClassStyleIndex("gyl-cover__bubble", "49914000"), parseStyleStringToObject(getSKUBubbleStyle(item))]);
          {
            let $n162 = $update2 ? $lepusGetElementRefByLepusID("component", 162) : null;
            let $compCreated = true;
            let $childLepusComponent = null;
            let $componentId = null;
            if (!$n162) {
              $compCreated = false;
              $n162 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 21, "", "sku_bubble", "components/sku_bubble/index", {});
              let $nid162 = $lepusStoreElementRefByLepusID($n162, 162, "sku_bubble");
              $componentId = $nid162[0];
              $childLepusComponent = $componentConstructor($componentId, $n162, "components/sku_bubble/index", 162);
              $createdIds.push($componentId + "");
              $cardInstance._currentOwner.componentIds.push($componentId);
              __AppendElement($n161, $n162);
            } else {
              $componentId = __GetElementUniqueID($n162);
              $childLepusComponent = $componentInfo[$componentId];
            }
            $comUpdatePropsSet = [];
            $childLepusComponent._setProp("data", item, $update2);
            if ($compCreated) {
              let $update_keys = $comUpdatePropsSet;
              let $childSlotUpdate = false;
              if ($update_keys.length > 0 || $childSlotUpdate) {
                $updatedIds.push($componentId + "");
                $renderComponents["components/sku_bubble/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
              }
            } else {
              $renderComponents["components/sku_bubble/index"].entry($n162, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
            }
          }
          $update2 = $temp2;
        }
        $lepusPopOwner(lastOwner);
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_163: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/cover_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.sku_list) == null ? undefined : _b.length) > 0 || ((_d = (_c = $data.data) == null ? undefined : _c.anchor) == null ? undefined : _d.length) > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n164 = $update2 ? $lepusGetElementRefByLepusID("view", 164) : null;
            let $temp2 = $update2;
            if (!$n164) {
              $update2 = false;
              $n164 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n164, 164, "view");
              __AppendElement($parent, $n164);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n164, [getClassStyleIndex("gyl-cover__mask", "49914000"), 373, ((_f = (_e = $data.data) == null ? undefined : _e.sku_list) == null ? undefined : _f.length) > 0 ? [372] : [], parseStyleStringToObject((_h = (_g = $data.data) == null ? undefined : _g.cover) == null ? undefined : _h.style)]);
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_167: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d;
    if (!$update2 || $renderComponents["components/cover_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo6 = $lepusPushFiberForNode($parent, 167, uniqueId),
          $forLepus = _$lepusPushFiberForNo6[0],
          $lastForLepus = _$lepusPushFiberForNo6[1];
      let $object = (_b = (_a = $data.data) == null ? undefined : _a.swiper) == null ? undefined : _b.items;
      let $length = _GetLength($object);
      let $oldObject = (_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.swiper) == null ? undefined : _d.items;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n168 = $update2 ? $lepusGetElementRefByLepusID("x-swiper-item", 168) : null;
          let $temp2 = $update2;
          if (!$n168) {
            $update2 = false;
            $n168 = __CreateElement("x-swiper-item", $currentComponentId);
            $lepusStoreElementRefByLepusID($n168, 168, "x-swiper-item");
            __SetStyleObject($n168, [getClassStyleIndex("gyl-cover__swiper-item", "49914000")]);
            __AppendElement($parent, $n168);
          }
          __SetAttribute($n168, "item-key", index);
          {
            let $n169 = $update2 ? $lepusGetElementRefByLepusID("image", 169) : null;
            let $temp3 = $update2;
            if (!$n169) {
              $update2 = false;
              $n169 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n169, 169, "image");
              __SetStyleObject($n169, [getClassStyleIndex("gyl-cover__swiper-item-image", "49914000")]);
              __SetAttribute($n169, "mode", "aspectFill");
              __SetAttribute($n169, "skip-redirection", true);
              __SetAttribute($n169, "downsampling", true);
              __SetAttribute($n169, "accessibility-element", "false");
              __AppendElement($n168, $n169);
            }
            __SetAttribute($n169, "src", item == null ? undefined : item.src);
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_170: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c;
    let $path = "components/cover_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_a = $data.data) == null ? undefined : _a.mask) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n171 = $update2 ? $lepusGetElementRefByLepusID("view", 171) : null;
            let $temp2 = $update2;
            if (!$n171) {
              $update2 = false;
              $n171 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n171, 171, "view");
              __AppendElement($parent, $n171);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n171, [getClassStyleIndex("gyl-cover__mask", "49914000"), parseStyleStringToObject((_c = (_b = $data.data) == null ? undefined : _b.mask) == null ? undefined : _c.style)]);
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_172: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e;
    let $path = "components/cover_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.tag) == null ? undefined : _b.tag_left) == null ? undefined : _c.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n173 = $update2 ? $lepusGetElementRefByLepusID("view", 173) : null;
            let $temp2 = $update2;
            if (!$n173) {
              $update2 = false;
              $n173 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n173, 173, "view");
              __AppendElement($parent, $n173);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n173, [getClassStyleIndex("gyl-cover__tag", "49914000"), parseStyleStringToObject((_e = (_d = $data.data) == null ? undefined : _d.tag) == null ? undefined : _e.style)]);
              }
            }
            {
              let $template_update = $update2;
              let $n174 = $update2 ? $lepusGetElementRefByLepusID("if", 174) : null;
              if (!$n174) {
                $update2 = false;
                $n174 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n174, 174, "if");
                __AppendElement($n173, $n174);
              }
              $renderComponents[$path].update_22898d8_174($lepusComponent, $n174, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let _$template_update3 = $update2;
              let $n176 = $update2 ? $lepusGetElementRefByLepusID("if", 176) : null;
              if (!$n176) {
                $update2 = false;
                $n176 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n176, 176, "if");
                __AppendElement($n173, $n176);
              }
              $renderComponents[$path].update_22898d8_176($lepusComponent, $n176, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update3;
            }
            {
              let _$template_update4 = $update2;
              let $n179 = $update2 ? $lepusGetElementRefByLepusID("if", 179) : null;
              if (!$n179) {
                $update2 = false;
                $n179 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n179, 179, "if");
                __AppendElement($n173, $n179);
              }
              $renderComponents[$path].update_22898d8_179($lepusComponent, $n179, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update4;
            }
            {
              let _$template_update5 = $update2;
              let $n181 = $update2 ? $lepusGetElementRefByLepusID("if", 181) : null;
              if (!$n181) {
                $update2 = false;
                $n181 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n181, 181, "if");
                __AppendElement($n173, $n181);
              }
              $renderComponents[$path].update_22898d8_181($lepusComponent, $n181, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update5;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_174: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.tag) == null ? undefined : _b.tag_image) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n175 = $update2 ? $lepusGetElementRefByLepusID("image", 175) : null;
          let $temp2 = $update2;
          if (!$n175) {
            $update2 = false;
            $n175 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n175, 175, "image");
            __SetAttribute($n175, "mode", "aspectFill");
            __SetAttribute($n175, "skip-redirection", true);
            __SetAttribute($n175, "downsampling", true);
            __SetAttribute($n175, "accessibility-element", false);
            __AppendElement($parent, $n175);
          }
          if (!$update2 || $renderComponents["components/cover_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n175, [getClassStyleIndex("gyl-cover__tag-image", "49914000"), parseStyleStringToObject((_e = (_d = (_c = $data.data) == null ? undefined : _c.tag) == null ? undefined : _d.tag_image) == null ? undefined : _e.style)]);
            }
            {
              let $value = (_h = (_g = (_f = $data.data) == null ? undefined : _f.tag) == null ? undefined : _g.tag_image) == null ? undefined : _h.url;
              if (!$update2 || $value !== ((_k = (_j = (_i = $lepusComponent._data.data) == null ? undefined : _i.tag) == null ? undefined : _j.tag_image) == null ? undefined : _k.url)) {
                __SetAttribute($n175, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_176: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    let $path = "components/cover_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.tag) == null ? undefined : _b.tag_left) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n177 = $update2 ? $lepusGetElementRefByLepusID("text", 177) : null;
          let $temp2 = $update2;
          if (!$n177) {
            $update2 = false;
            $n177 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n177, 177, "text");
            __SetAttribute($n177, "accessibility-element", false);
            __AppendElement($parent, $n177);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n177, [getClassStyleIndex("gyl-cover__tag-left", "49914000"), parseStyleStringToObject((_e = (_d = (_c = $data.data) == null ? undefined : _c.tag) == null ? undefined : _d.tag_left) == null ? undefined : _e.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $value = (_h = (_g = (_f = $data.data) == null ? undefined : _f.tag) == null ? undefined : _g.tag_left) == null ? undefined : _h.content;
              if (!$update2 || $value !== ((_k = (_j = (_i = $lepusComponent._data.data) == null ? undefined : _i.tag) == null ? undefined : _j.tag_left) == null ? undefined : _k.content)) {
                __SetAttribute($n177, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_179: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.tag) == null ? undefined : _b.tag_icon) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n180 = $update2 ? $lepusGetElementRefByLepusID("view", 180) : null;
          let $temp2 = $update2;
          if (!$n180) {
            $update2 = false;
            $n180 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n180, 180, "view");
            __AppendElement($parent, $n180);
          }
          if (!$update2 || $renderComponents["components/cover_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n180, [getClassStyleIndex("gyl-cover__tag-icon", "49914000"), parseStyleStringToObject((_e = (_d = (_c = $data.data) == null ? undefined : _c.tag) == null ? undefined : _d.tag_icon) == null ? undefined : _e.style)]);
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_181: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    let $path = "components/cover_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.tag) == null ? undefined : _b.tag_right) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n182 = $update2 ? $lepusGetElementRefByLepusID("text", 182) : null;
          let $temp2 = $update2;
          if (!$n182) {
            $update2 = false;
            $n182 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n182, 182, "text");
            __SetAttribute($n182, "accessibility-element", false);
            __AppendElement($parent, $n182);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n182, [getClassStyleIndex("gyl-cover__tag-right", "49914000"), parseStyleStringToObject((_e = (_d = (_c = $data.data) == null ? undefined : _c.tag) == null ? undefined : _d.tag_right) == null ? undefined : _e.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $value = (_h = (_g = (_f = $data.data) == null ? undefined : _f.tag) == null ? undefined : _g.tag_right) == null ? undefined : _h.content;
              if (!$update2 || $value !== ((_k = (_j = (_i = $lepusComponent._data.data) == null ? undefined : _i.tag) == null ? undefined : _j.tag_right) == null ? undefined : _k.content)) {
                __SetAttribute($n182, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_184: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d;
    if (!$update2 || $renderComponents["components/cover_common/index"].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_c = (_b = (_a = $data.data) == null ? undefined : _a.horizontal_skus) == null ? undefined : _b.sku_list) == null ? undefined : _c.length) > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let _$lepusPushOwner4 = $lepusPushOwner(uniqueId + "_0"),
              owner = _$lepusPushOwner4[0],
              lastOwner = _$lepusPushOwner4[1];
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n185 = $update2 ? $lepusGetElementRefByLepusID("view", 185) : null;
            let $temp2 = $update2;
            if (!$n185) {
              $update2 = false;
              $n185 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n185, 185, "view");
              __SetStyleObject($n185, [getClassStyleIndex("gyl-cover__horizontal_skus", "49914000")]);
              __AppendElement($parent, $n185);
            }
            {
              let $n186 = $update2 ? $lepusGetElementRefByLepusID("component", 186) : null;
              let $compCreated = true;
              let $childLepusComponent = null;
              let $componentId = null;
              if (!$n186) {
                $compCreated = false;
                $n186 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 22, "", "horizontal_skus", "components/horizontal_skus/index", {});
                let $nid186 = $lepusStoreElementRefByLepusID($n186, 186, "horizontal_skus");
                $componentId = $nid186[0];
                $childLepusComponent = $componentConstructor($componentId, $n186, "components/horizontal_skus/index", 186);
                $createdIds.push($componentId + "");
                $cardInstance._currentOwner.componentIds.push($componentId);
                __AppendElement($n185, $n186);
              } else {
                $componentId = __GetElementUniqueID($n186);
                $childLepusComponent = $componentInfo[$componentId];
              }
              $comUpdatePropsSet = [];
              $childLepusComponent._setProp("data", (_d = $data.data) == null ? undefined : _d.horizontal_skus, $update2 && $compCreated);
              if ($compCreated) {
                let $update_keys = $comUpdatePropsSet;
                let $childSlotUpdate = false;
                if ($update_keys.length > 0 || $childSlotUpdate) {
                  $updatedIds.push($componentId + "");
                  $renderComponents["components/horizontal_skus/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
                }
              } else {
                $renderComponents["components/horizontal_skus/index"].entry($n186, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
          $lepusPopOwner(lastOwner);
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_352c4e0_165: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e;
    let $path = "components/cover_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[6] || $renderComponents[$path].varUpdateState[5]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_c = (_b = (_a = $data.data) == null ? undefined : _a.swiper) == null ? undefined : _b.items) == null ? undefined : _c.length) && $data.isPlaying && $data.isCacheData === 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n166 = $update2 ? $lepusGetElementRefByLepusID("x-swiper", 166) : null;
            let $temp2 = $update2;
            if (!$n166) {
              $update2 = false;
              $n166 = __CreateElement("x-swiper", $currentComponentId);
              $lepusStoreElementRefByLepusID($n166, 166, "x-swiper");
              __SetAttribute($n166, "indicator-dots", false);
              __SetAttribute($n166, "interval", 900);
              __SetAttribute($n166, "duration", 1250);
              __SetAttribute($n166, "touchable", false);
              __SetAttribute($n166, "circular", true);
              __SetAttribute($n166, "ios-bind-change-type", 1);
              __SetAttribute($n166, "mode", "normal");
              __SetAttribute($n166, "clip-radius", true);
              __SetAttribute($n166, "scroll-before-detached", true);
              __SetAttribute($n166, "keep-item-view", true);
              __AddEvent($n166, "bindEvent", "change", "handleSwiperChange");
              __AppendElement($parent, $n166);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[5] || $renderComponents[$path].varUpdateState[6]) {
              {
                __SetStyleObject($n166, [getClassStyleIndex("gyl-cover__swiper", "49914000"), parseStyleStringToObject((_e = (_d = $data.data) == null ? undefined : _d.swiper) == null ? undefined : _e.style)]);
              }
              {
                let $value = $data.isPlaying && $data.isCacheData === 0;
                if (!$update2 || $value !== ($lepusComponent._data.isPlaying && $lepusComponent._data.isCacheData === 0)) {
                  __SetAttribute($n166, "autoplay", $value);
                }
              }
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n167 = $update2 ? $lepusGetElementRefByLepusID("for", 167) : null;
              if (!$n167) {
                $n167 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n167, 167, "for");
                __AppendElement($n166, $n167);
              }
              $renderComponents[$path].update_22898d8_167($lepusComponent, $n167, $data, $props, $update2, $slotUpdate);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s, _t;
    let $path = "components/cover_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    let $n154 = $lepusGetElementRefByLepusID("view", 154);
    if ($renderComponents[$path].varUpdateState[0]) {
      {
        __SetStyleObject($n154, [getClassStyleIndex("gyl-cover__container", "49914000"), parseStyleStringToObject($data.data.style)]);
      }
    }
    if ($renderComponents[$path].varUpdateState[1]) {
      let $n155 = $lepusGetElementRefByLepusID("view", 155);
      {
        let $value = "gyl-cover__wrapper " + (isLynx() && !$data.isNA ? "gyl-cover__absolute" : "");
        if (!$update2 || $value !== undefined) {
          __SetStyleObject($n155, [getClassStyleIndex("gyl-cover__wrapper " + (isLynx() && !$data.isNA ? "gyl-cover__absolute" : ""), "49914000")]);
        }
      }
    }
    if ($renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3] || $renderComponents[$path].varUpdateState[4]) {
      let $n156 = $lepusGetElementRefByLepusID("image", 156);
      {
        __SetStyleObject($n156, [getClassStyleIndex("gyl-cover__cover", "49914000"), ((_b = (_a = $data.data) == null ? undefined : _a.sku_list) == null ? undefined : _b.length) > 0 ? [372] : [], parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.cover) == null ? undefined : _d.style)]);
      }
      {
        let _$value15 = (_f = (_e = $data.data) == null ? undefined : _e.cover) == null ? undefined : _f.src;
        if (!$update2 || _$value15 !== ((_h = (_g = $lepusComponent._data.data) == null ? undefined : _g.cover) == null ? undefined : _h.src)) {
          __SetAttribute($n156, "src", _$value15);
        }
      }
      {
        let _$value16 = $data.usePiercedSrc ? (_j = (_i = $data.data) == null ? undefined : _i.cover) == null ? undefined : _j.pierced_src : "";
        if (!$update2 || _$value16 !== ($lepusComponent._data.usePiercedSrc ? (_l = (_k = $lepusComponent._data.data) == null ? undefined : _k.cover) == null ? undefined : _l.pierced_src : "")) {
          __SetAttribute($n156, "pierced-src", _$value16);
        }
      }
      {
        let _$value17 = $data.imageMonitorTag;
        if (!$update2 || _$value17 !== $lepusComponent._data.imageMonitorTag) {
          __SetAttribute($n156, "monitor-info", _$value17);
        }
      }
      {
        let _$value18 = !!((_n = (_m = $data.data) == null ? undefined : _m.cover) == null ? undefined : _n.downsampling);
        if (!$update2 || _$value18 !== !!((_p = (_o = $lepusComponent._data.data) == null ? undefined : _o.cover) == null ? undefined : _p.downsampling)) {
          __SetAttribute($n156, "downsampling", _$value18);
        }
      }
      {
        let _$value19 = $data.coverId;
        if (!$update2 || _$value19 !== $lepusComponent._data.coverId) {
          __AddDataset($n156, "coverId", _$value19);
        }
      }
      {
        let _$value20 = (_r = (_q = $data.data) == null ? undefined : _q.cover) == null ? undefined : _r.src;
        if (!$update2 || _$value20 !== ((_t = (_s = $lepusComponent._data.data) == null ? undefined : _s.cover) == null ? undefined : _t.src)) {
          __AddDataset($n156, "coverUrl", _$value20);
        }
      }
    }
    let $n157 = $lepusGetElementRefByLepusID("if", 157);
    $renderComponents[$path].update_22898d8_157($lepusComponent, $n157, $data, $props, $update2, $slotUpdate);
    let $n159 = $lepusGetElementRefByLepusID("if", 159);
    $renderComponents[$path].update_22898d8_159($lepusComponent, $n159, $data, $props, $update2, $slotUpdate);
    let $n163 = $lepusGetElementRefByLepusID("if", 163);
    $renderComponents[$path].update_22898d8_163($lepusComponent, $n163, $data, $props, $update2, $slotUpdate);
    let $n165 = $lepusGetElementRefByLepusID("if", 165);
    $renderComponents[$path].update_352c4e0_165($lepusComponent, $n165, $data, $props, $update2, $slotUpdate);
    let $n170 = $lepusGetElementRefByLepusID("if", 170);
    $renderComponents[$path].update_22898d8_170($lepusComponent, $n170, $data, $props, $update2, $slotUpdate);
    let $n172 = $lepusGetElementRefByLepusID("if", 172);
    $renderComponents[$path].update_22898d8_172($lepusComponent, $n172, $data, $props, $update2, $slotUpdate);
    let $n184 = $lepusGetElementRefByLepusID("if", 184);
    $renderComponents[$path].update_22898d8_184($lepusComponent, $n184, $data, $props, $update2, $slotUpdate);
    let $slot = $lepusComponent.slots["default"];
    if ($slot && (!$update2 || $slotUpdate)) {
      $slot.fn($slot.componentId, $n154, $update2);
    }
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    let $path = "components/cover_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n154 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n154, 154, "view");
      __SetStyleObject($n154, [getClassStyleIndex("gyl-cover__container", "49914000"), parseStyleStringToObject($data.data.style)]);
      __AddEvent($n154, "catchEvent", "tap", "handleClick");
      __AppendElement($component, $n154);
      let $n155 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n155, 155, "view");
      __SetStyleObject($n155, [getClassStyleIndex("gyl-cover__wrapper " + (isLynx() && !$data.isNA ? "gyl-cover__absolute" : ""), "49914000")]);
      __AppendElement($n154, $n155);
      let $n156 = __CreateImage($currentComponentId);
      $lepusStoreElementRefByLepusID($n156, 156, "image");
      __SetStyleObject($n156, [getClassStyleIndex("gyl-cover__cover", "49914000"), ((_b = (_a = $data.data) == null ? undefined : _a.sku_list) == null ? undefined : _b.length) > 0 ? [372] : [], parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.cover) == null ? undefined : _d.style)]);
      __SetAttribute($n156, "mode", "aspectFill");
      __SetAttribute($n156, "skip-redirection", true);
      __SetAttribute($n156, "accessibility-element", false);
      __SetAttribute($n156, "src", (_f = (_e = $data.data) == null ? undefined : _e.cover) == null ? undefined : _f.src);
      __SetAttribute($n156, "pierced-src", $data.usePiercedSrc ? (_h = (_g = $data.data) == null ? undefined : _g.cover) == null ? undefined : _h.pierced_src : "");
      __SetAttribute($n156, "monitor-info", $data.imageMonitorTag);
      __SetAttribute($n156, "downsampling", !!((_j = (_i = $data.data) == null ? undefined : _i.cover) == null ? undefined : _j.downsampling));
      __AddDataset($n156, "coverId", $data.coverId);
      __AddDataset($n156, "coverUrl", (_l = (_k = $data.data) == null ? undefined : _k.cover) == null ? undefined : _l.src);
      __AppendElement($n155, $n156);
      let $n157 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n157, 157, "if");
      __AppendElement($n155, $n157);
      $renderComponents[$path].update_22898d8_157($lepusComponent, $n157, $data, $props, $update2, $slotUpdate);
      let $n159 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n159, 159, "if");
      __AppendElement($n155, $n159);
      $renderComponents[$path].update_22898d8_159($lepusComponent, $n159, $data, $props, $update2, $slotUpdate);
      let $n163 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n163, 163, "if");
      __AppendElement($n155, $n163);
      $renderComponents[$path].update_22898d8_163($lepusComponent, $n163, $data, $props, $update2, $slotUpdate);
      let $n165 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n165, 165, "if");
      __AppendElement($n154, $n165);
      $renderComponents[$path].update_352c4e0_165($lepusComponent, $n165, $data, $props, $update2, $slotUpdate);
      let $n170 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n170, 170, "if");
      __AppendElement($n154, $n170);
      $renderComponents[$path].update_22898d8_170($lepusComponent, $n170, $data, $props, $update2, $slotUpdate);
      let $n172 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n172, 172, "if");
      __AppendElement($n154, $n172);
      $renderComponents[$path].update_22898d8_172($lepusComponent, $n172, $data, $props, $update2, $slotUpdate);
      let $n184 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n184, 184, "if");
      __AppendElement($n154, $n184);
      $renderComponents[$path].update_22898d8_184($lepusComponent, $n184, $data, $props, $update2, $slotUpdate);
      let $slot = $lepusComponent.slots["default"];
      if ($slot && (!$update2 || $slotUpdate)) {
        $slot.fn($slot.componentId, $n154, $update2);
      }
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/feedback_common/index"] = {
  variables: ["abValues", "limit", "items", "showAssociated"],
  varUpdateState: [],
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c;
    let $path = "components/feedback_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n187 = $lepusGetElementRefByLepusID("view", 187);
      {
        let $value = (((_a = $data.abValues) == null ? undefined : _a.use_new_grid) ? "border-radius: 16rpx" : "") + ";";
        if (!$update2 || $value !== (((_b = $lepusComponent._data.abValues) == null ? undefined : _b.use_new_grid) ? "border-radius: 16rpx" : "") + ";") {
          __SetStyleObject($n187, [getClassStyleIndex("feedback feedback-container", "54878000"), ((_c = $data.abValues) == null ? undefined : _c.use_new_grid) ? [54] : []]);
        }
      }
    }
    if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3]) {
      {
        let $n188 = $lepusGetElementRefByLepusID("component", 188);
        let $componentId = __GetElementUniqueID($n188);
        let $childLepusComponent = $componentInfo[$componentId];
        $comUpdatePropsSet = [];
        $childLepusComponent._setProp("limit", $data.limit, $update2);
        $childLepusComponent._setProp("items", $data.items, $update2);
        $childLepusComponent._setProp("showAssociated", $data.showAssociated, $update2);
        let $update_keys = $comUpdatePropsSet;
        let $childSlotUpdate = false;
        if ($update_keys.length > 0 || $childSlotUpdate) {
          $updatedIds.push($componentId + "");
          $renderComponents["components/feedback_ui_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
        }
      }
    }
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a;
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n187 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n187, 187, "view");
      __SetStyleObject($n187, [getClassStyleIndex("feedback feedback-container", "54878000"), ((_a = $data.abValues) == null ? undefined : _a.use_new_grid) ? [54] : []]);
      __AppendElement($component, $n187);
      {
        let $n188 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 16, "", "feedback-ui-common", "components/feedback_ui_common/index", {});
        let $nid188 = $lepusStoreElementRefByLepusID($n188, 188, "feedback-ui-common");
        let $childLepusComponent = $componentConstructor($nid188[0], $n188, "components/feedback_ui_common/index", 188);
        $cardInstance._currentOwner.componentIds.push($nid188[0]);
        $comUpdatePropsSet = [];
        $childLepusComponent._setProp("limit", $data.limit, $update2);
        $childLepusComponent._setProp("items", $data.items, $update2);
        $childLepusComponent._setProp("showAssociated", $data.showAssociated, $update2);
        __SetStyleObject($n188, [16]);
        __AddEvent($n188, "bindEvent", "FeedbackClick", "distributeClick");
        __AppendElement($n187, $n188);
        $renderComponents["components/feedback_ui_common/index"].entry($n188, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
        $createdIds.push($nid188[0] + "");
      }
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/feedback_ui_common/index"] = {
  variables: ["onMoreList", "items", "limit", "showAssociated"],
  varUpdateState: [],
  update_2532b48_192: function ($lepusComponent, $parent, $data, $props, $update2) {
    let $path = "components/feedback_ui_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo7 = $lepusPushFiberForNode($parent, 192, uniqueId),
          $forLepus = _$lepusPushFiberForNo7[0],
          $lastForLepus = _$lepusPushFiberForNo7[1];
      let $object = getPageOneItems($data.items, $data.limit);
      let $length = _GetLength($object);
      let $oldObject = undefined;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n193 = $update2 ? $lepusGetElementRefByLepusID("view", 193) : null;
          let $temp2 = $update2;
          if (!$n193) {
            $update2 = false;
            $n193 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n193, 193, "view");
            __SetAttribute($n193, "accessibility-element", "true");
            __AddDataset($n193, "area", getFeedbackItemName());
            __AddEvent($n193, "catchEvent", "tap", "dispatchClick");
            __AppendElement($parent, $n193);
          }
          __SetStyleObject($n193, [getClassStyleIndex(getPageOneItemClass($data.items, $data.limit, index), "39746000")]);
          __SetAttribute($n193, "accessibility-label", getButtonA11yLabel(item.text));
          {
            let $value = getButtonA11yTraits();
            if (!$update2 || $value !== undefined) {
              __SetAttribute($n193, "accessibility-traits", $value);
            }
          }
          __AddDataset($n193, "type", item.type);
          __AddDataset($n193, "index", index);
          {
            let $n194 = $update2 ? $lepusGetElementRefByLepusID("image", 194) : null;
            let $temp3 = $update2;
            if (!$n194) {
              $update2 = false;
              $n194 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n194, 194, "image");
              __SetStyleObject($n194, [getClassStyleIndex("gyl_feedback-neg--item-icon", "39746000")]);
              __SetAttribute($n194, "accessibility-element", "false");
              __AppendElement($n193, $n194);
            }
            __SetAttribute($n194, "src", item.icon);
            $update2 = $temp3;
          }
          {
            let $n195 = $update2 ? $lepusGetElementRefByLepusID("text", 195) : null;
            let _$temp17 = $update2;
            if (!$n195) {
              $update2 = false;
              $n195 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n195, 195, "text");
              __SetStyleObject($n195, [getClassStyleIndex("gyl_feedback-neg--item-text", "39746000")]);
              __SetAttribute($n195, "accessibility-element", "false");
              __AppendElement($n193, $n195);
            }
            __SetAttribute($n195, "text", item.text);
            $update2 = _$temp17;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_2532b48_197: function ($lepusComponent, $parent, $data, $props, $update2) {
    let $path = "components/feedback_ui_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (hasMore($data.items, $data.limit)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n198 = $update2 ? $lepusGetElementRefByLepusID("view", 198) : null;
          let $temp2 = $update2;
          if (!$n198) {
            $update2 = false;
            $n198 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n198, 198, "view");
            __SetStyleObject($n198, [getClassStyleIndex("gyl_feedback-neg--item gyl_feedback-neg--item-more", "39746000")]);
            __SetAttribute($n198, "accessibility-element", "true");
            __AddDataset($n198, "area", getFeedbackMoreButtonName());
            __AddEvent($n198, "catchEvent", "tap", "dispatchClick");
            __AppendElement($parent, $n198);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
            {
              let $value = getButtonA11yLabel(getMoreText($data.items, $data.limit));
              if (!$update2 || $value !== undefined) {
                __SetAttribute($n198, "accessibility-label", $value);
              }
            }
            {
              let _$value21 = getButtonA11yTraits();
              if (!$update2 || _$value21 !== undefined) {
                __SetAttribute($n198, "accessibility-traits", _$value21);
              }
            }
          }
          {
            let $n199 = $update2 ? $lepusGetElementRefByLepusID("image", 199) : null;
            let $temp3 = $update2;
            if (!$n199) {
              $update2 = false;
              $n199 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n199, 199, "image");
              __SetStyleObject($n199, [getClassStyleIndex("gyl_feedback-neg--item-icon", "39746000")]);
              __SetAttribute($n199, "src", "network_address");
              __SetAttribute($n199, "accessibility-element", "false");
              __AppendElement($n198, $n199);
            }
            $update2 = $temp3;
          }
          {
            let $n200 = $update2 ? $lepusGetElementRefByLepusID("text", 200) : null;
            let _$temp18 = $update2;
            if (!$n200) {
              $update2 = false;
              $n200 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n200, 200, "text");
              __SetAttribute($n200, "accessibility-element", "false");
              __AppendElement($n198, $n200);
            }
            {
              let _$value22 = "gyl_feedback-neg--item-text-more " + (getMoreText($data.items, $data.limit).length > 8 ? "gyl_feedback-neg--item-text-more-small" : "");
              if (!$update2 || _$value22 !== undefined) {
                __SetStyleObject($n200, [getClassStyleIndex("gyl_feedback-neg--item-text-more " + (getMoreText($data.items, $data.limit).length > 8 ? "gyl_feedback-neg--item-text-more-small" : ""), "39746000")]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
                __SetAttribute($n200, "text", getMoreText($data.items, $data.limit));
              }
            }
            $update2 = _$temp18;
          }
          {
            let $n202 = $update2 ? $lepusGetElementRefByLepusID("image", 202) : null;
            let _$temp19 = $update2;
            if (!$n202) {
              $update2 = false;
              $n202 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n202, 202, "image");
              __SetStyleObject($n202, [getClassStyleIndex("gyl_feedback-neg--item-icon-more", "39746000")]);
              __SetAttribute($n202, "src", "network_address");
              __SetAttribute($n202, "accessibility-element", "false");
              __AppendElement($n198, $n202);
            }
            $update2 = _$temp19;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_2532b48_204: function ($lepusComponent, $parent, $data, $props, $update2) {
    let $path = "components/feedback_ui_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo8 = $lepusPushFiberForNode($parent, 204, uniqueId),
          $forLepus = _$lepusPushFiberForNo8[0],
          $lastForLepus = _$lepusPushFiberForNo8[1];
      let $object = getPageTwoItems($data.items, $data.limit);
      let $length = _GetLength($object);
      let $oldObject = undefined;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n205 = $update2 ? $lepusGetElementRefByLepusID("view", 205) : null;
          let $temp2 = $update2;
          if (!$n205) {
            $update2 = false;
            $n205 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n205, 205, "view");
            __SetAttribute($n205, "accessibility-element", "true");
            __AddDataset($n205, "area", getFeedbackItemName());
            __AddEvent($n205, "catchEvent", "tap", "dispatchClick");
            __AppendElement($parent, $n205);
          }
          __SetStyleObject($n205, [getClassStyleIndex(getPageTwoItemClass($data.items, $data.limit, index), "39746000")]);
          __SetAttribute($n205, "accessibility-label", getButtonA11yLabel(item.text));
          {
            let $value = getButtonA11yTraits();
            if (!$update2 || $value !== undefined) {
              __SetAttribute($n205, "accessibility-traits", $value);
            }
          }
          __AddDataset($n205, "type", item.type);
          __AddDataset($n205, "index", index + $data.limit);
          {
            let $n206 = $update2 ? $lepusGetElementRefByLepusID("image", 206) : null;
            let $temp3 = $update2;
            if (!$n206) {
              $update2 = false;
              $n206 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n206, 206, "image");
              __SetStyleObject($n206, [getClassStyleIndex("gyl_feedback-neg--item-icon gyl_feedback-neg-pagetwo-item-icon", "39746000")]);
              __SetAttribute($n206, "accessibility-element", "false");
              __AppendElement($n205, $n206);
            }
            __SetAttribute($n206, "src", item.icon);
            $update2 = $temp3;
          }
          {
            let $n207 = $update2 ? $lepusGetElementRefByLepusID("text", 207) : null;
            let _$temp20 = $update2;
            if (!$n207) {
              $update2 = false;
              $n207 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n207, 207, "text");
              __SetStyleObject($n207, [getClassStyleIndex("gyl_feedback-neg--item-text", "39746000")]);
              __SetAttribute($n207, "accessibility-element", "false");
              __AppendElement($n205, $n207);
            }
            __SetAttribute($n207, "text", item.text);
            $update2 = _$temp20;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_38f1a30_190: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let $path = "components/feedback_ui_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!$data.onMoreList) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n191 = $update2 ? $lepusGetElementRefByLepusID("view", 191) : null;
            let $temp2 = $update2;
            if (!$n191) {
              $update2 = false;
              $n191 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n191, 191, "view");
              __SetStyleObject($n191, [getClassStyleIndex("gyl_feedback-neg", "39746000")]);
              __AppendElement($parent, $n191);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
              let $n192 = $update2 ? $lepusGetElementRefByLepusID("for", 192) : null;
              if (!$n192) {
                $n192 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n192, 192, "for");
                __AppendElement($n191, $n192);
              }
              $renderComponents[$path].update_2532b48_192($lepusComponent, $n192, $data, $props, $update2, $slotUpdate);
            }
            {
              let $template_update = $update2;
              let $n197 = $update2 ? $lepusGetElementRefByLepusID("if", 197) : null;
              if (!$n197) {
                $update2 = false;
                $n197 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n197, 197, "if");
                __AppendElement($n191, $n197);
              }
              $renderComponents[$path].update_2532b48_197($lepusComponent, $n197, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp21 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n203 = $update2 ? $lepusGetElementRefByLepusID("view", 203) : null;
            let _$temp22 = $update2;
            if (!$n203) {
              $update2 = false;
              $n203 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n203, 203, "view");
              __SetStyleObject($n203, [getClassStyleIndex("gyl_feedback-neg", "39746000")]);
              __AppendElement($parent, $n203);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
              let $n204 = $update2 ? $lepusGetElementRefByLepusID("for", 204) : null;
              if (!$n204) {
                $n204 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n204, 204, "for");
                __AppendElement($n203, $n204);
              }
              $renderComponents[$path].update_2532b48_204($lepusComponent, $n204, $data, $props, $update2, $slotUpdate);
            }
            $update2 = _$temp22;
          }
          $update2 = _$temp21;
        }
      }
    }
  },
  update_3a895a0_209: function ($lepusComponent, $parent, $data, $props, $update2) {
    let $path = "components/feedback_ui_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[3]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.showAssociated && !$data.onMoreList) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n210 = $update2 ? $lepusGetElementRefByLepusID("view", 210) : null;
            let $temp2 = $update2;
            if (!$n210) {
              $update2 = false;
              $n210 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n210, 210, "view");
              __SetStyleObject($n210, [getClassStyleIndex("gyl_feedback-similar", "39746000")]);
              __SetAttribute($n210, "accessibility-element", "true");
              __AddDataset($n210, "area", getFeedbackSimilarButtonName());
              __AddEvent($n210, "catchEvent", "tap", "dispatchClick");
              __AppendElement($parent, $n210);
            }
            {
              let $value = getButtonA11yLabel("Find Similar");
              if (!$update2 || $value !== undefined) {
                __SetAttribute($n210, "accessibility-label", $value);
              }
            }
            {
              let _$value23 = getButtonA11yTraits();
              if (!$update2 || _$value23 !== undefined) {
                __SetAttribute($n210, "accessibility-traits", _$value23);
              }
            }
            {
              let $n211 = $update2 ? $lepusGetElementRefByLepusID("text", 211) : null;
              let $temp3 = $update2;
              if (!$n211) {
                $update2 = false;
                $n211 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n211, 211, "text");
                __SetStyleObject($n211, [getClassStyleIndex("gyl_feedback-similar-text", "39746000")]);
                __SetAttribute($n211, "accessibility-element", "false");
                __AppendElement($n210, $n211);
              }
              __SetAttribute($n211, "text", "Find Similar");
              $update2 = $temp3;
            }
            {
              let $n213 = $update2 ? $lepusGetElementRefByLepusID("image", 213) : null;
              let _$temp23 = $update2;
              if (!$n213) {
                $update2 = false;
                $n213 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n213, 213, "image");
                __SetStyleObject($n213, [getClassStyleIndex("gyl_feedback-similar-arrow-icon", "39746000")]);
                __SetAttribute($n213, "src", "network_address");
                __SetAttribute($n213, "accessibility-element", "false");
                __AppendElement($n210, $n213);
              }
              $update2 = _$temp23;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_707510_215: function ($lepusComponent, $parent, $data, $props, $update2) {
    if (!$update2 || $renderComponents["components/feedback_ui_common/index"].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.onMoreList) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n216 = $update2 ? $lepusGetElementRefByLepusID("view", 216) : null;
            let $temp2 = $update2;
            if (!$n216) {
              $update2 = false;
              $n216 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n216, 216, "view");
              __SetStyleObject($n216, [getClassStyleIndex("gyl_feedback-header-back", "39746000")]);
              __SetAttribute($n216, "accessibility-element", "true");
              __AddDataset($n216, "area", getFeedbackBackButtonName());
              __AddEvent($n216, "catchEvent", "tap", "dispatchClick");
              __AppendElement($parent, $n216);
            }
            {
              let $value = getButtonA11yLabel("Go Back");
              if (!$update2 || $value !== undefined) {
                __SetAttribute($n216, "accessibility-label", $value);
              }
            }
            {
              let _$value24 = getButtonA11yTraits();
              if (!$update2 || _$value24 !== undefined) {
                __SetAttribute($n216, "accessibility-traits", _$value24);
              }
            }
            {
              let $n217 = $update2 ? $lepusGetElementRefByLepusID("image", 217) : null;
              let $temp3 = $update2;
              if (!$n217) {
                $update2 = false;
                $n217 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n217, 217, "image");
                __SetStyleObject($n217, [getClassStyleIndex("gyl_feedback-header-back-icon", "39746000")]);
                __SetAttribute($n217, "src", "network_address");
                __SetAttribute($n217, "accessibility-element", "false");
                __AppendElement($n216, $n217);
              }
              $update2 = $temp3;
            }
            {
              let $n218 = $update2 ? $lepusGetElementRefByLepusID("text", 218) : null;
              let _$temp24 = $update2;
              if (!$n218) {
                $update2 = false;
                $n218 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n218, 218, "text");
                __SetStyleObject($n218, [getClassStyleIndex("gyl_feedback-header-back-text", "39746000")]);
                __SetAttribute($n218, "accessibility-element", "false");
                __AppendElement($n216, $n218);
              }
              __SetAttribute($n218, "text", "Back");
              $update2 = _$temp24;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp25 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n220 = $update2 ? $lepusGetElementRefByLepusID("view", 220) : null;
            let _$temp26 = $update2;
            if (!$n220) {
              $update2 = false;
              $n220 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n220, 220, "view");
              __AppendElement($parent, $n220);
            }
            $update2 = _$temp26;
          }
          $update2 = _$temp25;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/feedback_ui_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 189);
    let $n190 = $lepusGetElementRefByLepusID("if", 190);
    $renderComponents[$path].update_38f1a30_190($lepusComponent, $n190, $data, $props, $update2, $slotUpdate);
    let $n209 = $lepusGetElementRefByLepusID("if", 209);
    $renderComponents[$path].update_3a895a0_209($lepusComponent, $n209, $data, $props, $update2, $slotUpdate);
    $lepusGetElementRefByLepusID("view", 214);
    let $n215 = $lepusGetElementRefByLepusID("if", 215);
    $renderComponents[$path].update_707510_215($lepusComponent, $n215, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $path = "components/feedback_ui_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n189 = __CreateView($currentComponentId);
      __SetStyleObject($n189, [getClassStyleIndex("gyl_feedback", "39746000")]);
      __AddDataset($n189, "area", getFeedbackMaskName());
      __AddEvent($n189, "catchEvent", "tap", "dispatchClick");
      __AppendElement($component, $n189);
      let $n190 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n190, 190, "if");
      __AppendElement($n189, $n190);
      $renderComponents[$path].update_38f1a30_190($lepusComponent, $n190, $data, $props, $update2, $slotUpdate);
      let $n209 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n209, 209, "if");
      __AppendElement($n189, $n209);
      $renderComponents[$path].update_3a895a0_209($lepusComponent, $n209, $data, $props, $update2, $slotUpdate);
      let $n214 = __CreateView($currentComponentId);
      __SetStyleObject($n214, [getClassStyleIndex("gyl_feedback-header", "39746000")]);
      __AppendElement($n189, $n214);
      let $n215 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n215, 215, "if");
      __AppendElement($n214, $n215);
      $renderComponents[$path].update_707510_215($lepusComponent, $n215, $data, $props, $update2, $slotUpdate);
      let $n221 = __CreateView($currentComponentId);
      __SetStyleObject($n221, [getClassStyleIndex("gyl_feedback-header-close", "39746000")]);
      __SetAttribute($n221, "accessibility-element", "true");
      __SetAttribute($n221, "accessibility-label", getButtonA11yLabel("Close"));
      __SetAttribute($n221, "accessibility-traits", getButtonA11yTraits());
      __AddDataset($n221, "area", getFeedbackCloseButtonName());
      __AddEvent($n221, "catchEvent", "tap", "dispatchClick");
      __AppendElement($n214, $n221);
      let $n222 = __CreateImage($currentComponentId);
      __SetStyleObject($n222, [getClassStyleIndex("gyl_feedback-header-close-icon", "39746000")]);
      __SetAttribute($n222, "src", "network_address");
      __SetAttribute($n222, "accessibility-element", "false");
      __AppendElement($n221, $n222);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/find_similar/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update_22898d8_225: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/find_similar/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo9 = $lepusPushFiberForNode($parent, 225, uniqueId),
          $forLepus = _$lepusPushFiberForNo9[0],
          $lastForLepus = _$lepusPushFiberForNo9[1];
      let $object = (_a = $data.data) == null ? undefined : _a.items;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.items;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n226 = $update2 ? $lepusGetElementRefByLepusID("view", 226) : null;
          let $temp2 = $update2;
          if (!$n226) {
            $update2 = false;
            $n226 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n226, 226, "view");
            __AppendElement($parent, $n226);
          }
          __SetStyleObject($n226, [index >= 1 ? [374] : []]);
          {
            let $n227 = $update2 ? $lepusGetElementRefByLepusID("image", 227) : null;
            let $temp3 = $update2;
            if (!$n227) {
              $update2 = false;
              $n227 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n227, 227, "image");
              __SetStyleObject($n227, [getClassStyleIndex("gyl-find_similar-cover", "26698000")]);
              __SetAttribute($n227, "mode", "aspectFill");
              __SetAttribute($n227, "skip-redirection", true);
              __SetAttribute($n227, "accessibility-element", false);
              __AppendElement($n226, $n227);
            }
            __SetAttribute($n227, "src", item == null ? undefined : item.image);
            $update2 = $temp3;
          }
          {
            let $n228 = $update2 ? $lepusGetElementRefByLepusID("view", 228) : null;
            let _$temp27 = $update2;
            if (!$n228) {
              $update2 = false;
              $n228 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n228, 228, "view");
              __SetStyleObject($n228, [getClassStyleIndex("gyl-find_similar-mask", "26698000")]);
              __AppendElement($n226, $n228);
            }
            $update2 = _$temp27;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/find_similar/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 223);
    $lepusGetElementRefByLepusID("view", 224);
    let $n225 = $lepusGetElementRefByLepusID("for", 225);
    $renderComponents[$path].update_22898d8_225($lepusComponent, $n225, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n223 = __CreateView($currentComponentId);
      __SetStyleObject($n223, [getClassStyleIndex("gyl-find_similar", "26698000")]);
      __AppendElement($component, $n223);
      let $n224 = __CreateView($currentComponentId);
      __SetStyleObject($n224, [getClassStyleIndex("gyl-find_similar-container", "26698000")]);
      __AppendElement($n223, $n224);
      let $n225 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n225, 225, "for");
      __AppendElement($n224, $n225);
      $renderComponents["components/find_similar/index"].update_22898d8_225($lepusComponent, $n225, $data, $props, $update2, false);
      let $n229 = __CreateText($currentComponentId);
      __SetStyleObject($n229, [getClassStyleIndex("gyl-find_similar-text", "26698000")]);
      __SetAttribute($n229, "accessibility-element", false);
      __AppendElement($n224, $n229);
      __SetAttribute($n229, "text", "Similar Items");
      let $n231 = __CreateImage($currentComponentId);
      __SetStyleObject($n231, [getClassStyleIndex("gyl-find_similar-arrow", "26698000")]);
      __SetAttribute($n231, "src", "network_address");
      __SetAttribute($n231, "skip-redirection", true);
      __SetAttribute($n231, "accessibility-element", false);
      __AppendElement($n224, $n231);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/horizontal_skus/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update_22898d8_233: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b;
    let $path = "components/horizontal_skus/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.sku_list) == null ? undefined : _b.length) <= 2) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n234 = $update2 ? $lepusGetElementRefByLepusID("view", 234) : null;
            let $temp2 = $update2;
            if (!$n234) {
              $update2 = false;
              $n234 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n234, 234, "view");
              __SetStyleObject($n234, [getClassStyleIndex("gyl-cover__horizontal-skus__short-card-wrapper", "17257000")]);
              __AppendElement($parent, $n234);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n235 = $update2 ? $lepusGetElementRefByLepusID("for", 235) : null;
              if (!$n235) {
                $n235 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n235, 235, "for");
                __AppendElement($n234, $n235);
              }
              $renderComponents[$path].update_22898d8_235($lepusComponent, $n235, $data, $props, $update2, $slotUpdate);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_235: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    if (!$update2 || $renderComponents["components/horizontal_skus/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo10 = $lepusPushFiberForNode($parent, 235, uniqueId),
          $forLepus = _$lepusPushFiberForNo10[0],
          $lastForLepus = _$lepusPushFiberForNo10[1];
      let $object = (_a = $data.data) == null ? undefined : _a.sku_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.sku_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n236 = $update2 ? $lepusGetElementRefByLepusID("view", 236) : null;
          let $temp2 = $update2;
          if (!$n236) {
            $update2 = false;
            $n236 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n236, 236, "view");
            __SetStyleObject($n236, [getClassStyleIndex("gyl-cover__horizontal-skus__short-card", "17257000")]);
            __AppendElement($parent, $n236);
          }
          {
            let $n237 = $update2 ? $lepusGetElementRefByLepusID("if", 237) : null;
            if (!$n237) {
              $n237 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n237, 237, "if");
              __AppendElement($n236, $n237);
            }
            let uniqueId2 = __GetElementUniqueID($n237);
            if (!$update2) {
              $conditionNodeIndex[uniqueId2] = -1;
            }
            let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
            if ((_c = item == null ? undefined : item.image) == null ? undefined : _c.url) {
              __UpdateIfNodeIndex($n237, 0);
              $conditionNodeIndex[uniqueId2] = 0;
              let $temp3 = $update2;
              if ($ifNodeIndex !== 0) {
                $update2 = false;
              }
              {
                let $n238 = $update2 ? $lepusGetElementRefByLepusID("image", 238) : null;
                let $temp4 = $update2;
                if (!$n238) {
                  $update2 = false;
                  $n238 = __CreateImage($currentComponentId);
                  $lepusStoreElementRefByLepusID($n238, 238, "image");
                  __SetAttribute($n238, "mode", "aspectFill");
                  __SetAttribute($n238, "skip-redirection", true);
                  __AppendElement($n237, $n238);
                }
                __SetStyleObject($n238, [getClassStyleIndex("gyl-cover__horizontal-skus__short-card-image", "17257000"), parseStyleStringToObject((_d = item == null ? undefined : item.image) == null ? undefined : _d.style)]);
                __SetAttribute($n238, "src", (_e = item == null ? undefined : item.image) == null ? undefined : _e.url);
                $update2 = $temp4;
              }
              $update2 = $temp3;
            } else {
              __UpdateIfNodeIndex($n237, -1);
              $conditionNodeIndex[uniqueId2] = -1;
            }
          }
          {
            let $n239 = $update2 ? $lepusGetElementRefByLepusID("view", 239) : null;
            let _$temp28 = $update2;
            if (!$n239) {
              $update2 = false;
              $n239 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n239, 239, "view");
              __SetStyleObject($n239, [getClassStyleIndex("gyl-cover__horizontal-skus__short-card-price-container", "17257000")]);
              __AppendElement($n236, $n239);
            }
            {
              let $n240 = $update2 ? $lepusGetElementRefByLepusID("if", 240) : null;
              if (!$n240) {
                $n240 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n240, 240, "if");
                __AppendElement($n239, $n240);
              }
              let _uniqueId = __GetElementUniqueID($n240);
              if (!$update2) {
                $conditionNodeIndex[_uniqueId] = -1;
              }
              let _$ifNodeIndex = $conditionNodeIndex[_uniqueId];
              if ((_f = item == null ? undefined : item.price) == null ? undefined : _f.content) {
                __UpdateIfNodeIndex($n240, 0);
                $conditionNodeIndex[_uniqueId] = 0;
                let _$temp29 = $update2;
                if (_$ifNodeIndex !== 0) {
                  $update2 = false;
                }
                {
                  let $n241 = $update2 ? $lepusGetElementRefByLepusID("text", 241) : null;
                  let $temp5 = $update2;
                  if (!$n241) {
                    $update2 = false;
                    $n241 = __CreateText($currentComponentId);
                    $lepusStoreElementRefByLepusID($n241, 241, "text");
                    __AppendElement($n240, $n241);
                  }
                  __SetStyleObject($n241, [getClassStyleIndex("gyl-cover__horizontal-skus__short-card-price", "17257000"), parseStyleStringToObject((_g = item == null ? undefined : item.price) == null ? undefined : _g.style)]);
                  __SetAttribute($n241, "text", "$" + ((_h = item == null ? undefined : item.price) == null ? undefined : _h.content));
                  $update2 = $temp5;
                }
                $update2 = _$temp29;
              } else {
                __UpdateIfNodeIndex($n240, -1);
                $conditionNodeIndex[_uniqueId] = -1;
              }
            }
            {
              let $n243 = $update2 ? $lepusGetElementRefByLepusID("if", 243) : null;
              if (!$n243) {
                $n243 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n243, 243, "if");
                __AppendElement($n239, $n243);
              }
              let _uniqueId2 = __GetElementUniqueID($n243);
              if (!$update2) {
                $conditionNodeIndex[_uniqueId2] = -1;
              }
              let _$ifNodeIndex2 = $conditionNodeIndex[_uniqueId2];
              if ((_i = item == null ? undefined : item.price_decimal) == null ? undefined : _i.content) {
                __UpdateIfNodeIndex($n243, 0);
                $conditionNodeIndex[_uniqueId2] = 0;
                let _$temp30 = $update2;
                if (_$ifNodeIndex2 !== 0) {
                  $update2 = false;
                }
                {
                  let $n244 = $update2 ? $lepusGetElementRefByLepusID("text", 244) : null;
                  let _$temp31 = $update2;
                  if (!$n244) {
                    $update2 = false;
                    $n244 = __CreateText($currentComponentId);
                    $lepusStoreElementRefByLepusID($n244, 244, "text");
                    __AppendElement($n243, $n244);
                  }
                  __SetStyleObject($n244, [getClassStyleIndex("gyl-cover__horizontal-skus__short-card-price-decimal", "17257000"), parseStyleStringToObject((_j = item == null ? undefined : item.price_decimal) == null ? undefined : _j.style)]);
                  __SetAttribute($n244, "text", (_k = item == null ? undefined : item.price_decimal) == null ? undefined : _k.content);
                  $update2 = _$temp31;
                }
                $update2 = _$temp30;
              } else {
                __UpdateIfNodeIndex($n243, -1);
                $conditionNodeIndex[_uniqueId2] = -1;
              }
            }
            $update2 = _$temp28;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_246: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b;
    let $path = "components/horizontal_skus/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.sku_list) == null ? undefined : _b.length) > 2) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n247 = $update2 ? $lepusGetElementRefByLepusID("view", 247) : null;
            let $temp2 = $update2;
            if (!$n247) {
              $update2 = false;
              $n247 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n247, 247, "view");
              __SetStyleObject($n247, [getClassStyleIndex("gyl-cover__horizontal-skus__long-card", "17257000")]);
              __AppendElement($parent, $n247);
            }
            {
              let $n248 = $update2 ? $lepusGetElementRefByLepusID("view", 248) : null;
              let $temp3 = $update2;
              if (!$n248) {
                $update2 = false;
                $n248 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n248, 248, "view");
                __SetStyleObject($n248, [getClassStyleIndex("gyl-cover__horizontal-skus__long-card-image-wrapper", "17257000")]);
                __AppendElement($n247, $n248);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $n249 = $update2 ? $lepusGetElementRefByLepusID("for", 249) : null;
                if (!$n249) {
                  $n249 = __CreateFor($currentComponentId);
                  $lepusStoreElementRefByLepusID($n249, 249, "for");
                  __AppendElement($n248, $n249);
                }
                $renderComponents[$path].update_22898d8_249($lepusComponent, $n249, $data, $props, $update2, $slotUpdate);
              }
              $update2 = $temp3;
            }
            {
              let $template_update = $update2;
              let $n253 = $update2 ? $lepusGetElementRefByLepusID("if", 253) : null;
              if (!$n253) {
                $update2 = false;
                $n253 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n253, 253, "if");
                __AppendElement($n247, $n253);
              }
              $renderComponents[$path].update_22898d8_253($lepusComponent, $n253, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_249: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e;
    if (!$update2 || $renderComponents["components/horizontal_skus/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo11 = $lepusPushFiberForNode($parent, 249, uniqueId),
          $forLepus = _$lepusPushFiberForNo11[0],
          $lastForLepus = _$lepusPushFiberForNo11[1];
      let $object = (_a = $data.data) == null ? undefined : _a.sku_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.sku_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n250 = $update2 ? $lepusGetElementRefByLepusID("view", 250) : null;
          let $temp2 = $update2;
          if (!$n250) {
            $update2 = false;
            $n250 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n250, 250, "view");
            __AppendElement($parent, $n250);
          }
          {
            let $n251 = $update2 ? $lepusGetElementRefByLepusID("if", 251) : null;
            if (!$n251) {
              $n251 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n251, 251, "if");
              __AppendElement($n250, $n251);
            }
            let uniqueId2 = __GetElementUniqueID($n251);
            if (!$update2) {
              $conditionNodeIndex[uniqueId2] = -1;
            }
            let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
            if ((_c = item == null ? undefined : item.image) == null ? undefined : _c.url) {
              __UpdateIfNodeIndex($n251, 0);
              $conditionNodeIndex[uniqueId2] = 0;
              let $temp3 = $update2;
              if ($ifNodeIndex !== 0) {
                $update2 = false;
              }
              {
                let $n252 = $update2 ? $lepusGetElementRefByLepusID("image", 252) : null;
                let $temp4 = $update2;
                if (!$n252) {
                  $update2 = false;
                  $n252 = __CreateImage($currentComponentId);
                  $lepusStoreElementRefByLepusID($n252, 252, "image");
                  __SetAttribute($n252, "mode", "aspectFill");
                  __SetAttribute($n252, "skip-redirection", true);
                  __AppendElement($n251, $n252);
                }
                __SetStyleObject($n252, [getClassStyleIndex("gyl-cover__horizontal-skus__long-card-image", "17257000"), parseStyleStringToObject((_d = item == null ? undefined : item.image) == null ? undefined : _d.style)]);
                __SetAttribute($n252, "src", (_e = item == null ? undefined : item.image) == null ? undefined : _e.url);
                $update2 = $temp4;
              }
              $update2 = $temp3;
            } else {
              __UpdateIfNodeIndex($n251, -1);
              $conditionNodeIndex[uniqueId2] = -1;
            }
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_253: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/horizontal_skus/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.min_price) == null ? undefined : _b.content) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n254 = $update2 ? $lepusGetElementRefByLepusID("view", 254) : null;
          let $temp2 = $update2;
          if (!$n254) {
            $update2 = false;
            $n254 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n254, 254, "view");
            __SetStyleObject($n254, [getClassStyleIndex("gyl-cover__horizontal-skus__long-card-price", "17257000")]);
            __AppendElement($parent, $n254);
          }
          {
            let $n255 = $update2 ? $lepusGetElementRefByLepusID("text", 255) : null;
            let $temp3 = $update2;
            if (!$n255) {
              $update2 = false;
              $n255 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n255, 255, "text");
              __AppendElement($n254, $n255);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n255, [getClassStyleIndex("gyl-cover__horizontal-skus__long-card-price-content", "17257000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.min_price) == null ? undefined : _d.style)]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $value = "$" + ((_f = (_e = $data.data) == null ? undefined : _e.min_price) == null ? undefined : _f.content);
                if (!$update2 || $value !== "$" + ((_h = (_g = $lepusComponent._data.data) == null ? undefined : _g.min_price) == null ? undefined : _h.content)) {
                  __SetAttribute($n255, "text", $value);
                }
              }
            }
            $update2 = $temp3;
          }
          {
            let $template_update = $update2;
            let $n257 = $update2 ? $lepusGetElementRefByLepusID("if", 257) : null;
            if (!$n257) {
              $update2 = false;
              $n257 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n257, 257, "if");
              __AppendElement($n254, $n257);
            }
            $renderComponents[$path].update_22898d8_257($lepusComponent, $n257, $data, $props, $update2, $slotUpdate);
            $update2 = $template_update;
          }
          {
            let $n260 = $update2 ? $lepusGetElementRefByLepusID("text", 260) : null;
            let _$temp32 = $update2;
            if (!$n260) {
              $update2 = false;
              $n260 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n260, 260, "text");
              __SetStyleObject($n260, [getClassStyleIndex("gyl-cover__horizontal-skus__long-card-price-suffix", "17257000")]);
              __AppendElement($n254, $n260);
            }
            __SetAttribute($n260, "text", "from");
            $update2 = _$temp32;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_257: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/horizontal_skus/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.min_price_decimal) == null ? undefined : _b.content) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n258 = $update2 ? $lepusGetElementRefByLepusID("text", 258) : null;
          let $temp2 = $update2;
          if (!$n258) {
            $update2 = false;
            $n258 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n258, 258, "text");
            __AppendElement($parent, $n258);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n258, [getClassStyleIndex("gyl-cover__horizontal-skus__long-card-price-decimal", "17257000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.min_price_decimal) == null ? undefined : _d.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $value = (_f = (_e = $data.data) == null ? undefined : _e.min_price_decimal) == null ? undefined : _f.content;
              if (!$update2 || $value !== ((_h = (_g = $lepusComponent._data.data) == null ? undefined : _g.min_price_decimal) == null ? undefined : _h.content)) {
                __SetAttribute($n258, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/horizontal_skus/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 232);
    let $n233 = $lepusGetElementRefByLepusID("if", 233);
    $renderComponents[$path].update_22898d8_233($lepusComponent, $n233, $data, $props, $update2, $slotUpdate);
    let $n246 = $lepusGetElementRefByLepusID("if", 246);
    $renderComponents[$path].update_22898d8_246($lepusComponent, $n246, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $path = "components/horizontal_skus/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n232 = __CreateView($currentComponentId);
      __SetStyleObject($n232, [getClassStyleIndex("gyl-cover__horizontal-skus__container", "17257000")]);
      __AppendElement($component, $n232);
      let $n233 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n233, 233, "if");
      __AppendElement($n232, $n233);
      $renderComponents[$path].update_22898d8_233($lepusComponent, $n233, $data, $props, $update2, $slotUpdate);
      let $n246 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n246, 246, "if");
      __AppendElement($n232, $n246);
      $renderComponents[$path].update_22898d8_246($lepusComponent, $n246, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/image/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c, _d;
    let $path = "components/image/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n262 = $lepusGetElementRefByLepusID("image", 262);
      {
        let $value = "width:" + $data.data.width + ";height:" + $data.data.height + ";";
        if (!$update2 || $value !== "width:" + $lepusComponent._data.data.width + ";height:" + $lepusComponent._data.data.height + ";") {
          __SetStyleObject($n262, [{
            27: $data.data.width + ""
          }, {
            26: $data.data.height + ""
          }]);
        }
      }
      {
        let _$value25 = (_b = (_a = $data.data) == null ? undefined : _a.url_list) == null ? undefined : _b[0];
        if (!$update2 || _$value25 !== ((_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.url_list) == null ? undefined : _d[0])) {
          __SetAttribute($n262, "src", _$value25);
        }
      }
    }
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent) {
    let _a, _b;
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n262 = __CreateImage($currentComponentId);
      $lepusStoreElementRefByLepusID($n262, 262, "image");
      __SetStyleObject($n262, [{
        27: $data.data.width + ""
      }, {
        26: $data.data.height + ""
      }]);
      __SetAttribute($n262, "mode", "aspectFill");
      __SetAttribute($n262, "flatten", false);
      __SetAttribute($n262, "clip-radius", true);
      __SetAttribute($n262, "src", (_b = (_a = $data.data) == null ? undefined : _a.url_list) == null ? undefined : _b[0]);
      __AppendElement($component, $n262);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/mall_cover_common/index"] = {
  variables: ["info", "dynamicInfo", "isCacheData", "imageMonitorTag"],
  varUpdateState: [],
  update_3dbf210_265: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o;
    let $path = "components/mall_cover_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.video_engine) == null ? undefined : _b.src) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let _$lepusPushOwner5 = $lepusPushOwner(uniqueId + "_0"),
              owner = _$lepusPushOwner5[0],
              lastOwner = _$lepusPushOwner5[1];
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n266 = $update2 ? $lepusGetElementRefByLepusID("component", 266) : null;
            let $compCreated = true;
            let $childLepusComponent = null;
            let $componentId = null;
            if (!$n266) {
              $compCreated = false;
              $n266 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 15, "", "video-engine", "components/video_engine/index", {});
              let $nid266 = $lepusStoreElementRefByLepusID($n266, 266, "video-engine");
              $componentId = $nid266[0];
              $childLepusComponent = $componentConstructor($componentId, $n266, "components/video_engine/index", 266);
              $createdIds.push($componentId + "");
              $cardInstance._currentOwner.componentIds.push($componentId);
              __SetAttribute($n266, "clip-radius", "true");
              __SetAttribute($n266, "user-interaction-enabled", false);
              __AddEvent($n266, "bindEvent", "ReportLog", "onReportVideoMetricLog");
              __AddEvent($n266, "bindEvent", "VideoEnd", "_handleVideoPlayEnd");
              __AddEvent($n266, "bindEvent", "VideoError", "_handleVideoError");
              __AddEvent($n266, "bindEvent", "VideoClick", "handleCoverComonClick");
              __AppendElement($parent, $n266);
            } else {
              $componentId = __GetElementUniqueID($n266);
              $childLepusComponent = $componentInfo[$componentId];
            }
            $comUpdatePropsSet = [];
            $childLepusComponent._setProp("playing", !!((_c = $data.dynamicInfo) == null ? undefined : _c.isVideoPlaying), $update2 && $compCreated);
            $childLepusComponent._setProp("src", (_e = (_d = $data.info) == null ? undefined : _d.video_engine) == null ? undefined : _e.src, $update2 && $compCreated);
            $childLepusComponent._setProp("tag", (_g = (_f = $data.info) == null ? undefined : _f.video_engine) == null ? undefined : _g.tag, $update2 && $compCreated);
            $childLepusComponent._setProp("subTag", (_i = (_h = $data.info) == null ? undefined : _h.video_engine) == null ? undefined : _i.subtag, $update2 && $compCreated);
            $childLepusComponent._setProp("startTime", (_k = (_j = $data.info) == null ? undefined : _j.video_engine) == null ? undefined : _k.start_time, $update2 && $compCreated);
            $childLepusComponent._setProp("endTime", (_m = (_l = $data.info) == null ? undefined : _l.video_engine) == null ? undefined : _m.end_time, $update2 && $compCreated);
            {
              __SetStyleObject($n266, [getClassStyleIndex("video_engine", "20117000"), parseStyleStringToObject((_o = (_n = $data.info) == null ? undefined : _n.video_engine) == null ? undefined : _o.style), parseStyleStringToObject(getVideoEngineStyle())]);
            }
            if ($compCreated) {
              let $update_keys = $comUpdatePropsSet;
              let $childSlotUpdate = false;
              if ($update_keys.length > 0 || $childSlotUpdate) {
                $updatedIds.push($componentId + "");
                $renderComponents["components/video_engine/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
              }
            } else {
              $renderComponents["components/video_engine/index"].entry($n266, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
            }
          }
          $update2 = $temp;
          $lepusPopOwner(lastOwner);
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_280618_267: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s, _t, _u, _v, _w, _x, _y, _z, _A, _B, _C, _D, _E, _F, _G, _H;
    let $path = "components/mall_cover_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.isCacheData === 0 && ((_b = (_a = $data.info) == null ? undefined : _a.live) == null ? undefined : _b.stream_data) && ((_c = $data.dynamicInfo) == null ? undefined : _c.allowLivePlay)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n268 = $update2 ? $lepusGetElementRefByLepusID("view", 268) : null;
            let $temp2 = $update2;
            if (!$n268) {
              $update2 = false;
              $n268 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n268, 268, "view");
              __SetStyleObject($n268, [getClassStyleIndex("live_wrapper", "20117000")]);
              __AppendElement($parent, $n268);
            }
            {
              let $n269 = $update2 ? $lepusGetElementRefByLepusID("x-live-ng", 269) : null;
              let $temp3 = $update2;
              if (!$n269) {
                $update2 = false;
                $n269 = __CreateElement("x-live-ng", $currentComponentId);
                $lepusStoreElementRefByLepusID($n269, 269, "x-live-ng");
                __SetAttribute($n269, "mute", true);
                __SetAttribute($n269, "objectfit", "cover");
                __AddEvent($n269, "bindEvent", "stop", "handleLiveStop");
                __AddEvent($n269, "bindEvent", "error", "handleLiveError");
                __AddEvent($n269, "bindEvent", "ended", "handleLiveEnded");
                __AddEvent($n269, "bindEvent", "firstframe", "handleLiveFirstFrame");
                __AddEvent($n269, "catchEvent", "tap", "handleCoverComonClick");
                __AppendElement($n268, $n269);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[0]) {
                {
                  let $value = "live " + (((_d = $data.dynamicInfo) == null ? undefined : _d.allowLiveShow) ? "live_show" : "");
                  if (!$update2 || $value !== "live " + (((_e = $lepusComponent._data.dynamicInfo) == null ? undefined : _e.allowLiveShow) ? "live_show" : "")) {
                    __SetStyleObject($n269, [getClassStyleIndex("live " + (((_f = $data.dynamicInfo) == null ? undefined : _f.allowLiveShow) ? "live_show" : ""), "20117000")]);
                  }
                }
                {
                  let _$value26 = (_h = (_g = $data.info) == null ? undefined : _g.live) == null ? undefined : _h.stream_data;
                  if (!$update2 || _$value26 !== ((_j = (_i = $lepusComponent._data.info) == null ? undefined : _i.live) == null ? undefined : _j.stream_data)) {
                    __SetAttribute($n269, "stream-data", _$value26);
                  }
                }
                {
                  let _$value27 = ((_l = (_k = $data.info) == null ? undefined : _k.live) == null ? undefined : _l.room_id) || "product_with_live_room_id";
                  if (!$update2 || _$value27 !== (((_n = (_m = $lepusComponent._data.info) == null ? undefined : _m.live) == null ? undefined : _n.room_id) || "product_with_live_room_id")) {
                    __SetAttribute($n269, "room-id", _$value27);
                  }
                }
                {
                  let _$value28 = ((_p = (_o = $data.info) == null ? undefined : _o.live) == null ? undefined : _p.biz_domain) || "product_with_live_biz_domain";
                  if (!$update2 || _$value28 !== (((_r = (_q = $lepusComponent._data.info) == null ? undefined : _q.live) == null ? undefined : _r.biz_domain) || "product_with_live_biz_domain")) {
                    __SetAttribute($n269, "biz-domain", _$value28);
                  }
                }
                {
                  let _$value29 = ((_t = (_s = $data.info) == null ? undefined : _s.live) == null ? undefined : _t.page) || "product_with_live_page";
                  if (!$update2 || _$value29 !== (((_v = (_u = $lepusComponent._data.info) == null ? undefined : _u.live) == null ? undefined : _v.page) || "product_with_live_page")) {
                    __SetAttribute($n269, "page", _$value29);
                  }
                }
                {
                  let _$value30 = ((_x = (_w = $data.info) == null ? undefined : _w.live) == null ? undefined : _x.block) || "product_with_live_block";
                  if (!$update2 || _$value30 !== (((_z = (_y = $lepusComponent._data.info) == null ? undefined : _y.live) == null ? undefined : _z.block) || "product_with_live_block")) {
                    __SetAttribute($n269, "block", _$value30);
                  }
                }
                {
                  let _$value31 = ((_B = (_A = $data.info) == null ? undefined : _A.live) == null ? undefined : _B.index) || "product_with_live_index";
                  if (!$update2 || _$value31 !== (((_D = (_C = $lepusComponent._data.info) == null ? undefined : _C.live) == null ? undefined : _D.index) || "product_with_live_index")) {
                    __SetAttribute($n269, "index", _$value31);
                  }
                }
                {
                  let _$value32 = ((_F = (_E = $data.info) == null ? undefined : _E.live) == null ? undefined : _F.id) || "product_with_live_id";
                  if (!$update2 || _$value32 !== (((_H = (_G = $lepusComponent._data.info) == null ? undefined : _G.live) == null ? undefined : _H.id) || "product_with_live_id")) {
                    __SetID($n269, _$value32);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_318dca8_270: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "components/mall_cover_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.info) == null ? undefined : _a.feed_back_button) == null ? undefined : _b.icon) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n271 = $update2 ? $lepusGetElementRefByLepusID("view", 271) : null;
            let $temp2 = $update2;
            if (!$n271) {
              $update2 = false;
              $n271 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n271, 271, "view");
              __SetStyleObject($n271, [getClassStyleIndex("feed_back_button", "20117000")]);
              __AddEvent($n271, "catchEvent", "tap", "handleFeedBackButtonClick");
              __AppendElement($parent, $n271);
            }
            {
              let $n272 = $update2 ? $lepusGetElementRefByLepusID("image", 272) : null;
              let $temp3 = $update2;
              if (!$n272) {
                $update2 = false;
                $n272 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n272, 272, "image");
                __SetStyleObject($n272, [getClassStyleIndex("feed_back_button_image", "20117000")]);
                __SetAttribute($n272, "skip-redirection", true);
                __AppendElement($n271, $n272);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  let $value = (_d = (_c = $data.info) == null ? undefined : _c.feed_back_button) == null ? undefined : _d.icon;
                  if (!$update2 || $value !== ((_f = (_e = $lepusComponent._data.info) == null ? undefined : _e.feed_back_button) == null ? undefined : _f.icon)) {
                    __SetAttribute($n272, "src", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c;
    let $path = "components/mall_cover_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 263);
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3]) {
      {
        let $n264 = $lepusGetElementRefByLepusID("component", 264);
        let $componentId = __GetElementUniqueID($n264);
        let $childLepusComponent = $componentInfo[$componentId];
        $comUpdatePropsSet = [];
        $childLepusComponent._setProp("data", $data.info, $update2);
        $childLepusComponent._setProp("usePiercedSrc", (_a = $data.dynamicInfo) == null ? undefined : _a.usePiercedSrc, $update2);
        $childLepusComponent._setProp("isPlaying", ((_b = $data.dynamicInfo) == null ? undefined : _b.isSwiperPlaying) || ((_c = $data.dynamicInfo) == null ? undefined : _c.isVideoPlaying), $update2);
        $childLepusComponent._setProp("isCacheData", $data.isCacheData, $update2);
        $childLepusComponent._setProp("imageMonitorTag", $data.imageMonitorTag, $update2);
        let $update_keys = $comUpdatePropsSet;
        let $childSlotUpdate = false;
        if ($update_keys.length > 0 || $childSlotUpdate) {
          $updatedIds.push($componentId + "");
          $renderComponents["components/cover_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
        }
      }
    }
    let $n265 = $lepusGetElementRefByLepusID("if", 265);
    $renderComponents[$path].update_3dbf210_265($lepusComponent, $n265, $data, $props, $update2, $slotUpdate);
    let $n267 = $lepusGetElementRefByLepusID("if", 267);
    $renderComponents[$path].update_280618_267($lepusComponent, $n267, $data, $props, $update2, $slotUpdate);
    let $n270 = $lepusGetElementRefByLepusID("if", 270);
    $renderComponents[$path].update_318dca8_270($lepusComponent, $n270, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a, _b, _c;
    let $path = "components/mall_cover_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n263 = __CreateView($currentComponentId);
      __SetStyleObject($n263, [getClassStyleIndex("container", "20117000")]);
      __AddDataset($n263, "type", "cover");
      __AppendElement($component, $n263);
      {
        let $n264 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 14, "", "cover-common", "components/cover_common/index", {});
        let $nid264 = $lepusStoreElementRefByLepusID($n264, 264, "cover-common");
        let $childLepusComponent = $componentConstructor($nid264[0], $n264, "components/cover_common/index", 264);
        $cardInstance._currentOwner.componentIds.push($nid264[0]);
        $comUpdatePropsSet = [];
        $childLepusComponent._setProp("isNA", true, $update2);
        $childLepusComponent._setProp("data", $data.info, $update2);
        $childLepusComponent._setProp("usePiercedSrc", (_a = $data.dynamicInfo) == null ? undefined : _a.usePiercedSrc, $update2);
        $childLepusComponent._setProp("isPlaying", ((_b = $data.dynamicInfo) == null ? undefined : _b.isSwiperPlaying) || ((_c = $data.dynamicInfo) == null ? undefined : _c.isVideoPlaying), $update2);
        $childLepusComponent._setProp("isCacheData", $data.isCacheData, $update2);
        $childLepusComponent._setProp("imageMonitorTag", $data.imageMonitorTag, $update2);
        __AddEvent($n264, "bindEvent", "Click", "handleCoverComonClick");
        __AddEvent($n264, "bindEvent", "VideoError", "_handleVideoError");
        __AddEvent($n264, "bindEvent", "VideoEnd", "_handleVideoPlayEnd");
        __AddEvent($n264, "bindEvent", "SwiperChange", "handleSwiperImagesChange");
        __AppendElement($n263, $n264);
        $renderComponents["components/cover_common/index"].entry($n264, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
        $createdIds.push($nid264[0] + "");
      }
      let $n265 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n265, 265, "if");
      __AppendElement($n263, $n265);
      $renderComponents[$path].update_3dbf210_265($lepusComponent, $n265, $data, $props, $update2, $slotUpdate);
      let $n267 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n267, 267, "if");
      __AppendElement($n263, $n267);
      $renderComponents[$path].update_280618_267($lepusComponent, $n267, $data, $props, $update2, $slotUpdate);
      let $n270 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n270, 270, "if");
      __AppendElement($n263, $n270);
      $renderComponents[$path].update_318dca8_270($lepusComponent, $n270, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/media_wrapper/index"] = {
  variables: ["enable", "mediatype", "name", "mediaName", "transition_element_id", "transition_item_id", "mediaid"],
  varUpdateState: [],
  update_335b378_273: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let $path = "components/media_wrapper/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3] || $renderComponents[$path].varUpdateState[4] || $renderComponents[$path].varUpdateState[5] || $renderComponents[$path].varUpdateState[6]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.enable && canUseMediaWrapper()) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n274 = $update2 ? $lepusGetElementRefByLepusID("ecom-media-wrapper", 274) : null;
            let $temp2 = $update2;
            if (!$n274) {
              $update2 = false;
              $n274 = __CreateElement("ecom-media-wrapper", $currentComponentId);
              $lepusStoreElementRefByLepusID($n274, 274, "ecom-media-wrapper");
              __SetStyleObject($n274, [5, 16]);
              __SetAttribute($n274, "is_na_mall", true);
              __AddEvent($n274, "bindEvent", "updateFilters", "handleUpdateFilters");
              __AppendElement($parent, $n274);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3] || $renderComponents[$path].varUpdateState[4] || $renderComponents[$path].varUpdateState[5] || $renderComponents[$path].varUpdateState[6]) {
              {
                let $value = $data.mediatype;
                if (!$update2 || $value !== $lepusComponent._data.mediatype) {
                  __SetAttribute($n274, "mediatype", $value);
                }
              }
              {
                let _$value33 = $data.name || $data.mediaName;
                if (!$update2 || _$value33 !== ($lepusComponent._data.name || $lepusComponent._data.mediaName)) {
                  __SetAttribute($n274, "name", _$value33);
                }
              }
              {
                let _$value34 = $data.transition_element_id;
                if (!$update2 || _$value34 !== $lepusComponent._data.transition_element_id) {
                  __SetAttribute($n274, "transition_element_id", _$value34);
                }
              }
              {
                let _$value35 = $data.transition_item_id;
                if (!$update2 || _$value35 !== $lepusComponent._data.transition_item_id) {
                  __SetAttribute($n274, "transition_item_id", _$value35);
                }
              }
              {
                let _$value36 = $data.mediaid;
                if (!$update2 || _$value36 !== $lepusComponent._data.mediaid) {
                  __SetID($n274, _$value36);
                }
              }
            }
            let $slot = $lepusComponent.slots["default"];
            if ($slot && (!$update2 || $slotUpdate)) {
              $slot.fn($slot.componentId, $n274, $update2);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp33 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          let _$slot = $lepusComponent.slots["default"];
          if (_$slot && (!$update2 || $slotUpdate)) {
            _$slot.fn(_$slot.componentId, $parent, $update2);
          }
          $update2 = _$temp33;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/media_wrapper/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    let $n273 = $lepusGetElementRefByLepusID("if", 273);
    $renderComponents[$path].update_335b378_273($lepusComponent, $n273, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n273 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n273, 273, "if");
      __AppendElement($component, $n273);
      $renderComponents["components/media_wrapper/index"].update_335b378_273($lepusComponent, $n273, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/price_common/index"] = {
  variables: ["data", "isCacheData"],
  varUpdateState: [],
  update_1200e30_278: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_c = (_b = (_a = $data.data) == null ? undefined : _a.price) == null ? undefined : _b.integers) == null ? undefined : _c.content) && ($data.isCacheData !== 0 || !((_d = $data.data) == null ? undefined : _d.integer_scroll_list))) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n279 = $update2 ? $lepusGetElementRefByLepusID("text", 279) : null;
            let $temp2 = $update2;
            if (!$n279) {
              $update2 = false;
              $n279 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n279, 279, "text");
              __SetAttribute($n279, "accessibility-element", false);
              __AppendElement($parent, $n279);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n279, [getClassStyleIndex("gyl-price__number gyl-price__space--ml-1", "51983000"), parseStyleStringToObject((_g = (_f = (_e = $data.data) == null ? undefined : _e.price) == null ? undefined : _f.integers) == null ? undefined : _g.style)]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $value = (_j = (_i = (_h = $data.data) == null ? undefined : _h.price) == null ? undefined : _i.integers) == null ? undefined : _j.content;
                if (!$update2 || $value !== ((_m = (_l = (_k = $lepusComponent._data.data) == null ? undefined : _k.price) == null ? undefined : _l.integers) == null ? undefined : _m.content)) {
                  __SetAttribute($n279, "text", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_1200e30_281: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_c = (_b = (_a = $data.data) == null ? undefined : _a.price) == null ? undefined : _b.decimals) == null ? undefined : _c.content) && ($data.isCacheData !== 0 || !((_d = $data.data) == null ? undefined : _d.integer_scroll_list))) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n282 = $update2 ? $lepusGetElementRefByLepusID("text", 282) : null;
            let $temp2 = $update2;
            if (!$n282) {
              $update2 = false;
              $n282 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n282, 282, "text");
              __SetAttribute($n282, "accessibility-element", false);
              __AppendElement($parent, $n282);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n282, [getClassStyleIndex("gyl-price__number", "51983000"), parseStyleStringToObject((_g = (_f = (_e = $data.data) == null ? undefined : _e.price) == null ? undefined : _f.decimals) == null ? undefined : _g.style)]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $value = (_j = (_i = (_h = $data.data) == null ? undefined : _h.price) == null ? undefined : _i.decimals) == null ? undefined : _j.content;
                if (!$update2 || $value !== ((_m = (_l = (_k = $lepusComponent._data.data) == null ? undefined : _k.price) == null ? undefined : _l.decimals) == null ? undefined : _m.content)) {
                  __SetAttribute($n282, "text", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_1200e30_284: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.isCacheData === 0 && ((_b = (_a = $data.data) == null ? undefined : _a.integer_scroll_list) == null ? undefined : _b.length)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n285 = $update2 ? $lepusGetElementRefByLepusID("view", 285) : null;
            let $temp2 = $update2;
            if (!$n285) {
              $update2 = false;
              $n285 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n285, 285, "view");
              __SetStyleObject($n285, [getClassStyleIndex("scroll_price_container", "51983000")]);
              __AppendElement($parent, $n285);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n286 = $update2 ? $lepusGetElementRefByLepusID("for", 286) : null;
              if (!$n286) {
                $n286 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n286, 286, "for");
                __AppendElement($n285, $n286);
              }
              $renderComponents[$path].update_22898d8_286($lepusComponent, $n286, $data, $props, $update2, $slotUpdate);
            }
            {
              let $template_update = $update2;
              let $n293 = $update2 ? $lepusGetElementRefByLepusID("if", 293) : null;
              if (!$n293) {
                $update2 = false;
                $n293 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n293, 293, "if");
                __AppendElement($n285, $n293);
              }
              $renderComponents[$path].update_22898d8_293($lepusComponent, $n293, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n296 = $update2 ? $lepusGetElementRefByLepusID("for", 296) : null;
              if (!$n296) {
                $n296 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n296, 296, "for");
                __AppendElement($n285, $n296);
              }
              $renderComponents[$path].update_22898d8_296($lepusComponent, $n296, $data, $props, $update2, $slotUpdate);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_286: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo12 = $lepusPushFiberForNode($parent, 286, uniqueId),
          $forLepus = _$lepusPushFiberForNo12[0],
          $lastForLepus = _$lepusPushFiberForNo12[1];
      let $object = (_a = $data.data) == null ? undefined : _a.integer_scroll_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.integer_scroll_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let scrollIndexInt = 0; scrollIndexInt < $length; ++scrollIndexInt) {
        $update2 = scrollIndexInt < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(scrollIndexInt);
        let scrollItemInt = $object[scrollIndexInt];
        $oldObject ? $oldObject[scrollIndexInt] : null;
        {
          let $n287 = $update2 ? $lepusGetElementRefByLepusID("view", 287) : null;
          let $temp2 = $update2;
          if (!$n287) {
            $update2 = false;
            $n287 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n287, 287, "view");
            __AppendElement($parent, $n287);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n287, [getClassStyleIndex("scroll_price_num_list", "51983000"), parseStyleStringToObject(getScrollPriceNumListStyle((_c = $data.data) == null ? undefined : _c.integer_scroll_list))]);
            }
          }
          {
            let $n288 = $update2 ? $lepusGetElementRefByLepusID("view", 288) : null;
            let $temp3 = $update2;
            if (!$n288) {
              $update2 = false;
              $n288 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n288, 288, "view");
              __SetAttribute($n288, "implicit-animation", "false");
              __SetAttribute($n288, "enable-new-animator", "true");
              __AppendElement($n287, $n288);
            }
            __SetStyleObject($n288, [getClassStyleIndex("scroll_price_num_list_wrapper", "51983000"), parseStyleStringToObject(getScrollPriceNumListWrapperStyle(scrollItemInt, (_e = (_d = $data.data) == null ? undefined : _d.integer_scroll_list) == null ? undefined : _e.length))]);
            __SetID($n288, "numListIntWrapper" + scrollIndexInt);
            let $n289 = $update2 ? $lepusGetElementRefByLepusID("for", 289) : null;
            if (!$n289) {
              $n289 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n289, 289, "for");
              __AppendElement($n288, $n289);
            }
            {
              let _$lepusPushFiberForNo13 = $lepusPushFiberForNode($n289, 289, undefined),
                  $forLepus2 = _$lepusPushFiberForNo13[0],
                  $lastForLepus2 = _$lepusPushFiberForNo13[1];
              let $object2 = scrollItemInt == null ? undefined : scrollItemInt.num_list;
              let $length2 = _GetLength($object2);
              __UpdateForChildCount($n289, $length2);
              let $temp4 = $update2;
              for (let index = 0; index < $length2; ++index) {
                $lepusUpdateFiberForNodeIndex(index);
                let numItem = $object2[index];
                {
                  let $n290 = $update2 ? $lepusGetElementRefByLepusID("view", 290) : null;
                  let $temp5 = $update2;
                  if (!$n290) {
                    $update2 = false;
                    $n290 = __CreateView($currentComponentId);
                    $lepusStoreElementRefByLepusID($n290, 290, "view");
                    __AppendElement($n289, $n290);
                  }
                  __SetStyleObject($n290, [getClassStyleIndex("scroll_price_num_list_item", "51983000"), parseStyleStringToObject(getScrollPriceNumItemStyle(scrollItemInt))]);
                  {
                    let $n291 = $update2 ? $lepusGetElementRefByLepusID("text", 291) : null;
                    let $temp6 = $update2;
                    if (!$n291) {
                      $update2 = false;
                      $n291 = __CreateText($currentComponentId);
                      $lepusStoreElementRefByLepusID($n291, 291, "text");
                      __AppendElement($n290, $n291);
                    }
                    __SetStyleObject($n291, [getClassStyleIndex("scroll_price_num_list_item_text", "51983000"), (scrollItemInt == null ? undefined : scrollItemInt.style) || []]);
                    __SetAttribute($n291, "text", numItem);
                    $update2 = $temp6;
                  }
                  $update2 = $temp5;
                }
              }
              $update2 = $temp4;
              $lepusPushFiberForNode($lastForLepus2, undefined, undefined);
            }
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_293: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.decimal_scroll_list) == null ? undefined : _b.length) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n294 = $update2 ? $lepusGetElementRefByLepusID("text", 294) : null;
          let $temp2 = $update2;
          if (!$n294) {
            $update2 = false;
            $n294 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n294, 294, "text");
            __SetStyleObject($n294, [getClassStyleIndex("scroll_price_point", "51983000")]);
            __AppendElement($parent, $n294);
          }
          __SetAttribute($n294, "text", ".");
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_296: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo14 = $lepusPushFiberForNode($parent, 296, uniqueId),
          $forLepus = _$lepusPushFiberForNo14[0],
          $lastForLepus = _$lepusPushFiberForNo14[1];
      let $object = (_a = $data.data) == null ? undefined : _a.decimal_scroll_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.decimal_scroll_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let scrollIndexDec = 0; scrollIndexDec < $length; ++scrollIndexDec) {
        $update2 = scrollIndexDec < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(scrollIndexDec);
        let scrollItemDec = $object[scrollIndexDec];
        $oldObject ? $oldObject[scrollIndexDec] : null;
        {
          let $n297 = $update2 ? $lepusGetElementRefByLepusID("view", 297) : null;
          let $temp2 = $update2;
          if (!$n297) {
            $update2 = false;
            $n297 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n297, 297, "view");
            __AppendElement($parent, $n297);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n297, [getClassStyleIndex("scroll_price_num_list translateY4", "51983000"), parseStyleStringToObject(getScrollPriceNumListStyle((_c = $data.data) == null ? undefined : _c.decimal_scroll_list))]);
            }
          }
          {
            let $n298 = $update2 ? $lepusGetElementRefByLepusID("view", 298) : null;
            let $temp3 = $update2;
            if (!$n298) {
              $update2 = false;
              $n298 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n298, 298, "view");
              __SetAttribute($n298, "implicit-animation", "false");
              __SetAttribute($n298, "enable-new-animator", "true");
              __AppendElement($n297, $n298);
            }
            __SetStyleObject($n298, [getClassStyleIndex("scroll_price_num_list_wrapper", "51983000"), parseStyleStringToObject(getScrollPriceNumListWrapperStyle(scrollItemDec, (_e = (_d = $data.data) == null ? undefined : _d.decimal_scroll_list) == null ? undefined : _e.length))]);
            __SetID($n298, "numListDecWrapper" + scrollIndexDec);
            let $n299 = $update2 ? $lepusGetElementRefByLepusID("for", 299) : null;
            if (!$n299) {
              $n299 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n299, 299, "for");
              __AppendElement($n298, $n299);
            }
            {
              let _$lepusPushFiberForNo15 = $lepusPushFiberForNode($n299, 299, undefined),
                  $forLepus2 = _$lepusPushFiberForNo15[0],
                  $lastForLepus2 = _$lepusPushFiberForNo15[1];
              let $object2 = scrollItemDec == null ? undefined : scrollItemDec.num_list;
              let $length2 = _GetLength($object2);
              __UpdateForChildCount($n299, $length2);
              let $temp4 = $update2;
              for (let index = 0; index < $length2; ++index) {
                $lepusUpdateFiberForNodeIndex(index);
                let numItem = $object2[index];
                {
                  let $n300 = $update2 ? $lepusGetElementRefByLepusID("view", 300) : null;
                  let $temp5 = $update2;
                  if (!$n300) {
                    $update2 = false;
                    $n300 = __CreateView($currentComponentId);
                    $lepusStoreElementRefByLepusID($n300, 300, "view");
                    __AppendElement($n299, $n300);
                  }
                  __SetStyleObject($n300, [getClassStyleIndex("scroll_price_num_list_item", "51983000"), parseStyleStringToObject(getScrollPriceNumItemStyle(scrollItemDec))]);
                  {
                    let $n301 = $update2 ? $lepusGetElementRefByLepusID("text", 301) : null;
                    let $temp6 = $update2;
                    if (!$n301) {
                      $update2 = false;
                      $n301 = __CreateText($currentComponentId);
                      $lepusStoreElementRefByLepusID($n301, 301, "text");
                      __AppendElement($n300, $n301);
                    }
                    __SetStyleObject($n301, [getClassStyleIndex("scroll_price_num_list_item_text", "51983000"), (scrollItemDec == null ? undefined : scrollItemDec.style) || []]);
                    __SetAttribute($n301, "text", numItem);
                    $update2 = $temp6;
                  }
                  $update2 = $temp5;
                }
              }
              $update2 = $temp4;
              $lepusPushFiberForNode($lastForLepus2, undefined, undefined);
            }
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_303: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.price) == null ? undefined : _b.unit) == null ? undefined : _c.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n304 = $update2 ? $lepusGetElementRefByLepusID("text", 304) : null;
            let $temp2 = $update2;
            if (!$n304) {
              $update2 = false;
              $n304 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n304, 304, "text");
              __SetAttribute($n304, "accessibility-element", false);
              __AppendElement($parent, $n304);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n304, [getClassStyleIndex("gyl-price__text gyl-price__space--ml-1", "51983000"), 375, parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.price) == null ? undefined : _e.unit) == null ? undefined : _f.style)]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.price) == null ? undefined : _h.unit) == null ? undefined : _i.content;
                if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.price) == null ? undefined : _k.unit) == null ? undefined : _l.content)) {
                  __SetAttribute($n304, "text", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_306: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.price_above) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n307 = $update2 ? $lepusGetElementRefByLepusID("text", 307) : null;
            let $temp2 = $update2;
            if (!$n307) {
              $update2 = false;
              $n307 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n307, 307, "text");
              __SetAttribute($n307, "accessibility-element", false);
              __AppendElement($parent, $n307);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n307, [getClassStyleIndex("gyl-price__text gyl-price__space--ml-1", "51983000"), 375, parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.price_above) == null ? undefined : _d.style)]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $value = (_f = (_e = $data.data) == null ? undefined : _e.price_above) == null ? undefined : _f.content;
                if (!$update2 || $value !== ((_h = (_g = $lepusComponent._data.data) == null ? undefined : _g.price_above) == null ? undefined : _h.content)) {
                  __SetAttribute($n307, "text", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_309: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/price_common/index"].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.price_property) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n310 = $update2 ? $lepusGetElementRefByLepusID("image", 310) : null;
            let $temp2 = $update2;
            if (!$n310) {
              $update2 = false;
              $n310 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n310, 310, "image");
              __SetStyleObject($n310, [getClassStyleIndex("gyl-price__slash gyl-price__space--mx-1-5", "51983000")]);
              __SetAttribute($n310, "accessibility-element", false);
              __SetAttribute($n310, "skip-redirection", true);
              __AppendElement($parent, $n310);
            }
            {
              let $value = getSlashImage();
              if (!$update2 || $value !== undefined) {
                __SetAttribute($n310, "src", $value);
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_311: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.price_property) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n312 = $update2 ? $lepusGetElementRefByLepusID("text", 312) : null;
            let $temp2 = $update2;
            if (!$n312) {
              $update2 = false;
              $n312 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n312, 312, "text");
              __SetAttribute($n312, "accessibility-element", false);
              __AppendElement($parent, $n312);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n312, [getClassStyleIndex("gyl-price__text", "51983000"), 375, parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.price_property) == null ? undefined : _d.style)]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $value = (_f = (_e = $data.data) == null ? undefined : _e.price_property) == null ? undefined : _f.content;
                if (!$update2 || $value !== ((_h = (_g = $lepusComponent._data.data) == null ? undefined : _g.price_property) == null ? undefined : _h.content)) {
                  __SetAttribute($n312, "text", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_314: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.price_label) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n315 = $update2 ? $lepusGetElementRefByLepusID("text", 315) : null;
            let $temp2 = $update2;
            if (!$n315) {
              $update2 = false;
              $n315 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n315, 315, "text");
              __SetAttribute($n315, "accessibility-element", false);
              __SetAttribute($n315, "skip-redirection", true);
              __AppendElement($parent, $n315);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n315, [getClassStyleIndex("gyl-price__text gyl-price__space--ml-2", "51983000"), 375, parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.price_label) == null ? undefined : _d.style)]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $value = (_f = (_e = $data.data) == null ? undefined : _e.price_label) == null ? undefined : _f.content;
                if (!$update2 || $value !== ((_h = (_g = $lepusComponent._data.data) == null ? undefined : _g.price_label) == null ? undefined : _h.content)) {
                  __SetAttribute($n315, "text", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_317: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coin_benefit) == null ? undefined : _b.content) == null ? undefined : _c.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n318 = $update2 ? $lepusGetElementRefByLepusID("view", 318) : null;
            let $temp2 = $update2;
            if (!$n318) {
              $update2 = false;
              $n318 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n318, 318, "view");
              __AppendElement($parent, $n318);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n318, [getClassStyleIndex("gyl-price__coin-wrapper gyl-price__space--ml-2", "51983000"), parseStyleStringToObject((_e = (_d = $data.data) == null ? undefined : _d.coin_benefit) == null ? undefined : _e.style)]);
              }
            }
            {
              let $template_update = $update2;
              let $n319 = $update2 ? $lepusGetElementRefByLepusID("if", 319) : null;
              if (!$n319) {
                $update2 = false;
                $n319 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n319, 319, "if");
                __AppendElement($n318, $n319);
              }
              $renderComponents[$path].update_22898d8_319($lepusComponent, $n319, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let $n321 = $update2 ? $lepusGetElementRefByLepusID("text", 321) : null;
              let $temp3 = $update2;
              if (!$n321) {
                $update2 = false;
                $n321 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n321, 321, "text");
                __AppendElement($n318, $n321);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n321, [getClassStyleIndex("gyl-price__text gyl-price__coin-text", "51983000"), parseStyleStringToObject((_h = (_g = (_f = $data.data) == null ? undefined : _f.coin_benefit) == null ? undefined : _g.content) == null ? undefined : _h.style)]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let $value = (_k = (_j = (_i = $data.data) == null ? undefined : _i.coin_benefit) == null ? undefined : _j.content) == null ? undefined : _k.content;
                  if (!$update2 || $value !== ((_n = (_m = (_l = $lepusComponent._data.data) == null ? undefined : _l.coin_benefit) == null ? undefined : _m.content) == null ? undefined : _n.content)) {
                    __SetAttribute($n321, "text", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_319: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coin_benefit) == null ? undefined : _b.icon_left) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n320 = $update2 ? $lepusGetElementRefByLepusID("image", 320) : null;
          let $temp2 = $update2;
          if (!$n320) {
            $update2 = false;
            $n320 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n320, 320, "image");
            __SetAttribute($n320, "skip-redirection", true);
            __SetAttribute($n320, "mode", "aspectFit");
            __SetAttribute($n320, "accessibility-element", false);
            __AppendElement($parent, $n320);
          }
          if (!$update2 || $renderComponents["components/price_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n320, [getClassStyleIndex("gyl-price__coin-img", "51983000"), parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.coin_benefit) == null ? undefined : _e.icon_left) == null ? undefined : _f.style)]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.coin_benefit) == null ? undefined : _h.icon_left) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.coin_benefit) == null ? undefined : _k.icon_left) == null ? undefined : _l.url)) {
                __SetAttribute($n320, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_323: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.sales_count) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n324 = $update2 ? $lepusGetElementRefByLepusID("text", 324) : null;
            let $temp2 = $update2;
            if (!$n324) {
              $update2 = false;
              $n324 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n324, 324, "text");
              __SetAttribute($n324, "accessibility-element", false);
              __AppendElement($parent, $n324);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n324, [getClassStyleIndex("gyl-price__text gyl-price__space--ml-4", "51983000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.sales_count) == null ? undefined : _d.style)]);
              }
            }
            {
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $value = (_f = (_e = $data.data) == null ? undefined : _e.sales_count) == null ? undefined : _f.content;
                if (!$update2 || $value !== ((_h = (_g = $lepusComponent._data.data) == null ? undefined : _g.sales_count) == null ? undefined : _h.content)) {
                  __SetAttribute($n324, "text", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_326: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c;
    let $path = "components/price_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_a = $data.data) == null ? undefined : _a.tag) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let _$lepusPushOwner6 = $lepusPushOwner(uniqueId + "_0"),
              owner = _$lepusPushOwner6[0],
              lastOwner = _$lepusPushOwner6[1];
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n327 = $update2 ? $lepusGetElementRefByLepusID("view", 327) : null;
            let $temp2 = $update2;
            if (!$n327) {
              $update2 = false;
              $n327 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n327, 327, "view");
              __AppendElement($parent, $n327);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n327, [getClassStyleIndex(isH5() ? "gyl-price__tag--h5" : "", "51983000"), 149, parseStyleStringToObject((_c = (_b = $data.data) == null ? undefined : _b.tag) == null ? undefined : _c.style)]);
              }
            }
            {
              let $n328 = $update2 ? $lepusGetElementRefByLepusID("component", 328) : null;
              let $compCreated = true;
              let $childLepusComponent = null;
              let $componentId = null;
              if (!$n328) {
                $compCreated = false;
                $n328 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 18, "", "ecom-tag-lynx-ttml", "components/tag/index", {});
                let $nid328 = $lepusStoreElementRefByLepusID($n328, 328, "ecom-tag-lynx-ttml");
                $componentId = $nid328[0];
                $childLepusComponent = $componentConstructor($componentId, $n328, "components/tag/index", 328);
                $createdIds.push($componentId + "");
                $cardInstance._currentOwner.componentIds.push($componentId);
                $childLepusComponent._setProp("height", 14, $update2);
                $childLepusComponent._setProp("isRpx", true, $update2);
                __AppendElement($n327, $n328);
              } else {
                $componentId = __GetElementUniqueID($n328);
                $childLepusComponent = $componentInfo[$componentId];
              }
              $comUpdatePropsSet = [];
              $childLepusComponent._setProp("data", getTagSdkData($data.data.tag.content), $update2 && $compCreated);
              if ($compCreated) {
                let $update_keys = $comUpdatePropsSet;
                let $childSlotUpdate = false;
                if ($update_keys.length > 0 || $childSlotUpdate) {
                  $updatedIds.push($componentId + "");
                  $renderComponents["components/tag/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
                }
              } else {
                $renderComponents["components/tag/index"].entry($n328, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
          $lepusPopOwner(lastOwner);
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c;
    let $path = "components/price_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 275);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n276 = $lepusGetElementRefByLepusID("text", 276);
      {
        __SetStyleObject($n276, [getClassStyleIndex("gyl-price__symbol", "51983000"), parseStyleStringToObject((_c = (_b = (_a = $data.data) == null ? undefined : _a.price) == null ? undefined : _b.symbol) == null ? undefined : _c.style)]);
      }
    }
    let $n278 = $lepusGetElementRefByLepusID("if", 278);
    $renderComponents[$path].update_1200e30_278($lepusComponent, $n278, $data, $props, $update2, $slotUpdate);
    let $n281 = $lepusGetElementRefByLepusID("if", 281);
    $renderComponents[$path].update_1200e30_281($lepusComponent, $n281, $data, $props, $update2, $slotUpdate);
    let $n284 = $lepusGetElementRefByLepusID("if", 284);
    $renderComponents[$path].update_1200e30_284($lepusComponent, $n284, $data, $props, $update2, $slotUpdate);
    let $n303 = $lepusGetElementRefByLepusID("if", 303);
    $renderComponents[$path].update_22898d8_303($lepusComponent, $n303, $data, $props, $update2, $slotUpdate);
    let $n306 = $lepusGetElementRefByLepusID("if", 306);
    $renderComponents[$path].update_22898d8_306($lepusComponent, $n306, $data, $props, $update2, $slotUpdate);
    let $n309 = $lepusGetElementRefByLepusID("if", 309);
    $renderComponents[$path].update_22898d8_309($lepusComponent, $n309, $data, $props, $update2, $slotUpdate);
    let $n311 = $lepusGetElementRefByLepusID("if", 311);
    $renderComponents[$path].update_22898d8_311($lepusComponent, $n311, $data, $props, $update2, $slotUpdate);
    let $n314 = $lepusGetElementRefByLepusID("if", 314);
    $renderComponents[$path].update_22898d8_314($lepusComponent, $n314, $data, $props, $update2, $slotUpdate);
    let $n317 = $lepusGetElementRefByLepusID("if", 317);
    $renderComponents[$path].update_22898d8_317($lepusComponent, $n317, $data, $props, $update2, $slotUpdate);
    let $n323 = $lepusGetElementRefByLepusID("if", 323);
    $renderComponents[$path].update_22898d8_323($lepusComponent, $n323, $data, $props, $update2, $slotUpdate);
    let $n326 = $lepusGetElementRefByLepusID("if", 326);
    $renderComponents[$path].update_22898d8_326($lepusComponent, $n326, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a, _b, _c;
    let $path = "components/price_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n275 = __CreateView($currentComponentId);
      __SetStyleObject($n275, [getClassStyleIndex("gyl-price " + ((isLynxAndroid() ? "gyl-price--lynx-android" : "") + (" " + (isLynxIOS() ? "gyl-price--lynx-ios" : ""))), "51983000")]);
      __AppendElement($component, $n275);
      let $n276 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n276, 276, "text");
      __SetStyleObject($n276, [getClassStyleIndex("gyl-price__symbol", "51983000"), parseStyleStringToObject((_c = (_b = (_a = $data.data) == null ? undefined : _a.price) == null ? undefined : _b.symbol) == null ? undefined : _c.style)]);
      __SetAttribute($n276, "accessibility-element", false);
      __AppendElement($n275, $n276);
      __SetAttribute($n276, "text", "$");
      let $n278 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n278, 278, "if");
      __AppendElement($n275, $n278);
      $renderComponents[$path].update_1200e30_278($lepusComponent, $n278, $data, $props, $update2, $slotUpdate);
      let $n281 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n281, 281, "if");
      __AppendElement($n275, $n281);
      $renderComponents[$path].update_1200e30_281($lepusComponent, $n281, $data, $props, $update2, $slotUpdate);
      let $n284 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n284, 284, "if");
      __AppendElement($n275, $n284);
      $renderComponents[$path].update_1200e30_284($lepusComponent, $n284, $data, $props, $update2, $slotUpdate);
      let $n303 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n303, 303, "if");
      __AppendElement($n275, $n303);
      $renderComponents[$path].update_22898d8_303($lepusComponent, $n303, $data, $props, $update2, $slotUpdate);
      let $n306 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n306, 306, "if");
      __AppendElement($n275, $n306);
      $renderComponents[$path].update_22898d8_306($lepusComponent, $n306, $data, $props, $update2, $slotUpdate);
      let $n309 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n309, 309, "if");
      __AppendElement($n275, $n309);
      $renderComponents[$path].update_22898d8_309($lepusComponent, $n309, $data, $props, $update2, $slotUpdate);
      let $n311 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n311, 311, "if");
      __AppendElement($n275, $n311);
      $renderComponents[$path].update_22898d8_311($lepusComponent, $n311, $data, $props, $update2, $slotUpdate);
      let $n314 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n314, 314, "if");
      __AppendElement($n275, $n314);
      $renderComponents[$path].update_22898d8_314($lepusComponent, $n314, $data, $props, $update2, $slotUpdate);
      let $n317 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n317, 317, "if");
      __AppendElement($n275, $n317);
      $renderComponents[$path].update_22898d8_317($lepusComponent, $n317, $data, $props, $update2, $slotUpdate);
      let $n323 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n323, 323, "if");
      __AppendElement($n275, $n323);
      $renderComponents[$path].update_22898d8_323($lepusComponent, $n323, $data, $props, $update2, $slotUpdate);
      let $n326 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n326, 326, "if");
      __AppendElement($n275, $n326);
      $renderComponents[$path].update_22898d8_326($lepusComponent, $n326, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/recommend_reason_common/index"] = {
  variables: ["data", "visible", "countdownComplete", "countdown", "countdownUpdate"],
  varUpdateState: [],
  update_1149898_332: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && ((_b = (_a = item == null ? undefined : item.enhance) == null ? undefined : _a.icon_left) == null ? undefined : _b.url)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n333 = $update2 ? $lepusGetElementRefByLepusID("image", 333) : null;
          let $temp2 = $update2;
          if (!$n333) {
            $update2 = false;
            $n333 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n333, 333, "image");
            __SetAttribute($n333, "skip-redirection", true);
            __SetAttribute($n333, "mode", "scaleToFill");
            __SetAttribute($n333, "accessibility-element", false);
            __AppendElement($parent, $n333);
          }
          __SetStyleObject($n333, [getClassStyleIndex(isH52() && isAndroid4() ? "gyl-recommend-reason__icon-left--android" : "", "47842000"), parseStyleStringToObject((_d = (_c = item == null ? undefined : item.enhance) == null ? undefined : _c.icon_left) == null ? undefined : _d.style)]);
          __SetAttribute($n333, "src", (_f = (_e = item == null ? undefined : item.enhance) == null ? undefined : _e.icon_left) == null ? undefined : _f.url);
          __SetAttribute($n333, "ignore-rounded-corner", getLeftIgnoreRoundedCorner((_h = (_g = item == null ? undefined : item.enhance) == null ? undefined : _g.icon_left) == null ? undefined : _h.style));
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1149898_334: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && !!((_a = item == null ? undefined : item.content) == null ? undefined : _a.content)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n335 = $update2 ? $lepusGetElementRefByLepusID("text", 335) : null;
          let $temp2 = $update2;
          if (!$n335) {
            $update2 = false;
            $n335 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n335, 335, "text");
            __SetAttribute($n335, "text-maxline", "1");
            __SetAttribute($n335, "accessibility-element", false);
            __AppendElement($parent, $n335);
          }
          __SetStyleObject($n335, [getClassStyleIndex("gyl-recommend-reason__text", "47842000"), parseStyleStringToObject((_b = item == null ? undefined : item.enhance) == null ? undefined : _b.content_style)]);
          __SetAttribute($n335, "text", (_c = item == null ? undefined : item.content) == null ? undefined : _c.content);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1149898_337: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c, _d, _e, _f;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && !!((_b = (_a = item == null ? undefined : item.enhance) == null ? undefined : _a.highlight_content) == null ? undefined : _b.content)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n338 = $update2 ? $lepusGetElementRefByLepusID("text", 338) : null;
          let $temp2 = $update2;
          if (!$n338) {
            $update2 = false;
            $n338 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n338, 338, "text");
            __SetAttribute($n338, "text-maxline", "1");
            __AppendElement($parent, $n338);
          }
          __SetStyleObject($n338, [getClassStyleIndex("gyl-recommend-reason__text", "47842000"), parseStyleStringToObject((_d = (_c = item == null ? undefined : item.enhance) == null ? undefined : _c.highlight_content) == null ? undefined : _d.style)]);
          __SetAttribute($n338, "text", (_f = (_e = item == null ? undefined : item.enhance) == null ? undefined : _e.highlight_content) == null ? undefined : _f.content);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1149898_340: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c, _d, _e, _f;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && ((_b = (_a = item == null ? undefined : item.enhance) == null ? undefined : _a.icon_right) == null ? undefined : _b.url)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n341 = $update2 ? $lepusGetElementRefByLepusID("image", 341) : null;
          let $temp2 = $update2;
          if (!$n341) {
            $update2 = false;
            $n341 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n341, 341, "image");
            __SetAttribute($n341, "skip-redirection", true);
            __SetAttribute($n341, "mode", "aspectFit");
            __SetAttribute($n341, "accessibility-element", false);
            __SetAttribute($n341, "ignore-rounded-corner", true);
            __AppendElement($parent, $n341);
          }
          __SetStyleObject($n341, [parseStyleStringToObject((_d = (_c = item == null ? undefined : item.enhance) == null ? undefined : _c.icon_right) == null ? undefined : _d.style)]);
          __SetAttribute($n341, "src", $data.visible && ((_f = (_e = item == null ? undefined : item.enhance) == null ? undefined : _e.icon_right) == null ? undefined : _f.url));
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1149898_356: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c, _d;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && ((_a = item == null ? undefined : item.icon_left) == null ? undefined : _a.url)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n357 = $update2 ? $lepusGetElementRefByLepusID("image", 357) : null;
          let $temp2 = $update2;
          if (!$n357) {
            $update2 = false;
            $n357 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n357, 357, "image");
            __SetAttribute($n357, "skip-redirection", true);
            __SetAttribute($n357, "mode", "aspectFit");
            __SetAttribute($n357, "accessibility-element", false);
            __AppendElement($parent, $n357);
          }
          __SetStyleObject($n357, [getClassStyleIndex(isH52() && isAndroid4() ? "gyl-recommend-reason__icon-left--android" : "", "47842000"), parseStyleStringToObject((_b = item == null ? undefined : item.icon_left) == null ? undefined : _b.style)]);
          __SetAttribute($n357, "src", (_c = item == null ? undefined : item.icon_left) == null ? undefined : _c.url);
          __SetAttribute($n357, "ignore-rounded-corner", getLeftIgnoreRoundedCorner((_d = item == null ? undefined : item.icon_left) == null ? undefined : _d.style));
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1149898_358: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && !!((_a = item == null ? undefined : item.header) == null ? undefined : _a.content)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n359 = $update2 ? $lepusGetElementRefByLepusID("text", 359) : null;
          let $temp2 = $update2;
          if (!$n359) {
            $update2 = false;
            $n359 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n359, 359, "text");
            __SetAttribute($n359, "text-maxline", "1");
            __SetAttribute($n359, "accessibility-element", false);
            __AppendElement($parent, $n359);
          }
          __SetStyleObject($n359, [getClassStyleIndex("gyl-recommend-reason__header " + (isAndroid4() ? "gyl-recommend-reason__header--android" : ""), "47842000"), parseStyleStringToObject((_b = item == null ? undefined : item.header) == null ? undefined : _b.style)]);
          __SetAttribute($n359, "text", (_c = item == null ? undefined : item.header) == null ? undefined : _c.content);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1149898_361: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && !!((_a = item == null ? undefined : item.sep_dot) == null ? undefined : _a.content)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n362 = $update2 ? $lepusGetElementRefByLepusID("text", 362) : null;
          let $temp2 = $update2;
          if (!$n362) {
            $update2 = false;
            $n362 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n362, 362, "text");
            __SetAttribute($n362, "text-maxline", "1");
            __SetAttribute($n362, "accessibility-element", false);
            __AppendElement($parent, $n362);
          }
          __SetStyleObject($n362, [getClassStyleIndex("gyl-recommend-reason__sep-dot", "47842000"), parseStyleStringToObject((_b = item == null ? undefined : item.sep_dot) == null ? undefined : _b.style)]);
          __SetAttribute($n362, "text", (_c = item == null ? undefined : item.sep_dot) == null ? undefined : _c.content);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1149898_364: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && !!((_a = item == null ? undefined : item.content) == null ? undefined : _a.content)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n365 = $update2 ? $lepusGetElementRefByLepusID("text", 365) : null;
          let $temp2 = $update2;
          if (!$n365) {
            $update2 = false;
            $n365 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n365, 365, "text");
            __SetAttribute($n365, "text-maxline", "1");
            __SetAttribute($n365, "accessibility-element", false);
            __AppendElement($parent, $n365);
          }
          __SetStyleObject($n365, [getClassStyleIndex("gyl-recommend-reason__text", "47842000"), parseStyleStringToObject((_b = item == null ? undefined : item.content) == null ? undefined : _b.style)]);
          __SetAttribute($n365, "text", (_c = item == null ? undefined : item.content) == null ? undefined : _c.content);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1149898_367: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item) {
    let _a, _b, _c;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && ((_a = item == null ? undefined : item.icon_right) == null ? undefined : _a.url)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n368 = $update2 ? $lepusGetElementRefByLepusID("image", 368) : null;
          let $temp2 = $update2;
          if (!$n368) {
            $update2 = false;
            $n368 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n368, 368, "image");
            __SetAttribute($n368, "skip-redirection", true);
            __SetAttribute($n368, "mode", "aspectFit");
            __SetAttribute($n368, "accessibility-element", false);
            __SetAttribute($n368, "ignore-rounded-corner", true);
            __AppendElement($parent, $n368);
          }
          __SetStyleObject($n368, [parseStyleStringToObject((_b = item == null ? undefined : item.icon_right) == null ? undefined : _b.style)]);
          __SetAttribute($n368, "src", $data.visible && ((_c = item == null ? undefined : item.icon_right) == null ? undefined : _c.url));
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_14c6d90_344: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j;
    let $path = "components/recommend_reason_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((((_a = $data.countdown) == null ? undefined : _a.time) > 0 || ((_b = $data.countdown) == null ? undefined : _b.recText)) && !$data.countdownComplete) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n345 = $update2 ? $lepusGetElementRefByLepusID("text", 345) : null;
          let $temp2 = $update2;
          if (!$n345) {
            $update2 = false;
            $n345 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n345, 345, "text");
            __AppendElement($parent, $n345);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
            {
              __SetStyleObject($n345, [getClassStyleIndex("gyl-recommend-reason__countdown", "47842000"), parseStyleStringToObject((_d = (_c = $data.countdown) == null ? undefined : _c.textStyle) == null ? undefined : _d.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
              let $value = ((_e = $data.countdown) == null ? undefined : _e.prefixText) + (((_f = $data.countdown) == null ? undefined : _f.showTime) ? "" : (_g = $data.countdown) == null ? undefined : _g.recText);
              if (!$update2 || $value !== ((_h = $lepusComponent._data.countdown) == null ? undefined : _h.prefixText) + (((_i = $lepusComponent._data.countdown) == null ? undefined : _i.showTime) ? "" : (_j = $lepusComponent._data.countdown) == null ? undefined : _j.recText)) {
                __SetAttribute($n345, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_14c6d90_347: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_a = $data.countdown) == null ? undefined : _a.time) > 0 && !$data.countdownComplete) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner7 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner7[0],
            lastOwner = _$lepusPushOwner7[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n348 = $update2 ? $lepusGetElementRefByLepusID("view", 348) : null;
          let $temp2 = $update2;
          if (!$n348) {
            $update2 = false;
            $n348 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n348, 348, "view");
            __AppendElement($parent, $n348);
          }
          {
            let $n349 = $update2 ? $lepusGetElementRefByLepusID("text", 349) : null;
            let $temp3 = $update2;
            if (!$n349) {
              $update2 = false;
              $n349 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n349, 349, "text");
              __AppendElement($n348, $n349);
            }
            if (!$update2 || $renderComponents["components/recommend_reason_common/index"].varUpdateState[3]) {
              {
                __SetStyleObject($n349, [getClassStyleIndex("gyl-recommend-reason__countdown", "47842000"), 377, parseStyleStringToObject((_c = (_b = $data.countdown) == null ? undefined : _b.textStyle) == null ? undefined : _c.style)]);
              }
            }
            __SetAttribute($n349, "text", "88:88:88");
            $update2 = $temp3;
          }
          {
            let $n351 = $update2 ? $lepusGetElementRefByLepusID("component", 351) : null;
            let $compCreated = true;
            let $childLepusComponent = null;
            let $componentId = null;
            if (!$n351) {
              $compCreated = false;
              $n351 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 17, "", "countdown", "components/countdown/index", {});
              let $nid351 = $lepusStoreElementRefByLepusID($n351, 351, "countdown");
              $componentId = $nid351[0];
              $childLepusComponent = $componentConstructor($componentId, $n351, "components/countdown/index", 351);
              $createdIds.push($componentId + "");
              $cardInstance._currentOwner.componentIds.push($componentId);
              __SetStyleObject($n351, [15]);
              __AddEvent($n351, "bindEvent", "Updated", "handleCountdownUpdate");
              __AddEvent($n351, "bindEvent", "Completed", "handleCountdownComplete");
              __AppendElement($n348, $n351);
            } else {
              $componentId = __GetElementUniqueID($n351);
              $childLepusComponent = $componentInfo[$componentId];
            }
            $comUpdatePropsSet = [];
            $childLepusComponent._setProp("duration", (_d = $data.countdown) == null ? undefined : _d.time, $update2 && $compCreated);
            $childLepusComponent._setProp("textStyle", (_e = $data.countdown) == null ? undefined : _e.textStyle, $update2 && $compCreated);
            $childLepusComponent._setProp("useAdaptiveScheduler", !!((_f = $data.countdown) == null ? undefined : _f.useAdaptiveScheduler), $update2 && $compCreated);
            if ($compCreated) {
              let $update_keys = $comUpdatePropsSet;
              let $childSlotUpdate = false;
              if ($update_keys.length > 0 || $childSlotUpdate) {
                $updatedIds.push($componentId + "");
                $renderComponents["components/countdown/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
              }
            } else {
              $renderComponents["components/countdown/index"].entry($n351, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_14c6d90_352: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "components/recommend_reason_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_a = $data.countdown) == null ? undefined : _a.showTime) && ((_b = $data.countdown) == null ? undefined : _b.time) <= 0 || $data.countdownComplete) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n353 = $update2 ? $lepusGetElementRefByLepusID("text", 353) : null;
          let $temp2 = $update2;
          if (!$n353) {
            $update2 = false;
            $n353 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n353, 353, "text");
            __AppendElement($parent, $n353);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
            {
              __SetStyleObject($n353, [getClassStyleIndex("gyl-recommend-reason__countdown", "47842000"), parseStyleStringToObject((_d = (_c = $data.countdown) == null ? undefined : _c.textStyle) == null ? undefined : _d.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
              let $value = (_e = $data.countdown) == null ? undefined : _e.expiredText;
              if (!$update2 || $value !== ((_f = $lepusComponent._data.countdown) == null ? undefined : _f.expiredText)) {
                __SetAttribute($n353, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_14c6d90_371: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j;
    let $path = "components/recommend_reason_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((((_a = $data.countdown) == null ? undefined : _a.time) > 0 || ((_b = $data.countdown) == null ? undefined : _b.recText)) && !$data.countdownComplete) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n372 = $update2 ? $lepusGetElementRefByLepusID("text", 372) : null;
          let $temp2 = $update2;
          if (!$n372) {
            $update2 = false;
            $n372 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n372, 372, "text");
            __AppendElement($parent, $n372);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
            {
              __SetStyleObject($n372, [getClassStyleIndex("gyl-recommend-reason__countdown", "47842000"), parseStyleStringToObject((_d = (_c = $data.countdown) == null ? undefined : _c.textStyle) == null ? undefined : _d.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
              let $value = ((_e = $data.countdown) == null ? undefined : _e.prefixText) + (((_f = $data.countdown) == null ? undefined : _f.showTime) ? "" : (_g = $data.countdown) == null ? undefined : _g.recText);
              if (!$update2 || $value !== ((_h = $lepusComponent._data.countdown) == null ? undefined : _h.prefixText) + (((_i = $lepusComponent._data.countdown) == null ? undefined : _i.showTime) ? "" : (_j = $lepusComponent._data.countdown) == null ? undefined : _j.recText)) {
                __SetAttribute($n372, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_14c6d90_374: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_a = $data.countdown) == null ? undefined : _a.time) > 0 && !$data.countdownComplete) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner8 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner8[0],
            lastOwner = _$lepusPushOwner8[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n375 = $update2 ? $lepusGetElementRefByLepusID("view", 375) : null;
          let $temp2 = $update2;
          if (!$n375) {
            $update2 = false;
            $n375 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n375, 375, "view");
            __AppendElement($parent, $n375);
          }
          {
            let $n376 = $update2 ? $lepusGetElementRefByLepusID("text", 376) : null;
            let $temp3 = $update2;
            if (!$n376) {
              $update2 = false;
              $n376 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n376, 376, "text");
              __AppendElement($n375, $n376);
            }
            if (!$update2 || $renderComponents["components/recommend_reason_common/index"].varUpdateState[3]) {
              {
                __SetStyleObject($n376, [getClassStyleIndex("gyl-recommend-reason__countdown", "47842000"), 377, parseStyleStringToObject((_c = (_b = $data.countdown) == null ? undefined : _b.textStyle) == null ? undefined : _c.style)]);
              }
            }
            __SetAttribute($n376, "text", "188:88:88");
            $update2 = $temp3;
          }
          {
            let $n378 = $update2 ? $lepusGetElementRefByLepusID("component", 378) : null;
            let $compCreated = true;
            let $childLepusComponent = null;
            let $componentId = null;
            if (!$n378) {
              $compCreated = false;
              $n378 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 17, "", "countdown", "components/countdown/index", {});
              let $nid378 = $lepusStoreElementRefByLepusID($n378, 378, "countdown");
              $componentId = $nid378[0];
              $childLepusComponent = $componentConstructor($componentId, $n378, "components/countdown/index", 378);
              $createdIds.push($componentId + "");
              $cardInstance._currentOwner.componentIds.push($componentId);
              __SetStyleObject($n378, [15]);
              __AddEvent($n378, "bindEvent", "Updated", "handleCountdownUpdate");
              __AddEvent($n378, "bindEvent", "Completed", "handleCountdownComplete");
              __AppendElement($n375, $n378);
            } else {
              $componentId = __GetElementUniqueID($n378);
              $childLepusComponent = $componentInfo[$componentId];
            }
            $comUpdatePropsSet = [];
            $childLepusComponent._setProp("duration", (_d = $data.countdown) == null ? undefined : _d.time, $update2 && $compCreated);
            $childLepusComponent._setProp("textStyle", (_e = $data.countdown) == null ? undefined : _e.textStyle, $update2 && $compCreated);
            $childLepusComponent._setProp("useAdaptiveScheduler", !!((_f = $data.countdown) == null ? undefined : _f.useAdaptiveScheduler), $update2 && $compCreated);
            if ($compCreated) {
              let $update_keys = $comUpdatePropsSet;
              let $childSlotUpdate = false;
              if ($update_keys.length > 0 || $childSlotUpdate) {
                $updatedIds.push($componentId + "");
                $renderComponents["components/countdown/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
              }
            } else {
              $renderComponents["components/countdown/index"].entry($n378, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_14c6d90_379: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "components/recommend_reason_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_a = $data.countdown) == null ? undefined : _a.showTime) && ((_b = $data.countdown) == null ? undefined : _b.time) <= 0 || $data.countdownComplete) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n380 = $update2 ? $lepusGetElementRefByLepusID("text", 380) : null;
          let $temp2 = $update2;
          if (!$n380) {
            $update2 = false;
            $n380 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n380, 380, "text");
            __AppendElement($parent, $n380);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
            {
              __SetStyleObject($n380, [getClassStyleIndex("gyl-recommend-reason__countdown", "47842000"), parseStyleStringToObject((_d = (_c = $data.countdown) == null ? undefined : _c.textStyle) == null ? undefined : _d.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[3]) {
              let $value = (_e = $data.countdown) == null ? undefined : _e.expiredText;
              if (!$update2 || $value !== ((_f = $lepusComponent._data.countdown) == null ? undefined : _f.expiredText)) {
                __SetAttribute($n380, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_32eed18_342: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item, old_item) {
    let _a, _b, _c, _d, _e;
    let $path = "components/recommend_reason_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (index == 0 && (((_a = $data.countdown) == null ? undefined : _a.showTime) || ((_b = $data.countdown) == null ? undefined : _b.recText))) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner9 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner9[0],
            lastOwner = _$lepusPushOwner9[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n343 = $update2 ? $lepusGetElementRefByLepusID("view", 343) : null;
          let $temp2 = $update2;
          if (!$n343) {
            $update2 = false;
            $n343 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n343, 343, "view");
            __AppendElement($parent, $n343);
          }
          __SetStyleObject($n343, [0, 61, {
            23: (!$data.countdownUpdate && ((_c = $data.countdown) == null ? undefined : _c.time) > 0 ? 0 : 1) + ""
          }, ((_d = $data.countdown) == null ? undefined : _d.time) > 0 && !$data.countdownComplete ? [376] : [], parseStyleStringToObject((_e = item == null ? undefined : item.enhance) == null ? undefined : _e.count_down_style)]);
          let $n344 = $update2 ? $lepusGetElementRefByLepusID("if", 344) : null;
          if (!$n344) {
            $update2 = false;
            $n344 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n344, 344, "if");
            __AppendElement($n343, $n344);
          }
          $renderComponents[$path].update_14c6d90_344($lepusComponent, $n344, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n347 = $update2 ? $lepusGetElementRefByLepusID("if", 347) : null;
          if (!$n347) {
            $update2 = false;
            $n347 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n347, 347, "if");
            __AppendElement($n343, $n347);
          }
          $renderComponents[$path].update_14c6d90_347($lepusComponent, $n347, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n352 = $update2 ? $lepusGetElementRefByLepusID("if", 352) : null;
          if (!$n352) {
            $update2 = false;
            $n352 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n352, 352, "if");
            __AppendElement($n343, $n352);
          }
          $renderComponents[$path].update_14c6d90_352($lepusComponent, $n352, $data, $props, $update2, $slotUpdate, index, item, old_item);
          $update2 = $temp2;
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_32eed18_369: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item, old_item) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/recommend_reason_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (index == 0 && (((_a = $data.countdown) == null ? undefined : _a.showTime) || ((_b = $data.countdown) == null ? undefined : _b.recText))) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner10 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner10[0],
            lastOwner = _$lepusPushOwner10[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n370 = $update2 ? $lepusGetElementRefByLepusID("view", 370) : null;
          let $temp2 = $update2;
          if (!$n370) {
            $update2 = false;
            $n370 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n370, 370, "view");
            __AppendElement($parent, $n370);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[4] || $renderComponents[$path].varUpdateState[3] || $renderComponents[$path].varUpdateState[2]) {
            {
              let $value = "display:flex;align-items:center;opacity:" + (!$data.countdownUpdate && ((_c = $data.countdown) == null ? undefined : _c.time) > 0 ? 0 : 1) + ";" + (((_d = $data.countdown) == null ? undefined : _d.time) > 0 && !$data.countdownComplete ? "margin-right: -11rpx;" : "") + ";";
              if (!$update2 || $value !== "display:flex;align-items:center;opacity:" + (!$lepusComponent._data.countdownUpdate && ((_e = $lepusComponent._data.countdown) == null ? undefined : _e.time) > 0 ? 0 : 1) + ";" + (((_f = $lepusComponent._data.countdown) == null ? undefined : _f.time) > 0 && !$lepusComponent._data.countdownComplete ? "margin-right: -11rpx;" : "") + ";") {
                __SetStyleObject($n370, [0, 61, {
                  23: (!$data.countdownUpdate && ((_g = $data.countdown) == null ? undefined : _g.time) > 0 ? 0 : 1) + ""
                }, ((_h = $data.countdown) == null ? undefined : _h.time) > 0 && !$data.countdownComplete ? [376] : []]);
              }
            }
          }
          let $n371 = $update2 ? $lepusGetElementRefByLepusID("if", 371) : null;
          if (!$n371) {
            $update2 = false;
            $n371 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n371, 371, "if");
            __AppendElement($n370, $n371);
          }
          $renderComponents[$path].update_14c6d90_371($lepusComponent, $n371, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n374 = $update2 ? $lepusGetElementRefByLepusID("if", 374) : null;
          if (!$n374) {
            $update2 = false;
            $n374 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n374, 374, "if");
            __AppendElement($n370, $n374);
          }
          $renderComponents[$path].update_14c6d90_374($lepusComponent, $n374, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n379 = $update2 ? $lepusGetElementRefByLepusID("if", 379) : null;
          if (!$n379) {
            $update2 = false;
            $n379 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n379, 379, "if");
            __AppendElement($n370, $n379);
          }
          $renderComponents[$path].update_14c6d90_379($lepusComponent, $n379, $data, $props, $update2, $slotUpdate, index, item, old_item);
          $update2 = $temp2;
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_19b608_330: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index, item, old_item) {
    let _a, _b, _c;
    let $path = "components/recommend_reason_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.visible && !!((_a = item == null ? undefined : item.enhance) == null ? undefined : _a.type) && !($data.countdownComplete || ((_b = $data.countdown) == null ? undefined : _b.time) < 0)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner11 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner11[0],
            lastOwner = _$lepusPushOwner11[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
          $deletedOwnerIds.push(uniqueId + "_1");
        }
        {
          let $n331 = $update2 ? $lepusGetElementRefByLepusID("view", 331) : null;
          let $temp2 = $update2;
          if (!$n331) {
            $update2 = false;
            $n331 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n331, 331, "view");
            __AddEvent($n331, "catchEvent", "tap", "handleClick");
            __AppendElement($parent, $n331);
          }
          __SetStyleObject($n331, [getClassStyleIndex("gyl-recommend-reason__container", "47842000"), 6, parseStyleStringToObject((_c = item == null ? undefined : item.enhance) == null ? undefined : _c.parent_style)]);
          __AddDataset($n331, "index", index);
          let $n332 = $update2 ? $lepusGetElementRefByLepusID("if", 332) : null;
          if (!$n332) {
            $update2 = false;
            $n332 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n332, 332, "if");
            __AppendElement($n331, $n332);
          }
          $renderComponents[$path].update_1149898_332($lepusComponent, $n332, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n334 = $update2 ? $lepusGetElementRefByLepusID("if", 334) : null;
          if (!$n334) {
            $update2 = false;
            $n334 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n334, 334, "if");
            __AppendElement($n331, $n334);
          }
          $renderComponents[$path].update_1149898_334($lepusComponent, $n334, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n337 = $update2 ? $lepusGetElementRefByLepusID("if", 337) : null;
          if (!$n337) {
            $update2 = false;
            $n337 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n337, 337, "if");
            __AppendElement($n331, $n337);
          }
          $renderComponents[$path].update_1149898_337($lepusComponent, $n337, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n340 = $update2 ? $lepusGetElementRefByLepusID("if", 340) : null;
          if (!$n340) {
            $update2 = false;
            $n340 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n340, 340, "if");
            __AppendElement($n331, $n340);
          }
          $renderComponents[$path].update_1149898_340($lepusComponent, $n340, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n342 = $update2 ? $lepusGetElementRefByLepusID("if", 342) : null;
          if (!$n342) {
            $update2 = false;
            $n342 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n342, 342, "if");
            __AppendElement($n331, $n342);
          }
          $renderComponents[$path].update_32eed18_342($lepusComponent, $n342, $data, $props, $update2, $slotUpdate, index, item, old_item);
          $update2 = $temp2;
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$lepusPushOwner12 = $lepusPushOwner(uniqueId + "_1"),
            _owner = _$lepusPushOwner12[0],
            _lastOwner = _$lepusPushOwner12[1];
        let _$temp34 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
          $deletedOwnerIds.push(uniqueId + "_0");
        }
        {
          let $n355 = $update2 ? $lepusGetElementRefByLepusID("view", 355) : null;
          let _$temp35 = $update2;
          if (!$n355) {
            $update2 = false;
            $n355 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n355, 355, "view");
            __AddEvent($n355, "catchEvent", "tap", "handleClick");
            __AppendElement($parent, $n355);
          }
          __SetStyleObject($n355, [getClassStyleIndex("gyl-recommend-reason__container", "47842000"), 6, parseStyleStringToObject(item == null ? undefined : item.style)]);
          __AddDataset($n355, "index", index);
          let $n356 = $update2 ? $lepusGetElementRefByLepusID("if", 356) : null;
          if (!$n356) {
            $update2 = false;
            $n356 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n356, 356, "if");
            __AppendElement($n355, $n356);
          }
          $renderComponents[$path].update_1149898_356($lepusComponent, $n356, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n358 = $update2 ? $lepusGetElementRefByLepusID("if", 358) : null;
          if (!$n358) {
            $update2 = false;
            $n358 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n358, 358, "if");
            __AppendElement($n355, $n358);
          }
          $renderComponents[$path].update_1149898_358($lepusComponent, $n358, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n361 = $update2 ? $lepusGetElementRefByLepusID("if", 361) : null;
          if (!$n361) {
            $update2 = false;
            $n361 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n361, 361, "if");
            __AppendElement($n355, $n361);
          }
          $renderComponents[$path].update_1149898_361($lepusComponent, $n361, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n364 = $update2 ? $lepusGetElementRefByLepusID("if", 364) : null;
          if (!$n364) {
            $update2 = false;
            $n364 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n364, 364, "if");
            __AppendElement($n355, $n364);
          }
          $renderComponents[$path].update_1149898_364($lepusComponent, $n364, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n367 = $update2 ? $lepusGetElementRefByLepusID("if", 367) : null;
          if (!$n367) {
            $update2 = false;
            $n367 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n367, 367, "if");
            __AppendElement($n355, $n367);
          }
          $renderComponents[$path].update_1149898_367($lepusComponent, $n367, $data, $props, $update2, $slotUpdate, index, item, old_item);
          let $n369 = $update2 ? $lepusGetElementRefByLepusID("if", 369) : null;
          if (!$n369) {
            $update2 = false;
            $n369 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n369, 369, "if");
            __AppendElement($n355, $n369);
          }
          $renderComponents[$path].update_32eed18_369($lepusComponent, $n369, $data, $props, $update2, $slotUpdate, index, item, old_item);
          $update2 = _$temp35;
        }
        $update2 = _$temp34;
        $lepusPopOwner(_lastOwner);
      }
    }
  },
  update_26c1a18_329: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b;
    let $path = "components/recommend_reason_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[3] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[4]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo16 = $lepusPushFiberForNode($parent, 329, uniqueId),
          $forLepus = _$lepusPushFiberForNo16[0],
          $lastForLepus = _$lepusPushFiberForNo16[1];
      let $object = (_a = $data.data) == null ? undefined : _a.rec_reasons;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.rec_reasons;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let _$lepusPushOwner13 = $lepusPushOwner(uniqueId + "_" + index),
            owner = _$lepusPushOwner13[0],
            lastOwner = _$lepusPushOwner13[1];
        let item = $object[index];
        let old_item = $oldObject ? $oldObject[index] : null;
        let $n330 = $update2 ? $lepusGetElementRefByLepusID("if", 330) : null;
        if (!$n330) {
          $n330 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n330, 330, "if");
          __AppendElement($parent, $n330);
        }
        $renderComponents[$path].update_19b608_330($lepusComponent, $n330, $data, $props, $update2, $slotUpdate, index, item, old_item);
        $lepusPopOwner(lastOwner);
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/recommend_reason_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    let $n329 = $lepusGetElementRefByLepusID("for", 329);
    $renderComponents[$path].update_26c1a18_329($lepusComponent, $n329, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n329 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n329, 329, "for");
      __AppendElement($component, $n329);
      $renderComponents["components/recommend_reason_common/index"].update_26c1a18_329($lepusComponent, $n329, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/scroll_to_number/index"] = {
  variables: ["num", "showAni", "isDataReady"],
  varUpdateState: [],
  update_3d91b8_384: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "components/scroll_to_number/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo17 = $lepusPushFiberForNode($parent, 384, uniqueId),
          $forLepus = _$lepusPushFiberForNo17[0],
          $lastForLepus = _$lepusPushFiberForNo17[1];
      let $object = (_a = $data.num) == null ? undefined : _a.num_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.num) == null ? undefined : _b.num_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n385 = $update2 ? $lepusGetElementRefByLepusID("view", 385) : null;
          let $temp2 = $update2;
          if (!$n385) {
            $update2 = false;
            $n385 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n385, 385, "view");
            __AppendElement($parent, $n385);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              let $value = "height:" + 2 * ((_c = $data.num) == null ? undefined : _c.font_size) + "rpx;";
              if (!$update2 || $value !== "height:" + 2 * ((_d = $lepusComponent._data.num) == null ? undefined : _d.font_size) + "rpx;") {
                __SetStyleObject($n385, [getClassStyleIndex("gyl-scroll-to-number__item", "369000"), {
                  26: 2 * ((_e = $data.num) == null ? undefined : _e.font_size) + "rpx"
                }]);
              }
            }
          }
          {
            let $n386 = $update2 ? $lepusGetElementRefByLepusID("text", 386) : null;
            let $temp3 = $update2;
            if (!$n386) {
              $update2 = false;
              $n386 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n386, 386, "text");
              __AppendElement($n385, $n386);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n386, [getClassStyleIndex("gyl-scroll-to-number__number", "369000"), parseStyleStringToObject((_f = $data.num) == null ? undefined : _f.style)]);
              }
            }
            __SetAttribute($n386, "text", item);
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s, _t, _u, _v, _w, _x, _y, _z, _A;
    let $path = "components/scroll_to_number/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n382 = $lepusGetElementRefByLepusID("view", 382);
      {
        let $value = "height:" + 2 * ((_a = $data.num) == null ? undefined : _a.font_size) + "rpx;width:" + 1.1 * ((_b = $data.num) == null ? undefined : _b.font_size) + "rpx;";
        if (!$update2 || $value !== "height:" + 2 * ((_c = $lepusComponent._data.num) == null ? undefined : _c.font_size) + "rpx;width:" + 1.1 * ((_d = $lepusComponent._data.num) == null ? undefined : _d.font_size) + "rpx;") {
          __SetStyleObject($n382, [getClassStyleIndex("gyl-scroll-to-number__container", "369000"), {
            26: 2 * ((_e = $data.num) == null ? undefined : _e.font_size) + "rpx"
          }, {
            27: 1.1 * ((_f = $data.num) == null ? undefined : _f.font_size) + "rpx"
          }]);
        }
      }
    }
    if ($renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[0]) {
      let $n383 = $lepusGetElementRefByLepusID("view", 383);
      {
        let _$value37 = "top:" + (!$data.showAni || $data.isDataReady ? -((_g = $data.num) == null ? undefined : _g.font_size) * 2 * (((_h = $data.num) == null ? undefined : _h.end_num) - ((_i = $data.num) == null ? undefined : _i.start_num)) : 0) + "rpx;height:" + 2 * ((_j = $data.num) == null ? undefined : _j.font_size) * (((_k = $data.num) == null ? undefined : _k.end_num) - ((_l = $data.num) == null ? undefined : _l.start_num) + 1) + "rpx;transition-duration:" + (!$data.showAni ? 0 : ((_m = $data.num) == null ? undefined : _m.duration) / 1e3) + "s;" + (!$data.showAni ? "transition: none" : "") + ";";
        if (!$update2 || _$value37 !== "top:" + (!$lepusComponent._data.showAni || $lepusComponent._data.isDataReady ? -((_n = $lepusComponent._data.num) == null ? undefined : _n.font_size) * 2 * (((_o = $lepusComponent._data.num) == null ? undefined : _o.end_num) - ((_p = $lepusComponent._data.num) == null ? undefined : _p.start_num)) : 0) + "rpx;height:" + 2 * ((_q = $lepusComponent._data.num) == null ? undefined : _q.font_size) * (((_r = $lepusComponent._data.num) == null ? undefined : _r.end_num) - ((_s = $lepusComponent._data.num) == null ? undefined : _s.start_num) + 1) + "rpx;transition-duration:" + (!$lepusComponent._data.showAni ? 0 : ((_t = $lepusComponent._data.num) == null ? undefined : _t.duration) / 1e3) + "s;" + (!$lepusComponent._data.showAni ? "transition: none" : "") + ";") {
          __SetStyleObject($n383, [getClassStyleIndex("gyl-scroll-to-number__wrap", "369000"), {
            1: (!$data.showAni || $data.isDataReady ? -((_u = $data.num) == null ? undefined : _u.font_size) * 2 * (((_v = $data.num) == null ? undefined : _v.end_num) - ((_w = $data.num) == null ? undefined : _w.start_num)) : 0) + "rpx"
          }, {
            26: 2 * ((_x = $data.num) == null ? undefined : _x.font_size) * (((_y = $data.num) == null ? undefined : _y.end_num) - ((_z = $data.num) == null ? undefined : _z.start_num) + 1) + "rpx"
          }, {
            111: (!$data.showAni ? 0 : ((_A = $data.num) == null ? undefined : _A.duration) / 1e3) + "s"
          }, !$data.showAni ? [378] : []]);
        }
      }
    }
    let $n384 = $lepusGetElementRefByLepusID("for", 384);
    $renderComponents[$path].update_3d91b8_384($lepusComponent, $n384, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i;
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n382 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n382, 382, "view");
      __SetStyleObject($n382, [getClassStyleIndex("gyl-scroll-to-number__container", "369000"), {
        26: 2 * ((_a = $data.num) == null ? undefined : _a.font_size) + "rpx"
      }, {
        27: 1.1 * ((_b = $data.num) == null ? undefined : _b.font_size) + "rpx"
      }]);
      __AppendElement($component, $n382);
      let $n383 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n383, 383, "view");
      __SetStyleObject($n383, [getClassStyleIndex("gyl-scroll-to-number__wrap", "369000"), {
        1: (!$data.showAni || $data.isDataReady ? -((_c = $data.num) == null ? undefined : _c.font_size) * 2 * (((_d = $data.num) == null ? undefined : _d.end_num) - ((_e = $data.num) == null ? undefined : _e.start_num)) : 0) + "rpx"
      }, {
        26: 2 * ((_f = $data.num) == null ? undefined : _f.font_size) * (((_g = $data.num) == null ? undefined : _g.end_num) - ((_h = $data.num) == null ? undefined : _h.start_num) + 1) + "rpx"
      }, {
        111: (!$data.showAni ? 0 : ((_i = $data.num) == null ? undefined : _i.duration) / 1e3) + "s"
      }, !$data.showAni ? [378] : []]);
      __SetAttribute($n383, "implicit-animation", "false");
      __AppendElement($n382, $n383);
      let $n384 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n384, 384, "for");
      __AppendElement($n383, $n384);
      $renderComponents["components/scroll_to_number/index"].update_3d91b8_384($lepusComponent, $n384, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/sku_bubble/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s, _t, _u, _v, _w, _x, _y, _z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M;
    let $path = "components/sku_bubble/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 388);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n390 = $lepusGetElementRefByLepusID("view", 390);
      {
        let $value = "left:" + (((_a = $data.data) == null ? undefined : _a.direction) === "left" ? "-14rpx" : "10rpx") + ";";
        if (!$update2 || $value !== "left:" + (((_b = $lepusComponent._data.data) == null ? undefined : _b.direction) === "left" ? "-14rpx" : "10rpx") + ";") {
          __SetStyleObject($n390, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-stroke", "38422000"), {
            2: ((_c = $data.data) == null ? undefined : _c.direction) === "left" ? "-14rpx" : "10rpx"
          }]);
        }
      }
    }
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n391 = $lepusGetElementRefByLepusID("view", 391);
      {
        let _$value38 = "max-width:" + (((_d = $data.data) == null ? undefined : _d.direction) === "left" ? 360 * ((_f = (_e = $data.data) == null ? undefined : _e.position) == null ? undefined : _f[0]) - 26 : 360 - 26 - 360 * ((_h = (_g = $data.data) == null ? undefined : _g.position) == null ? undefined : _h[0])) + "rpx;" + (((_i = $data.data) == null ? undefined : _i.direction) === "left" ? "right: 26rpx" : "left: 26rpx") + ";";
        if (!$update2 || _$value38 !== "max-width:" + (((_j = $lepusComponent._data.data) == null ? undefined : _j.direction) === "left" ? 360 * ((_l = (_k = $lepusComponent._data.data) == null ? undefined : _k.position) == null ? undefined : _l[0]) - 26 : 360 - 26 - 360 * ((_n = (_m = $lepusComponent._data.data) == null ? undefined : _m.position) == null ? undefined : _n[0])) + "rpx;" + (((_o = $lepusComponent._data.data) == null ? undefined : _o.direction) === "left" ? "right: 26rpx" : "left: 26rpx") + ";") {
          __SetStyleObject($n391, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-container", "38422000"), {
            28: (((_p = $data.data) == null ? undefined : _p.direction) === "left" ? 360 * ((_r = (_q = $data.data) == null ? undefined : _q.position) == null ? undefined : _r[0]) - 26 : 360 - 26 - 360 * ((_t = (_s = $data.data) == null ? undefined : _s.position) == null ? undefined : _t[0])) + "rpx"
          }, ((_u = $data.data) == null ? undefined : _u.direction) === "left" ? [379] : [380]]);
        }
      }
    }
    $lepusGetElementRefByLepusID("view", 392);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n393 = $lepusGetElementRefByLepusID("text", 393);
      {
        __SetStyleObject($n393, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-title-text", "38422000"), parseStyleStringToObject((_w = (_v = $data.data) == null ? undefined : _v.title) == null ? undefined : _w.style)]);
      }
    }
    if ($renderComponents[$path].varUpdateState[0]) {
      let _$value39 = (_y = (_x = $data.data) == null ? undefined : _x.title) == null ? undefined : _y.content;
      if (_$value39 !== ((_A = (_z = $lepusComponent._data.data) == null ? undefined : _z.title) == null ? undefined : _A.content)) {
        let _$n = $lepusGetElementRefByLepusID("text", 393);
        __SetAttribute(_$n, "text", _$value39);
      }
    }
    $lepusGetElementRefByLepusID("text", 395);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n396 = $lepusGetElementRefByLepusID("text", 396);
      {
        __SetStyleObject($n396, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-price-integer", "38422000"), {
          128: (isAndroid5() ? "0.6" : "-0.4") + "rpx"
        }, parseStyleStringToObject((_C = (_B = $data.data) == null ? undefined : _B.price) == null ? undefined : _C.style)]);
      }
    }
    if ($renderComponents[$path].varUpdateState[0]) {
      let _$value40 = "$" + ((_E = (_D = $data.data) == null ? undefined : _D.price) == null ? undefined : _E.content);
      if (_$value40 !== "$" + ((_G = (_F = $lepusComponent._data.data) == null ? undefined : _F.price) == null ? undefined : _G.content)) {
        let _$n2 = $lepusGetElementRefByLepusID("text", 396);
        __SetAttribute(_$n2, "text", _$value40);
      }
    }
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n399 = $lepusGetElementRefByLepusID("text", 399);
      {
        __SetStyleObject($n399, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-price-label", "38422000"), parseStyleStringToObject((_I = (_H = $data.data) == null ? undefined : _H.price_label) == null ? undefined : _I.style)]);
      }
    }
    if ($renderComponents[$path].varUpdateState[0]) {
      let _$value41 = (_K = (_J = $data.data) == null ? undefined : _J.price_label) == null ? undefined : _K.content;
      if (_$value41 !== ((_M = (_L = $lepusComponent._data.data) == null ? undefined : _L.price_label) == null ? undefined : _M.content)) {
        let _$n3 = $lepusGetElementRefByLepusID("text", 399);
        __SetAttribute(_$n3, "text", _$value41);
      }
    }
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s;
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n388 = __CreateView($currentComponentId);
      __SetStyleObject($n388, [0]);
      __AppendElement($component, $n388);
      let $n389 = __CreateImage($currentComponentId);
      __SetStyleObject($n389, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-pic", "38422000")]);
      __SetAttribute($n389, "src", "network_address");
      __AppendElement($n388, $n389);
      let $n390 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n390, 390, "view");
      __SetStyleObject($n390, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-stroke", "38422000"), {
        2: ((_a = $data.data) == null ? undefined : _a.direction) === "left" ? "-14rpx" : "10rpx"
      }]);
      __AppendElement($n388, $n390);
      let $n391 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n391, 391, "view");
      __SetStyleObject($n391, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-container", "38422000"), {
        28: (((_b = $data.data) == null ? undefined : _b.direction) === "left" ? 360 * ((_d = (_c = $data.data) == null ? undefined : _c.position) == null ? undefined : _d[0]) - 26 : 360 - 26 - 360 * ((_f = (_e = $data.data) == null ? undefined : _e.position) == null ? undefined : _f[0])) + "rpx"
      }, ((_g = $data.data) == null ? undefined : _g.direction) === "left" ? [379] : [380]]);
      __AppendElement($n388, $n391);
      let $n392 = __CreateView($currentComponentId);
      __SetStyleObject($n392, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-title", "38422000")]);
      __AppendElement($n391, $n392);
      let $n393 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n393, 393, "text");
      __SetStyleObject($n393, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-title-text", "38422000"), parseStyleStringToObject((_i = (_h = $data.data) == null ? undefined : _h.title) == null ? undefined : _i.style)]);
      __SetAttribute($n393, "text-maxline", "1");
      __AppendElement($n392, $n393);
      __SetAttribute($n393, "text", (_k = (_j = $data.data) == null ? undefined : _j.title) == null ? undefined : _k.content);
      let $n395 = __CreateText($currentComponentId);
      __SetStyleObject($n395, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-price", "38422000")]);
      __SetAttribute($n395, "text-maxline", "1");
      __AppendElement($n391, $n395);
      let $n396 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n396, 396, "text");
      __SetStyleObject($n396, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-price-integer", "38422000"), {
        128: (isAndroid5() ? "0.6" : "-0.4") + "rpx"
      }, parseStyleStringToObject((_m = (_l = $data.data) == null ? undefined : _l.price) == null ? undefined : _m.style)]);
      __AppendElement($n395, $n396);
      __SetAttribute($n396, "text", "$" + ((_o = (_n = $data.data) == null ? undefined : _n.price) == null ? undefined : _o.content));
      let $n398 = __CreateImage($currentComponentId);
      __SetStyleObject($n398, [381]);
      __SetAttribute($n398, "src", "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAMAAAAoyzS7AAAAA1BMVEVHcEyC+tLSAAAAAXRSTlMAQObYZgAAAApJREFUCJljYAAAAAIAAfRxZKYAAAAASUVORK5CYII=");
      __SetAttribute($n398, "skip-redirection", true);
      __AppendElement($n395, $n398);
      let $n399 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n399, 399, "text");
      __SetStyleObject($n399, [getClassStyleIndex("gyl_buy_together-main_pic-bubble-price-label", "38422000"), parseStyleStringToObject((_q = (_p = $data.data) == null ? undefined : _p.price_label) == null ? undefined : _q.style)]);
      __AppendElement($n395, $n399);
      __SetAttribute($n399, "text", (_s = (_r = $data.data) == null ? undefined : _r.price_label) == null ? undefined : _s.content);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/sku_pics/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update_22898d8_402: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/sku_pics/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo18 = $lepusPushFiberForNode($parent, 402, uniqueId),
          $forLepus = _$lepusPushFiberForNo18[0],
          $lastForLepus = _$lepusPushFiberForNo18[1];
      let $object = (_a = $data.data) == null ? undefined : _a.covers;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.covers;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n403 = $update2 ? $lepusGetElementRefByLepusID("view", 403) : null;
          let $temp2 = $update2;
          if (!$n403) {
            $update2 = false;
            $n403 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n403, 403, "view");
            __AppendElement($parent, $n403);
          }
          __SetStyleObject($n403, [index >= 1 ? [382] : []]);
          {
            let $n404 = $update2 ? $lepusGetElementRefByLepusID("image", 404) : null;
            let $temp3 = $update2;
            if (!$n404) {
              $update2 = false;
              $n404 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n404, 404, "image");
              __SetAttribute($n404, "mode", "aspectFill");
              __SetAttribute($n404, "skip-redirection", true);
              __SetAttribute($n404, "accessibility-element", false);
              __AppendElement($n403, $n404);
            }
            __SetStyleObject($n404, [getClassStyleIndex("gyl-sku_pics-cover", "35645000"), parseStyleStringToObject(item == null ? undefined : item.style)]);
            __SetAttribute($n404, "src", item == null ? undefined : item.url);
            $update2 = $temp3;
          }
          {
            let $n405 = $update2 ? $lepusGetElementRefByLepusID("view", 405) : null;
            let _$temp36 = $update2;
            if (!$n405) {
              $update2 = false;
              $n405 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n405, 405, "view");
              __SetStyleObject($n405, [getClassStyleIndex("gyl-sku_pics-mask", "35645000")]);
              __AppendElement($n403, $n405);
            }
            $update2 = _$temp36;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a;
    let $path = "components/sku_pics/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n401 = $lepusGetElementRefByLepusID("view", 401);
      {
        __SetStyleObject($n401, [getClassStyleIndex("gyl-sku_pics", "35645000"), parseStyleStringToObject((_a = $data.data) == null ? undefined : _a.style)]);
      }
    }
    let $n402 = $lepusGetElementRefByLepusID("for", 402);
    $renderComponents[$path].update_22898d8_402($lepusComponent, $n402, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a;
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n401 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n401, 401, "view");
      __SetStyleObject($n401, [getClassStyleIndex("gyl-sku_pics", "35645000"), parseStyleStringToObject((_a = $data.data) == null ? undefined : _a.style)]);
      __AppendElement($component, $n401);
      let $n402 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n402, 402, "for");
      __AppendElement($n401, $n402);
      $renderComponents["components/sku_pics/index"].update_22898d8_402($lepusComponent, $n402, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/sku_product/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update_22898d8_411: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    if (!$update2 || $renderComponents["components/sku_product/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo19 = $lepusPushFiberForNode($parent, 411, uniqueId),
          $forLepus = _$lepusPushFiberForNo19[0],
          $lastForLepus = _$lepusPushFiberForNo19[1];
      let $object = (_a = $data.data) == null ? undefined : _a.sku_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.sku_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n412 = $update2 ? $lepusGetElementRefByLepusID("view", 412) : null;
          let $temp2 = $update2;
          if (!$n412) {
            $update2 = false;
            $n412 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n412, 412, "view");
            __SetStyleObject($n412, [getClassStyleIndex("gyl-cover__cover-blur-sku_products", "35681000")]);
            __AppendElement($parent, $n412);
          }
          {
            let $n413 = $update2 ? $lepusGetElementRefByLepusID("if", 413) : null;
            if (!$n413) {
              $n413 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n413, 413, "if");
              __AppendElement($n412, $n413);
            }
            let uniqueId2 = __GetElementUniqueID($n413);
            if (!$update2) {
              $conditionNodeIndex[uniqueId2] = -1;
            }
            let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
            if ((_c = item == null ? undefined : item.image) == null ? undefined : _c.url) {
              __UpdateIfNodeIndex($n413, 0);
              $conditionNodeIndex[uniqueId2] = 0;
              let $temp3 = $update2;
              if ($ifNodeIndex !== 0) {
                $update2 = false;
              }
              {
                let $n414 = $update2 ? $lepusGetElementRefByLepusID("image", 414) : null;
                let $temp4 = $update2;
                if (!$n414) {
                  $update2 = false;
                  $n414 = __CreateImage($currentComponentId);
                  $lepusStoreElementRefByLepusID($n414, 414, "image");
                  __SetAttribute($n414, "mode", "aspectFill");
                  __SetAttribute($n414, "skip-redirection", true);
                  __AppendElement($n413, $n414);
                }
                __SetStyleObject($n414, [getClassStyleIndex("gyl-cover__cover-blur-sku_products-image", "35681000"), parseStyleStringToObject((_d = item == null ? undefined : item.image) == null ? undefined : _d.style)]);
                __SetAttribute($n414, "src", (_e = item == null ? undefined : item.image) == null ? undefined : _e.url);
                $update2 = $temp4;
              }
              $update2 = $temp3;
            } else {
              __UpdateIfNodeIndex($n413, -1);
              $conditionNodeIndex[uniqueId2] = -1;
            }
          }
          {
            let $n415 = $update2 ? $lepusGetElementRefByLepusID("view", 415) : null;
            let _$temp37 = $update2;
            if (!$n415) {
              $update2 = false;
              $n415 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n415, 415, "view");
              __SetStyleObject($n415, [getClassStyleIndex("gyl-cover__cover-blur-sku_products-price-container", "35681000")]);
              __AppendElement($n412, $n415);
            }
            {
              let $n416 = $update2 ? $lepusGetElementRefByLepusID("if", 416) : null;
              if (!$n416) {
                $n416 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n416, 416, "if");
                __AppendElement($n415, $n416);
              }
              let _uniqueId3 = __GetElementUniqueID($n416);
              if (!$update2) {
                $conditionNodeIndex[_uniqueId3] = -1;
              }
              let _$ifNodeIndex3 = $conditionNodeIndex[_uniqueId3];
              if ((_f = item == null ? undefined : item.price) == null ? undefined : _f.content) {
                __UpdateIfNodeIndex($n416, 0);
                $conditionNodeIndex[_uniqueId3] = 0;
                let _$temp38 = $update2;
                if (_$ifNodeIndex3 !== 0) {
                  $update2 = false;
                }
                {
                  let $n417 = $update2 ? $lepusGetElementRefByLepusID("text", 417) : null;
                  let $temp5 = $update2;
                  if (!$n417) {
                    $update2 = false;
                    $n417 = __CreateText($currentComponentId);
                    $lepusStoreElementRefByLepusID($n417, 417, "text");
                    __AppendElement($n416, $n417);
                  }
                  __SetStyleObject($n417, [getClassStyleIndex("gyl-cover__cover-blur-sku_products-price", "35681000"), parseStyleStringToObject((_g = item == null ? undefined : item.price) == null ? undefined : _g.style)]);
                  __SetAttribute($n417, "text", "$" + ((_h = item == null ? undefined : item.price) == null ? undefined : _h.content));
                  $update2 = $temp5;
                }
                $update2 = _$temp38;
              } else {
                __UpdateIfNodeIndex($n416, -1);
                $conditionNodeIndex[_uniqueId3] = -1;
              }
            }
            {
              let $n419 = $update2 ? $lepusGetElementRefByLepusID("if", 419) : null;
              if (!$n419) {
                $n419 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n419, 419, "if");
                __AppendElement($n415, $n419);
              }
              let _uniqueId4 = __GetElementUniqueID($n419);
              if (!$update2) {
                $conditionNodeIndex[_uniqueId4] = -1;
              }
              let _$ifNodeIndex4 = $conditionNodeIndex[_uniqueId4];
              if ((_i = item == null ? undefined : item.price_decimal) == null ? undefined : _i.content) {
                __UpdateIfNodeIndex($n419, 0);
                $conditionNodeIndex[_uniqueId4] = 0;
                let _$temp39 = $update2;
                if (_$ifNodeIndex4 !== 0) {
                  $update2 = false;
                }
                {
                  let $n420 = $update2 ? $lepusGetElementRefByLepusID("text", 420) : null;
                  let _$temp40 = $update2;
                  if (!$n420) {
                    $update2 = false;
                    $n420 = __CreateText($currentComponentId);
                    $lepusStoreElementRefByLepusID($n420, 420, "text");
                    __AppendElement($n419, $n420);
                  }
                  __SetStyleObject($n420, [getClassStyleIndex("gyl-cover__cover-blur-sku_products-price-decimal", "35681000"), parseStyleStringToObject((_j = item == null ? undefined : item.price_decimal) == null ? undefined : _j.style)]);
                  __SetAttribute($n420, "text", (_k = item == null ? undefined : item.price_decimal) == null ? undefined : _k.content);
                  $update2 = _$temp40;
                }
                $update2 = _$temp39;
              } else {
                __UpdateIfNodeIndex($n419, -1);
                $conditionNodeIndex[_uniqueId4] = -1;
              }
            }
            $update2 = _$temp37;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e;
    let $path = "components/sku_product/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    $lepusGetElementRefByLepusID("view", 406);
    $lepusGetElementRefByLepusID("view", 407);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n408 = $lepusGetElementRefByLepusID("image", 408);
      {
        let $value = (_b = (_a = $data.data) == null ? undefined : _a.cover) == null ? undefined : _b.src;
        if (!$update2 || $value !== ((_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.cover) == null ? undefined : _d.src)) {
          __SetAttribute($n408, "src", $value);
        }
      }
    }
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n410 = $lepusGetElementRefByLepusID("view", 410);
      {
        __SetStyleObject($n410, [getClassStyleIndex("gyl-cover__cover-blur-container", "35681000"), parseStyleStringToObject((_e = $data.data) == null ? undefined : _e.sku_list_style)]);
      }
    }
    let $n411 = $lepusGetElementRefByLepusID("for", 411);
    $renderComponents[$path].update_22898d8_411($lepusComponent, $n411, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a, _b, _c;
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n406 = __CreateView($currentComponentId);
      __AppendElement($component, $n406);
      let $n407 = __CreateView($currentComponentId);
      __SetStyleObject($n407, [getClassStyleIndex("gyl-cover__cover-blur", "35681000")]);
      __AppendElement($n406, $n407);
      let $n408 = __CreateImage($currentComponentId);
      $lepusStoreElementRefByLepusID($n408, 408, "image");
      __SetStyleObject($n408, [getClassStyleIndex("gyl-cover__cover-blur-img", "35681000")]);
      __SetAttribute($n408, "skip-redirection", true);
      __SetAttribute($n408, "src", (_b = (_a = $data.data) == null ? undefined : _a.cover) == null ? undefined : _b.src);
      __AppendElement($n407, $n408);
      let $n409 = __CreateView($currentComponentId);
      __SetStyleObject($n409, [getClassStyleIndex("gyl-cover__cover-mask", "35681000")]);
      __AppendElement($n406, $n409);
      let $n410 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n410, 410, "view");
      __SetStyleObject($n410, [getClassStyleIndex("gyl-cover__cover-blur-container", "35681000"), parseStyleStringToObject((_c = $data.data) == null ? undefined : _c.sku_list_style)]);
      __AppendElement($n406, $n410);
      let $n411 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n411, 411, "for");
      __AppendElement($n410, $n411);
      $renderComponents["components/sku_product/index"].update_22898d8_411($lepusComponent, $n411, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/splitor/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update_22898d8_422: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let $path = "components/splitor/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.data.splitor_style === 1) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n423 = $update2 ? $lepusGetElementRefByLepusID("view", 423) : null;
            let $temp2 = $update2;
            if (!$n423) {
              $update2 = false;
              $n423 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n423, 423, "view");
              __AppendElement($parent, $n423);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                let $value = "height:" + $data.data.height + ";box-sizing:border-box;width:" + $data.data.width + ";border-left:" + $data.data.width + " solid " + $data.data.color + ";";
                if (!$update2 || $value !== "height:" + $lepusComponent._data.data.height + ";box-sizing:border-box;width:" + $lepusComponent._data.data.width + ";border-left:" + $lepusComponent._data.data.width + " solid " + $lepusComponent._data.data.color + ";") {
                  __SetStyleObject($n423, [{
                    26: $data.data.height + ""
                  }, 171, {
                    27: $data.data.width + ""
                  }, {
                    106: $data.data.width + " solid " + $data.data.color
                  }]);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp41 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $template_update = $update2;
            let $n424 = $update2 ? $lepusGetElementRefByLepusID("if", 424) : null;
            if (!$n424) {
              $update2 = false;
              $n424 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n424, 424, "if");
              __AppendElement($parent, $n424);
            }
            $renderComponents[$path].update_22898d8_424($lepusComponent, $n424, $data, $props, $update2, $slotUpdate);
            $update2 = $template_update;
          }
          $update2 = _$temp41;
        }
      }
    }
  },
  update_22898d8_424: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let $path = "components/splitor/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.data.splitor_style === 2) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n425 = $update2 ? $lepusGetElementRefByLepusID("view", 425) : null;
          let $temp2 = $update2;
          if (!$n425) {
            $update2 = false;
            $n425 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n425, 425, "view");
            __AppendElement($parent, $n425);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              let $value = "height:" + $data.data.height + ";box-sizing:border-box;width:" + $data.data.width + ";border-left:" + $data.data.width + " dotted " + $data.data.color + ";";
              if (!$update2 || $value !== "height:" + $lepusComponent._data.data.height + ";box-sizing:border-box;width:" + $lepusComponent._data.data.width + ";border-left:" + $lepusComponent._data.data.width + " dotted " + $lepusComponent._data.data.color + ";") {
                __SetStyleObject($n425, [{
                  26: $data.data.height + ""
                }, 171, {
                  27: $data.data.width + ""
                }, {
                  106: $data.data.width + " dotted " + $data.data.color
                }]);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 3);
        $conditionNodeIndex[uniqueId] = 3;
        let _$temp42 = $update2;
        if ($ifNodeIndex !== 3) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n426 = $update2 ? $lepusGetElementRefByLepusID("if", 426) : null;
          if (!$n426) {
            $update2 = false;
            $n426 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n426, 426, "if");
            __AppendElement($parent, $n426);
          }
          $renderComponents[$path].update_22898d8_426($lepusComponent, $n426, $data, $props, $update2, $slotUpdate);
          $update2 = $template_update;
        }
        $update2 = _$temp42;
      }
    }
  },
  update_22898d8_426: function ($lepusComponent, $parent, $data, $props, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.data.splitor_style === 3) {
        __UpdateIfNodeIndex($parent, 4);
        $conditionNodeIndex[uniqueId] = 4;
        let $temp = $update2;
        if ($ifNodeIndex !== 4) {
          $update2 = false;
        }
        {
          let $n427 = $update2 ? $lepusGetElementRefByLepusID("view", 427) : null;
          let $temp2 = $update2;
          if (!$n427) {
            $update2 = false;
            $n427 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n427, 427, "view");
            __AppendElement($parent, $n427);
          }
          if (!$update2 || $renderComponents["components/splitor/index"].varUpdateState[0]) {
            {
              let $value = "height:" + $data.data.height + ";box-sizing:border-box;width:" + $data.data.width + ";border-radius:50%;background-color:" + $data.data.color + " ;";
              if (!$update2 || $value !== "height:" + $lepusComponent._data.data.height + ";box-sizing:border-box;width:" + $lepusComponent._data.data.width + ";border-radius:50%;background-color:" + $lepusComponent._data.data.color + " ;") {
                __SetStyleObject($n427, [{
                  26: $data.data.height + ""
                }, 171, {
                  27: $data.data.width + ""
                }, 383, {
                  7: $data.data.color + " "
                }]);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 5);
        $conditionNodeIndex[uniqueId] = 5;
        let _$temp43 = $update2;
        if ($ifNodeIndex !== 5) {
          $update2 = false;
        }
        {
          let $n428 = $update2 ? $lepusGetElementRefByLepusID("view", 428) : null;
          let _$temp44 = $update2;
          if (!$n428) {
            $update2 = false;
            $n428 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n428, 428, "view");
            __AppendElement($parent, $n428);
          }
          $update2 = _$temp44;
        }
        $update2 = _$temp43;
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/splitor/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    let $n422 = $lepusGetElementRefByLepusID("if", 422);
    $renderComponents[$path].update_22898d8_422($lepusComponent, $n422, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n422 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n422, 422, "if");
      __AppendElement($component, $n422);
      $renderComponents["components/splitor/index"].update_22898d8_422($lepusComponent, $n422, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/suspend_common/index"] = {
  variables: ["data", "isCouponBarCountDownCompleted", "couponBarLoading", "isCouponBarCountDownUpdated"],
  varUpdateState: [],
  update_22898d8_430: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.instant_pruchase) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n431 = $update2 ? $lepusGetElementRefByLepusID("view", 431) : null;
            let $temp2 = $update2;
            if (!$n431) {
              $update2 = false;
              $n431 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n431, 431, "view");
              __AppendElement($parent, $n431);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n431, [getClassStyleIndex("gyl-suspend__btn", "27959000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.instant_pruchase) == null ? undefined : _d.style)]);
              }
            }
            {
              let $n432 = $update2 ? $lepusGetElementRefByLepusID("text", 432) : null;
              let $temp3 = $update2;
              if (!$n432) {
                $update2 = false;
                $n432 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n432, 432, "text");
                __SetAttribute($n432, "accessibility-element", false);
                __AppendElement($n431, $n432);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n432, [parseStyleStringToObject((_f = (_e = $data.data) == null ? undefined : _e.instant_pruchase) == null ? undefined : _f.style)]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let $value = (_h = (_g = $data.data) == null ? undefined : _g.instant_pruchase) == null ? undefined : _h.content;
                  if (!$update2 || $value !== ((_j = (_i = $lepusComponent._data.data) == null ? undefined : _i.instant_pruchase) == null ? undefined : _j.content)) {
                    __SetAttribute($n432, "text", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_434: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.topic) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n435 = $update2 ? $lepusGetElementRefByLepusID("view", 435) : null;
            let $temp2 = $update2;
            if (!$n435) {
              $update2 = false;
              $n435 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n435, 435, "view");
              __SetStyleObject($n435, [getClassStyleIndex("gyl-suspend__line", "27959000")]);
              __AppendElement($parent, $n435);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_436: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.topic) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n437 = $update2 ? $lepusGetElementRefByLepusID("view", 437) : null;
            let $temp2 = $update2;
            if (!$n437) {
              $update2 = false;
              $n437 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n437, 437, "view");
              __AppendElement($parent, $n437);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n437, [getClassStyleIndex("gyl-suspend__topic", "27959000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.topic) == null ? undefined : _d.style)]);
              }
            }
            {
              let $template_update = $update2;
              let $n438 = $update2 ? $lepusGetElementRefByLepusID("if", 438) : null;
              if (!$n438) {
                $update2 = false;
                $n438 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n438, 438, "if");
                __AppendElement($n437, $n438);
              }
              $renderComponents[$path].update_22898d8_438($lepusComponent, $n438, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let $n440 = $update2 ? $lepusGetElementRefByLepusID("text", 440) : null;
              let $temp3 = $update2;
              if (!$n440) {
                $update2 = false;
                $n440 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n440, 440, "text");
                __SetAttribute($n440, "accessibility-element", false);
                __AppendElement($n437, $n440);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n440, [getClassStyleIndex("gyl-suspend__topic-text", "27959000"), parseStyleStringToObject((_g = (_f = (_e = $data.data) == null ? undefined : _e.topic) == null ? undefined : _f.content) == null ? undefined : _g.style)]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let $value = (_j = (_i = (_h = $data.data) == null ? undefined : _h.topic) == null ? undefined : _i.content) == null ? undefined : _j.content;
                  if (!$update2 || $value !== ((_m = (_l = (_k = $lepusComponent._data.data) == null ? undefined : _k.topic) == null ? undefined : _l.content) == null ? undefined : _m.content)) {
                    __SetAttribute($n440, "text", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            {
              let _$template_update6 = $update2;
              let $n442 = $update2 ? $lepusGetElementRefByLepusID("if", 442) : null;
              if (!$n442) {
                $update2 = false;
                $n442 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n442, 442, "if");
                __AppendElement($n437, $n442);
              }
              $renderComponents[$path].update_22898d8_442($lepusComponent, $n442, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update6;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_438: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.topic) == null ? undefined : _b.icon) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n439 = $update2 ? $lepusGetElementRefByLepusID("image", 439) : null;
          let $temp2 = $update2;
          if (!$n439) {
            $update2 = false;
            $n439 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n439, 439, "image");
            __SetAttribute($n439, "skip-redirection", true);
            __SetAttribute($n439, "accessibility-element", false);
            __AppendElement($parent, $n439);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n439, [parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.topic) == null ? undefined : _e.icon) == null ? undefined : _f.style)]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.topic) == null ? undefined : _h.icon) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.topic) == null ? undefined : _k.icon) == null ? undefined : _l.url)) {
                __SetAttribute($n439, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_442: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.topic) == null ? undefined : _b.arrow) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n443 = $update2 ? $lepusGetElementRefByLepusID("image", 443) : null;
          let $temp2 = $update2;
          if (!$n443) {
            $update2 = false;
            $n443 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n443, 443, "image");
            __SetAttribute($n443, "skip-redirection", true);
            __SetAttribute($n443, "accessibility-element", false);
            __AppendElement($parent, $n443);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n443, [parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.topic) == null ? undefined : _e.arrow) == null ? undefined : _f.style)]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.topic) == null ? undefined : _h.arrow) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.topic) == null ? undefined : _k.arrow) == null ? undefined : _l.url)) {
                __SetAttribute($n443, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_444: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.coin_btn) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n445 = $update2 ? $lepusGetElementRefByLepusID("view", 445) : null;
            let $temp2 = $update2;
            if (!$n445) {
              $update2 = false;
              $n445 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n445, 445, "view");
              __AppendElement($parent, $n445);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n445, [getClassStyleIndex("gyl-suspend__coin", "27959000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.coin_btn) == null ? undefined : _d.style)]);
              }
            }
            {
              let $template_update = $update2;
              let $n446 = $update2 ? $lepusGetElementRefByLepusID("if", 446) : null;
              if (!$n446) {
                $update2 = false;
                $n446 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n446, 446, "if");
                __AppendElement($n445, $n446);
              }
              $renderComponents[$path].update_22898d8_446($lepusComponent, $n446, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let $n448 = $update2 ? $lepusGetElementRefByLepusID("text", 448) : null;
              let $temp3 = $update2;
              if (!$n448) {
                $update2 = false;
                $n448 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n448, 448, "text");
                __SetAttribute($n448, "accessibility-element", false);
                __AppendElement($n445, $n448);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n448, [getClassStyleIndex("gyl-suspend__coin-text", "27959000"), parseStyleStringToObject((_g = (_f = (_e = $data.data) == null ? undefined : _e.coin_btn) == null ? undefined : _f.content) == null ? undefined : _g.style)]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let $value = (_j = (_i = (_h = $data.data) == null ? undefined : _h.coin_btn) == null ? undefined : _i.content) == null ? undefined : _j.content;
                  if (!$update2 || $value !== ((_m = (_l = (_k = $lepusComponent._data.data) == null ? undefined : _k.coin_btn) == null ? undefined : _l.content) == null ? undefined : _m.content)) {
                    __SetAttribute($n448, "text", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_446: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coin_btn) == null ? undefined : _b.icon) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n447 = $update2 ? $lepusGetElementRefByLepusID("image", 447) : null;
          let $temp2 = $update2;
          if (!$n447) {
            $update2 = false;
            $n447 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n447, 447, "image");
            __SetAttribute($n447, "skip-redirection", true);
            __SetAttribute($n447, "accessibility-element", false);
            __AppendElement($parent, $n447);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n447, [parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.coin_btn) == null ? undefined : _e.icon) == null ? undefined : _f.style)]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.coin_btn) == null ? undefined : _h.icon) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.coin_btn) == null ? undefined : _k.icon) == null ? undefined : _l.url)) {
                __SetAttribute($n447, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_450: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.instant_store_suspend) == null ? undefined : _b.content) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n451 = $update2 ? $lepusGetElementRefByLepusID("view", 451) : null;
            let $temp2 = $update2;
            if (!$n451) {
              $update2 = false;
              $n451 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n451, 451, "view");
              __AppendElement($parent, $n451);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n451, [getClassStyleIndex("gyl-suspend--store", "27959000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.instant_store_suspend) == null ? undefined : _d.style)]);
              }
            }
            {
              let $n452 = $update2 ? $lepusGetElementRefByLepusID("text", 452) : null;
              let $temp3 = $update2;
              if (!$n452) {
                $update2 = false;
                $n452 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n452, 452, "text");
                __SetAttribute($n452, "text-maxline", "1");
                __SetAttribute($n452, "accessibility-element", false);
                __AppendElement($n451, $n452);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n452, [getClassStyleIndex("gyl-suspend--store-text", "27959000"), parseStyleStringToObject((_g = (_f = (_e = $data.data) == null ? undefined : _e.instant_store_suspend) == null ? undefined : _f.content) == null ? undefined : _g.style)]);
                }
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  let $value = (_j = (_i = (_h = $data.data) == null ? undefined : _h.instant_store_suspend) == null ? undefined : _i.content) == null ? undefined : _j.content;
                  if (!$update2 || $value !== ((_m = (_l = (_k = $lepusComponent._data.data) == null ? undefined : _k.instant_store_suspend) == null ? undefined : _l.content) == null ? undefined : _m.content)) {
                    __SetAttribute($n452, "text", $value);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_454: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.content) && ((_d = (_c = $data.data) == null ? undefined : _c.coupon_bar) == null ? undefined : _d.use_new_style) && ((_f = (_e = $data.data) == null ? undefined : _e.coupon_bar) == null ? undefined : _f.go_full_discount_detail)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n455 = $update2 ? $lepusGetElementRefByLepusID("view", 455) : null;
            let $temp2 = $update2;
            if (!$n455) {
              $update2 = false;
              $n455 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n455, 455, "view");
              __AppendElement($parent, $n455);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n455, [getClassStyleIndex("gyl-suspend__coupon-bar-legou", "27959000"), parseStyleStringToObject((_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.style)]);
              }
            }
            {
              let $template_update = $update2;
              let $n456 = $update2 ? $lepusGetElementRefByLepusID("if", 456) : null;
              if (!$n456) {
                $update2 = false;
                $n456 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n456, 456, "if");
                __AppendElement($n455, $n456);
              }
              $renderComponents[$path].update_22898d8_456($lepusComponent, $n456, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let _$template_update7 = $update2;
              let $n459 = $update2 ? $lepusGetElementRefByLepusID("if", 459) : null;
              if (!$n459) {
                $update2 = false;
                $n459 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n459, 459, "if");
                __AppendElement($n455, $n459);
              }
              $renderComponents[$path].update_22898d8_459($lepusComponent, $n459, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update7;
            }
            {
              let _$template_update8 = $update2;
              let $n461 = $update2 ? $lepusGetElementRefByLepusID("if", 461) : null;
              if (!$n461) {
                $update2 = false;
                $n461 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n461, 461, "if");
                __AppendElement($n455, $n461);
              }
              $renderComponents[$path].update_22898d8_461($lepusComponent, $n461, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update8;
            }
            {
              let $n463 = $update2 ? $lepusGetElementRefByLepusID("view", 463) : null;
              let $temp3 = $update2;
              if (!$n463) {
                $update2 = false;
                $n463 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n463, 463, "view");
                __SetStyleObject($n463, [getClassStyleIndex("gyl-suspend__coupon-bar-anim-box-item gyl-suspend__coupon-bar-legou-box-item " + (isAndroid6() ? "" : "gyl-suspend__coupon-bar-legou-box-item_iOS"), "27959000")]);
                __AppendElement($n455, $n463);
              }
              {
                let $n464 = $update2 ? $lepusGetElementRefByLepusID("text", 464) : null;
                let $temp4 = $update2;
                if (!$n464) {
                  $update2 = false;
                  $n464 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n464, 464, "text");
                  __SetAttribute($n464, "text-maxline", "1");
                  __SetAttribute($n464, "accessibility-element", false);
                  __AppendElement($n463, $n464);
                }
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  {
                    __SetStyleObject($n464, [getClassStyleIndex("gyl-suspend__coupon-bar-text", "27959000"), parseStyleStringToObject((_k = (_j = (_i = $data.data) == null ? undefined : _i.coupon_bar) == null ? undefined : _j.content) == null ? undefined : _k.style)]);
                  }
                }
                {
                  if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                    __SetAttribute($n464, "text", getCouponBarTitle($data.data, false, false));
                  }
                }
                $update2 = $temp4;
              }
              {
                let _$template_update9 = $update2;
                let $n466 = $update2 ? $lepusGetElementRefByLepusID("if", 466) : null;
                if (!$n466) {
                  $update2 = false;
                  $n466 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n466, 466, "if");
                  __AppendElement($n463, $n466);
                }
                $renderComponents[$path].update_22898d8_466($lepusComponent, $n466, $data, $props, $update2, $slotUpdate);
                $update2 = _$template_update9;
              }
              $update2 = $temp3;
            }
            {
              let _$template_update10 = $update2;
              let $n469 = $update2 ? $lepusGetElementRefByLepusID("if", 469) : null;
              if (!$n469) {
                $update2 = false;
                $n469 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n469, 469, "if");
                __AppendElement($n455, $n469);
              }
              $renderComponents[$path].update_22898d8_469($lepusComponent, $n469, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update10;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_456: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    let $path = "components/suspend_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.header_content) == null ? undefined : _c.content) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n457 = $update2 ? $lepusGetElementRefByLepusID("text", 457) : null;
          let $temp2 = $update2;
          if (!$n457) {
            $update2 = false;
            $n457 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n457, 457, "text");
            __AppendElement($parent, $n457);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n457, [getClassStyleIndex("gyl-suspend__coupon-bar-legou-header-text", "27959000"), parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.header_content) == null ? undefined : _f.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.header_content) == null ? undefined : _i.content;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.coupon_bar) == null ? undefined : _k.header_content) == null ? undefined : _l.content)) {
                __SetAttribute($n457, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_459: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_c = (_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.icon) == null ? undefined : _c.url) && !((_f = (_e = (_d = $data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.header_content) == null ? undefined : _f.content)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n460 = $update2 ? $lepusGetElementRefByLepusID("image", 460) : null;
          let $temp2 = $update2;
          if (!$n460) {
            $update2 = false;
            $n460 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n460, 460, "image");
            __SetAttribute($n460, "skip-redirection", true);
            __SetAttribute($n460, "mode", "aspectFit");
            __SetAttribute($n460, "accessibility-element", false);
            __AppendElement($parent, $n460);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n460, [parseStyleStringToObject((_i = (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.icon) == null ? undefined : _i.style)]);
            }
            {
              let $value = (_l = (_k = (_j = $data.data) == null ? undefined : _j.coupon_bar) == null ? undefined : _k.icon) == null ? undefined : _l.url;
              if (!$update2 || $value !== ((_o = (_n = (_m = $lepusComponent._data.data) == null ? undefined : _m.coupon_bar) == null ? undefined : _n.icon) == null ? undefined : _o.url)) {
                __SetAttribute($n460, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_461: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.show_divider) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n462 = $update2 ? $lepusGetElementRefByLepusID("view", 462) : null;
          let $temp2 = $update2;
          if (!$n462) {
            $update2 = false;
            $n462 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n462, 462, "view");
            __SetStyleObject($n462, [getClassStyleIndex("gyl-suspend__coupon-bar-legou-divider", "27959000")]);
            __AppendElement($parent, $n462);
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_466: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    let $path = "components/suspend_common/index";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.link_content.content) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n467 = $update2 ? $lepusGetElementRefByLepusID("text", 467) : null;
          let $temp2 = $update2;
          if (!$n467) {
            $update2 = false;
            $n467 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n467, 467, "text");
            __SetAttribute($n467, "text-maxline", "1");
            __SetAttribute($n467, "accessibility-element", false);
            __AppendElement($parent, $n467);
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n467, [getClassStyleIndex("gyl-suspend__coupon-bar-text-coudan", "27959000"), parseStyleStringToObject((_e = (_d = (_c = $data.data) == null ? undefined : _c.coupon_bar) == null ? undefined : _d.link_content) == null ? undefined : _e.style)]);
            }
          }
          {
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $value = (_h = (_g = (_f = $data.data) == null ? undefined : _f.coupon_bar) == null ? undefined : _g.link_content) == null ? undefined : _h.content;
              if (!$update2 || $value !== ((_k = (_j = (_i = $lepusComponent._data.data) == null ? undefined : _i.coupon_bar) == null ? undefined : _j.link_content) == null ? undefined : _k.content)) {
                __SetAttribute($n467, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_469: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.arrow) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n470 = $update2 ? $lepusGetElementRefByLepusID("image", 470) : null;
          let $temp2 = $update2;
          if (!$n470) {
            $update2 = false;
            $n470 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n470, 470, "image");
            __SetAttribute($n470, "skip-redirection", true);
            __SetAttribute($n470, "mode", "aspectFit");
            __SetAttribute($n470, "accessibility-element", false);
            __AppendElement($parent, $n470);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n470, [parseStyleStringToObject((((_f = (_e = (_d = $data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.arrow) == null ? undefined : _f.style) || "").replace(/margin-left\s*:\s*[^;]+;/g, ""))]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.arrow) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.coupon_bar) == null ? undefined : _k.arrow) == null ? undefined : _l.url)) {
                __SetAttribute($n470, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_471: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.content) && ((_d = (_c = $data.data) == null ? undefined : _c.coupon_bar) == null ? undefined : _d.use_new_style) && !((_f = (_e = $data.data) == null ? undefined : _e.coupon_bar) == null ? undefined : _f.go_full_discount_detail)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n472 = $update2 ? $lepusGetElementRefByLepusID("view", 472) : null;
            let $temp2 = $update2;
            if (!$n472) {
              $update2 = false;
              $n472 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n472, 472, "view");
              __AppendElement($parent, $n472);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n472, [getClassStyleIndex("gyl-suspend__coupon-bar-legou", "27959000"), parseStyleStringToObject((_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.style)]);
              }
            }
            {
              let $template_update = $update2;
              let $n473 = $update2 ? $lepusGetElementRefByLepusID("if", 473) : null;
              if (!$n473) {
                $update2 = false;
                $n473 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n473, 473, "if");
                __AppendElement($n472, $n473);
              }
              $renderComponents[$path].update_22898d8_473($lepusComponent, $n473, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let $n475 = $update2 ? $lepusGetElementRefByLepusID("view", 475) : null;
              let $temp3 = $update2;
              if (!$n475) {
                $update2 = false;
                $n475 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n475, 475, "view");
                __SetStyleObject($n475, [getClassStyleIndex("gyl-suspend__coupon-bar-anim-box-item", "27959000")]);
                __AppendElement($n472, $n475);
              }
              {
                let $n476 = $update2 ? $lepusGetElementRefByLepusID("text", 476) : null;
                let $temp4 = $update2;
                if (!$n476) {
                  $update2 = false;
                  $n476 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n476, 476, "text");
                  __SetAttribute($n476, "text-maxline", "1");
                  __SetAttribute($n476, "accessibility-element", false);
                  __AppendElement($n475, $n476);
                }
                if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                  {
                    __SetStyleObject($n476, [getClassStyleIndex("gyl-suspend__coupon-bar-text", "27959000"), parseStyleStringToObject((_k = (_j = (_i = $data.data) == null ? undefined : _i.coupon_bar) == null ? undefined : _j.content) == null ? undefined : _k.style)]);
                  }
                }
                {
                  if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                    __SetAttribute($n476, "text", getCouponBarTitle($data.data, false, false));
                  }
                }
                $update2 = $temp4;
              }
              {
                let _$template_update11 = $update2;
                let $n478 = $update2 ? $lepusGetElementRefByLepusID("if", 478) : null;
                if (!$n478) {
                  $update2 = false;
                  $n478 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n478, 478, "if");
                  __AppendElement($n475, $n478);
                }
                $renderComponents[$path].update_22898d8_478($lepusComponent, $n478, $data, $props, $update2, $slotUpdate);
                $update2 = _$template_update11;
              }
              $update2 = $temp3;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_473: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.icon) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n474 = $update2 ? $lepusGetElementRefByLepusID("image", 474) : null;
          let $temp2 = $update2;
          if (!$n474) {
            $update2 = false;
            $n474 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n474, 474, "image");
            __SetAttribute($n474, "skip-redirection", true);
            __SetAttribute($n474, "mode", "aspectFit");
            __SetAttribute($n474, "accessibility-element", false);
            __AppendElement($parent, $n474);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n474, [parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.icon) == null ? undefined : _f.style)]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.icon) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.coupon_bar) == null ? undefined : _k.icon) == null ? undefined : _l.url)) {
                __SetAttribute($n474, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_478: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.arrow) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n479 = $update2 ? $lepusGetElementRefByLepusID("image", 479) : null;
          let $temp2 = $update2;
          if (!$n479) {
            $update2 = false;
            $n479 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n479, 479, "image");
            __SetAttribute($n479, "skip-redirection", true);
            __SetAttribute($n479, "mode", "aspectFit");
            __SetAttribute($n479, "accessibility-element", false);
            __AppendElement($parent, $n479);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n479, [parseStyleStringToObject((((_f = (_e = (_d = $data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.arrow) == null ? undefined : _f.style) || "").replace(/margin-right\s*:\s*[^;]+;/g, ""))]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.arrow) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.coupon_bar) == null ? undefined : _k.arrow) == null ? undefined : _l.url)) {
                __SetAttribute($n479, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_482: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.icon) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n483 = $update2 ? $lepusGetElementRefByLepusID("image", 483) : null;
          let $temp2 = $update2;
          if (!$n483) {
            $update2 = false;
            $n483 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n483, 483, "image");
            __SetAttribute($n483, "skip-redirection", true);
            __SetAttribute($n483, "accessibility-element", false);
            __AppendElement($parent, $n483);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n483, [parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.icon) == null ? undefined : _f.style)]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.icon) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.coupon_bar) == null ? undefined : _k.icon) == null ? undefined : _l.url)) {
                __SetAttribute($n483, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_494: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_c = (_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.arrow) == null ? undefined : _c.url) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n495 = $update2 ? $lepusGetElementRefByLepusID("image", 495) : null;
          let $temp2 = $update2;
          if (!$n495) {
            $update2 = false;
            $n495 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n495, 495, "image");
            __SetAttribute($n495, "skip-redirection", true);
            __SetAttribute($n495, "accessibility-element", false);
            __AppendElement($parent, $n495);
          }
          if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
            {
              __SetStyleObject($n495, [parseStyleStringToObject((_f = (_e = (_d = $data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.arrow) == null ? undefined : _f.style)]);
            }
            {
              let $value = (_i = (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.arrow) == null ? undefined : _i.url;
              if (!$update2 || $value !== ((_l = (_k = (_j = $lepusComponent._data.data) == null ? undefined : _j.coupon_bar) == null ? undefined : _k.arrow) == null ? undefined : _l.url)) {
                __SetAttribute($n495, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_22898d8_496: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_c = (_b = (_a = $data.data) == null ? undefined : _a.guide) == null ? undefined : _b.title) == null ? undefined : _c.length) > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n497 = $update2 ? $lepusGetElementRefByLepusID("view", 497) : null;
            let $temp2 = $update2;
            if (!$n497) {
              $update2 = false;
              $n497 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n497, 497, "view");
              __AppendElement($parent, $n497);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n497, [getClassStyleIndex("gyl-suspend__guid", "27959000"), parseStyleStringToObject((_e = (_d = $data.data) == null ? undefined : _d.guide) == null ? undefined : _e.style)]);
              }
            }
            {
              let $n498 = $update2 ? $lepusGetElementRefByLepusID("view", 498) : null;
              let $temp3 = $update2;
              if (!$n498) {
                $update2 = false;
                $n498 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n498, 498, "view");
                __SetStyleObject($n498, [getClassStyleIndex("gyl-suspend__guide-icon_left", "27959000")]);
                __AppendElement($n497, $n498);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $n499 = $update2 ? $lepusGetElementRefByLepusID("for", 499) : null;
                if (!$n499) {
                  $n499 = __CreateFor($currentComponentId);
                  $lepusStoreElementRefByLepusID($n499, 499, "for");
                  __AppendElement($n498, $n499);
                }
                $renderComponents[$path].update_22898d8_499($lepusComponent, $n499, $data, $props, $update2, $slotUpdate);
              }
              $update2 = $temp3;
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n501 = $update2 ? $lepusGetElementRefByLepusID("for", 501) : null;
              if (!$n501) {
                $n501 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n501, 501, "for");
                __AppendElement($n497, $n501);
              }
              $renderComponents[$path].update_22898d8_501($lepusComponent, $n501, $data, $props, $update2, $slotUpdate);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n505 = $update2 ? $lepusGetElementRefByLepusID("for", 505) : null;
              if (!$n505) {
                $n505 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n505, 505, "for");
                __AppendElement($n497, $n505);
              }
              $renderComponents[$path].update_22898d8_505($lepusComponent, $n505, $data, $props, $update2, $slotUpdate);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_499: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d;
    if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo20 = $lepusPushFiberForNode($parent, 499, uniqueId),
          $forLepus = _$lepusPushFiberForNo20[0],
          $lastForLepus = _$lepusPushFiberForNo20[1];
      let $object = (_b = (_a = $data.data) == null ? undefined : _a.guide) == null ? undefined : _b.icon_left;
      let $length = _GetLength($object);
      let $oldObject = (_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.guide) == null ? undefined : _d.icon_left;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n500 = $update2 ? $lepusGetElementRefByLepusID("image", 500) : null;
          let $temp2 = $update2;
          if (!$n500) {
            $update2 = false;
            $n500 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n500, 500, "image");
            __SetAttribute($n500, "mode", "aspectFill");
            __SetAttribute($n500, "skip-redirection", true);
            __SetAttribute($n500, "accessibility-element", false);
            __AppendElement($parent, $n500);
          }
          __SetStyleObject($n500, [parseStyleStringToObject(item == null ? undefined : item.style)]);
          __SetAttribute($n500, "src", item == null ? undefined : item.url);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_501: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d;
    if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo21 = $lepusPushFiberForNode($parent, 501, uniqueId),
          $forLepus = _$lepusPushFiberForNo21[0],
          $lastForLepus = _$lepusPushFiberForNo21[1];
      let $object = (_b = (_a = $data.data) == null ? undefined : _a.guide) == null ? undefined : _b.title;
      let $length = _GetLength($object);
      let $oldObject = (_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.guide) == null ? undefined : _d.title;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n502 = $update2 ? $lepusGetElementRefByLepusID("view", 502) : null;
          let $temp2 = $update2;
          if (!$n502) {
            $update2 = false;
            $n502 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n502, 502, "view");
            __SetStyleObject($n502, [getClassStyleIndex("gyl-suspend__guide-text", "27959000")]);
            __AppendElement($parent, $n502);
          }
          {
            let $n503 = $update2 ? $lepusGetElementRefByLepusID("text", 503) : null;
            let $temp3 = $update2;
            if (!$n503) {
              $update2 = false;
              $n503 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n503, 503, "text");
              __SetAttribute($n503, "text-maxline", "1");
              __SetAttribute($n503, "accessibility-element", false);
              __AppendElement($n502, $n503);
            }
            __SetStyleObject($n503, [parseStyleStringToObject(item == null ? undefined : item.style)]);
            __SetAttribute($n503, "text", item == null ? undefined : item.content);
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_505: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d;
    if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo22 = $lepusPushFiberForNode($parent, 505, uniqueId),
          $forLepus = _$lepusPushFiberForNo22[0],
          $lastForLepus = _$lepusPushFiberForNo22[1];
      let $object = (_b = (_a = $data.data) == null ? undefined : _a.guide) == null ? undefined : _b.icon_right;
      let $length = _GetLength($object);
      let $oldObject = (_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.guide) == null ? undefined : _d.icon_right;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n506 = $update2 ? $lepusGetElementRefByLepusID("view", 506) : null;
          let $temp2 = $update2;
          if (!$n506) {
            $update2 = false;
            $n506 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n506, 506, "view");
            __SetStyleObject($n506, [getClassStyleIndex("gyl-suspend__guide-icon_right", "27959000")]);
            __AppendElement($parent, $n506);
          }
          {
            let $n507 = $update2 ? $lepusGetElementRefByLepusID("image", 507) : null;
            let $temp3 = $update2;
            if (!$n507) {
              $update2 = false;
              $n507 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n507, 507, "image");
              __SetAttribute($n507, "skip-redirection", true);
              __SetAttribute($n507, "accessibility-element", false);
              __AppendElement($n506, $n507);
            }
            __SetStyleObject($n507, [parseStyleStringToObject(item == null ? undefined : item.style)]);
            __SetAttribute($n507, "src", item == null ? undefined : item.url);
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_508: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_c = (_b = (_a = $data.data) == null ? undefined : _a.guide_new) == null ? undefined : _b.title) == null ? undefined : _c.length) > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n509 = $update2 ? $lepusGetElementRefByLepusID("view", 509) : null;
            let $temp2 = $update2;
            if (!$n509) {
              $update2 = false;
              $n509 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n509, 509, "view");
              __AppendElement($parent, $n509);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n509, [getClassStyleIndex("gyl-suspend__guide_new", "27959000"), parseStyleStringToObject((_e = (_d = $data.data) == null ? undefined : _d.guide_new) == null ? undefined : _e.style)]);
              }
            }
            {
              let $n510 = $update2 ? $lepusGetElementRefByLepusID("view", 510) : null;
              let $temp3 = $update2;
              if (!$n510) {
                $update2 = false;
                $n510 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n510, 510, "view");
                __SetStyleObject($n510, [getClassStyleIndex("gyl-suspend__guide_new-icon_left", "27959000")]);
                __AppendElement($n509, $n510);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                let $n511 = $update2 ? $lepusGetElementRefByLepusID("for", 511) : null;
                if (!$n511) {
                  $n511 = __CreateFor($currentComponentId);
                  $lepusStoreElementRefByLepusID($n511, 511, "for");
                  __AppendElement($n510, $n511);
                }
                $renderComponents[$path].update_22898d8_511($lepusComponent, $n511, $data, $props, $update2, $slotUpdate);
              }
              $update2 = $temp3;
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n513 = $update2 ? $lepusGetElementRefByLepusID("for", 513) : null;
              if (!$n513) {
                $n513 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n513, 513, "for");
                __AppendElement($n509, $n513);
              }
              $renderComponents[$path].update_22898d8_513($lepusComponent, $n513, $data, $props, $update2, $slotUpdate);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n517 = $update2 ? $lepusGetElementRefByLepusID("for", 517) : null;
              if (!$n517) {
                $n517 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n517, 517, "for");
                __AppendElement($n509, $n517);
              }
              $renderComponents[$path].update_22898d8_517($lepusComponent, $n517, $data, $props, $update2, $slotUpdate);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_511: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d;
    if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo23 = $lepusPushFiberForNode($parent, 511, uniqueId),
          $forLepus = _$lepusPushFiberForNo23[0],
          $lastForLepus = _$lepusPushFiberForNo23[1];
      let $object = (_b = (_a = $data.data) == null ? undefined : _a.guide_new) == null ? undefined : _b.icon_left;
      let $length = _GetLength($object);
      let $oldObject = (_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.guide_new) == null ? undefined : _d.icon_left;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n512 = $update2 ? $lepusGetElementRefByLepusID("image", 512) : null;
          let $temp2 = $update2;
          if (!$n512) {
            $update2 = false;
            $n512 = __CreateImage($currentComponentId);
            $lepusStoreElementRefByLepusID($n512, 512, "image");
            __SetAttribute($n512, "mode", "aspectFill");
            __SetAttribute($n512, "skip-redirection", true);
            __SetAttribute($n512, "accessibility-element", false);
            __AppendElement($parent, $n512);
          }
          __SetStyleObject($n512, [parseStyleStringToObject(item == null ? undefined : item.style)]);
          __SetAttribute($n512, "src", item == null ? undefined : item.url);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_513: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d;
    if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo24 = $lepusPushFiberForNode($parent, 513, uniqueId),
          $forLepus = _$lepusPushFiberForNo24[0],
          $lastForLepus = _$lepusPushFiberForNo24[1];
      let $object = (_b = (_a = $data.data) == null ? undefined : _a.guide_new) == null ? undefined : _b.title;
      let $length = _GetLength($object);
      let $oldObject = (_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.guide_new) == null ? undefined : _d.title;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n514 = $update2 ? $lepusGetElementRefByLepusID("view", 514) : null;
          let $temp2 = $update2;
          if (!$n514) {
            $update2 = false;
            $n514 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n514, 514, "view");
            __SetStyleObject($n514, [getClassStyleIndex("gyl-suspend__guide_new-text", "27959000")]);
            __AppendElement($parent, $n514);
          }
          {
            let $n515 = $update2 ? $lepusGetElementRefByLepusID("text", 515) : null;
            let $temp3 = $update2;
            if (!$n515) {
              $update2 = false;
              $n515 = __CreateText($currentComponentId);
              $lepusStoreElementRefByLepusID($n515, 515, "text");
              __SetAttribute($n515, "text-maxline", "1");
              __SetAttribute($n515, "accessibility-element", false);
              __AppendElement($n514, $n515);
            }
            __SetStyleObject($n515, [parseStyleStringToObject(item == null ? undefined : item.style)]);
            __SetAttribute($n515, "text", item == null ? undefined : item.content);
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_517: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d;
    if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo25 = $lepusPushFiberForNode($parent, 517, uniqueId),
          $forLepus = _$lepusPushFiberForNo25[0],
          $lastForLepus = _$lepusPushFiberForNo25[1];
      let $object = (_b = (_a = $data.data) == null ? undefined : _a.guide_new) == null ? undefined : _b.icon_right;
      let $length = _GetLength($object);
      let $oldObject = (_d = (_c = $lepusComponent._data.data) == null ? undefined : _c.guide_new) == null ? undefined : _d.icon_right;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n518 = $update2 ? $lepusGetElementRefByLepusID("view", 518) : null;
          let $temp2 = $update2;
          if (!$n518) {
            $update2 = false;
            $n518 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n518, 518, "view");
            __SetStyleObject($n518, [getClassStyleIndex("gyl-suspend__guide_new-icon_right", "27959000")]);
            __AppendElement($parent, $n518);
          }
          {
            let $n519 = $update2 ? $lepusGetElementRefByLepusID("image", 519) : null;
            let $temp3 = $update2;
            if (!$n519) {
              $update2 = false;
              $n519 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n519, 519, "image");
              __SetAttribute($n519, "skip-redirection", true);
              __SetAttribute($n519, "accessibility-element", false);
              __AppendElement($n518, $n519);
            }
            __SetStyleObject($n519, [parseStyleStringToObject(item == null ? undefined : item.style)]);
            __SetAttribute($n519, "src", item == null ? undefined : item.url);
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_520: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.buy_together) == null ? undefined : _b.length) > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let _$lepusPushOwner14 = $lepusPushOwner(uniqueId + "_0"),
              owner = _$lepusPushOwner14[0],
              lastOwner = _$lepusPushOwner14[1];
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n521 = $update2 ? $lepusGetElementRefByLepusID("view", 521) : null;
            let $temp2 = $update2;
            if (!$n521) {
              $update2 = false;
              $n521 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n521, 521, "view");
              __SetStyleObject($n521, [getClassStyleIndex("gyl-suspend__buy_together", "27959000")]);
              __AppendElement($parent, $n521);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              let $n522 = $update2 ? $lepusGetElementRefByLepusID("for", 522) : null;
              if (!$n522) {
                $n522 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n522, 522, "for");
                __AppendElement($n521, $n522);
              }
              $renderComponents[$path].update_22898d8_522($lepusComponent, $n522, $data, $props, $update2, $slotUpdate);
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
          $lepusPopOwner(lastOwner);
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_522: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e;
    if (!$update2 || $renderComponents["components/suspend_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo26 = $lepusPushFiberForNode($parent, 522, uniqueId),
          $forLepus = _$lepusPushFiberForNo26[0],
          $lastForLepus = _$lepusPushFiberForNo26[1];
      let $object = (_a = $data.data) == null ? undefined : _a.buy_together;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.buy_together;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let _$lepusPushOwner15 = $lepusPushOwner(uniqueId + "_" + index),
            owner = _$lepusPushOwner15[0],
            lastOwner = _$lepusPushOwner15[1];
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n523 = $update2 ? $lepusGetElementRefByLepusID("view", 523) : null;
          let $temp2 = $update2;
          if (!$n523) {
            $update2 = false;
            $n523 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n523, 523, "view");
            __AppendElement($parent, $n523);
          }
          __SetStyleObject($n523, [getClassStyleIndex("gyl-suspend__buy_together-wrapper", "27959000"), index >= 1 ? [384] : []]);
          {
            let $n524 = $update2 ? $lepusGetElementRefByLepusID("image", 524) : null;
            let $temp3 = $update2;
            if (!$n524) {
              $update2 = false;
              $n524 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n524, 524, "image");
              __SetAttribute($n524, "skip-redirection", true);
              __SetAttribute($n524, "accessibility-element", false);
              __AppendElement($n523, $n524);
            }
            __SetStyleObject($n524, [getClassStyleIndex("gyl-suspend__buy_together-cover", "27959000"), parseStyleStringToObject((_c = item == null ? undefined : item.cover) == null ? undefined : _c.style)]);
            __SetAttribute($n524, "src", (_d = item == null ? undefined : item.cover) == null ? undefined : _d.content);
            $update2 = $temp3;
          }
          {
            let $n525 = $update2 ? $lepusGetElementRefByLepusID("component", 525) : null;
            let $compCreated = true;
            let $childLepusComponent = null;
            let $componentId = null;
            if (!$n525) {
              $compCreated = false;
              $n525 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 8, "", "price-common", "components/price_common/index", {});
              let $nid525 = $lepusStoreElementRefByLepusID($n525, 525, "price-common");
              $componentId = $nid525[0];
              $childLepusComponent = $componentConstructor($componentId, $n525, "components/price_common/index", 525);
              $createdIds.push($componentId + "");
              $cardInstance._currentOwner.componentIds.push($componentId);
              __AppendElement($n523, $n525);
            } else {
              $componentId = __GetElementUniqueID($n525);
              $childLepusComponent = $componentInfo[$componentId];
            }
            $comUpdatePropsSet = [];
            $childLepusComponent._setProp("data", item == null ? undefined : item.price, $update2);
            __SetStyleObject($n525, [getClassStyleIndex("gyl-suspend__buy_together-price", "27959000"), parseStyleStringToObject((_e = item == null ? undefined : item.price) == null ? undefined : _e.style)]);
            if ($compCreated) {
              let $update_keys = $comUpdatePropsSet;
              let $childSlotUpdate = false;
              if ($update_keys.length > 0 || $childSlotUpdate) {
                $updatedIds.push($componentId + "");
                $renderComponents["components/price_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
              }
            } else {
              $renderComponents["components/price_common/index"].entry($n525, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
            }
          }
          {
            let $n526 = $update2 ? $lepusGetElementRefByLepusID("view", 526) : null;
            let _$temp45 = $update2;
            if (!$n526) {
              $update2 = false;
              $n526 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n526, 526, "view");
              __SetStyleObject($n526, [getClassStyleIndex("gyl-suspend__buy_together-mask", "27959000")]);
              __AppendElement($n523, $n526);
            }
            $update2 = _$temp45;
          }
          $update2 = $temp2;
        }
        $lepusPopOwner(lastOwner);
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_2825d50_489: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_c = (_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.extra) == null ? undefined : _c.count_down) && !$data.isCouponBarCountDownCompleted) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner16 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner16[0],
            lastOwner = _$lepusPushOwner16[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n490 = $update2 ? $lepusGetElementRefByLepusID("component", 490) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n490) {
            $compCreated = false;
            $n490 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 17, "", "countdown", "components/countdown/index", {});
            let $nid490 = $lepusStoreElementRefByLepusID($n490, 490, "countdown");
            $componentId = $nid490[0];
            $childLepusComponent = $componentConstructor($componentId, $n490, "components/countdown/index", 490);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __SetStyleObject($n490, [getClassStyleIndex("gyl-suspend__coupon-bar-countdown", "27959000")]);
            __AddEvent($n490, "bindEvent", "Updated", "handleCouponBarCountDownUpdated");
            __AddEvent($n490, "bindEvent", "Completed", "handleCouponBarCountDownCompleted");
            __AppendElement($parent, $n490);
          } else {
            $componentId = __GetElementUniqueID($n490);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("duration", (_f = (_e = (_d = $data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.extra) == null ? undefined : _f.count_down, $update2 && $compCreated);
          $childLepusComponent._setProp("textStyle", (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.content, $update2 && $compCreated);
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/countdown/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/countdown/index"].entry($n490, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1820450_480: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r;
    let $path = "components/suspend_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.coupon_bar) == null ? undefined : _b.content) && !((_d = (_c = $data.data) == null ? undefined : _c.coupon_bar) == null ? undefined : _d.use_new_style)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let _$lepusPushOwner17 = $lepusPushOwner(uniqueId + "_0"),
              owner = _$lepusPushOwner17[0],
              lastOwner = _$lepusPushOwner17[1];
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n481 = $update2 ? $lepusGetElementRefByLepusID("view", 481) : null;
            let $temp2 = $update2;
            if (!$n481) {
              $update2 = false;
              $n481 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n481, 481, "view");
              __AppendElement($parent, $n481);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n481, [getClassStyleIndex("gyl-suspend__coupon-bar " + ($data.isCouponBarCountDownCompleted ? "gyl-suspend__coupon-bar--countdown-completed" : ""), "27959000"), parseStyleStringToObject((_f = (_e = $data.data) == null ? undefined : _e.coupon_bar) == null ? undefined : _f.style)]);
              }
            }
            {
              let $template_update = $update2;
              let $n482 = $update2 ? $lepusGetElementRefByLepusID("if", 482) : null;
              if (!$n482) {
                $update2 = false;
                $n482 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n482, 482, "if");
                __AppendElement($n481, $n482);
              }
              $renderComponents[$path].update_22898d8_482($lepusComponent, $n482, $data, $props, $update2, $slotUpdate);
              $update2 = $template_update;
            }
            {
              let $n484 = $update2 ? $lepusGetElementRefByLepusID("view", 484) : null;
              let $temp3 = $update2;
              if (!$n484) {
                $update2 = false;
                $n484 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n484, 484, "view");
                __SetStyleObject($n484, [getClassStyleIndex("gyl-suspend__coupon-bar--anim-box", "27959000")]);
                __AppendElement($n481, $n484);
              }
              {
                let $n485 = $update2 ? $lepusGetElementRefByLepusID("view", 485) : null;
                let $temp4 = $update2;
                if (!$n485) {
                  $update2 = false;
                  $n485 = __CreateView($currentComponentId);
                  $lepusStoreElementRefByLepusID($n485, 485, "view");
                  __AppendElement($n484, $n485);
                }
                {
                  let $value = "gyl-suspend__coupon-bar--anim-box-offset " + ($data.couponBarLoading ? "gyl-suspend__coupon-bar--anim-box-loading" : "");
                  if (!$update2 || $value !== "gyl-suspend__coupon-bar--anim-box-offset " + ($lepusComponent._data.couponBarLoading ? "gyl-suspend__coupon-bar--anim-box-loading" : "")) {
                    __SetStyleObject($n485, [getClassStyleIndex("gyl-suspend__coupon-bar--anim-box-offset " + ($data.couponBarLoading ? "gyl-suspend__coupon-bar--anim-box-loading" : ""), "27959000")]);
                  }
                }
                {
                  let $n486 = $update2 ? $lepusGetElementRefByLepusID("view", 486) : null;
                  let $temp5 = $update2;
                  if (!$n486) {
                    $update2 = false;
                    $n486 = __CreateView($currentComponentId);
                    $lepusStoreElementRefByLepusID($n486, 486, "view");
                    __SetStyleObject($n486, [getClassStyleIndex("gyl-suspend__coupon-bar-anim-box-item", "27959000")]);
                    __AppendElement($n485, $n486);
                  }
                  {
                    let $n487 = $update2 ? $lepusGetElementRefByLepusID("text", 487) : null;
                    let $temp6 = $update2;
                    if (!$n487) {
                      $update2 = false;
                      $n487 = __CreateText($currentComponentId);
                      $lepusStoreElementRefByLepusID($n487, 487, "text");
                      __SetAttribute($n487, "text-maxline", "1");
                      __SetAttribute($n487, "accessibility-element", false);
                      __AppendElement($n486, $n487);
                    }
                    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                      {
                        __SetStyleObject($n487, [getClassStyleIndex("gyl-suspend__coupon-bar-text", "27959000"), parseStyleStringToObject((_i = (_h = (_g = $data.data) == null ? undefined : _g.coupon_bar) == null ? undefined : _h.content) == null ? undefined : _i.style)]);
                      }
                    }
                    {
                      if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[3]) {
                        __SetAttribute($n487, "text", getCouponBarTitle($data.data, $data.isCouponBarCountDownCompleted, $data.isCouponBarCountDownUpdated));
                      }
                    }
                    $update2 = $temp6;
                  }
                  {
                    let _$template_update12 = $update2;
                    let $n489 = $update2 ? $lepusGetElementRefByLepusID("if", 489) : null;
                    if (!$n489) {
                      $update2 = false;
                      $n489 = __CreateIf($currentComponentId);
                      $lepusStoreElementRefByLepusID($n489, 489, "if");
                      __AppendElement($n486, $n489);
                    }
                    $renderComponents[$path].update_2825d50_489($lepusComponent, $n489, $data, $props, $update2, $slotUpdate);
                    $update2 = _$template_update12;
                  }
                  $update2 = $temp5;
                }
                {
                  let $n491 = $update2 ? $lepusGetElementRefByLepusID("view", 491) : null;
                  let _$temp46 = $update2;
                  if (!$n491) {
                    $update2 = false;
                    $n491 = __CreateView($currentComponentId);
                    $lepusStoreElementRefByLepusID($n491, 491, "view");
                    __SetStyleObject($n491, [getClassStyleIndex("gyl-suspend__coupon-bar-anim-box-item", "27959000")]);
                    __AppendElement($n485, $n491);
                  }
                  {
                    let $n492 = $update2 ? $lepusGetElementRefByLepusID("text", 492) : null;
                    let _$temp47 = $update2;
                    if (!$n492) {
                      $update2 = false;
                      $n492 = __CreateText($currentComponentId);
                      $lepusStoreElementRefByLepusID($n492, 492, "text");
                      __SetAttribute($n492, "text-maxline", "1");
                      __SetAttribute($n492, "accessibility-element", false);
                      __AppendElement($n491, $n492);
                    }
                    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                      {
                        __SetStyleObject($n492, [getClassStyleIndex("gyl-suspend__coupon-bar-text", "27959000"), parseStyleStringToObject((_l = (_k = (_j = $data.data) == null ? undefined : _j.coupon_bar) == null ? undefined : _k.content) == null ? undefined : _l.style)]);
                      }
                    }
                    {
                      if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                        let _$value42 = ((_o = (_n = (_m = $data.data) == null ? undefined : _m.coupon_bar) == null ? undefined : _n.extra) == null ? undefined : _o.loading_text) || "Loading discount";
                        if (!$update2 || _$value42 !== (((_r = (_q = (_p = $lepusComponent._data.data) == null ? undefined : _p.coupon_bar) == null ? undefined : _q.extra) == null ? undefined : _r.loading_text) || "Loading discount")) {
                          __SetAttribute($n492, "text", _$value42);
                        }
                      }
                    }
                    $update2 = _$temp47;
                  }
                  $update2 = _$temp46;
                }
                $update2 = $temp4;
              }
              $update2 = $temp3;
            }
            {
              let _$template_update13 = $update2;
              let $n494 = $update2 ? $lepusGetElementRefByLepusID("if", 494) : null;
              if (!$n494) {
                $update2 = false;
                $n494 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n494, 494, "if");
                __AppendElement($n481, $n494);
              }
              $renderComponents[$path].update_22898d8_494($lepusComponent, $n494, $data, $props, $update2, $slotUpdate);
              $update2 = _$template_update13;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
          $lepusPopOwner(lastOwner);
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e;
    let $path = "components/suspend_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n429 = $lepusGetElementRefByLepusID("view", 429);
      {
        __SetStyleObject($n429, [getClassStyleIndex("gyl-suspend__container", "27959000"), parseStyleStringToObject((_a = $data.data) == null ? undefined : _a.style)]);
      }
      {
        let $value = ((_c = (_b = $data.data) == null ? undefined : _b.coupon_bar) == null ? undefined : _c.content) ? "1" : "0";
        if (!$update2 || $value !== (((_e = (_d = $lepusComponent._data.data) == null ? undefined : _d.coupon_bar) == null ? undefined : _e.content) ? "1" : "0")) {
          __AddDataset($n429, "isCouponBar", $value);
        }
      }
    }
    let $n430 = $lepusGetElementRefByLepusID("if", 430);
    $renderComponents[$path].update_22898d8_430($lepusComponent, $n430, $data, $props, $update2, $slotUpdate);
    let $n434 = $lepusGetElementRefByLepusID("if", 434);
    $renderComponents[$path].update_22898d8_434($lepusComponent, $n434, $data, $props, $update2, $slotUpdate);
    let $n436 = $lepusGetElementRefByLepusID("if", 436);
    $renderComponents[$path].update_22898d8_436($lepusComponent, $n436, $data, $props, $update2, $slotUpdate);
    let $n444 = $lepusGetElementRefByLepusID("if", 444);
    $renderComponents[$path].update_22898d8_444($lepusComponent, $n444, $data, $props, $update2, $slotUpdate);
    let $n450 = $lepusGetElementRefByLepusID("if", 450);
    $renderComponents[$path].update_22898d8_450($lepusComponent, $n450, $data, $props, $update2, $slotUpdate);
    let $n454 = $lepusGetElementRefByLepusID("if", 454);
    $renderComponents[$path].update_22898d8_454($lepusComponent, $n454, $data, $props, $update2, $slotUpdate);
    let $n471 = $lepusGetElementRefByLepusID("if", 471);
    $renderComponents[$path].update_22898d8_471($lepusComponent, $n471, $data, $props, $update2, $slotUpdate);
    let $n480 = $lepusGetElementRefByLepusID("if", 480);
    $renderComponents[$path].update_1820450_480($lepusComponent, $n480, $data, $props, $update2, $slotUpdate);
    let $n496 = $lepusGetElementRefByLepusID("if", 496);
    $renderComponents[$path].update_22898d8_496($lepusComponent, $n496, $data, $props, $update2, $slotUpdate);
    let $n508 = $lepusGetElementRefByLepusID("if", 508);
    $renderComponents[$path].update_22898d8_508($lepusComponent, $n508, $data, $props, $update2, $slotUpdate);
    let $n520 = $lepusGetElementRefByLepusID("if", 520);
    $renderComponents[$path].update_22898d8_520($lepusComponent, $n520, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a, _b, _c;
    let $path = "components/suspend_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n429 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n429, 429, "view");
      __SetStyleObject($n429, [getClassStyleIndex("gyl-suspend__container", "27959000"), parseStyleStringToObject((_a = $data.data) == null ? undefined : _a.style)]);
      __AddDataset($n429, "isCouponBar", ((_c = (_b = $data.data) == null ? undefined : _b.coupon_bar) == null ? undefined : _c.content) ? "1" : "0");
      __AddEvent($n429, "catchEvent", "tap", "handleSuspendClick");
      __AppendElement($component, $n429);
      let $n430 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n430, 430, "if");
      __AppendElement($n429, $n430);
      $renderComponents[$path].update_22898d8_430($lepusComponent, $n430, $data, $props, $update2, $slotUpdate);
      let $n434 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n434, 434, "if");
      __AppendElement($n429, $n434);
      $renderComponents[$path].update_22898d8_434($lepusComponent, $n434, $data, $props, $update2, $slotUpdate);
      let $n436 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n436, 436, "if");
      __AppendElement($n429, $n436);
      $renderComponents[$path].update_22898d8_436($lepusComponent, $n436, $data, $props, $update2, $slotUpdate);
      let $n444 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n444, 444, "if");
      __AppendElement($n429, $n444);
      $renderComponents[$path].update_22898d8_444($lepusComponent, $n444, $data, $props, $update2, $slotUpdate);
      let $n450 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n450, 450, "if");
      __AppendElement($n429, $n450);
      $renderComponents[$path].update_22898d8_450($lepusComponent, $n450, $data, $props, $update2, $slotUpdate);
      let $n454 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n454, 454, "if");
      __AppendElement($n429, $n454);
      $renderComponents[$path].update_22898d8_454($lepusComponent, $n454, $data, $props, $update2, $slotUpdate);
      let $n471 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n471, 471, "if");
      __AppendElement($n429, $n471);
      $renderComponents[$path].update_22898d8_471($lepusComponent, $n471, $data, $props, $update2, $slotUpdate);
      let $n480 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n480, 480, "if");
      __AppendElement($n429, $n480);
      $renderComponents[$path].update_1820450_480($lepusComponent, $n480, $data, $props, $update2, $slotUpdate);
      let $n496 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n496, 496, "if");
      __AppendElement($n429, $n496);
      $renderComponents[$path].update_22898d8_496($lepusComponent, $n496, $data, $props, $update2, $slotUpdate);
      let $n508 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n508, 508, "if");
      __AppendElement($n429, $n508);
      $renderComponents[$path].update_22898d8_508($lepusComponent, $n508, $data, $props, $update2, $slotUpdate);
      let $n520 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n520, 520, "if");
      __AppendElement($n429, $n520);
      $renderComponents[$path].update_22898d8_520($lepusComponent, $n520, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/tag/index"] = {
  variables: ["newData", "backgroundStyle", "ui_items"],
  varUpdateState: [],
  update_274a980_528: function ($lepusComponent, $parent, $data, $props, $update2) {
    if (!$update2 || $renderComponents["components/tag/index"].varUpdateState[2]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo27 = $lepusPushFiberForNode($parent, 528, uniqueId),
          $forLepus = _$lepusPushFiberForNo27[0],
          $lastForLepus = _$lepusPushFiberForNo27[1];
      let $object = $data.ui_items;
      let $length = _GetLength($object);
      let $oldObject = $lepusComponent._data.ui_items;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let _$lepusPushOwner18 = $lepusPushOwner(uniqueId + "_" + index),
            owner = _$lepusPushOwner18[0],
            lastOwner = _$lepusPushOwner18[1];
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n529 = $update2 ? $lepusGetElementRefByLepusID("view", 529) : null;
          let $temp2 = $update2;
          if (!$n529) {
            $update2 = false;
            $n529 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n529, 529, "view");
            __AppendElement($parent, $n529);
          }
          {
            let $n530 = $update2 ? $lepusGetElementRefByLepusID("if", 530) : null;
            if (!$n530) {
              $n530 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n530, 530, "if");
              __AppendElement($n529, $n530);
            }
            let uniqueId2 = __GetElementUniqueID($n530);
            if (!$update2) {
              $conditionNodeIndex[uniqueId2] = -1;
            }
            let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
            if (item.item_type === 0) {
              __UpdateIfNodeIndex($n530, 0);
              $conditionNodeIndex[uniqueId2] = 0;
              let _$lepusPushOwner19 = $lepusPushOwner(uniqueId2 + "_0"),
                  owner2 = _$lepusPushOwner19[0],
                  lastOwner2 = _$lepusPushOwner19[1];
              let $temp3 = $update2;
              if ($ifNodeIndex !== 0) {
                $update2 = false;
                $deletedOwnerIds.push(uniqueId2 + "_1");
              }
              {
                let $n531 = $update2 ? $lepusGetElementRefByLepusID("component", 531) : null;
                let $compCreated = true;
                let $childLepusComponent = null;
                let $componentId = null;
                if (!$n531) {
                  $compCreated = false;
                  $n531 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 25, "", "ecom-text", "components/text/index", {});
                  let $nid531 = $lepusStoreElementRefByLepusID($n531, 531, "ecom-text");
                  $componentId = $nid531[0];
                  $childLepusComponent = $componentConstructor($componentId, $n531, "components/text/index", 531);
                  $createdIds.push($componentId + "");
                  $cardInstance._currentOwner.componentIds.push($componentId);
                  __AppendElement($n530, $n531);
                } else {
                  $componentId = __GetElementUniqueID($n531);
                  $childLepusComponent = $componentInfo[$componentId];
                }
                $comUpdatePropsSet = [];
                $childLepusComponent._setProp("data", item.text, $update2);
                if ($compCreated) {
                  let $update_keys = $comUpdatePropsSet;
                  let $childSlotUpdate = false;
                  if ($update_keys.length > 0 || $childSlotUpdate) {
                    $updatedIds.push($componentId + "");
                    $renderComponents["components/text/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
                  }
                } else {
                  $renderComponents["components/text/index"].entry($n531, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
                }
              }
              $update2 = $temp3;
              $lepusPopOwner(lastOwner2);
            } else {
              __UpdateIfNodeIndex($n530, 1);
              $conditionNodeIndex[uniqueId2] = 1;
              let _$lepusPushOwner20 = $lepusPushOwner(uniqueId2 + "_1"),
                  _owner2 = _$lepusPushOwner20[0],
                  _lastOwner2 = _$lepusPushOwner20[1];
              let _$temp48 = $update2;
              if ($ifNodeIndex !== 1) {
                $update2 = false;
                $deletedOwnerIds.push(uniqueId2 + "_0");
              }
              {
                let $n532 = $update2 ? $lepusGetElementRefByLepusID("if", 532) : null;
                if (!$n532) {
                  $update2 = false;
                  $n532 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n532, 532, "if");
                  __AppendElement($n530, $n532);
                }
                let uniqueId3 = __GetElementUniqueID($n532);
                if (!$update2) {
                  $conditionNodeIndex[uniqueId3] = -1;
                }
                let $ifNodeIndex2 = $conditionNodeIndex[uniqueId3];
                if (item.item_type === 1) {
                  __UpdateIfNodeIndex($n532, 2);
                  $conditionNodeIndex[uniqueId3] = 2;
                  let _$lepusPushOwner21 = $lepusPushOwner(uniqueId3 + "_2"),
                      owner3 = _$lepusPushOwner21[0],
                      lastOwner3 = _$lepusPushOwner21[1];
                  let $temp4 = $update2;
                  if ($ifNodeIndex2 !== 2) {
                    $update2 = false;
                    $deletedOwnerIds.push(uniqueId3 + "_3");
                  }
                  {
                    let $n533 = $update2 ? $lepusGetElementRefByLepusID("component", 533) : null;
                    let _$compCreated = true;
                    let _$childLepusComponent = null;
                    let _$componentId = null;
                    if (!$n533) {
                      _$compCreated = false;
                      $n533 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 23, "", "ecom-image", "components/image/index", {});
                      let $nid533 = $lepusStoreElementRefByLepusID($n533, 533, "ecom-image");
                      _$componentId = $nid533[0];
                      _$childLepusComponent = $componentConstructor(_$componentId, $n533, "components/image/index", 533);
                      $createdIds.push(_$componentId + "");
                      $cardInstance._currentOwner.componentIds.push(_$componentId);
                      __AppendElement($n532, $n533);
                    } else {
                      _$componentId = __GetElementUniqueID($n533);
                      _$childLepusComponent = $componentInfo[_$componentId];
                    }
                    $comUpdatePropsSet = [];
                    _$childLepusComponent._setProp("data", item.image, $update2);
                    if (_$compCreated) {
                      let _$update_keys = $comUpdatePropsSet;
                      let _$childSlotUpdate = false;
                      if (_$update_keys.length > 0 || _$childSlotUpdate) {
                        $updatedIds.push(_$componentId + "");
                        $renderComponents["components/image/index"].update(_$childLepusComponent, _$childLepusComponent.data, _$childLepusComponent.properties, _$update_keys, false, true, _$childSlotUpdate);
                      }
                    } else {
                      $renderComponents["components/image/index"].entry($n533, _$childLepusComponent.data, _$childLepusComponent.properties, _$childLepusComponent, false);
                    }
                  }
                  $update2 = $temp4;
                  $lepusPopOwner(lastOwner3);
                } else {
                  __UpdateIfNodeIndex($n532, 3);
                  $conditionNodeIndex[uniqueId3] = 3;
                  let _$lepusPushOwner22 = $lepusPushOwner(uniqueId3 + "_3"),
                      _owner3 = _$lepusPushOwner22[0],
                      _lastOwner3 = _$lepusPushOwner22[1];
                  let _$temp49 = $update2;
                  if ($ifNodeIndex2 !== 3) {
                    $update2 = false;
                    $deletedOwnerIds.push(uniqueId3 + "_2");
                  }
                  {
                    let $n534 = $update2 ? $lepusGetElementRefByLepusID("if", 534) : null;
                    if (!$n534) {
                      $update2 = false;
                      $n534 = __CreateIf($currentComponentId);
                      $lepusStoreElementRefByLepusID($n534, 534, "if");
                      __AppendElement($n532, $n534);
                    }
                    let uniqueId4 = __GetElementUniqueID($n534);
                    if (!$update2) {
                      $conditionNodeIndex[uniqueId4] = -1;
                    }
                    let $ifNodeIndex3 = $conditionNodeIndex[uniqueId4];
                    if (item.item_type === 2) {
                      __UpdateIfNodeIndex($n534, 4);
                      $conditionNodeIndex[uniqueId4] = 4;
                      let _$lepusPushOwner23 = $lepusPushOwner(uniqueId4 + "_4"),
                          owner4 = _$lepusPushOwner23[0],
                          lastOwner4 = _$lepusPushOwner23[1];
                      let $temp5 = $update2;
                      if ($ifNodeIndex3 !== 4) {
                        $update2 = false;
                      }
                      {
                        let $n535 = $update2 ? $lepusGetElementRefByLepusID("component", 535) : null;
                        let _$compCreated2 = true;
                        let _$childLepusComponent2 = null;
                        let _$componentId2 = null;
                        if (!$n535) {
                          _$compCreated2 = false;
                          $n535 = __CreateComponent($lepusComponent._uniqueId, $cardInstance.fiberComponentId + "", 24, "", "ecom-splitor", "components/splitor/index", {});
                          let $nid535 = $lepusStoreElementRefByLepusID($n535, 535, "ecom-splitor");
                          _$componentId2 = $nid535[0];
                          _$childLepusComponent2 = $componentConstructor(_$componentId2, $n535, "components/splitor/index", 535);
                          $createdIds.push(_$componentId2 + "");
                          $cardInstance._currentOwner.componentIds.push(_$componentId2);
                          __AppendElement($n534, $n535);
                        } else {
                          _$componentId2 = __GetElementUniqueID($n535);
                          _$childLepusComponent2 = $componentInfo[_$componentId2];
                        }
                        $comUpdatePropsSet = [];
                        _$childLepusComponent2._setProp("data", item.splitor, $update2);
                        if (_$compCreated2) {
                          let _$update_keys2 = $comUpdatePropsSet;
                          let _$childSlotUpdate2 = false;
                          if (_$update_keys2.length > 0 || _$childSlotUpdate2) {
                            $updatedIds.push(_$componentId2 + "");
                            $renderComponents["components/splitor/index"].update(_$childLepusComponent2, _$childLepusComponent2.data, _$childLepusComponent2.properties, _$update_keys2, false, true, _$childSlotUpdate2);
                          }
                        } else {
                          $renderComponents["components/splitor/index"].entry($n535, _$childLepusComponent2.data, _$childLepusComponent2.properties, _$childLepusComponent2, false);
                        }
                      }
                      $update2 = $temp5;
                      $lepusPopOwner(lastOwner4);
                    } else {
                      __UpdateIfNodeIndex($n534, 5);
                      $conditionNodeIndex[uniqueId4] = 5;
                      let _$temp50 = $update2;
                      if ($ifNodeIndex3 !== 5) {
                        $update2 = false;
                        $deletedOwnerIds.push(uniqueId4 + "_4");
                      }
                      {
                        let $n536 = $update2 ? $lepusGetElementRefByLepusID("view", 536) : null;
                        let $temp6 = $update2;
                        if (!$n536) {
                          $update2 = false;
                          $n536 = __CreateView($currentComponentId);
                          $lepusStoreElementRefByLepusID($n536, 536, "view");
                          __AppendElement($n534, $n536);
                        }
                        $update2 = $temp6;
                      }
                      $update2 = _$temp50;
                    }
                  }
                  $update2 = _$temp49;
                  $lepusPopOwner(_lastOwner3);
                }
              }
              $update2 = _$temp48;
              $lepusPopOwner(_lastOwner2);
            }
          }
          $update2 = $temp2;
        }
        $lepusPopOwner(lastOwner);
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/tag/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1]) {
      let $n527 = $lepusGetElementRefByLepusID("view", 527);
      {
        __SetStyleObject($n527, [getClassStyleIndex("container", "19453000"), {
          7: $data.newData.background.bg_color + ""
        }, {
          26: $data.newData.background.height + ""
        }, {
          60: $data.newData.background.border_color + ""
        }, {
          17: $data.newData.background.border_width + ""
        }, {
          12: $data.newData.background.radius + ""
        }, parseStyleStringToObject($data.backgroundStyle)]);
      }
    }
    let $n528 = $lepusGetElementRefByLepusID("for", 528);
    $renderComponents[$path].update_274a980_528($lepusComponent, $n528, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n527 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n527, 527, "view");
      __SetStyleObject($n527, [getClassStyleIndex("container", "19453000"), {
        7: $data.newData.background.bg_color + ""
      }, {
        26: $data.newData.background.height + ""
      }, {
        60: $data.newData.background.border_color + ""
      }, {
        17: $data.newData.background.border_width + ""
      }, {
        12: $data.newData.background.radius + ""
      }, parseStyleStringToObject($data.backgroundStyle)]);
      __SetAttribute($n527, "clip-radius", true);
      __AppendElement($component, $n527);
      let $n528 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n528, 528, "for");
      __AppendElement($n527, $n528);
      $renderComponents["components/tag/index"].update_274a980_528($lepusComponent, $n528, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/text/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b;
    let $path = "components/text/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n537 = $lepusGetElementRefByLepusID("view", 537);
      {
        let $value = "display:flex;align-items:center;background-color:" + $data.data.text_bg_color + ";height:" + $data.data.height + ";border-color:" + $data.data.text_border_color.join(" ") + ";border-width:" + $data.data.border_width.join(" ") + ";border-style:solid;padding:" + $data.data.padding.join(" ") + ";border-radius:" + $data.data.radius.join(" ") + ";font-family:" + $data.data.font_family + ";font-style:" + $data.data.font_style + ";";
        if (!$update2 || $value !== undefined) {
          __SetStyleObject($n537, [0, 61, {
            7: $data.data.text_bg_color + ""
          }, {
            26: $data.data.height + ""
          }, {
            60: $data.data.text_border_color.join(" ") + ""
          }, {
            17: $data.data.border_width.join(" ") + ""
          }, 308, {
            32: $data.data.padding.join(" ") + ""
          }, {
            12: $data.data.radius.join(" ") + ""
          }, {
            61: $data.data.font_family + ""
          }, {
            62: $data.data.font_style + ""
          }]);
        }
      }
    }
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n538 = $lepusGetElementRefByLepusID("text", 538);
      {
        let _$value43 = "color:" + $data.data.text_color + ";font-weight:" + $data.data.weight + ";font-size:" + $data.data.font_size + ";";
        if (!$update2 || _$value43 !== "color:" + $lepusComponent._data.data.text_color + ";font-weight:" + $lepusComponent._data.data.weight + ";font-size:" + $lepusComponent._data.data.font_size + ";") {
          __SetStyleObject($n538, [{
            22: $data.data.text_color + ""
          }, {
            48: $data.data.weight + ""
          }, {
            47: $data.data.font_size + ""
          }]);
        }
      }
    }
    if ($renderComponents[$path].varUpdateState[0]) {
      let _$value44 = (_a = $data.data) == null ? undefined : _a.text;
      if (_$value44 !== ((_b = $lepusComponent._data.data) == null ? undefined : _b.text)) {
        let _$n4 = $lepusGetElementRefByLepusID("text", 538);
        __SetAttribute(_$n4, "text", _$value44);
      }
    }
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent) {
    let _a;
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n537 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n537, 537, "view");
      __SetStyleObject($n537, [0, 61, {
        7: $data.data.text_bg_color + ""
      }, {
        26: $data.data.height + ""
      }, {
        60: $data.data.text_border_color.join(" ") + ""
      }, {
        17: $data.data.border_width.join(" ") + ""
      }, 308, {
        32: $data.data.padding.join(" ") + ""
      }, {
        12: $data.data.radius.join(" ") + ""
      }, {
        61: $data.data.font_family + ""
      }, {
        62: $data.data.font_style + ""
      }]);
      __SetAttribute($n537, "clip-radius", true);
      __AppendElement($component, $n537);
      let $n538 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n538, 538, "text");
      __SetStyleObject($n538, [{
        22: $data.data.text_color + ""
      }, {
        48: $data.data.weight + ""
      }, {
        47: $data.data.font_size + ""
      }]);
      __AppendElement($n537, $n538);
      __SetAttribute($n538, "text", (_a = $data.data) == null ? undefined : _a.text);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
function __CreateRawText(a) {}
$renderComponents["components/title_common/index"] = {
  variables: ["data"],
  varUpdateState: [],
  update_22898d8_541: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b;
    if (!$update2 || $renderComponents["components/title_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo28 = $lepusPushFiberForNode($parent, 541, uniqueId),
          $forLepus = _$lepusPushFiberForNo28[0],
          $lastForLepus = _$lepusPushFiberForNo28[1];
      let $object = (_a = $data.data) == null ? undefined : _a.tag_list;
      let $length = _GetLength($object);
      let $oldObject = (_b = $lepusComponent._data.data) == null ? undefined : _b.tag_list;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n542 = $update2 ? $lepusGetElementRefByLepusID("if", 542) : null;
          if (!$n542) {
            $n542 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n542, 542, "if");
            __AppendElement($parent, $n542);
          }
          let uniqueId2 = __GetElementUniqueID($n542);
          if (!$update2) {
            $conditionNodeIndex[uniqueId2] = -1;
          }
          let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
          if (item == null ? undefined : item.url) {
            __UpdateIfNodeIndex($n542, 0);
            $conditionNodeIndex[uniqueId2] = 0;
            let $temp2 = $update2;
            if ($ifNodeIndex !== 0) {
              $update2 = false;
            }
            {
              let $n543 = $update2 ? $lepusGetElementRefByLepusID("image", 543) : null;
              let $temp3 = $update2;
              if (!$n543) {
                $update2 = false;
                $n543 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n543, 543, "image");
                __SetAttribute($n543, "skip-redirection", true);
                __SetAttribute($n543, "accessibility-element", false);
                __AppendElement($n542, $n543);
              }
              __SetStyleObject($n543, [getClassStyleIndex("gyl-title__tag " + ((isAndroid7() ? "gyl-title__tag--android" : "") + (" " + (isH53() ? "gyl-title__tag--h5" : ""))), "9017000"), parseStyleStringToObject(item == null ? undefined : item.style)]);
              __SetAttribute($n543, "src", item == null ? undefined : item.url);
              $update2 = $temp3;
            }
            $update2 = $temp2;
          } else {
            __UpdateIfNodeIndex($n542, -1);
            $conditionNodeIndex[uniqueId2] = -1;
          }
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_22898d8_544: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j;
    let $path = "components/title_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_b = (_a = $data.data) == null ? undefined : _a.tag) == null ? undefined : _b.url) && !((_d = (_c = $data.data) == null ? undefined : _c.tag_list) == null ? undefined : _d.length)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n545 = $update2 ? $lepusGetElementRefByLepusID("image", 545) : null;
            let $temp2 = $update2;
            if (!$n545) {
              $update2 = false;
              $n545 = __CreateImage($currentComponentId);
              $lepusStoreElementRefByLepusID($n545, 545, "image");
              __SetAttribute($n545, "skip-redirection", true);
              __SetAttribute($n545, "accessibility-element", false);
              __AppendElement($parent, $n545);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n545, [getClassStyleIndex("gyl-title__tag " + ((isAndroid7() ? "gyl-title__tag--android" : "") + (" " + (isH53() ? "gyl-title__tag--h5" : ""))), "9017000"), parseStyleStringToObject((_f = (_e = $data.data) == null ? undefined : _e.tag) == null ? undefined : _f.style)]);
              }
              {
                let $value = (_h = (_g = $data.data) == null ? undefined : _g.tag) == null ? undefined : _h.url;
                if (!$update2 || $value !== ((_j = (_i = $lepusComponent._data.data) == null ? undefined : _i.tag) == null ? undefined : _j.url)) {
                  __SetAttribute($n545, "src", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_22898d8_547: function ($lepusComponent, $parent, $data, $props, $update2) {
    let _a, _b, _c, _d, _e, _f;
    if (!$update2 || $renderComponents["components/title_common/index"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo29 = $lepusPushFiberForNode($parent, 547, uniqueId),
          $forLepus = _$lepusPushFiberForNo29[0],
          $lastForLepus = _$lepusPushFiberForNo29[1];
      let $object = getTitleSegments((_b = (_a = $data.data) == null ? undefined : _a.content) == null ? undefined : _b.content);
      let $length = _GetLength($object);
      let $oldObject = undefined;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let titleSegment = $object[index];
        $oldObject ? $oldObject[index] : null;
        {
          let $n548 = $update2 ? $lepusGetElementRefByLepusID("text", 548) : null;
          let $temp2 = $update2;
          if (!$n548) {
            $update2 = false;
            $n548 = __CreateText($currentComponentId);
            $lepusStoreElementRefByLepusID($n548, 548, "text");
            __AppendElement($parent, $n548);
          }
          __SetStyleObject($n548, [getClassStyleIndex(getTitleTextSegClass("gyl-title__seg", titleSegment), "9017000"), parseStyleStringToObject(isNarrowChar(titleSegment) ? (_d = (_c = $data.data) == null ? undefined : _c.narrow_char) == null ? undefined : _d.style : (_f = (_e = $data.data) == null ? undefined : _e.content) == null ? undefined : _f.style)]);
          __SetAttribute($n548, "text", titleSegment);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_2c5b78_546: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "components/title_common/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ((_b = (_a = $data.data) == null ? undefined : _a.narrow_char) == null ? undefined : _b.style) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
            let $n547 = $update2 ? $lepusGetElementRefByLepusID("for", 547) : null;
            if (!$n547) {
              $n547 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n547, 547, "for");
              __AppendElement($parent, $n547);
            }
            $renderComponents[$path].update_22898d8_547($lepusComponent, $n547, $data, $props, $update2, $slotUpdate);
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp51 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n550 = $update2 ? $lepusGetElementRefByLepusID("raw-text", 550) : null;
            if (!$n550) {
              $n550 = __CreateRawText((_d = (_c = $data.data) == null ? undefined : _c.content) == null ? undefined : _d.content);
              $lepusStoreElementRefByLepusID($n550, 550, "raw-text");
              __AppendElement($parent, $n550);
            } else {
              if ($renderComponents[$path].varUpdateState[0]) {
                let $value = (_f = (_e = $data.data) == null ? undefined : _e.content) == null ? undefined : _f.content;
                if (!$update2 || $value !== ((_h = (_g = $lepusComponent._data.data) == null ? undefined : _g.content) == null ? undefined : _h.content)) {
                  __SetAttribute($n550, "text", $value);
                }
              }
            }
          }
          $update2 = _$temp51;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a, _b, _c, _d;
    let $path = "components/title_common/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0]) {
      let $n540 = $lepusGetElementRefByLepusID("text", 540);
      {
        __SetStyleObject($n540, [getClassStyleIndex("gyl-title", "9017000"), parseStyleStringToObject((_b = (_a = $data.data) == null ? undefined : _a.content) == null ? undefined : _b.style)]);
      }
      {
        let $value = (_c = $data.data) == null ? undefined : _c.max_line;
        if (!$update2 || $value !== ((_d = $lepusComponent._data.data) == null ? undefined : _d.max_line)) {
          __SetAttribute($n540, "text-maxline", $value);
        }
      }
    }
    let $n541 = $lepusGetElementRefByLepusID("for", 541);
    $renderComponents[$path].update_22898d8_541($lepusComponent, $n541, $data, $props, $update2, $slotUpdate);
    let $n544 = $lepusGetElementRefByLepusID("if", 544);
    $renderComponents[$path].update_22898d8_544($lepusComponent, $n544, $data, $props, $update2, $slotUpdate);
    let $n546 = $lepusGetElementRefByLepusID("if", 546);
    $renderComponents[$path].update_2c5b78_546($lepusComponent, $n546, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a, _b, _c;
    let $path = "components/title_common/index";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n540 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n540, 540, "text");
      __SetStyleObject($n540, [getClassStyleIndex("gyl-title", "9017000"), parseStyleStringToObject((_b = (_a = $data.data) == null ? undefined : _a.content) == null ? undefined : _b.style)]);
      __SetAttribute($n540, "accessibility-element", false);
      __SetAttribute($n540, "text-maxline", (_c = $data.data) == null ? undefined : _c.max_line);
      __AppendElement($component, $n540);
      let $n541 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n541, 541, "for");
      __AppendElement($n540, $n541);
      $renderComponents[$path].update_22898d8_541($lepusComponent, $n541, $data, $props, $update2, $slotUpdate);
      let $n544 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n544, 544, "if");
      __AppendElement($n540, $n544);
      $renderComponents[$path].update_22898d8_544($lepusComponent, $n544, $data, $props, $update2, $slotUpdate);
      let $n546 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n546, 546, "if");
      __AppendElement($n540, $n546);
      $renderComponents[$path].update_2c5b78_546($lepusComponent, $n546, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/video_engine/index"] = {
  variables: ["videoEngineStatus", "tag", "subTag", "src", "id"],
  varUpdateState: [],
  update_1706880_551: function ($lepusComponent, $parent, $data, $props, $update2) {
    let $path = "components/video_engine/index";
    if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3] || $renderComponents[$path].varUpdateState[4]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (canUseVideoEngine()) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n552 = $update2 ? $lepusGetElementRefByLepusID("x-video-engine", 552) : null;
            let $temp2 = $update2;
            if (!$n552) {
              $update2 = false;
              $n552 = __CreateElement("x-video-engine", $currentComponentId);
              $lepusStoreElementRefByLepusID($n552, 552, "x-video-engine");
              __SetAttribute($n552, "muted", true);
              __SetAttribute($n552, "object-fit", "cover");
              __SetAttribute($n552, "auto-prepare", true);
              __SetAttribute($n552, "skip-redirection", true);
              __AddEvent($n552, "bindEvent", "redirect", "onVideoRedirect");
              __AddEvent($n552, "bindEvent", "playbackstatechanged", "onVideoPlaybackStateChanged");
              __AddEvent($n552, "bindEvent", "completion", "onVideoCompletion");
              __AddEvent($n552, "bindEvent", "canplay", "onVideoCanPlay");
              __AddEvent($n552, "bindEvent", "error", "onVideoError");
              __AddEvent($n552, "bindEvent", "firstframe", "onVideoFirstFrame");
              __AddEvent($n552, "bindEvent", "timeupdate", "onVideoTimeUpdate");
              __AppendElement($parent, $n552);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[3] || $renderComponents[$path].varUpdateState[4]) {
              {
                __SetStyleObject($n552, [getClassStyleIndex("gyl-video-engine", "50448000"), parseStyleStringToObject(getOpacity($data.videoEngineStatus))]);
              }
              {
                let $value = $data.tag;
                if (!$update2 || $value !== $lepusComponent._data.tag) {
                  __SetAttribute($n552, "tag", $value);
                }
              }
              {
                let _$value45 = $data.subTag;
                if (!$update2 || _$value45 !== $lepusComponent._data.subTag) {
                  __SetAttribute($n552, "sub-tag", _$value45);
                }
              }
              {
                let _$value46 = $data.src;
                if (!$update2 || _$value46 !== $lepusComponent._data.src) {
                  __SetAttribute($n552, "play-url", _$value46);
                }
              }
              {
                let _$value47 = $data.id;
                if (!$update2 || _$value47 !== $lepusComponent._data.id) {
                  __SetID($n552, _$value47);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let $path = "components/video_engine/index";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    let $n551 = $lepusGetElementRefByLepusID("if", 551);
    $renderComponents[$path].update_1706880_551($lepusComponent, $n551, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    {
      let $n551 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n551, 551, "if");
      __AppendElement($component, $n551);
      $renderComponents["components/video_engine/index"].update_1706880_551($lepusComponent, $n551, $data, $props, $update2, false);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$renderComponents["components/wind_vane/wind_vane"] = {
  variables: ["abValues", "animationStyle", "data"],
  varUpdateState: [],
  update_22898d8_561: function ($lepusComponent, $parent, $data, $props, $update2) {
    let $path = "components/wind_vane/wind_vane";
    if (!$update2 || $renderComponents[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (hasMarketingItemCard($data.data) === true) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n562 = $update2 ? $lepusGetElementRefByLepusID("view", 562) : null;
            let $temp2 = $update2;
            if (!$n562) {
              $update2 = false;
              $n562 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n562, 562, "view");
              __AddEvent($n562, "bindEvent", "tap", "handleMarketingProductClick");
              __AppendElement($parent, $n562);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[2]) {
              {
                __SetStyleObject($n562, [getClassStyleIndex("marketing_item_card", "57778000"), parseStyleStringToObject(getMarketingItemCardStyle($data.data))]);
              }
            }
            {
              let $n563 = $update2 ? $lepusGetElementRefByLepusID("text", 563) : null;
              let $temp3 = $update2;
              if (!$n563) {
                $update2 = false;
                $n563 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n563, 563, "text");
                __SetStyleObject($n563, [getClassStyleIndex("marketing_item_card-price", "57778000"), 385, 386]);
                __AppendElement($n562, $n563);
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[2]) {
                  __SetAttribute($n563, "text", getMarketingPrice($data.data));
                }
              }
              $update2 = $temp3;
            }
            {
              let $n565 = $update2 ? $lepusGetElementRefByLepusID("text", 565) : null;
              let _$temp52 = $update2;
              if (!$n565) {
                $update2 = false;
                $n565 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n565, 565, "text");
                __SetStyleObject($n565, [getClassStyleIndex("marketing_item_card-price_unit", "57778000"), 387, 388, 386]);
                __AppendElement($n562, $n565);
              }
              __SetAttribute($n565, "text", "CNY");
              $update2 = _$temp52;
            }
            {
              let $n567 = $update2 ? $lepusGetElementRefByLepusID("text", 567) : null;
              let _$temp53 = $update2;
              if (!$n567) {
                $update2 = false;
                $n567 = __CreateText($currentComponentId);
                $lepusStoreElementRefByLepusID($n567, 567, "text");
                __SetStyleObject($n567, [getClassStyleIndex("marketing_item_card-text", "57778000"), 389, 390, 386]);
                __AppendElement($n562, $n567);
              }
              {
                if (!$update2 || $renderComponents[$path].varUpdateState[2]) {
                  __SetAttribute($n567, "text", getMarketingText($data.data));
                }
              }
              $update2 = _$temp53;
            }
            {
              let $n569 = $update2 ? $lepusGetElementRefByLepusID("view", 569) : null;
              let _$temp54 = $update2;
              if (!$n569) {
                $update2 = false;
                $n569 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n569, 569, "view");
                __SetStyleObject($n569, [getClassStyleIndex("marketing_item_card-button_bg", "57778000"), 391, 392, 386]);
                __AppendElement($n562, $n569);
              }
              {
                let $n570 = $update2 ? $lepusGetElementRefByLepusID("text", 570) : null;
                let $temp4 = $update2;
                if (!$n570) {
                  $update2 = false;
                  $n570 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n570, 570, "text");
                  __SetStyleObject($n570, [getClassStyleIndex("marketing_item_card-button_text", "57778000"), 393]);
                  __AppendElement($n569, $n570);
                }
                {
                  if (!$update2 || $renderComponents[$path].varUpdateState[2]) {
                    __SetAttribute($n570, "text", getMarketingButtonText($data.data));
                  }
                }
                $update2 = $temp4;
              }
              $update2 = _$temp54;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update_1d2e370_578: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate, index) {
    let _a, _b, _c, _d;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.abValues) == null ? undefined : _a.use_design_v2025) == null ? undefined : _b.enable) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n579 = $update2 ? $lepusGetElementRefByLepusID("view", 579) : null;
          let $temp2 = $update2;
          if (!$n579) {
            $update2 = false;
            $n579 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n579, 579, "view");
            __AppendElement($parent, $n579);
          }
          if (!$update2 || $renderComponents["components/wind_vane/wind_vane"].varUpdateState[0]) {
            __SetStyleObject($n579, [getClassStyleIndex(index < 2 && ((_d = (_c = $data.abValues) == null ? undefined : _c.use_design_v2025) == null ? undefined : _d.borderless) ? "item-top-layer-masking-gradient" : "item-top-layer-masking", "57778000"), 394, 395, parseStyleStringToObject(getBorderStyle($data.abValues))]);
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_56e230_573: function ($lepusComponent, $parent, $data, $props, $update2, $slotUpdate) {
    let $path = "components/wind_vane/wind_vane";
    if (!$update2 || $renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo30 = $lepusPushFiberForNode($parent, 573, uniqueId),
          $forLepus = _$lepusPushFiberForNo30[0],
          $lastForLepus = _$lepusPushFiberForNo30[1];
      let $object = getItemList($data.data);
      let $length = _GetLength($object);
      let $oldObject = undefined;
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let i = $length; i < $forLepus._lastLength; ++i) {
        $deletedOwnerIds.push(uniqueId + "_" + i);
      }
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        let old_item = $oldObject ? $oldObject[index] : null;
        {
          let $n574 = $update2 ? $lepusGetElementRefByLepusID("view", 574) : null;
          let $temp2 = $update2;
          if (!$n574) {
            $update2 = false;
            $n574 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n574, 574, "view");
            __SetAttribute($n574, "accessibility-element", "true");
            __AddEvent($n574, "bindEvent", "tap", "handleProductClick");
            __AppendElement($parent, $n574);
          }
          __SetStyleObject($n574, [getClassStyleIndex("prefer-card--products-item", "57778000"), parseStyleStringToObject(getItemStyle($data.abValues, index))]);
          __SetAttribute($n574, "accessibility-label", getKeyword(item) + "button");
          __SetID($n574, getId(item));
          __AddDataset($n574, "idx", index);
          {
            let $n575 = $update2 ? $lepusGetElementRefByLepusID("view", 575) : null;
            let $temp3 = $update2;
            if (!$n575) {
              $update2 = false;
              $n575 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n575, 575, "view");
              __AppendElement($n574, $n575);
            }
            if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
              {
                __SetStyleObject($n575, [getClassStyleIndex("item-bg", "57778000"), 391, parseStyleStringToObject(getBorderStyle($data.abValues))]);
              }
            }
            {
              let $n576 = $update2 ? $lepusGetElementRefByLepusID("view", 576) : null;
              let $temp4 = $update2;
              if (!$n576) {
                $update2 = false;
                $n576 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n576, 576, "view");
                __AppendElement($n575, $n576);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n576, [getClassStyleIndex("item-masking", "57778000"), 394, 395, parseStyleStringToObject(getBorderStyle($data.abValues))]);
                }
              }
              $update2 = $temp4;
            }
            {
              let $n577 = $update2 ? $lepusGetElementRefByLepusID("image", 577) : null;
              let _$temp55 = $update2;
              if (!$n577) {
                $update2 = false;
                $n577 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n577, 577, "image");
                __SetAttribute($n577, "skip-redirection", true);
                __SetAttribute($n577, "mode", "aspectFill");
                __SetAttribute($n577, "accessibility-element", "false");
                __AppendElement($n575, $n577);
              }
              {
                __SetStyleObject($n577, [getClassStyleIndex("item-image", "57778000"), 394, 395, parseStyleStringToObject(getBorderStyle($data.abValues))]);
              }
              __SetAttribute($n577, "src", getCoverUrl(item));
              $update2 = _$temp55;
            }
            let $n578 = $update2 ? $lepusGetElementRefByLepusID("if", 578) : null;
            if (!$n578) {
              $n578 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n578, 578, "if");
              __AppendElement($n575, $n578);
            }
            $renderComponents[$path].update_1d2e370_578($lepusComponent, $n578, $data, $props, $update2, $slotUpdate, index, item, old_item);
            $update2 = $temp3;
          }
          {
            let $n580 = $update2 ? $lepusGetElementRefByLepusID("view", 580) : null;
            let _$temp56 = $update2;
            if (!$n580) {
              $update2 = false;
              $n580 = __CreateView($currentComponentId);
              $lepusStoreElementRefByLepusID($n580, 580, "view");
              __AppendElement($n574, $n580);
            }
            {
              let $value = getItemTitleClassName("item-title", $data.abValues);
              if (!$update2 || $value !== undefined) {
                __SetStyleObject($n580, [getClassStyleIndex(getItemTitleClassName("item-title", $data.abValues), "57778000"), 391]);
              }
            }
            {
              let $n581 = $update2 ? $lepusGetElementRefByLepusID("if", 581) : null;
              if (!$n581) {
                $n581 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n581, 581, "if");
                __AppendElement($n580, $n581);
              }
              let uniqueId2 = __GetElementUniqueID($n581);
              if (!$update2) {
                $conditionNodeIndex[uniqueId2] = -1;
              }
              let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
              if (getTagBgUrl(item) !== "") {
                __UpdateIfNodeIndex($n581, 0);
                $conditionNodeIndex[uniqueId2] = 0;
                let _$temp57 = $update2;
                if ($ifNodeIndex !== 0) {
                  $update2 = false;
                }
                {
                  let $n582 = $update2 ? $lepusGetElementRefByLepusID("image", 582) : null;
                  let $temp5 = $update2;
                  if (!$n582) {
                    $update2 = false;
                    $n582 = __CreateImage($currentComponentId);
                    $lepusStoreElementRefByLepusID($n582, 582, "image");
                    __SetStyleObject($n582, [getClassStyleIndex("item-category-logo", "57778000"), 385, 386]);
                    __SetAttribute($n582, "skip-redirection", true);
                    __SetAttribute($n582, "accessibility-element", "false");
                    __AppendElement($n581, $n582);
                  }
                  __SetAttribute($n582, "src", getTagBgUrl(item));
                  $update2 = $temp5;
                }
                $update2 = _$temp57;
              } else {
                __UpdateIfNodeIndex($n581, -1);
                $conditionNodeIndex[uniqueId2] = -1;
              }
            }
            {
              let $n583 = $update2 ? $lepusGetElementRefByLepusID("view", 583) : null;
              let _$temp58 = $update2;
              if (!$n583) {
                $update2 = false;
                $n583 = __CreateView($currentComponentId);
                $lepusStoreElementRefByLepusID($n583, 583, "view");
                __AppendElement($n580, $n583);
              }
              __SetStyleObject($n583, [getClassStyleIndex("item-category", "57778000"), 387, 388, 386, parseStyleStringToObject(getTextMaxWidth(item))]);
              {
                let $n584 = $update2 ? $lepusGetElementRefByLepusID("text", 584) : null;
                let _$temp59 = $update2;
                if (!$n584) {
                  $update2 = false;
                  $n584 = __CreateText($currentComponentId);
                  $lepusStoreElementRefByLepusID($n584, 584, "text");
                  __SetAttribute($n584, "accessibility-element", "false");
                  __SetAttribute($n584, "text-maxline", "2");
                  __AppendElement($n583, $n584);
                }
                __SetStyleObject($n584, [getClassStyleIndex(getCategoryNameUI($data.abValues), "57778000"), parseStyleStringToObject(getTextMaxWidth(item))]);
                __SetAttribute($n584, "text", getLimitLenKeyword(item));
                $update2 = _$temp59;
              }
              $update2 = _$temp58;
            }
            {
              let $n586 = $update2 ? $lepusGetElementRefByLepusID("image", 586) : null;
              let _$temp60 = $update2;
              if (!$n586) {
                $update2 = false;
                $n586 = __CreateImage($currentComponentId);
                $lepusStoreElementRefByLepusID($n586, 586, "image");
                __SetStyleObject($n586, [getClassStyleIndex("item-category-icon", "57778000"), 390, 386]);
                __SetAttribute($n586, "skip-redirection", true);
                __SetAttribute($n586, "accessibility-element", "false");
                __AppendElement($n580, $n586);
              }
              if (!$update2 || $renderComponents[$path].varUpdateState[0]) {
                {
                  let _$value48 = getCategoryIconUrl($data.abValues);
                  if (!$update2 || _$value48 !== undefined) {
                    __SetAttribute($n586, "src", _$value48);
                  }
                }
              }
              $update2 = _$temp60;
            }
            $update2 = _$temp56;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusComponent, $data, $props, $array, $fromSetData, $update2, $slotUpdate) {
    let _a;
    let $path = "components/wind_vane/wind_vane";
    $slotUpdate = $slotUpdate || false;
    let $lastComponent = $componentUpdate($lepusComponent, $path, $array, $fromSetData);
    if ($renderComponents[$path].varUpdateState[0] || $renderComponents[$path].varUpdateState[1] || $renderComponents[$path].varUpdateState[2]) {
      let $n553 = $lepusGetElementRefByLepusID("view", 553);
      {
        __SetStyleObject($n553, [getClassStyleIndex(getContainerClassName($data.abValues), "57778000"), parseStyleStringToObject($data.animationStyle), parseStyleStringToObject(getPreferCardBg($data.data)), parseStyleStringToObject(((_a = $data.abValues) == null ? undefined : _a.use_new_grid) ? getWindVaneCardHeight($data.abValues) : getCardHeight($data.abValues))]);
      }
    }
    if ($renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[0]) {
      let $n554 = $lepusGetElementRefByLepusID("view", 554);
      {
        __SetStyleObject($n554, [getClassStyleIndex("prefer-card--title", "57778000"), parseStyleStringToObject(getPreferCardTitleStyle($data.data, $data.abValues))]);
      }
    }
    if ($renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[0]) {
      let $n555 = $lepusGetElementRefByLepusID("image", 555);
      {
        let $value = getPreferCardTitleIcon($data.data, $data.abValues);
        if (!$update2 || $value !== undefined) {
          __SetAttribute($n555, "src", $value);
        }
      }
    }
    if ($renderComponents[$path].varUpdateState[2] || $renderComponents[$path].varUpdateState[0]) {
      let $n556 = $lepusGetElementRefByLepusID("text", 556);
      {
        __SetStyleObject($n556, [getClassStyleIndex("prefer-card--title-text", "57778000"), parseStyleStringToObject(getPreferCardTitleTextStyle($data.data, $data.abValues))]);
      }
    }
    if ($renderComponents[$path].varUpdateState[2]) {
      let _$value49 = getTitle($data.data);
      if (_$value49 !== undefined) {
        let _$n5 = $lepusGetElementRefByLepusID("text", 556);
        __SetAttribute(_$n5, "text", _$value49);
      }
    }
    $lepusGetElementRefByLepusID("text", 558);
    if ($renderComponents[$path].varUpdateState[2]) {
      let _$value50 = getPreferCardMoreIconText($data.data);
      if (_$value50 !== undefined) {
        let $n5582 = $lepusGetElementRefByLepusID("text", 558);
        __SetAttribute($n5582, "text", _$value50);
      }
    }
    let $n561 = $lepusGetElementRefByLepusID("if", 561);
    $renderComponents[$path].update_22898d8_561($lepusComponent, $n561, $data, $props, $update2, $slotUpdate);
    $lepusGetElementRefByLepusID("view", 572);
    let $n573 = $lepusGetElementRefByLepusID("for", 573);
    $renderComponents[$path].update_56e230_573($lepusComponent, $n573, $data, $props, $update2, $slotUpdate);
    $lepusPushFiberComponentNode($lastComponent);
  },
  entry: function ($component, $data, $props, $lepusComponent, $update2) {
    let _a;
    let $path = "components/wind_vane/wind_vane";
    let $lastComponent = $lepusPushFiberComponentNode($lepusComponent);
    let $slotUpdate = false;
    {
      let $n553 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n553, 553, "view");
      __SetStyleObject($n553, [getClassStyleIndex(getContainerClassName($data.abValues), "57778000"), parseStyleStringToObject($data.animationStyle), parseStyleStringToObject(getPreferCardBg($data.data)), parseStyleStringToObject(((_a = $data.abValues) == null ? undefined : _a.use_new_grid) ? getWindVaneCardHeight($data.abValues) : getCardHeight($data.abValues))]);
      __AddEvent($n553, "bindEvent", "animationend", "onAnimationEnd");
      __AppendElement($component, $n553);
      let $n554 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n554, 554, "view");
      __SetStyleObject($n554, [getClassStyleIndex("prefer-card--title", "57778000"), parseStyleStringToObject(getPreferCardTitleStyle($data.data, $data.abValues))]);
      __AppendElement($n553, $n554);
      let $n555 = __CreateImage($currentComponentId);
      $lepusStoreElementRefByLepusID($n555, 555, "image");
      __SetStyleObject($n555, [getClassStyleIndex("prefer-card--title-icon", "57778000")]);
      __SetAttribute($n555, "skip-redirection", true);
      __SetAttribute($n555, "accessibility-element", "false");
      __SetAttribute($n555, "src", getPreferCardTitleIcon($data.data, $data.abValues));
      __AppendElement($n554, $n555);
      let $n556 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n556, 556, "text");
      __SetStyleObject($n556, [getClassStyleIndex("prefer-card--title-text", "57778000"), parseStyleStringToObject(getPreferCardTitleTextStyle($data.data, $data.abValues))]);
      __AppendElement($n554, $n556);
      __SetAttribute($n556, "text", getTitle($data.data));
      let $n558 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n558, 558, "text");
      __SetStyleObject($n558, [getClassStyleIndex("prefer-card--title-coupon", "57778000"), parseStyleStringToObject(getPreferCardTitleCouponStyle())]);
      __AddEvent($n558, "bindEvent", "tap", "handleTitleClick");
      __AppendElement($n554, $n558);
      __SetAttribute($n558, "text", getPreferCardMoreIconText($data.data));
      let $n560 = __CreateRawText("/>");
      __AppendElement($n554, $n560);
      let $n561 = __CreateIf($currentComponentId);
      $lepusStoreElementRefByLepusID($n561, 561, "if");
      __AppendElement($n553, $n561);
      $renderComponents[$path].update_22898d8_561($lepusComponent, $n561, $data, $props, $update2, $slotUpdate);
      let $n572 = __CreateView($currentComponentId);
      __SetStyleObject($n572, [getClassStyleIndex("prefer-card--products", "57778000")]);
      __AppendElement($n553, $n572);
      let $n573 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n573, 573, "for");
      __AppendElement($n572, $n573);
      $renderComponents[$path].update_56e230_573($lepusComponent, $n573, $data, $props, $update2, $slotUpdate);
    }
    $lepusPushFiberComponentNode($lastComponent);
  }
};
$componentUpdate = function ($lepusComponent, $path, $array) {
  $renderComponents[$path].variables.forEach(function (it, index) {
    $renderComponents[$path].varUpdateState[index] = $array.includes(it);
  });
  let globalIndex = $renderComponents[$path].variables.length;
  if ($array.includes("__globalProps")) {
    $renderComponents[$path].varUpdateState[globalIndex] = true;
  } else {
    $renderComponents[$path].varUpdateState[globalIndex] = false;
  }
  return $lepusPushFiberComponentNode($lepusComponent);
};
function $$update_29e2e68_5($parent, $data, $update2) {
  let _a, _b, _c, _d;
  if (!$update2 || $varUpdateState[3] || $varUpdateState[1] || $varUpdateState[0] || $varUpdateState[4] || $varUpdateState[5] || $varUpdateState[6] || $varUpdateState[7]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.cover_common) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner24 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner24[0],
            lastOwner = _$lepusPushOwner24[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n6 = $update2 ? $lepusGetElementRefByLepusID("component", 6) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n6) {
            $compCreated = false;
            $n6 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 1, "", "cover-common", "components/mall_cover_common/index", {});
            let $nid6 = $lepusStoreElementRefByLepusID($n6, 6, "cover-common");
            $componentId = $nid6[0];
            $childLepusComponent = $componentConstructor($componentId, $n6, "components/mall_cover_common/index", 6);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __SetStyleObject($n6, [149]);
            __AddDataset($n6, "type", "cover");
            __AddEvent($n6, "bindEvent", "OtherClick", "handleProductClick");
            __AddEvent($n6, "bindEvent", "FeedBackClick", "handleLongPress");
            __AppendElement($parent, $n6);
          } else {
            $componentId = __GetElementUniqueID($n6);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("isCacheData", $data.isCacheData, $update2 && $compCreated);
          $childLepusComponent._setProp("info", (_b = $data.data) == null ? undefined : _b.cover_common, $update2 && $compCreated);
          $childLepusComponent._setProp("dynamicInfo", getDynamicInfo($data.extra, $data.coverDynamicInfo), $update2 && $compCreated);
          $childLepusComponent._setProp("logData", ((_c = $data.log_meta_set) == null ? undefined : _c.meta_list) || $data.log_data, $update2 && $compCreated);
          $childLepusComponent._setProp("imageMonitorTag", consturctSceneTag2((_d = $data.extra) == null ? undefined : _d.image_scene_tag_suffix, "product"), $update2 && $compCreated);
          $childLepusComponent._setProp("ecLynxPropsExtra", $data.ec_lynx_props_extra, $update2 && $compCreated);
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/mall_cover_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/mall_cover_common/index"].entry($n6, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_2a4a2c0_7($parent, $data, $update2) {
  let _a, _b, _c;
  if (!$update2 || $varUpdateState[3] || $varUpdateState[9] || $varUpdateState[8]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.hideSimilar && !((_a = $data.data) == null ? undefined : _a.suspend_area_common) && ((_c = (_b = $data.find_similar) == null ? undefined : _b.items) == null ? undefined : _c.length) >= 2) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner25 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner25[0],
            lastOwner = _$lepusPushOwner25[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n8 = $update2 ? $lepusGetElementRefByLepusID("component", 8) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n8) {
            $compCreated = false;
            $n8 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 12, "", "find-similar", "components/find_similar/index", {});
            let $nid8 = $lepusStoreElementRefByLepusID($n8, 8, "find-similar");
            $componentId = $nid8[0];
            $childLepusComponent = $componentConstructor($componentId, $n8, "components/find_similar/index", 8);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __SetStyleObject($n8, [getClassStyleIndex("similar", "27472000")]);
            __AddEvent($n8, "catchEvent", "tap", "handleSimilarClick");
            __AppendElement($parent, $n8);
          } else {
            $componentId = __GetElementUniqueID($n8);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("data", $data.find_similar, $update2 && $compCreated);
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/find_similar/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/find_similar/index"].entry($n8, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_22898d8_9($parent, $data, $update2) {
  let _a, _b, _c, _d;
  if (!$update2 || $varUpdateState[3]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.belt_common) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner26 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner26[0],
            lastOwner = _$lepusPushOwner26[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n10 = $update2 ? $lepusGetElementRefByLepusID("component", 10) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n10) {
            $compCreated = false;
            $n10 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 13, "", "belt-common", "components/belt_common/index", {});
            let $nid10 = $lepusStoreElementRefByLepusID($n10, 10, "belt-common");
            $componentId = $nid10[0];
            $childLepusComponent = $componentConstructor($componentId, $n10, "components/belt_common/index", 10);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __AddEvent($n10, "bindEvent", "tap", "handleProductClick");
            __AppendElement($parent, $n10);
          } else {
            $componentId = __GetElementUniqueID($n10);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("data", (_b = $data.data) == null ? undefined : _b.belt_common, $update2 && $compCreated);
          {
            __SetStyleObject($n10, [getClassStyleIndex("belt_common", "27472000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.belt_common) == null ? undefined : _d.style)]);
          }
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/belt_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/belt_common/index"].entry($n10, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_22898d8_12($parent, $data, $update2) {
  let _a, _b, _c, _d;
  if (!$update2 || $varUpdateState[3]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.sku_pics) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner27 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner27[0],
            lastOwner = _$lepusPushOwner27[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n13 = $update2 ? $lepusGetElementRefByLepusID("component", 13) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n13) {
            $compCreated = false;
            $n13 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 11, "", "sku-pics", "components/sku_pics/index", {});
            let $nid13 = $lepusStoreElementRefByLepusID($n13, 13, "sku-pics");
            $componentId = $nid13[0];
            $childLepusComponent = $componentConstructor($componentId, $n13, "components/sku_pics/index", 13);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __AppendElement($parent, $n13);
          } else {
            $componentId = __GetElementUniqueID($n13);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("data", (_b = $data.data) == null ? undefined : _b.sku_pics, $update2 && $compCreated);
          {
            __SetStyleObject($n13, [364, 365, 366, parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.sku_pics) == null ? undefined : _d.style)]);
          }
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/sku_pics/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/sku_pics/index"].entry($n13, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_22898d8_14($parent, $data, $update2) {
  let _a, _b, _c, _d;
  if (!$update2 || $varUpdateState[3]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.title_common) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner28 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner28[0],
            lastOwner = _$lepusPushOwner28[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n15 = $update2 ? $lepusGetElementRefByLepusID("component", 15) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n15) {
            $compCreated = false;
            $n15 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 10, "", "title-common", "components/title_common/index", {});
            let $nid15 = $lepusStoreElementRefByLepusID($n15, 15, "title-common");
            $componentId = $nid15[0];
            $childLepusComponent = $componentConstructor($componentId, $n15, "components/title_common/index", 15);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __AppendElement($parent, $n15);
          } else {
            $componentId = __GetElementUniqueID($n15);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("data", (_b = $data.data) == null ? undefined : _b.title_common, $update2 && $compCreated);
          {
            __SetStyleObject($n15, [parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.title_common) == null ? undefined : _d.style)]);
          }
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/title_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/title_common/index"].entry($n15, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_3d68370_16($parent, $data, $update2) {
  let _a, _b, _c, _d;
  if (!$update2 || $varUpdateState[3] || $varUpdateState[1] || $varUpdateState[10]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.price_common) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner29 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner29[0],
            lastOwner = _$lepusPushOwner29[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n17 = $update2 ? $lepusGetElementRefByLepusID("component", 17) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n17) {
            $compCreated = false;
            $n17 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 8, "", "price-common", "components/price_common/index", {});
            let $nid17 = $lepusStoreElementRefByLepusID($n17, 17, "price-common");
            $componentId = $nid17[0];
            $childLepusComponent = $componentConstructor($componentId, $n17, "components/price_common/index", 17);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __AddDataset($n17, "type", "price");
            __AddEvent($n17, "catchEvent", "tap", "handleProductClick");
            __AddEvent($n17, "bindEvent", "PriceScrolledV2", "handlePriceScrolledV2");
            __AppendElement($parent, $n17);
          } else {
            $componentId = __GetElementUniqueID($n17);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("data", (_b = $data.data) == null ? undefined : _b.price_common, $update2 && $compCreated);
          $childLepusComponent._setProp("isCacheData", $data.isCacheData, $update2 && $compCreated);
          $childLepusComponent._setProp("isPriceScrolledV2", $data.isPriceScrolledV2, $update2 && $compCreated);
          {
            __SetStyleObject($n17, [getClassStyleIndex("price", "27472000"), parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.price_common) == null ? undefined : _d.style)]);
          }
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/price_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/price_common/index"].entry($n17, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_2d4de68_18($parent, $data, $update2) {
  let _a, _b, _c, _d, _e;
  if (!$update2 || $varUpdateState[3] || $varUpdateState[11] || $varUpdateState[1] || $varUpdateState[12]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.rec_reason_common) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner30 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner30[0],
            lastOwner = _$lepusPushOwner30[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n19 = $update2 ? $lepusGetElementRefByLepusID("component", 19) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n19) {
            $compCreated = false;
            $n19 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 6, "", "recommend-reason-common", "components/recommend_reason_common/index", {});
            let $nid19 = $lepusStoreElementRefByLepusID($n19, 19, "recommend-reason-common");
            $componentId = $nid19[0];
            $childLepusComponent = $componentConstructor($componentId, $n19, "components/recommend_reason_common/index", 19);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __AddDataset($n19, "type", "recommend");
            __AddEvent($n19, "bindEvent", "Click", "handleProductClick");
            __AppendElement($parent, $n19);
          } else {
            $componentId = __GetElementUniqueID($n19);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("data", getRecReasonsWithReplacement2((_b = $data.data) == null ? undefined : _b.rec_reason_common, $data.recReasonReplacementTags), $update2 && $compCreated);
          $childLepusComponent._setProp("countdown", $data.isCacheData ? "" : getRecReasonCountdown2((_c = $data.data) == null ? undefined : _c.rec_reason_common, $data.recReasonReplacementTags, $data.recReasonCountStartTs), $update2 && $compCreated);
          {
            __SetStyleObject($n19, [parseStyleStringToObject((_e = (_d = $data.data) == null ? undefined : _d.rec_reason_common) == null ? undefined : _e.style)]);
          }
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/recommend_reason_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/recommend_reason_common/index"].entry($n19, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_1c98cd0_20($parent, $data, $update2) {
  let _a, _b, _c, _d;
  if (!$update2 || $varUpdateState[3] || $varUpdateState[13]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.suspend_area_common) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner31 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner31[0],
            lastOwner = _$lepusPushOwner31[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n21 = $update2 ? $lepusGetElementRefByLepusID("component", 21) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n21) {
            $compCreated = false;
            $n21 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 9, "", "suspend-common", "components/suspend_common/index", {});
            let $nid21 = $lepusStoreElementRefByLepusID($n21, 21, "suspend-common");
            $componentId = $nid21[0];
            $childLepusComponent = $componentConstructor($componentId, $n21, "components/suspend_common/index", 21);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __AddDataset($n21, "type", "suspend");
            __AddEvent($n21, "bindEvent", "SuspendClick", "handleSuspendClick");
            __AppendElement($parent, $n21);
          } else {
            $componentId = __GetElementUniqueID($n21);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("data", (_b = $data.data) == null ? undefined : _b.suspend_area_common, $update2 && $compCreated);
          $childLepusComponent._setProp("couponBarLoading", $data.couponBarLoading, $update2 && $compCreated);
          {
            __SetStyleObject($n21, [parseStyleStringToObject((_d = (_c = $data.data) == null ? undefined : _c.suspend_area_common) == null ? undefined : _d.style)]);
          }
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/suspend_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/suspend_common/index"].entry($n21, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_31b6518_22($parent, $data, $update2) {
  let _a, _b;
  if (!$update2 || $varUpdateState[3] || $varUpdateState[1] || $varUpdateState[14] || $varUpdateState[15]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.absolute_common) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner32 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner32[0],
            lastOwner = _$lepusPushOwner32[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n23 = $update2 ? $lepusGetElementRefByLepusID("component", 23) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n23) {
            $compCreated = false;
            $n23 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 2, "", "absolute-common", "components/absolute_common/index", {});
            let $nid23 = $lepusStoreElementRefByLepusID($n23, 23, "absolute-common");
            $componentId = $nid23[0];
            $childLepusComponent = $componentConstructor($componentId, $n23, "components/absolute_common/index", 23);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            $childLepusComponent._setProp("removeComponentElement", true, $update2);
            __AppendElement($parent, $n23);
          } else {
            $componentId = __GetElementUniqueID($n23);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("isCacheData", $data.isCacheData, $update2 && $compCreated);
          $childLepusComponent._setProp("info", (_b = $data.data) == null ? undefined : _b.absolute_common, $update2 && $compCreated);
          $childLepusComponent._setProp("dynamicInfo", $data.absoluteDynamicInfo, $update2 && $compCreated);
          $childLepusComponent._setProp("dislikeEntryVisible", $data.dislikeEntryVisible, $update2 && $compCreated);
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/absolute_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/absolute_common/index"].entry($n23, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_3ceda8_24($parent, $data, $update2) {
  let _a, _b;
  if (!$update2 || $varUpdateState[3] || $varUpdateState[16]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.data) == null ? undefined : _a.ad_common) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner33 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner33[0],
            lastOwner = _$lepusPushOwner33[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n25 = $update2 ? $lepusGetElementRefByLepusID("component", 25) : null;
          let $compCreated = true;
          let $childLepusComponent = null;
          let $componentId = null;
          if (!$n25) {
            $compCreated = false;
            $n25 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 3, "", "ad-common", "components/ad_common/index", {});
            let $nid25 = $lepusStoreElementRefByLepusID($n25, 25, "ad-common");
            $componentId = $nid25[0];
            $childLepusComponent = $componentConstructor($componentId, $n25, "components/ad_common/index", 25);
            $createdIds.push($componentId + "");
            $cardInstance._currentOwner.componentIds.push($componentId);
            __SetStyleObject($n25, [getClassStyleIndex("ad-common", "27472000")]);
            __AddEvent($n25, "bindEvent", "OtherClick", "handleProductClick");
            __AppendElement($parent, $n25);
          } else {
            $componentId = __GetElementUniqueID($n25);
            $childLepusComponent = $componentInfo[$componentId];
          }
          $comUpdatePropsSet = [];
          $childLepusComponent._setProp("info", (_b = $data.data) == null ? undefined : _b.ad_common, $update2 && $compCreated);
          $childLepusComponent._setProp("dynamicInfo", $data.adDynamicInfo, $update2 && $compCreated);
          if ($compCreated) {
            let $update_keys = $comUpdatePropsSet;
            let $childSlotUpdate = false;
            if ($update_keys.length > 0 || $childSlotUpdate) {
              $updatedIds.push($componentId + "");
              $renderComponents["components/ad_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
            }
          } else {
            $renderComponents["components/ad_common/index"].entry($n25, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
          }
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_f7a288_26($parent, $data, $update2) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[3] || $varUpdateState[17] || $varUpdateState[18] || $varUpdateState[7] || $varUpdateState[0]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.feedbackVisible) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner34 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner34[0],
            lastOwner = _$lepusPushOwner34[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n27 = $update2 ? $lepusGetElementRefByLepusID("view", 27) : null;
          let $temp2 = $update2;
          if (!$n27) {
            $update2 = false;
            $n27 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n27, 27, "view");
            __SetStyleObject($n27, [getClassStyleIndex("feedback-wp", "27472000")]);
            __AppendElement($parent, $n27);
          }
          {
            let $n28 = $update2 ? $lepusGetElementRefByLepusID("component", 28) : null;
            let $compCreated = true;
            let $childLepusComponent = null;
            let $componentId = null;
            if (!$n28) {
              $compCreated = false;
              $n28 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 4, "", "feedback", "components/feedback_common/index", {});
              let $nid28 = $lepusStoreElementRefByLepusID($n28, 28, "feedback");
              $componentId = $nid28[0];
              $childLepusComponent = $componentConstructor($componentId, $n28, "components/feedback_common/index", 28);
              $createdIds.push($componentId + "");
              $cardInstance._currentOwner.componentIds.push($componentId);
              $childLepusComponent._setProp("item_type", 2, $update2);
              __SetStyleObject($n28, [5, 16]);
              __AddEvent($n28, "bindEvent", "FeedbackClose", "handleCloseFeedback");
              __AppendElement($n27, $n28);
            } else {
              $componentId = __GetElementUniqueID($n28);
              $childLepusComponent = $componentInfo[$componentId];
            }
            $comUpdatePropsSet = [];
            $childLepusComponent._setProp("showAssociated", !!((_b = (_a = $data.data) == null ? undefined : _a.feedback_common) == null ? undefined : _b.show_associated_link) && !isHarmony3(), $update2 && $compCreated);
            $childLepusComponent._setProp("associatedLink", (_d = (_c = $data.data) == null ? undefined : _c.feedback_common) == null ? undefined : _d.associated_link, $update2 && $compCreated);
            $childLepusComponent._setProp("item_id", (_f = (_e = $data.data) == null ? undefined : _e.feedback_common) == null ? undefined : _f.product_id, $update2 && $compCreated);
            $childLepusComponent._setProp("items", $data.feedbackData, $update2 && $compCreated);
            $childLepusComponent._setProp("limit", $data.feedbackLimit, $update2 && $compCreated);
            $childLepusComponent._setProp("ecomType", (_h = (_g = $data.data) == null ? undefined : _g.feedback_common) == null ? undefined : _h.ecom_type, $update2 && $compCreated);
            $childLepusComponent._setProp("logId", $data.ec_lynx_props_extra.track_common_data.request_id, $update2 && $compCreated);
            $childLepusComponent._setProp("recommendInfo", $data.ec_lynx_props_extra.recommend_info || ((_i = $data.extra) == null ? undefined : _i.recommend_info), $update2 && $compCreated);
            $childLepusComponent._setProp("ad", ((_k = (_j = $data.data) == null ? undefined : _j.feedback_common) == null ? undefined : _k.ad) || {}, $update2 && $compCreated);
            $childLepusComponent._setProp("isAD", (_m = (_l = $data.data) == null ? undefined : _l.feedback_common) == null ? undefined : _m.is_ad, $update2 && $compCreated);
            $childLepusComponent._setProp("abValues", (_n = $data.extra) == null ? undefined : _n.ab_values, $update2 && $compCreated);
            $childLepusComponent._setProp("coverTrack", (_p = (_o = $data.data) == null ? undefined : _o.feedback_common) == null ? undefined : _p.cover_params, $update2 && $compCreated);
            $childLepusComponent._setProp("commonTrack", (_r = (_q = $data.data) == null ? undefined : _q.feedback_common) == null ? undefined : _r.common_params, $update2 && $compCreated);
            if ($compCreated) {
              let $update_keys = $comUpdatePropsSet;
              let $childSlotUpdate = false;
              if ($update_keys.length > 0 || $childSlotUpdate) {
                $updatedIds.push($componentId + "");
                $renderComponents["components/feedback_common/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
              }
            } else {
              $renderComponents["components/feedback_common/index"].entry($n28, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$update_3caa848_29($parent, $data, $update2) {
  let _a, _b, _c, _d, _e;
  if (!$update2 || $varUpdateState[0] || $varUpdateState[19] || $varUpdateState[20] || $varUpdateState[21]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (showWindVane($data.wind_vane_data, null, (_a = $data.extra) == null ? undefined : _a.ab_values)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let _$lepusPushOwner35 = $lepusPushOwner(uniqueId + "_0"),
            owner = _$lepusPushOwner35[0],
            lastOwner = _$lepusPushOwner35[1];
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n30 = $update2 ? $lepusGetElementRefByLepusID("view", 30) : null;
          let $temp2 = $update2;
          if (!$n30) {
            $update2 = false;
            $n30 = __CreateView($currentComponentId);
            $lepusStoreElementRefByLepusID($n30, 30, "view");
            __AppendElement($parent, $n30);
          }
          if (!$update2 || $varUpdateState[0]) {
            {
              let $value = "margin-top:" + getWindVaneMarginTopRpx((_b = $data.extra) == null ? undefined : _b.ab_values) + "rpx;overflow:hidden;";
              if (!$update2 || $value !== undefined) {
                __SetStyleObject($n30, [{
                  40: getWindVaneMarginTopRpx((_c = $data.extra) == null ? undefined : _c.ab_values) + "rpx"
                }, 3]);
              }
            }
          }
          {
            let $n31 = $update2 ? $lepusGetElementRefByLepusID("component", 31) : null;
            let $compCreated = true;
            let $childLepusComponent = null;
            let $componentId = null;
            if (!$n31) {
              $compCreated = false;
              $n31 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 5, "", "wind_vane", "components/wind_vane/wind_vane", {});
              let $nid31 = $lepusStoreElementRefByLepusID($n31, 31, "wind_vane");
              $componentId = $nid31[0];
              $childLepusComponent = $componentConstructor($componentId, $n31, "components/wind_vane/wind_vane", 31);
              $createdIds.push($componentId + "");
              $cardInstance._currentOwner.componentIds.push($componentId);
              __AppendElement($n30, $n31);
            } else {
              $componentId = __GetElementUniqueID($n31);
              $childLepusComponent = $componentInfo[$componentId];
            }
            $comUpdatePropsSet = [];
            $childLepusComponent._setProp("data", JSON.parse($data.wind_vane_data.lynx_data).data, $update2 && $compCreated);
            $childLepusComponent._setProp("abValues", (_d = $data.extra) == null ? undefined : _d.ab_values, $update2 && $compCreated);
            $childLepusComponent._setProp("scrollToTopData", {
              offset: $data.cardHeight + +getWindVaneMarginTopRpx((_e = $data.extra) == null ? undefined : _e.ab_values) / 2,
              itemID: $data.extra.product_id,
              index: $data.cardIndex
            }, $update2 && $compCreated);
            if ($compCreated) {
              let $update_keys = $comUpdatePropsSet;
              let $childSlotUpdate = false;
              if ($update_keys.length > 0 || $childSlotUpdate) {
                $updatedIds.push($componentId + "");
                $renderComponents["components/wind_vane/wind_vane"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
              }
            } else {
              $renderComponents["components/wind_vane/wind_vane"].entry($n31, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
        $lepusPopOwner(lastOwner);
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  }
}
function $$slot_default_2($componentId, $parent, $update2) {
  let _a, _b, _c, _d, _e, _f, _g;
  let $savedId = $currentComponentId;
  let $savedComponent = $cardInstance._currentComponentElement;
  let $slotComponent = $componentId == $cardInstance._componentId ? undefined : $componentInfo[$componentId];
  let $data = $cardInstance.data;
  $currentComponentId = $componentId;
  $cardInstance._currentComponentElement = $slotComponent;
  if (!$update2) {
    let $n3 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n3, 3, "view");
    __SetStyleObject($n3, [getClassStyleIndex("product " + (((_a = $data.extra) == null ? undefined : _a.use_chip_view) === 1 ? "" : "use-base-background"), "27472000"), 16, parseStyleStringToObject(getBorderStyle2((_b = $data.extra) == null ? undefined : _b.ab_values))]);
    __SetID($n3, "trainsition-view" + $data.extra.product_id);
    __AddEvent($n3, "bindEvent", "longpress", "handleLongPress");
    __AppendElement($parent, $n3);
    let $n4 = __CreateView($currentComponentId);
    __AppendElement($n3, $n4);
    let $n5 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n5, 5, "if");
    __AppendElement($n4, $n5);
    $$update_29e2e68_5($n5, $data, $update2);
    let $n7 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n7, 7, "if");
    __AppendElement($n4, $n7);
    $$update_2a4a2c0_7($n7, $data, $update2);
    let $n9 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n9, 9, "if");
    __AppendElement($n3, $n9);
    $$update_22898d8_9($n9, $data, $update2);
    let $n11 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n11, 11, "view");
    __SetStyleObject($n11, [getClassStyleIndex(getProductInfoClassName((_c = $data.extra) == null ? undefined : _c.ab_values), "27472000")]);
    __AddEvent($n11, "bindEvent", "tap", "handleProductClick");
    __AppendElement($n3, $n11);
    let $n12 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n12, 12, "if");
    __AppendElement($n11, $n12);
    $$update_22898d8_12($n12, $data, $update2);
    let $n14 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n14, 14, "if");
    __AppendElement($n11, $n14);
    $$update_22898d8_14($n14, $data, $update2);
    let $n16 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n16, 16, "if");
    __AppendElement($n11, $n16);
    $$update_3d68370_16($n16, $data, $update2);
    let $n18 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n18, 18, "if");
    __AppendElement($n11, $n18);
    $$update_2d4de68_18($n18, $data, $update2);
    let $n20 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n20, 20, "if");
    __AppendElement($n11, $n20);
    $$update_1c98cd0_20($n20, $data, $update2);
    let $n22 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n22, 22, "if");
    __AppendElement($n3, $n22);
    $$update_31b6518_22($n22, $data, $update2);
    let $n24 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n24, 24, "if");
    __AppendElement($n3, $n24);
    $$update_3ceda8_24($n24, $data, $update2);
    let $n26 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n26, 26, "if");
    __AppendElement($n3, $n26);
    $$update_f7a288_26($n26, $data, $update2);
  } else {
    if ($varUpdateState[0]) {
      let _$n17 = $lepusGetElementRefByLepusID("view", 3);
      {
        __SetStyleObject(_$n17, [getClassStyleIndex("product " + (((_d = $data.extra) == null ? undefined : _d.use_chip_view) === 1 ? "" : "use-base-background"), "27472000"), 16, parseStyleStringToObject(getBorderStyle2((_e = $data.extra) == null ? undefined : _e.ab_values))]);
      }
      {
        let $value = "trainsition-view" + $data.extra.product_id;
        if (!$update2 || $value !== "trainsition-view" + $cardInstance._data.extra.product_id) {
          __SetID(_$n17, $value);
        }
      }
    }
    $lepusGetElementRefByLepusID("view", 4);
    let _$n6 = $lepusGetElementRefByLepusID("if", 5);
    $$update_29e2e68_5(_$n6, $data, $update2);
    let _$n7 = $lepusGetElementRefByLepusID("if", 7);
    $$update_2a4a2c0_7(_$n7, $data, $update2);
    let _$n8 = $lepusGetElementRefByLepusID("if", 9);
    $$update_22898d8_9(_$n8, $data, $update2);
    if ($varUpdateState[0]) {
      let _$n18 = $lepusGetElementRefByLepusID("view", 11);
      {
        let _$value51 = getProductInfoClassName((_f = $data.extra) == null ? undefined : _f.ab_values);
        if (!$update2 || _$value51 !== undefined) {
          __SetStyleObject(_$n18, [getClassStyleIndex(getProductInfoClassName((_g = $data.extra) == null ? undefined : _g.ab_values), "27472000")]);
        }
      }
    }
    let _$n9 = $lepusGetElementRefByLepusID("if", 12);
    $$update_22898d8_12(_$n9, $data, $update2);
    let _$n10 = $lepusGetElementRefByLepusID("if", 14);
    $$update_22898d8_14(_$n10, $data, $update2);
    let _$n11 = $lepusGetElementRefByLepusID("if", 16);
    $$update_3d68370_16(_$n11, $data, $update2);
    let _$n12 = $lepusGetElementRefByLepusID("if", 18);
    $$update_2d4de68_18(_$n12, $data, $update2);
    let _$n13 = $lepusGetElementRefByLepusID("if", 20);
    $$update_1c98cd0_20(_$n13, $data, $update2);
    let _$n14 = $lepusGetElementRefByLepusID("if", 22);
    $$update_31b6518_22(_$n14, $data, $update2);
    let _$n15 = $lepusGetElementRefByLepusID("if", 24);
    $$update_3ceda8_24(_$n15, $data, $update2);
    let _$n16 = $lepusGetElementRefByLepusID("if", 26);
    $$update_f7a288_26(_$n16, $data, $update2);
  }
  $cardInstance._currentComponentElement = $savedComponent;
  $currentComponentId = $savedId;
}
function __GetDiffData(a,b,c) {}
function __FlushElementTree(a) {}
function __CreatePage(a,b) {}
updatePage = function ($newData, options) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q;
  if (!$initAppService) {
    $initAppService = true;
    Object.keys($cardInstance.data).forEach(function (item) {
      $cardInstance._data[item] = $deepClone($cardInstance.data[item]);
    });
  }
  $update = true;
  __globalProps = lynx.__globalProps;
  let $result = __GetDiffData($cardInstance.data, $newData, options);
  let $data = $result["new_data"];
  let $array = $result["diff_key_array"];
  $cardVariables.forEach(function (it, index) {
    $varUpdateState[index] = $array.includes(it);
  });
  $array.forEach(function (item) {
    $cardInstance.data[item] = $data[item];
  });
  $data = $cardInstance.data;
  if ($varUpdateState[0] || $varUpdateState[1]) {
    let $n1 = $lepusGetElementRefByLepusID("view", 1);
    {
      __SetStyleObject($n1, [getClassStyleIndex("card " + (((_a = $data.extra) == null ? undefined : _a.need_animation) ? "card-animation" : ""), "27472000"), parseStyleStringToObject(card_width($data.isCacheData, "product_card")), parseStyleStringToObject(getBorderStyle2((_b = $data.extra) == null ? undefined : _b.ab_values))]);
    }
    {
      let $value = "product-card-container" + $data.extra.product_id;
      if (!$update || $value !== "product-card-container" + $cardInstance._data.extra.product_id) {
        __SetID($n1, $value);
      }
    }
  }
  if (!$update || $varUpdateState[3] || $varUpdateState[0] || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[4] || $varUpdateState[5] || $varUpdateState[6] || $varUpdateState[7] || $varUpdateState[9] || $varUpdateState[8] || $varUpdateState[10] || $varUpdateState[11] || $varUpdateState[12] || $varUpdateState[13] || $varUpdateState[14] || $varUpdateState[15] || $varUpdateState[16] || $varUpdateState[17] || $varUpdateState[18]) {
    {
      let $n2 = $lepusGetElementRefByLepusID("component", 2);
      let $componentId = __GetElementUniqueID($n2);
      let $childLepusComponent = $componentInfo[$componentId];
      $comUpdatePropsSet = [];
      $childLepusComponent._setProp("enable", (_d = (_c = $data.data) == null ? undefined : _c.media_wrapper) == null ? undefined : _d.enable, $update);
      $childLepusComponent._setProp("mediatype", (_f = (_e = $data.data) == null ? undefined : _e.media_wrapper) == null ? undefined : _f.media_type, $update);
      $childLepusComponent._setProp("mediaName", (_h = (_g = $data.data) == null ? undefined : _g.media_wrapper) == null ? undefined : _h.media_name, $update);
      $childLepusComponent._setProp("mediaid", (_j = (_i = $data.data) == null ? undefined : _i.media_wrapper) == null ? undefined : _j.media_id, $update);
      $childLepusComponent._setProp("transition_element_id", (_l = (_k = $data.data) == null ? undefined : _k.media_wrapper) == null ? undefined : _l.transition_element_id, $update);
      $childLepusComponent._setProp("transition_item_id", (_n = (_m = $data.data) == null ? undefined : _m.media_wrapper) == null ? undefined : _n.transition_item_id, $update);
      {
        __SetStyleObject($n2, [getClassStyleIndex("product-media-wrapper", "27472000"), parseStyleStringToObject(getCardHeight2((_o = $data.extra) == null ? undefined : _o.ab_values))]);
      }
      {
        let _$value52 = $data.feedbackVisible ? false : true;
        if (!$update || _$value52 !== ($cardInstance._data.feedbackVisible ? false : true)) {
          __SetAttribute($n2, "accessibility-element", _$value52);
        }
      }
      {
        let _$value53 = (_p = $data.extra) == null ? undefined : _p.accessibility_label;
        if (!$update || _$value53 !== ((_q = $cardInstance._data.extra) == null ? undefined : _q.accessibility_label)) {
          __SetAttribute($n2, "accessibility-label", _$value53);
        }
      }
      let $update_keys = $comUpdatePropsSet;
      let $childSlotUpdate = $varUpdateState[0] || $varUpdateState[3] || $varUpdateState[1] || $varUpdateState[4] || $varUpdateState[5] || $varUpdateState[6] || $varUpdateState[7] || $varUpdateState[9] || $varUpdateState[8] || $varUpdateState[10] || $varUpdateState[11] || $varUpdateState[12] || $varUpdateState[13] || $varUpdateState[14] || $varUpdateState[15] || $varUpdateState[16] || $varUpdateState[2] || $varUpdateState[17] || $varUpdateState[18];
      if ($update_keys.length > 0 || $childSlotUpdate) {
        $updatedIds.push($componentId + "");
        $renderComponents["components/media_wrapper/index"].update($childLepusComponent, $childLepusComponent.data, $childLepusComponent.properties, $update_keys, false, true, $childSlotUpdate);
      }
    }
  }
  let $n29 = $lepusGetElementRefByLepusID("if", 29);
  $$update_3caa848_29($n29, $data, $update);
  $array.forEach(function (item) {
    $cardInstance._data[item] = $deepClone($data[item]);
  });
  __FlushElementTree($page);
  $componentsDeleteLifeCycle();
  $componentsCreateLifeCycle();
  $componentsUpdateLifeCycle();
  return true;
};
renderPage = function ($renderData) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p;
  __globalProps = lynx.__globalProps;
  SystemInfo = lynx.SystemInfo;
  $airFirstScreen = true;
  $outPre = true;
  $page = __CreatePage("0", 0);
  $cardInstance = $cardConstructor($currentComponentId, "src/cards/guessyoulike/product_card_v2/index");
  if ($renderData) {
    Object.assign($cardInstance.data, $renderData);
  }
  let _$lepusPushOwner36 = $lepusPushOwner("0"),
      owner = _$lepusPushOwner36[0],
      lastOwner = _$lepusPushOwner36[1];
  let $data = $cardInstance.data;
  let $n1 = __CreateView($currentComponentId);
  $lepusStoreElementRefByLepusID($n1, 1, "view");
  __SetStyleObject($n1, [getClassStyleIndex("card " + (((_a = $data.extra) == null ? undefined : _a.need_animation) ? "card-animation" : ""), "27472000"), parseStyleStringToObject(card_width($data.isCacheData, "product_card")), parseStyleStringToObject(getBorderStyle2((_b = $data.extra) == null ? undefined : _b.ab_values))]);
  __SetAttribute($n1, "clip-radius", "true");
  __SetAttribute($n1, "name", "product-card");
  __SetID($n1, "product-card-container" + $data.extra.product_id);
  __AddEvent($n1, "bindEvent", "animationend", "endAnimation");
  __AppendElement($page, $n1);
  {
    let $n2 = __CreateComponent($cardInstance._uniqueId, $cardInstance.fiberComponentId + "", 7, "", "media-wrapper", "components/media_wrapper/index", {});
    let $nid2 = $lepusStoreElementRefByLepusID($n2, 2, "media-wrapper");
    let $childLepusComponent = $componentConstructor($nid2[0], $n2, "components/media_wrapper/index", 2);
    $cardInstance._currentOwner.componentIds.push($nid2[0]);
    $comUpdatePropsSet = [];
    $childLepusComponent._setProp("enable", (_d = (_c = $data.data) == null ? undefined : _c.media_wrapper) == null ? undefined : _d.enable, $update);
    $childLepusComponent._setProp("mediatype", (_f = (_e = $data.data) == null ? undefined : _e.media_wrapper) == null ? undefined : _f.media_type, $update);
    $childLepusComponent._setProp("mediaName", (_h = (_g = $data.data) == null ? undefined : _g.media_wrapper) == null ? undefined : _h.media_name, $update);
    $childLepusComponent._setProp("mediaid", (_j = (_i = $data.data) == null ? undefined : _i.media_wrapper) == null ? undefined : _j.media_id, $update);
    $childLepusComponent._setProp("transition_element_id", (_l = (_k = $data.data) == null ? undefined : _k.media_wrapper) == null ? undefined : _l.transition_element_id, $update);
    $childLepusComponent._setProp("transition_item_id", (_n = (_m = $data.data) == null ? undefined : _m.media_wrapper) == null ? undefined : _n.transition_item_id, $update);
    __SetStyleObject($n2, [getClassStyleIndex("product-media-wrapper", "27472000"), parseStyleStringToObject(getCardHeight2((_o = $data.extra) == null ? undefined : _o.ab_values))]);
    __SetAttribute($n2, "accessibility-traits", "none");
    __SetAttribute($n2, "accessibility-element", $data.feedbackVisible ? false : true);
    __SetAttribute($n2, "accessibility-label", (_p = $data.extra) == null ? undefined : _p.accessibility_label);
    __AddEvent($n2, "bindEvent", "updateFilters", "handleUpdateFilters");
    __AppendElement($n1, $n2);
    let slotName = "default";
    $cardInstance.slots[slotName] = {
      fn: $$slot_default_2,
      componentId: $cardInstance._componentId
    };
    $childLepusComponent.slots[slotName] = $cardInstance.slots[slotName];
    $renderComponents["components/media_wrapper/index"].entry($n2, $childLepusComponent.data, $childLepusComponent.properties, $childLepusComponent, false);
    $createdIds.push($nid2[0] + "");
  }
  let $n29 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n29, 29, "if");
  __AppendElement($n1, $n29);
  $$update_3caa848_29($n29, $data, $update);
  $airFirstScreen = false;
  $cardVariables = ["extra", "isCacheData", "feedbackVisible", "data", "coverDynamicInfo", "log_meta_set", "log_data", "ec_lynx_props_extra", "hideSimilar", "find_similar", "isPriceScrolledV2", "recReasonReplacementTags", "recReasonCountStartTs", "couponBarLoading", "absoluteDynamicInfo", "dislikeEntryVisible", "adDynamicInfo", "feedbackData", "feedbackLimit", "wind_vane_data", "cardHeight", "cardIndex"];
  $lepusPopOwner(lastOwner);
  $componentsCreateLifeCycle();
  return true;
};
//# sourceMappingURL=network_addresss.js.map
