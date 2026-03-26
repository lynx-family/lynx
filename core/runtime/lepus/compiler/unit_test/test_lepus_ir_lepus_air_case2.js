// [TEST_TARGET: IR]
let lynx = {};
let $currentComponentId = 10;
let $lepusElementLepusIdMap = {};
let $cardInstance;
let $page;
let $cardOptions;
let $airFirstScreen = false;
let $update = false;
let $initAppService = false;
let __globalProps;
let $lepusGetElementRefByLepusID;
let $lepusStoreElementRefByLepusID;
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
function $getLepusUniqId(a, b) {
  return (a ^ b) * 31;
}
function $getLepusHash(lepusUniqueId, lepusId) {
  lepusId = lepusId * 103;
  return 0.5 * (lepusUniqueId + lepusId) * (lepusUniqueId + lepusId + 1) + lepusId;
}
function $getKeyForCreatedElement(lepusId) {
  let key = lepusId;
  let uniqueKey = lepusId;
  let forElement = $cardInstance._currentForElement;
  let templateElement = $cardInstance._currentTemplateElement;
  let templateElementId = templateElement ? templateElement["_templateId"] : -1;
  let forElementId = forElement ? forElement["_uniqueId"] : -1;
  let maxId = Math.max(templateElementId, forElementId);
  if (maxId === -1) {
    return [key, key];
  }
  if (maxId === templateElementId) {
    key = templateElementId;
    uniqueKey = templateElementId;
  } else if (maxId === forElementId) {
    uniqueKey = $getLepusUniqId(forElementId, forElement["activeIndex"]);
  }
  if (forElementId > 0) {
    key = $getLepusUniqId(forElement["_lepusId"], forElement["activeIndex"]);
  }
  return [key, uniqueKey];
}
$lepusGetElementRefByLepusID = function (tag, lepusId) {
  let _$getKeyForCreatedEle = $getKeyForCreatedElement(lepusId, tag),
      lepusUniqueId = _$getKeyForCreatedEle[0],
      uniqId = _$getKeyForCreatedEle[1];
  let elementId = $lepusElementLepusIdMap[$getLepusHash(uniqId, lepusId)];
  if (elementId) {
    return null;
  }
  return null;
};
function $cardConstructor(componentId) {
  let _a;
  $cardOptions = $cardOptions != null ? $cardOptions : {};
  $cardOptions.data = (_a = $cardOptions.data) != null ? _a : {};
  $cardOptions._componentId = componentId;
  $cardOptions._uniqueId = componentId;
  $cardOptions._data = {};
  $cardOptions.forCache = {};
  $cardOptions._currentForElement = undefined;
  $cardOptions._currentComponentElement = undefined;
  $cardOptions._currentTemplateElement = undefined;
  $cardInstance = $cardOptions;
  return $cardInstance;
}
$lepusStoreElementRefByLepusID = function (elementRef, lepusId, tag) {
  let _$getKeyForCreatedEle2 = $getKeyForCreatedElement(lepusId, tag),
      lepusUniqueId = _$getKeyForCreatedEle2[0],
      uniqId = _$getKeyForCreatedEle2[1];
  let uniqueId = 111;
  $lepusElementLepusIdMap[$getLepusHash(uniqId, lepusId)] = uniqueId;
  return [uniqueId, lepusUniqueId];
};
let renderPage = null;
let updatePage = null;
let $cardVariables = [];
let $varUpdateState = [];
$cardOptions = {
  data: {
    "product": {
      "product_id": "3605885711154301084",
      "title": "森马集团GENIOLAMODE我只想睡觉短袖t恤男纯棉高街潮ins百搭半袖",
      "sales": 6985,
      "price": {
        "min_price": 2990,
        "max_price": 0,
        "market_price": 4990,
        "show_price": 2990,
        "show_price_type": "min_price",
        "show_sku_id": "1760686381335604"
      },
      "sec_kol_id": "",
      "promotion_id": "3605885711154301084",
      "commodity_type": 6,
      "recommend_info": '{"uid":51006002173,"gid":3605885711154301084,"ghs5":"ws0e6","chr":0,"bat":0,"har":0,"ohr":0,"prd":{"pnum":2,"pname":"xtab_homepage","rsn":"ecom_center_u2i_ab1:399:0.448593,vk_gul_price:155:0.463420,vk_gul:196:0.517274","new":1,"thr_new":0,"pay_new":0,"clevel_new":"L5","gender":1,"mcsgb":2,"spmkt":0,"cu_idx":80,"idx":37,"nfs_1h":"0:1:0:0:0:0","nfs_24h":"1:2:1:0:0:0","nfs_all":"7:2:1:0:0:0","impr":13,"act":0,"14d_lctg":0,"qversion":69750268890925,"vk_gul":"0.517274","vk_gul_price":"0.463420","simid":3606230315674302238,"spu_simid":7190281894276383036,"lctg":20172,"ctg1":20009,"ai_ctg":20009,"dprice":2990,"aprice":2899,"sprice":2807.0,"fprice":2590,"promo_user_tag":"","promo_biz":"","promo_type":"","promo_value":"","ai_lctg":20172,"price_comp":2,"price_comp_v2":4,"price_comp_sc":"1.720690","pr_cmp_v3":5,"pr_cmp_v3_rt":5,"pr_cmp_outer":5,"pr_cmp_outer_mix":5,"pr_cmp_outer_mix_rt":5,"pr_cmp_priority":0,"pr_cmp_real_his_out":-1,"pr_cmp_real_his_in":-1,"pr_cmp_his_out":2,"brand":836463623,"brand_pl":"P4","trig_clk":"9,3,3,20073_4,27836_4,20073_27836_2097070","trig_buy":"9,7,8,20115_3,28483_2,20073_27836_2097090","ut_ad":"","ei_basic_fea":"","pred_sc":"0.000001","r_dw_sc":0.0,"prd_clk":"0.001452","ord_sub":"0.023239","prd_cas":"0.074524","bst":7.112880706787109,"be_bst":1.5982273282588721e-7,"ori_ctr":"0.002165","ori_cvr":"0.023239","ori_gcvr":"0.000000","sku_seq_sku_id":"1760686381336619,1760686381336603,1760686381336635,1760686381337611,1760686381336587,1760686381344779,1760686381337643,1760686381344795,1760686381337627,1760686381344811","ori_cu":"0.000001","dcvr":"0.117981","order_high":"0.000355","order_good":"0.999512","rctr_cross":"0.006371","rctr":"0.002449","rcvr_cross":"0.047324","rcvr":"0.038055","rcascade_cross":"0.799798","rcascade":"0.922112","kgplv":3,"rprice":"28.990000","rscore":"29362.7999527825","rscore_ori":"29362.8007812500","rscore_ab":"0.0000000000","merge_pos":1605},"pred":{"clevel_x_pay":"L5_14"},"cs_s":0,"mcs_s":0,"shop_cs":{"cs_s":0,"cs_val":0.0},"chnid":1111868,"chnid_bak":1111868,"ent":"guess_u_like","appid":2329,"cs_force_insert":{"u_cs_ctr":0.0,"u_cs_cvr":0.0,"u_cs_ctcvr":0.0,"u_cs_cu":0.0},"rt":0,"traffic_from":"","pitaya_show_len":0,"pitaya_action_len":0,"pitaya_origin_len":0,"req_id":"202402221737301B0D94620E42E117B008","loc":440105,"tab":0,"sub_req_id":"4275D","sess":"fba889c201f0683a54bf2c854f456b27","zerovk":0,"tsv_num":"2","tsv_ctg1":"20073","tsv_rk":"0","pv_14d":1194463,"cs_pv_thr":1500,"mt_cs":0,"in_rebuy_time":0,"spw":{"enable":0},"impr":{"size":196,"d_size":0,"fg_size":32},"coupon_type":0,"coupon_price":-1,"coupon_thres":0,"llt":1706497583,"m_new_t":"ecom_center_new_user","ctxp":"","pctx":{"sid":0},"mix":{"ctr":0.001452,"cvr":0.023239,"pos":7,"tabid":0,"pnum":2,"is_new":1,"is_new_t":0,"sort_index":37,"rule_fail_num":0,"pctr":0.007931,"pcvr":0.023339,"pgmv":27.461961,"wt":2.25298,"mix_sc":0.431561,"price":29,"type":1003,"pred_pos":30,"sig":0,"dcvr":0.11798,"dcvrl":0.0,"pay_cnt_90d":1},"uplift":{"treatment":0.0,"rand_treatment":0.0,"mix_uplift_version":0.0},"mrc":3}',
      "biz_kind": 0,
      "product_icon": {
        "uri": "",
        "url_list": ["https://test.com/obj/eden-cn/ljylttvjl_lmp/ljhwZthlaukjlkulzlp/ecom_center_fusion/chaozhigou.png"],
        "width": 49,
        "height": 14
      },
      "icon_name": "allowance_goods",
      "extra": {
        "insurance_commodity_flag": 0,
        "similar_link": "sslocal://webcast_webview?type=fullscreen&web_bg_color=ffffff&url=https%3A%2F%2Fffh.jinritemai.com%2Ffalcon%2Fe_commerce%2Fecommerce_webcast_gaia%2Fpages%2Fcentralization%2Fsimilar_find%2Findex.html%3Fproduct_id%3D3605885711154301084&hide_nav_bar=1&title=%E7%9B%B8%E4%BC%BC%E6%8E%A8%E8%8D%90",
        "cover_is_white_bg": 0,
        "tag_track_params": ['"tag_type":"service_guarantee","product_id":"3605885711154301084","tag_code":"support_7days_refund"', '"tag_code":"freight_insurance","tag_type":"service_guarantee","product_id":"3605885711154301084"', '"tag_code":"fast_refund","tag_type":"service_guarantee","product_id":"3605885711154301084"', '"tag_type":"service_guarantee","product_id":"3605885711154301084","tag_code":"fake_compensate_ten"']
      },
      "tags": [{
        "type": 12,
        "content": "7天无理由退货",
        "extra": '{"product_tag":"{\\"tag_type\\":\\"service_guarantee\\",\\"product_id\\":\\"3605885711154301084\\",\\"tag_code\\":\\"support_7days_refund\\"}"}'
      }, {
        "type": 12,
        "content": "运费险",
        "extra": '{"product_tag":"{\\"tag_code\\":\\"freight_insurance\\",\\"tag_type\\":\\"service_guarantee\\",\\"product_id\\":\\"3605885711154301084\\"}"}'
      }, {
        "type": 12,
        "content": "极速退款",
        "extra": '{"product_tag":"{\\"tag_code\\":\\"fast_refund\\",\\"tag_type\\":\\"service_guarantee\\",\\"product_id\\":\\"3605885711154301084\\"}"}'
      }, {
        "type": 12,
        "content": "假一赔十",
        "extra": '{"product_tag":"{\\"tag_type\\":\\"service_guarantee\\",\\"product_id\\":\\"3605885711154301084\\",\\"tag_code\\":\\"fake_compensate_ten\\"}"}'
      }],
      "campagin_sales": 0,
      "campagin_stock": 0,
      "cover_long": {
        "uri": "",
        "url_list": [],
        "width": 0,
        "height": 0
      },
      "cover": "https://p3.ecombdimg.com/img/ecom-shop-material/LjDprJPA_m_59a6ad08699d1b6818bfbfb8b2c32369_sx_123345_www800-800~tplv-5mmsx3fupr-re_cp:500:0:q80.heic?scene_tag=ProductCover_more",
      "cover_white_bg": "",
      "rec_reason": {
        "cover": ""
      },
      "shop_info": {},
      "cover_track": {
        "prod_pic_type": "11_pic",
        "bigsale_border_template_id": "",
        "is_long_pic": "0",
        "is_smart_pic": "0",
        "uri": "ecom-shop-material/LjDprJPA_m_59a6ad08699d1b6818bfbfb8b2c32369_sx_123345_www800-800"
      },
      "sorted_cover": {
        "uri": "ecom-shop-material/LjDprJPA_m_59a6ad08699d1b6818bfbfb8b2c32369_sx_123345_www800-800",
        "url_list": ["https://p3.ecombdimg.com/img/ecom-shop-material/LjDprJPA_m_59a6ad08699d1b6818bfbfb8b2c32369_sx_123345_www800-800~tplv-5mmsx3fupr-re_cp:500:0:q80.heic?scene_tag=ProductCover_more", "https://p6.ecombdimg.com/img/ecom-shop-material/LjDprJPA_m_59a6ad08699d1b6818bfbfb8b2c32369_sx_123345_www800-800~tplv-5mmsx3fupr-re_cp:500:0:q80.heic"],
        "width": 500,
        "height": 0,
        "cover_type": "11_pic",
        "tmpl_code": "",
        "is_from_cover_long": "0",
        "is_from_cover_recommend": "0"
      },
      "common_track": {
        "is_show_inherit_sales": "0"
      }
    }
  }
};
function __GetDiffData(a, b, c) {
    return {};
}

function __FlushElementTree(a) {

}

updatePage = function ($newData, options) {
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
  $array.forEach(function (item) {
    $cardInstance._data[item] = $deepClone($data[item]);
  });
  __FlushElementTree($page);
  return true;
};

function __CreatePage(a, b) {
    return {};
}
function __CreateView(a) {
    return {};
}
function __SetAttribute(a, b, c) {
}
function __AddEvent(a, b, c, d) {
}
function __AppendElement(a, b) {
}


renderPage = function ($renderData) {
  __globalProps = lynx.__globalProps;
  $airFirstScreen = true;
  $page = __CreatePage("0", 0);
  $cardInstance = $cardConstructor($currentComponentId);
  if ($renderData) {
    Object.assign($cardInstance.data, $renderData);
  }
  let $data = $cardInstance.data;
  let $n1 = __CreateView($currentComponentId);
  __SetAttribute($n1, 1004, 1);
  __AddEvent($n1, "bindEvent", "tap", $data.type === "test" ? "onTestClick" : "onClick");
  __AppendElement($page, $n1);
  $airFirstScreen = false;
  $cardVariables = ["type"];
  return true;
};
//# sourceMappingURL=http://10.91.108.134:8787/slice-product/intermediate/lepus.js.map
