let __assert_fail_count = 0;
function SoftAssert(v, msg) {
  if (!v) {
    __assert_fail_count = __assert_fail_count + 1;
    print(msg);
  }
}

function __IsArray(a) {
  if (a) {
    if (a.push === [].push) {
      return true;
    }
  }
  return false;
}

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

let getCssPropertyIDObj;
let $selectorStyleIndexMap;
let $selectorStyleIndexMap2;
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

let lynx = {
  __globalProps: {
    os: "android",
    testNum: "1000",
    test_num: "1000",
    appVersion: "1.2.3",
    screenWidth: 390,
    ec_extra: {
      font_size_pref: 2,
      big_font_enabled: true
    }
  },
  SystemInfo: {}
};
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

let __nextElementId = 1;
function __CreateElement(tag, componentId) {
  return {
    _id: __nextElementId++,
    _tag: tag,
    _componentId: componentId,
    children: [],
    attrs: {},
    text: ""
  };
}
function __CreateView(a) {
  return __CreateElement("view", a);
}
function __CreateText(a) {
  return __CreateElement("text", a);
}
function __CreatePage(a, b) {
  let page = __CreateElement("page", a);
  page.attrs.path = b;
  return page;
}
function __SetAttribute(a, b, c) {
  if (!a) return;
  a.attrs[b + ""] = c;
  if (b === "text") {
    a.text = c;
  }
}
function __AppendElement(a, b) {
  if (a && b) {
    a.children.push(b);
  }
}
function __FlushElementTree(a) {}
function __GetDiffData(a, b, c) {
  return { oldData: a, nextData: b, options: c, changed: true };
}

let $currentComponentId = 10;
let $page;
let $cardInstance;
let $cardOptions = { data: {} };
let $update = false;
let renderPage = null;
let updatePage = null;

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
  "gyl-suspend__coupon-bar-text_27959000": [146, 35, 297, 36, 147, 149, 117]
};

$selectorStyleIndexMap2 = {
  "card_dup_27472000": [0, 1, 2, 3],
  "card-animation_dup_27472000": [4],
  "product_dup_27472000": [5, 2, 0, 1, 6, 3],
  "use-base-background_dup_27472000": [7],
  "product-info_dup_27472000": [7, 8, 6, 3, 9],
  "product-info-borderless_dup_27472000": [10],
  "product-info-smallfs_dup_27472000": [11],
  "product-info-price-atmosphere_dup_27472000": [12],
  "price_dup_27472000": [0, 13, 14],
  "ad-common_dup_27472000": [15, 5, 16, 17, 18, 3],
  "feedback-wp_dup_27472000": [15, 17, 18, 19, 5, 16],
  "similar_dup_27472000": [15, 5, 20, 18],
  "belt_dup_common_27472000": [21, 22, 23],
  "page_dup_21316000": [],
  "mall-font-mini_dup_21316000": [],
  "mall-font-medium_dup_21316000": [],
  "mall-font-large_dup_21316000": [],
  "mall-font-larger_dup_21316000": [],
  "container_dup_21316000": [15, 5, 16, 17, 18],
  "product_dup_with_live_tag_21316000": [15, 24, 25, 26, 27, 0],
  "product_dup_with_live_tag-icon_21316000": [28, 29, 30, 31],
  "product_dup_with_live_tag-text_21316000": [32, 33, 34, 35, 36, 37, 38, 38, 39],
  "corner-mark_dup_21316000": [15, 40, 41, 29, 42, 43],
  "corner-mark-text_dup_21316000": [44, 45, 46, 47, 48, 49, 35, 50, 51, 34],
  "product-cover-border-image-container_dup_21316000": [5, 52, 15, 17, 18, 53],
  "product-cover-border-image_dup_21316000": [5, 52, 15, 17, 18, 54],
  "just_dup_see_see_21316000": [15, 17, 18],
  "just_dup_see_see-icon_21316000": [55, 56],
  "just_dup_see_see-text_21316000": [15, 35, 57, 25, 34],
  "just_dup_see_see_logo_21316000": [15, 24, 25, 58, 59, 60],
  "decoration_dup_21316000": [15, 29, 0, 61, 40, 41, 27, 26],
  "decoration-collocate_dup_21316000": [24, 25],
  "decoration-icon_dup_21316000": [28, 29, 26],
  "decoration-icon-collocate_dup_21316000": [62, 63, 32],
  "product-video-tag_dup_21316000": [15, 40, 41],
  "product-video-tag-icon_dup_21316000": [30, 31],
  "decoration-text_dup_21316000": [29, 37, 64, 35, 34],
  "decoration-text-collocate_dup_21316000": [65],
  "ad-tag_dup_21316000": [15, 28, 66, 67, 68, 0, 69, 70],
  "ad-tag-text_dup_21316000": [71, 72, 5, 73, 50],
  "ad-pot_dup_21316000": [74, 75, 15, 76, 20, 77],
  "border_dup_21316000": [15, 18, 17, 5, 16, 78, 79, 80, 81, 82, 83, 84, 19],
  "product-coupon_dup_21316000": [15, 85, 86, 87, 3, 17, 88, 89, 22, 90, 91, 14],
  "product-coupon-container_dup_21316000": [92, 93, 94, 14, 0, 23],
  "product-coupon-container-legou_dup_21316000": [92, 95, 96, 14, 0, 23],
  "product-coupon-symbol_dup_21316000": [34, 97, 98, 99, 100],
  "product-coupon-integer_dup_21316000": [34, 101, 98, 100, 102],
  "product-redPacket_dup_21316000": [15, 103, 104, 87, 3, 105, 22, 90, 91, 106, 107, 108, 109, 110, 111, 112, 113],
  "product-redPacket-wrapper_dup_21316000": [1, 114, 115, 61, 23],
  "product-redPacket-container_dup_21316000": [14, 0, 23],
  "product-redPacket-num_dup_21316000": [34, 116, 36, 117],
  "product-redPacket-symbol_dup_21316000": [34, 118, 119, 99, 120],
  "product-redPacket-subtitle_dup_21316000": [121, 97, 122, 36, 123],
  "dislike-entry_dup_21316000": [16, 5, 61, 124, 125],
  "mask-animation-show_dup_21316000": [126, 127],
  "mask-animation-hide_dup_21316000": [128, 129],
  "title-show_dup_21316000": [130],
  "title-hide_dup_21316000": [131],
  "dislike-lottie_dup_21316000": [132, 133],
  "dislike-title_dup_21316000": [134],
  "dislike-subtitle_dup_21316000": [135, 0],
  "dislike-entry-text_dup_21316000": [34, 50, 136, 137, 138],
  "text-yellow_dup_21316000": [139],
  "container_dup_12169000": [15, 5, 16, 17, 18, 3],
  "animation_dup_12169000": [140, 141, 15, 3, 142, 143],
  "animation-lottie_dup_12169000": [5, 16],
  "mask_dup_12169000": [15, 88, 17, 5, 16, 79, 3],
  "mask-image_dup_12169000": [5, 16, 79],
  "gyl-belt_dup_32391000": [0, 61, 5, 16, 144],
  "gyl-belt_dup__main_32391000": [0, 61],
  "gyl-belt_dup__action_32391000": [0],
  "gyl-belt_dup__main-icon_left_32391000": [6],
  "gyl-belt_dup__main-text_32391000": [0, 61],
  "gyl-countdown_dup_20523000": [5, 145, 61, 13, 0, 23],
  "gyl-countdown_dup__num_20523000": [146, 35, 36, 147],
  "gyl-countdown_dup__split-text_20523000": [146, 35, 36, 147],
  "gyl-cover_dup__container_49914000": [5, 6],
  "gyl-cover_dup__wrapper_49914000": [5, 0, 16],
  "gyl-cover_dup__absolute_49914000": [15],
  "gyl-cover_dup__cover_49914000": [5, 148, 149],
  "gyl-cover_dup__mask_49914000": [5, 148, 149, 15, 15, 18, 88, 17, 20],
  "gyl-cover_dup__video_49914000": [5, 16, 15, 17, 18, 148],
  "gyl-cover_dup__swiper_49914000": [5, 16, 15, 17, 18, 150, 148],
  "gyl-cover_dup__swiper-item_49914000": [5, 16],
  "gyl-cover_dup__swiper-item-image_49914000": [5, 16, 151],
  "gyl-cover_dup__tag_49914000": [15, 25, 152, 153, 154, 149, 155, 156, 0, 61, 33],
  "gyl-cover_dup__tag-image_49914000": [62, 63, 155, 157, 158],
  "gyl-cover_dup__tag-left_49914000": [159, 35, 122, 36, 160, 149],
  "gyl-cover_dup__tag-icon_49914000": [161, 66, 162, 163, 32, 164],
  "gyl-cover_dup__tag-right_49914000": [159, 35, 122, 36, 160],
  "gyl-cover_dup__bubble_49914000": [15],
  "gyl-cover_dup__horizontal_skus_49914000": [5],
  "feedback_dup_54878000": [5, 16, 165, 3],
  "feedback-container_dup_54878000": [166, 5, 16, 79, 0, 1, 167, 168, 6, 169, 3],
  "gyl_dup_feedback_39746000": [15, 17, 18, 5, 16, 0, 1, 167, 168],
  "gyl_dup_feedback-neg_39746000": [5, 170, 0, 1, 23],
  "gyl_dup_feedback-neg--item_39746000": [171, 0, 172, 173, 174, 175, 61, 176, 177],
  "gyl_dup_feedback-neg--item-more_39746000": [178],
  "gyl_dup_feedback-neg-pagetwo-item_39746000": [23],
  "gyl_dup_feedback-neg--item-last_39746000": [178],
  "gyl_dup_feedback-neg-pagetwo-item-icon_39746000": [179],
  "gyl_dup_feedback-neg--item-icon_39746000": [180, 181, 33],
  "gyl_dup_feedback-neg--item-icon-more_39746000": [182, 183, 184, 185],
  "gyl_dup_feedback-neg--item-text_39746000": [34, 118, 119],
  "gyl_dup_feedback-neg--item-text-more_39746000": [34, 118, 119],
  "gyl_dup_feedback-neg--item-text-more-small_39746000": [34, 186, 119],
  "gyl_dup_feedback-similar_39746000": [5, 187, 43, 0, 188, 61, 23, 189, 190],
  "gyl_dup_feedback-similar-arrow-icon_39746000": [182, 183, 184],
  "gyl_dup_feedback-similar-text_39746000": [119, 191, 192, 34],
  "gyl_dup_feedback-header_39746000": [15, 5, 193, 17, 18, 194],
  "gyl_dup_feedback-header-back_39746000": [0, 195, 196, 15, 18],
  "gyl_dup_feedback-header-back-icon_39746000": [183, 182, 197, 198],
  "gyl_dup_feedback-header-back-text_39746000": [34, 118, 36, 199],
  "gyl_dup_feedback-header-close_39746000": [196, 200, 0, 15, 88],
  "gyl_dup_feedback-header-close-icon_39746000": [183, 182, 201],
  "gyl-find_dup_similar_26698000": [5, 202, 203],
  "gyl-find_dup_similar-container_26698000": [0, 61, 204, 5, 202, 205, 206, 22, 21],
  "gyl-find_dup_similar-cover_26698000": [79, 207, 208, 209],
  "gyl-find_dup_similar-mask_26698000": [15, 18, 17, 207, 208, 210, 79],
  "gyl-find_dup_similar-text_26698000": [211, 35, 119, 48, 212],
  "gyl-find_dup_similar-arrow_26698000": [182, 183, 120],
  "gyl-cover_dup__horizontal-skus__container_17257000": [0, 61, 144, 213, 7, 5],
  "gyl-cover_dup__horizontal-skus__short-card-wrapper_17257000": [5, 0, 61, 144],
  "gyl-cover_dup__horizontal-skus__short-card_17257000": [149, 0, 61, 214, 215, 216, 217, 79, 9],
  "gyl-cover_dup__horizontal-skus__short-card:first-child_17257000": [157],
  "gyl-cover_dup__horizontal-skus__short-card-image_17257000": [149, 218, 219],
  "gyl-cover_dup__horizontal-skus__short-card-price-container_17257000": [0, 220],
  "gyl-cover_dup__horizontal-skus__short-card-price_17257000": [149, 3, 221],
  "gyl-cover_dup__horizontal-skus__long-card_17257000": [149, 0, 61, 144, 222, 216, 5, 79],
  "gyl-cover_dup__horizontal-skus__long-card-image-wrapper_17257000": [149, 0, 61, 214],
  "gyl-cover_dup__horizontal-skus__long-card-image_17257000": [218, 219, 223],
  "gyl-cover_dup__horizontal-skus__long-card-price_17257000": [0, 149, 220],
  "gyl-cover_dup__horizontal-skus__long-card-price-suffix_17257000": [224, 225, 36, 226, 227],
  "container_dup_20117000": [5, 6],
  "swiper-item_dup_20117000": [5, 16],
  "swiper-item-image_dup_20117000": [5, 16, 151],
  "live_dup_wrapper_20117000": [5, 16, 15, 17, 18],
  "live_dup_20117000": [5, 16, 126],
  "live_dup_show_20117000": [128],
  "feed_dup_back_button_20117000": [15, 17, 88],
  "feed_dup_back_button_image_20117000": [228, 59],
  "video_dup_engine_20117000": [15, 17, 18, 20, 88, 3],
  "scroll_dup_price_container_51983000": [0, 13, 14, 229],
  "scroll_dup_price_num_list_51983000": [0, 1, 14, 23, 3, 230],
  "translateY4_dup_51983000": [231],
  "scroll_dup_price_num_list_wrapper_51983000": [0, 1, 61],
  "scroll_dup_price_num_list_item_51983000": [0, 232, 149, 107, 233, 5],
  "scroll_dup_price_num_list_item_text_51983000": [50, 117, 146, 221],
  "scroll_dup_price_point_51983000": [118, 146, 117, 234],
  "gyl-price_dup_51983000": [0, 13, 220, 170],
  "gyl-price--lynx-android_dup_51983000": [],
  "gyl-price_dup__symbol_51983000": [235],
  "gyl-price_dup__number_51983000": [221, 236],
  "gyl-price_dup__slash_51983000": [237, 66],
  "gyl-price_dup__text_51983000": [221, 238, 239, 240],
  "gyl-price_dup__coin-wrapper_51983000": [0, 13, 61, 155, 241, 154, 160, 215, 242],
  "gyl-price_dup__coin-text_51983000": [35, 160, 243, 119],
  "gyl-price_dup__coin-img_51983000": [62, 63, 244],
  "gyl-price_dup__tag--h5_51983000": [245],
  "gyl-price_dup__scroll_51983000": [239],
  "gyl-price_dup__scroll--integer_51983000": [246, 14, 234],
  "gyl-price_dup__scroll--decimal_51983000": [246, 14, 231],
  "gyl-price_dup__scroll--point_51983000": [118, 146, 117, 234],
  "gyl-price_dup__space--ml-1_51983000": [120],
  "gyl-price_dup__space--ml-4_51983000": [247],
  "gyl-price_dup__space--mx-1-5_51983000": [248, 249],
  "gyl-price_dup__space--ml-2_51983000": [227],
  "gyl-recommend-reason_dup__container_47842000": [0, 149, 13, 61],
  "gyl-recommend-reason_dup__header_47842000": [136],
  "gyl-recommend-reason_dup__header--android_47842000": [250],
  "gyl-recommend-reason_dup__icon-left--android_47842000": [251],
  "gyl-recommend-reason_dup__text_47842000": [147],
  "gyl-recommend-reason_dup__countdown_47842000": [146, 36],
  "gyl-scroll-to-number_dup__container_369000": [0, 1, 14, 23, 3],
  "gyl-scroll-to-number_dup__wrap_369000": [15, 17, 18, 0, 1, 14, 5, 252, 253],
  "gyl-scroll-to-number_dup__item_369000": [0, 232, 149, 107, 233, 5],
  "gyl-scroll-to-number_dup__number_369000": [5, 16, 50, 117, 146, 221],
  "gyl_dup_buy_together-main_pic-bubble-pic_38422000": [254, 255],
  "gyl_dup_buy_together-main_pic-bubble-stroke_38422000": [15, 256, 257, 57, 258],
  "gyl_dup_buy_together-main_pic-bubble-container_38422000": [259, 172, 54, 260, 144, 261, 15, 262],
  "gyl_dup_buy_together-main_pic-bubble-title_38422000": [263],
  "gyl_dup_buy_together-main_pic-bubble-title-text_38422000": [264, 97, 36, 160],
  "gyl_dup_buy_together-main_pic-bubble-price-integer_38422000": [265, 117, 122, 36, 35, 48, 266],
  "gyl_dup_buy_together-main_pic-bubble-price-label_38422000": [265, 97, 122, 36, 48],
  "gyl-sku_dup_pics_35645000": [0, 61],
  "gyl-sku_dup_pics-cover_35645000": [79, 207, 208, 209],
  "gyl-sku_dup_pics-mask_35645000": [15, 18, 17, 207, 208, 210, 79],
  "gyl-cover_dup__cover-blur_35681000": [267, 3, 6, 239, 268],
  "gyl-cover_dup__cover-mask_35681000": [15, 269, 270, 17, 18, 271],
  "gyl-cover_dup__cover-blur-img_35681000": [6, 272, 269, 270, 273, 274, 275],
  "gyl-cover_dup__cover-blur-container_35681000": [15, 17, 18, 5, 16, 0, 1, 61, 144, 276],
  "gyl-cover_dup__cover-blur-sku_products_35681000": [0, 1, 61],
  "gyl-cover_dup__cover-blur-sku_products-image_35681000": [277, 172],
  "gyl-cover_dup__cover-blur-sku_products-price_35681000": [226, 117, 186, 36, 278],
  "gyl-cover_dup__cover-blur-sku_products-price-container_35681000": [0, 220],
  "gyl-suspend_dup__btn_27959000": [0, 61, 23, 279, 164, 219, 79, 280],
  "gyl-suspend_dup__line_27959000": [281, 257, 282, 283],
  "gyl-suspend_dup__topic_27959000": [0, 13, 61],
  "gyl-suspend_dup__topic-text_27959000": [9],
  "gyl-suspend_dup__background_27959000": [284],
  "gyl-suspend_dup__coin_27959000": [0, 23, 61],
  "gyl-suspend_dup__coin-text_27959000": [50],
  "gyl-suspend--store_dup_27959000": [0, 61, 181, 285, 26, 286, 287],
  "gyl-suspend--store-text_dup_27959000": [50, 288, 3, 289],
  "gyl-suspend_dup__guid_27959000": [0, 13, 61],
  "gyl-suspend_dup__guide_new_27959000": [0, 13, 61],
  "gyl-suspend_dup__guide-icon_right_27959000": [290],
  "gyl-suspend_dup__coupon-bar_27959000": [0, 13, 61, 154, 291, 21, 22],
  "gyl-suspend_dup__coupon-bar--countdown-completed_27959000": [292, 293],
  "gyl-suspend_dup__coupon-bar--anim-box_27959000": [170, 16, 3],
  "gyl-suspend_dup__coupon-bar--anim-box-offset_27959000": [5, 15, 18, 88, 17, 294],
  "gyl-suspend_dup__coupon-bar--anim-box-loading_27959000": [295],
  "gyl-suspend_dup__coupon-bar-anim-box-item_27959000": [0, 61, 5, 16, 296, 3],
  "gyl-suspend_dup__coupon-bar-text_27959000": [146, 35, 297, 36, 147, 149, 117]
};

function $cardConstructor(componentId, path) {
  let _a;
  $cardOptions = $cardOptions != null ? $cardOptions : {};
  $cardOptions.data = (_a = $cardOptions.data) != null ? _a : {};
  $cardOptions._componentId = componentId;
  $cardOptions._uniqueId = componentId;
  $cardOptions._data = {};
  $cardOptions.forCache = {};
  $cardOptions.ownerCache = {};
  $cardOptions.slots = {};
  $cardOptions.$path = path;
  $cardInstance = $cardOptions;
  return $cardInstance;
}

function case10DataProcessor(raw) {
  let data = raw || {};
  let seed = data.seed || 0;
  let limit = data.limit || 1;
  let label = data.label || "default";
  let cardStyle = $selectorStyleIndexMap["card_27472000"];
  let productStyle = $selectorStyleIndexMap["product_27472000"];
  let adCommonStyle = $selectorStyleIndexMap["ad-common_27472000"];
  let beltStyle = $selectorStyleIndexMap["belt_common_27472000"];
  let couponSymbolStyle = $selectorStyleIndexMap["product-coupon-symbol_21316000"];
  let dupCardStyle = $selectorStyleIndexMap2["card_dup_27472000"];
  let dupProductStyle = $selectorStyleIndexMap2["product_dup_27472000"];
  let dupAdCommonStyle = $selectorStyleIndexMap2["ad-common_dup_27472000"];
  let pickA = cardStyle[seed];
  let dupAnchor = dupCardStyle[0] + dupCardStyle[1] + dupCardStyle[2] + dupCardStyle[3] + dupProductStyle[0] + dupProductStyle[1] + dupProductStyle[2] + dupProductStyle[3] + dupProductStyle[4] + dupProductStyle[5] + dupAdCommonStyle[0] + dupAdCommonStyle[1] + dupAdCommonStyle[2] + dupAdCommonStyle[3] + dupAdCommonStyle[4] + dupAdCommonStyle[5];
  let pickB = adCommonStyle[1] + adCommonStyle[2] + adCommonStyle[3] + adCommonStyle[4] + adCommonStyle[5] + 7 + dupAnchor - 97;
  let pickC = couponSymbolStyle[0] + couponSymbolStyle[1] + couponSymbolStyle[2] + couponSymbolStyle[3] + couponSymbolStyle[4] - 177;
  let stable = cardStyle[0] + cardStyle[1] + cardStyle[2] + cardStyle[3] + productStyle[0] + productStyle[1] + productStyle[2] + productStyle[3] + productStyle[4] + productStyle[5] + beltStyle[0] + beltStyle[1] + beltStyle[2] + 484;
  let bias = stable + pickA + pickB - pickC;
  let className = getClassName("case10-summary");
  let maskStyle = getMaskPosition(seed % 2 === 0 ? "1" : "2");
  let fontScale = getFontScale();
  let appVersion = getAppVersion();
  return {
    seed: seed,
    limit: limit,
    label: label,
    pickA: pickA,
    pickB: pickB,
    pickC: pickC,
    stable: stable,
    bias: bias,
    className: className,
    maskStyle: maskStyle,
    fontScale: fontScale,
    appVersion: appVersion
  };
}

function normalizeTemplateState(raw, mode) {
  let sum0 = raw.pickA + raw.pickB;
  let sum1 = raw.pickC + raw.stable;
  let total = 0;
  for (let i = 0; i < raw.limit; i = i + 1) {
    let lane = i % 2 === 0 ? sum0 + raw.seed + i : sum1 - raw.seed - i;
    total = total + lane;
  }
  let checksum = total + raw.bias + raw.limit;
  let accent = raw.pickB + raw.pickC + raw.limit;
  let summary = mode + "|" + raw.label + "|" + total + "|" + checksum + "|" + raw.pickC;
  let detail = "bias=" + raw.bias + "|stable=" + raw.stable + "|pick=" + raw.pickA + "," + raw.pickB + "," + raw.pickC;
  return {
    seed: raw.seed,
    limit: raw.limit,
    label: raw.label,
    pickA: raw.pickA,
    pickB: raw.pickB,
    pickC: raw.pickC,
    stable: raw.stable,
    bias: raw.bias,
    total: total,
    checksum: checksum,
    accent: accent,
    className: raw.className,
    maskStyle: raw.maskStyle,
    fontScale: raw.fontScale,
    appVersion: raw.appVersion,
    summary: summary,
    detail: detail
  };
}

function renderRoot($root, $data) {
  let summary = $root.children[0];
  let metrics = $root.children[1];
  if (!summary) {
    summary = __CreateText($currentComponentId);
    __AppendElement($root, summary);
  }
  if (!metrics) {
    metrics = __CreateText($currentComponentId);
    __AppendElement($root, metrics);
  }
  __SetAttribute(summary, "class", $data.className);
  __SetAttribute(summary, "text", $data.summary);
  __SetAttribute(metrics, "mask-style", $data.maskStyle);
  __SetAttribute(metrics, "font-scale", $data.fontScale);
  __SetAttribute(metrics, "version", $data.appVersion);
  __SetAttribute(metrics, "text", $data.detail);
}

renderPage = function ($renderData) {
  __globalProps = lynx.__globalProps;
  SystemInfo = lynx.SystemInfo;
  $page = __CreatePage("0", 0);
  $cardInstance = $cardConstructor($currentComponentId, "src/cards/case10/index");
  if ($renderData) {
    Object.assign($cardInstance.data, $renderData);
  }
  let processed = processData($cardInstance.data, "case10_data_processor");
  let nextData = normalizeTemplateState(processed, "render");
  $cardInstance._data = {};
  $cardInstance.data = nextData;
  let $n1 = __CreateView($currentComponentId);
  __AppendElement($page, $n1);
  renderRoot($n1, nextData);
  __FlushElementTree($page);
  return true;
};

updatePage = function ($newData, options) {
  let merged = $deepClone($cardInstance.data);
  Object.assign(merged, $newData || {});
  let processed = processData(merged, "case10_data_processor");
  let nextData = normalizeTemplateState(processed, "update");
  let diff = __GetDiffData($cardInstance.data, nextData, options);
  $cardInstance._data = $deepClone($cardInstance.data);
  $cardInstance.data = diff.nextData;
  $update = true;
  renderRoot($page.children[0], diff.nextData);
  __FlushElementTree($page);
  return diff;
};

registerDataProcessor(case10DataProcessor, "case10_data_processor");
renderPage({ seed: 1, limit: 2, label: "alpha" });
let diff = updatePage({ seed: 0, limit: 6, label: "omega" }, { mode: "case10" });
let root = $page.children[0];
var result = root.children[0].text + "|" + root.children[1].text;

Assert(diff.changed === true, "case10: diff.changed");
Assert($update === true, "case10: update flag");
Assert($cardInstance.data.total === 2667, "case10: total");
Assert($cardInstance.data.checksum === 3061, "case10: checksum");
Assert($cardInstance.data.bias === 388, "case10: bias");
Assert($cardInstance.data.accent === 323, "case10: accent");
Assert(root.children[0].attrs["class"] === "case10-summary mall-font-large", "case10: summary class");
Assert(root.children[1].attrs["font-scale"] === 1.15, "case10: font scale");
Assert(root.children[1].attrs["version"] === 1, "case10: version");
Assert(root.children[1].attrs["mask-style"] === "width: 180.5px;height: 286.507936507937px", "case10: mask style");
Assert(root.children[0].text === "update|omega|2667|3061|251", "case10: summary text");
Assert(root.children[1].text === "bias=388|stable=573|pick=0,66,251", "case10: detail text");
Assert(result === "update|omega|2667|3061|251|bias=388|stable=573|pick=0,66,251", "case10: result");
Assert(__assert_fail_count === 0);
