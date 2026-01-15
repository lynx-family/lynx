let Enums2;
(function (Enums3) {
  let ColorType;
  (function (ColorType2) {
    ColorType2['PURE'] = 'pure';
    ColorType2['GRADIENT'] = 'gradient';
  })((ColorType = Enums3.ColorType || (Enums3.ColorType = {})));
  let RefreshEventType;
  (function (RefreshEventType2) {
    RefreshEventType2[(RefreshEventType2['START'] = 1)] = 'START';
    RefreshEventType2[(RefreshEventType2['END'] = 2)] = 'END';
  })(
    (RefreshEventType =
      Enums3.RefreshEventType || (Enums3.RefreshEventType = {}))
  );
  let PROMOTION_PAGE_TYPE;
  (function (PROMOTION_PAGE_TYPE2) {
    PROMOTION_PAGE_TYPE2['FLASH_SALE_PROMOTION_PAGE'] =
      'flash_sale_promotion_page';
    PROMOTION_PAGE_TYPE2['NEW_USER_PROMOTION_PAGE'] = 'new_user_promotion_page';
  })(
    (PROMOTION_PAGE_TYPE =
      Enums3.PROMOTION_PAGE_TYPE || (Enums3.PROMOTION_PAGE_TYPE = {}))
  );
  let UserRightsEntranceType;
  (function (UserRightsEntranceType2) {
    UserRightsEntranceType2['Normal'] = 'normal';
    UserRightsEntranceType2['Guarantee'] = 'guarantee';
  })(
    (UserRightsEntranceType =
      Enums3.UserRightsEntranceType || (Enums3.UserRightsEntranceType = {}))
  );
  let LayoutType;
  (function (LayoutType2) {
    LayoutType2['Default'] = 'layout_default';
    LayoutType2['Background'] = 'layout_background';
    LayoutType2['Flat'] = 'layout_flat';
    LayoutType2['ScrollX'] = 'layout_scroll_x';
    LayoutType2['ScrollY'] = 'layout_scroll_y';
    LayoutType2['Collapse'] = 'layout_collapse';
  })((LayoutType = Enums3.LayoutType || (Enums3.LayoutType = {})));
  let LayoutKey;
  (function (LayoutKey2) {
    LayoutKey2['FreeShipping'] =
      'mall_home_new_user_exclusive_deal_free_shipping';
    LayoutKey2['FirstOrder'] = 'mall_home_new_user_exclusive_deal_first_order';
    LayoutKey2['FlashSale'] = 'mall_home_flash_sale';
    LayoutKey2['FlashSaleHalf'] = 'mall_home_flash_sale_half';
    LayoutKey2['LiveHalf'] = 'live_channel';
    LayoutKey2['LiveChannelOneFourth'] = 'live_channel_one_fourth';
    LayoutKey2['CouponHalf'] = 'coupon';
    LayoutKey2['CouponChannelOneFourth'] = 'coupon_channel_one_fourth';
    LayoutKey2['PromotionBanner'] = 'promotion_banner';
    LayoutKey2['PromotionBackground'] = 'promotion_background';
    LayoutKey2['RecommendFeed'] = 'recommend_feed';
    LayoutKey2['BrandHalf'] = 'brand_channel_half';
    LayoutKey2['UserRights'] = 'mall_home_user_rights';
    LayoutKey2['RacunHalf'] = 'racun_channel';
    LayoutKey2['RankingChannelOneHalf'] = 'ranking_channel_one_half';
    LayoutKey2['RankingChannelOneFourth'] = 'ranking_channel_one_fourth';
    LayoutKey2['FlexLayoutCardContainer'] = 'flex_layout_card_container';
  })((LayoutKey = Enums3.LayoutKey || (Enums3.LayoutKey = {})));
  let NativeDataType;
  (function (NativeDataType2) {
    NativeDataType2['PrefetchCache'] = 'prefetch_cache';
    NativeDataType2['Prefetch'] = 'prefetch';
    NativeDataType2['Refresh'] = 'refresh';
    NativeDataType2['ApiErrorRetry'] = 'api_error_retry';
    NativeDataType2['TemplateErrorRetry'] = 'template_error_retry';
    NativeDataType2['NetworkUnavailableRetry'] = 'network_unavailable_retry';
  })((NativeDataType = Enums3.NativeDataType || (Enums3.NativeDataType = {})));
  let DATASOURCE;
  (function (DATASOURCE2) {
    DATASOURCE2['Initial'] = 'initial';
    DATASOURCE2['LocalStorage'] = 'local_storage';
    DATASOURCE2['Fetch'] = 'prefetch';
    DATASOURCE2['Refresh'] = 'refresh';
    DATASOURCE2['Error'] = 'error';
  })((DATASOURCE = Enums3.DATASOURCE || (Enums3.DATASOURCE = {})));
  let UserType;
  (function (UserType2) {
    UserType2['NewUser'] = 'new_user';
    UserType2['OldUser'] = 'old_user';
  })((UserType = Enums3.UserType || (Enums3.UserType = {})));
  let OpsBlockType;
  (function (OpsBlockType2) {
    OpsBlockType2['NEW_USER'] = 'mall_home_new_user_exclusive_deal';
    OpsBlockType2['FLASH_SALE'] = 'mall_home_flash_sale';
    OpsBlockType2['LIVE'] = 'mall_home_live';
    OpsBlockType2['UNKNOWN'] = 'unknown';
  })((OpsBlockType = Enums3.OpsBlockType || (Enums3.OpsBlockType = {})));
  let OpsSkinType;
  (function (OpsSkinType2) {
    OpsSkinType2[(OpsSkinType2['DEFAULT'] = 0)] = 'DEFAULT';
  })((OpsSkinType = Enums3.OpsSkinType || (Enums3.OpsSkinType = {})));
  let RacunCardType;
  (function (RacunCardType2) {
    RacunCardType2['Live'] = 'live';
    RacunCardType2['Video'] = 'video';
  })((RacunCardType = Enums3.RacunCardType || (Enums3.RacunCardType = {})));
  let LiveCardType;
  (function (LiveCardType2) {
    LiveCardType2['Product'] = 'product';
    LiveCardType2['Live'] = 'live';
  })((LiveCardType = Enums3.LiveCardType || (Enums3.LiveCardType = {})));
})(Enums2 || (Enums2 = {}));

let obj_actual = {
  ColorType: { PURE: 'pure', GRADIENT: 'gradient' },
  RefreshEventType: { 1: 'START', 2: 'END', START: 1, END: 2 },
  PROMOTION_PAGE_TYPE: {
    FLASH_SALE_PROMOTION_PAGE: 'flash_sale_promotion_page',
    NEW_USER_PROMOTION_PAGE: 'new_user_promotion_page',
  },
  UserRightsEntranceType: { Normal: 'normal', Guarantee: 'guarantee' },
  LayoutType: {
    Default: 'layout_default',
    Background: 'layout_background',
    Flat: 'layout_flat',
    ScrollX: 'layout_scroll_x',
    ScrollY: 'layout_scroll_y',
    Collapse: 'layout_collapse',
  },
  LayoutKey: {
    FreeShipping: 'mall_home_new_user_exclusive_deal_free_shipping',
    FirstOrder: 'mall_home_new_user_exclusive_deal_first_order',
    FlashSale: 'mall_home_flash_sale',
    FlashSaleHalf: 'mall_home_flash_sale_half',
    LiveHalf: 'live_channel',
    LiveChannelOneFourth: 'live_channel_one_fourth',
    CouponHalf: 'coupon',
    CouponChannelOneFourth: 'coupon_channel_one_fourth',
    PromotionBanner: 'promotion_banner',
    PromotionBackground: 'promotion_background',
    RecommendFeed: 'recommend_feed',
    BrandHalf: 'brand_channel_half',
    UserRights: 'mall_home_user_rights',
    RacunHalf: 'racun_channel',
    RankingChannelOneHalf: 'ranking_channel_one_half',
    RankingChannelOneFourth: 'ranking_channel_one_fourth',
    FlexLayoutCardContainer: 'flex_layout_card_container',
  },
  NativeDataType: {
    PrefetchCache: 'prefetch_cache',
    Prefetch: 'prefetch',
    Refresh: 'refresh',
    ApiErrorRetry: 'api_error_retry',
    TemplateErrorRetry: 'template_error_retry',
    NetworkUnavailableRetry: 'network_unavailable_retry',
  },
  DATASOURCE: {
    Initial: 'initial',
    LocalStorage: 'local_storage',
    Fetch: 'prefetch',
    Refresh: 'refresh',
    Error: 'error',
  },
  UserType: { NewUser: 'new_user', OldUser: 'old_user' },
  OpsBlockType: {
    NEW_USER: 'mall_home_new_user_exclusive_deal',
    FLASH_SALE: 'mall_home_flash_sale',
    LIVE: 'mall_home_live',
    UNKNOWN: 'unknown',
  },
  OpsSkinType: { 0: 'DEFAULT', DEFAULT: 0 },
  RacunCardType: { Live: 'live', Video: 'video' },
  LiveCardType: { Product: 'product', Live: 'live' },
};

Assert(obj_actual == Enums2);

function ftest(string) {
  return string;
}

let obj = {
  prop: 'test1',
};

Assert(ftest((obj.prop += 'test2')) == 'test1test2');

function ftest2(arg1, arg2) {
  return arg1 + arg2;
}

Assert(ftest2('test3', (obj.prop += 'test4')) == 'test3test1test2test4');
