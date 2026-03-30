// [TEST_TARGET: IR]
let lynx = {}
function toPrice(price) {
  return price / 100;
}
function __GetElementByUniqueID(a){}
function __GetElementUniqueID(a){}
function __CreateView(a) {}
function __SetAttribute(a, b, c){}
function __AppendElement(a, b) {}
function __CreateText(a) {}
function __CreateIf(a) {}
function __UpdateIfNodeIndex(a, b) {}
function __CreateImage(a) {}
function __GetDiffData(a, b, c) {}
function __FlushElementTree(a) {}
function __CreatePage(a) {}

function toSales(sales) {
  let salesShow = "";
  if (sales < 1e4) {
    salesShow = sales;
  } else {
    salesShow = sales / 1e4 + "万";
  }
  return salesShow;
}
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
  return lepusUniqueId * 65536 | lepusId;
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
  let hash = $getLepusHash(uniqId, lepusId);
  let elementId = $lepusElementLepusIdMap[hash + ""];
  if (elementId) {
    return __GetElementByUniqueID(elementId);
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
  let uniqueId = __GetElementUniqueID(elementRef);
  let hash = $getLepusHash(uniqId, lepusId);
  $lepusElementLepusIdMap[hash + ""] = uniqueId;
  return [uniqueId, lepusUniqueId];
};
let renderPage = null;
let updatePage = null;
let $cardVariables = [];
let $varUpdateState = [];
let $conditionNodeIndex = {};
$cardOptions = {
};
function $$update_3bff218_4($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[0]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.product.rec_reason.content) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n5 = $update2 ? $lepusGetElementRefByLepusID("text", 5) : null;
          let $temp2 = $update2;
          if (!$n5) {
            $update2 = false;
            $n5 = __CreateText($currentComponentId);
            let $nid5 = $lepusStoreElementRefByLepusID($n5, 5, "text");
            __SetAttribute($n5, 1004, $nid5[1]);
            __SetStyleObject($n5, [4, 7, 8]);
            __AppendElement($parent, $n5);
          }
          {
            if (!$update2 || $varUpdateState[0]) {
              let $value = $data.product.rec_reason.content;
              if (!$update2 || $value !== $cardInstance._data.product.rec_reason.content) {
                __SetAttribute($n5, "text", $value);
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
}
function $$update_3bff218_8($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[0]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.product.product_icon) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n9 = $update2 ? $lepusGetElementRefByLepusID("image", 9) : null;
          let $temp2 = $update2;
          if (!$n9) {
            $update2 = false;
            $n9 = __CreateImage($currentComponentId);
            let $nid9 = $lepusStoreElementRefByLepusID($n9, 9, "image");
            __SetAttribute($n9, 1004, $nid9[1]);
            __SetAttribute($n9, "skip-redirection", true);
            __AppendElement($parent, $n9);
          }
          if (!$update2 || $varUpdateState[0]) {
            {
              let $value = "width:" + ($data.product.product_icon.width / $data.product.product_icon.height * 28 + "rpx;") + "height:28rpx;margin-right:4rpx;linear-layout-gravity:center-vertical;";
              if (!$update2 || $value !== "width:" + ($cardInstance._data.product.product_icon.width / $cardInstance._data.product.product_icon.height * 28 + "rpx;") + "height:28rpx;margin-right:4rpx;linear-layout-gravity:center-vertical;") {
                __SetStyleObject($n9, [10, 11, 12, {
                  27: $data.product.product_icon.width / $data.product.product_icon.height * 28 + "rpx"
                }]);
              }
            }
            {
              let _$value = $data.product.product_icon.url_list[0];
              if (!$update2 || _$value !== $cardInstance._data.product.product_icon.url_list[0]) {
                __SetAttribute($n9, "src", _$value);
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
}
function $$update_25e4af0_17($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[0] || $varUpdateState[1]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.product.tags) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n18 = $update2 ? $lepusGetElementRefByLepusID("text", 18) : null;
          let $temp2 = $update2;
          if (!$n18) {
            $update2 = false;
            $n18 = __CreateText($currentComponentId);
            let $nid18 = $lepusStoreElementRefByLepusID($n18, 18, "text");
            __SetAttribute($n18, 1004, $nid18[1]);
            __SetStyleObject($n18, [19, 20, 21, 15, 16]);
            __AppendElement($parent, $n18);
          }
          {
            if (!$update2 || $varUpdateState[0] || $varUpdateState[1]) {
              __SetAttribute($n18, "text", "已售" + toSales($data.product.sales));
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
          let $n20 = $update2 ? $lepusGetElementRefByLepusID("text", 20) : null;
          let _$temp2 = $update2;
          if (!$n20) {
            $update2 = false;
            $n20 = __CreateText($currentComponentId);
            let $nid20 = $lepusStoreElementRefByLepusID($n20, 20, "text");
            __SetAttribute($n20, 1004, $nid20[1]);
            __SetStyleObject($n20, [22, 20, 23, 24, 25, 26, 16]);
            __AppendElement($parent, $n20);
          }
          {
            if (!$update2 || $varUpdateState[0]) {
              let $value = $data.product.tags[0].content;
              if (!$update2 || $value !== $cardInstance._data.product.tags[0].content) {
                __SetAttribute($n20, "text", $value);
              }
            }
          }
          $update2 = _$temp2;
        }
        $update2 = _$temp;
      }
    }
  }
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
  if ($varUpdateState[0]) {
    let $n2 = $lepusGetElementRefByLepusID("image", 2);
    {
      let $value = $data.product.cover;
      if (!$update || $value !== $cardInstance._data.product.cover) {
        __SetAttribute($n2, "src", $value);
      }
    }
  }
  let $n4 = $lepusGetElementRefByLepusID("if", 4);
  $$update_3bff218_4($n4, $data, $update);
  let $n8 = $lepusGetElementRefByLepusID("if", 8);
  $$update_3bff218_8($n8, $data, $update);
  if ($varUpdateState[0]) {
    let _$value2 = $data.product.title;
    if (_$value2 !== $cardInstance._data.product.title) {
      let $n10 = $lepusGetElementRefByLepusID("text", 10);
      __SetAttribute($n10, "text", _$value2);
    }
  }
  if ($varUpdateState[0] || $varUpdateState[1]) {
    let _$value3 = toPrice($data.product.price.min_price);
    if (_$value3 !== undefined) {
      let $n15 = $lepusGetElementRefByLepusID("text", 15);
      __SetAttribute($n15, "text", _$value3);
    }
  }
  let $n17 = $lepusGetElementRefByLepusID("if", 17);
  $$update_25e4af0_17($n17, $data, $update);
  $array.forEach(function (item) {
    $cardInstance._data[item] = $deepClone($data[item]);
  });
  __FlushElementTree($page);
  return true;
};
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
  __SetStyleObject($n1, [0, 1]);
  __AppendElement($page, $n1);
  let $n2 = __CreateImage($currentComponentId);
  $lepusStoreElementRefByLepusID($n2, 2, "image");
  __SetAttribute($n2, 1004, 2);
  __SetStyleObject($n2, [2, 3]);
  __SetAttribute($n2, "skip-redirection", true);
  __SetAttribute($n2, "src", $data.product.cover);
  __AppendElement($n1, $n2);
  let $n3 = __CreateView($currentComponentId);
  __SetAttribute($n3, 1004, 3);
  __SetStyleObject($n3, [4, 5, 6]);
  __AppendElement($n1, $n3);
  let $n4 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n4, 4, "if");
  __AppendElement($n3, $n4);
  $$update_3bff218_4($n4, $data, $update);
  let $n7 = __CreateView($currentComponentId);
  __SetAttribute($n7, 1004, 7);
  __SetStyleObject($n7, [4, 9]);
  __AppendElement($n3, $n7);
  let $n8 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n8, 8, "if");
  __AppendElement($n7, $n8);
  $$update_3bff218_8($n8, $data, $update);
  let $n10 = __CreateText($currentComponentId);
  $lepusStoreElementRefByLepusID($n10, 10, "text");
  __SetAttribute($n10, 1004, 10);
  __SetStyleObject($n10, [8, 12]);
  __AppendElement($n7, $n10);
  __SetAttribute($n10, "text", $data.product.title);
  let $n12 = __CreateView($currentComponentId);
  __SetAttribute($n12, 1004, 12);
  __SetStyleObject($n12, [4, 9, 13]);
  __AppendElement($n3, $n12);
  let $n13 = __CreateText($currentComponentId);
  __SetAttribute($n13, 1004, 13);
  __SetStyleObject($n13, [14, 8, 15, 16]);
  __AppendElement($n12, $n13);
  __SetAttribute($n13, "text", "¥");
  let $n15 = __CreateText($currentComponentId);
  $lepusStoreElementRefByLepusID($n15, 15, "text");
  __SetAttribute($n15, 1004, 15);
  __SetStyleObject($n15, [17, 14, 18, 16]);
  __AppendElement($n12, $n15);
  __SetAttribute($n15, "text", toPrice($data.product.price.min_price));
  let $n17 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n17, 17, "if");
  __AppendElement($n12, $n17);
  $$update_25e4af0_17($n17, $data, $update);
  $airFirstScreen = false;
  $cardVariables = ["product"];
  return true;
};
//# sourceMappingURL=http://192.168.10.35:8788/slice-product/intermediate/lepus.js.map
