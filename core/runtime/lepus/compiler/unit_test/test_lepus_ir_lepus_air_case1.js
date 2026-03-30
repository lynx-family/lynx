// [TEST_TARGET: IR]
let $$update_2ef9e10_15;
let $$update_2ef9e10_23;
let $$update_2ef9e10_26;
let $$update_2ef9e10_28;
let $$update_2ef9e10_38;
let $$update_2ef9e10_40;
let $$update_2ef9e10_51;
let $$update_2ef9e10_53;
let $$update_100f540_7;
let $$update_100f540_9;
let $$update_100f540_10;
let $$update_100f540_20;
let $$update_100f540_44;
let $$update_100f540_46;
let $$update_100f540_111;
let $$update_100f540_117;
let $$update_1a82dd8_30;
let $$update_1a82dd8_84;
let $$update_1a82dd8_90;
let $$update_1a82dd8_93;
let $$update_1a82dd8_97;
let $$update_1a82dd8_102;
let $$update_1a82dd8_105;
let $$update_1a82dd8_108;
let $$update_1a82dd8_113;
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
function __GetElementByUniqueID(a) {}
$lepusGetElementRefByLepusID = function (tag, lepusId) {
  let _$getKeyForCreatedEle = $getKeyForCreatedElement(lepusId, tag),
      lepusUniqueId = _$getKeyForCreatedEle[0],
      uniqId = _$getKeyForCreatedEle[1];
  let elementId = $lepusElementLepusIdMap[$getLepusHash(uniqId, lepusId)];
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
function $lepusUpdateFiberForNodeIndex(index) {
  $cardInstance._currentForElement.activeIndex = index;
}
function __GetElementUniqueID(a) {

}
$lepusStoreElementRefByLepusID = function (elementRef, lepusId, tag) {
  let _$getKeyForCreatedEle2 = $getKeyForCreatedElement(lepusId, tag),
      lepusUniqueId = _$getKeyForCreatedEle2[0],
      uniqId = _$getKeyForCreatedEle2[1];
  let uniqueId = __GetElementUniqueID(elementRef);
  $lepusElementLepusIdMap[$getLepusHash(uniqId, lepusId)] = uniqueId;
  return [uniqueId, lepusUniqueId];
};
let renderPage = null;
let updatePage = null;
let $cardVariables = [];
let $varUpdateState = [];
let $conditionNodeIndex = {};
$cardOptions = {
  data: {}
};
function __UpdateIfNodeIndex(a, b) {}
function __CreateImage(a){}
function __SetAttribute(a,b,c){}
function __AppendElement(a,b) {}
function __CreateIf(a) {}
function _GetLength(a) {}
function __UpdateForChildCount(a, b){}
function __CreateText(a) {}
function __CreateView(a) {}
function __CreateFor(a) {}
function __CreateElement(a, b) {}
function __SetID(a,b) {}
let lynx = {};
function __GetDiffData(a, b, c) {}
function __FlushElementTree(a) {}
function __CreatePage(a, b) {}
function __AddEventListener(a, b, c, d) {}

function $$update_2ef9e10_13($parent, $data, $update2, index, item, tagIndex, tagItem) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 2 && item.img_info != null) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n14 = $update2 ? $lepusGetElementRefByLepusID("image", 14) : null;
        let $temp2 = $update2;
        if (!$n14) {
          $update2 = false;
          $n14 = __CreateImage($currentComponentId);
          let $nid14 = $lepusStoreElementRefByLepusID($n14, 14, "image");
          __SetAttribute($n14, 1004, $nid14[1]);
          __SetAttribute($n14, "mode", "scaleToFill");
          __AppendElement($parent, $n14);
        }
        __SetStyleObject($n14, [22, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          12: (item.img_info.radius != null ? item.img_info.radius : 0) + "px"
        }, {
          26: item.img_info.height * parseFloat($data.control_info.font_scale) + "px"
        }, {
          27: item.img_info.width * parseFloat($data.control_info.font_scale) + "px"
        }]);
        __SetAttribute($n14, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      let $n15 = $update2 ? $lepusGetElementRefByLepusID("if", 15) : null;
      if (!$n15) {
        $update2 = false;
        $n15 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n15, 15, "if");
        __AppendElement($parent, $n15);
      }
      $$update_2ef9e10_15($n15, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp;
    }
  }
}
$$update_2ef9e10_15 = function ($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 3) {
      __UpdateIfNodeIndex($parent, 4);
      $conditionNodeIndex[uniqueId] = 4;
      let $temp = $update2;
      if ($ifNodeIndex !== 4) {
        $update2 = false;
      }
      {
        let $n16 = $update2 ? $lepusGetElementRefByLepusID("image", 16) : null;
        let $temp2 = $update2;
        if (!$n16) {
          $update2 = false;
          $n16 = __CreateImage($currentComponentId);
          let $nid16 = $lepusStoreElementRefByLepusID($n16, 16, "image");
          __SetAttribute($n16, 1004, $nid16[1]);
          __SetAttribute($n16, "mode", "aspectFill");
          __SetAttribute($n16, "autoplay", "true");
          __SetAttribute($n16, "loop-count", "0");
          __SetAttribute($n16, "flatten", "false");
          __AppendElement($parent, $n16);
        }
        __SetStyleObject($n16, [22, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          12: (item.img_info.radius != null ? item.img_info.radius : 0) + "px"
        }, {
          26: item.img_info.height * parseFloat($data.control_info.font_scale) + "px"
        }, {
          27: item.img_info.width * parseFloat($data.control_info.font_scale) + "px"
        }]);
        __SetAttribute($n16, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
function $$update_2ef9e10_22($parent, $data, $update2, tagIndex, tagItem) {
  if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo = $lepusPushFiberForNode($parent, 22, uniqueId),
        $forLepus = _$lepusPushFiberForNo[0],
        $lastForLepus = _$lepusPushFiberForNo[1];
    let $object = tagItem.items;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      let $n23 = $update2 ? $lepusGetElementRefByLepusID("if", 23) : null;
      if (!$n23) {
        $n23 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n23, 23, "if");
        __AppendElement($parent, $n23);
      }
      $$update_2ef9e10_23($n23, $data, $update2, index, item, tagIndex, tagItem);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}
$$update_2ef9e10_23 = function ($parent, $data, $update2, index, item, tagIndex, tagItem) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 1) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n24 = $update2 ? $lepusGetElementRefByLepusID("text", 24) : null;
        let $temp2 = $update2;
        if (!$n24) {
          $update2 = false;
          $n24 = __CreateText($currentComponentId);
          let $nid24 = $lepusStoreElementRefByLepusID($n24, 24, "text");
          __SetAttribute($n24, 1004, $nid24[1]);
          __SetAttribute($n24, "text-maxline", "1");
          __AppendElement($parent, $n24);
        }
        __SetStyleObject($n24, [20, 21, {
          38: item.left_margin + "px"
        }, {
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#FFFFFF") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false ? "400" : "500"
        }, {
          47: item.text_info.text_size == null ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n24, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp2 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n26 = $update2 ? $lepusGetElementRefByLepusID("if", 26) : null;
      if (!$n26) {
        $update2 = false;
        $n26 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n26, 26, "if");
        __AppendElement($parent, $n26);
      }
      $$update_2ef9e10_26($n26, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp2;
    }
  }
};
$$update_2ef9e10_26 = function ($parent, $data, $update2, index, item, tagIndex, tagItem) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 2 && item.img_info != null) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n27 = $update2 ? $lepusGetElementRefByLepusID("image", 27) : null;
        let $temp2 = $update2;
        if (!$n27) {
          $update2 = false;
          $n27 = __CreateImage($currentComponentId);
          let $nid27 = $lepusStoreElementRefByLepusID($n27, 27, "image");
          __SetAttribute($n27, 1004, $nid27[1]);
          __SetAttribute($n27, "mode", "scaleToFill");
          __AppendElement($parent, $n27);
        }
        __SetStyleObject($n27, [29, 30, 22, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          12: (item.img_info.radius != null ? item.img_info.radius : "0px") + ""
        }, {
          26: item.img_info.height * parseFloat($data.control_info.font_scale) + "px"
        }, {
          27: item.img_info.width * parseFloat($data.control_info.font_scale) + "px"
        }]);
        __SetAttribute($n27, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp3 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      let $n28 = $update2 ? $lepusGetElementRefByLepusID("if", 28) : null;
      if (!$n28) {
        $update2 = false;
        $n28 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n28, 28, "if");
        __AppendElement($parent, $n28);
      }
      $$update_2ef9e10_28($n28, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp3;
    }
  }
};
$$update_2ef9e10_28 = function ($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 3) {
      __UpdateIfNodeIndex($parent, 4);
      $conditionNodeIndex[uniqueId] = 4;
      let $temp = $update2;
      if ($ifNodeIndex !== 4) {
        $update2 = false;
      }
      {
        let $n29 = $update2 ? $lepusGetElementRefByLepusID("image", 29) : null;
        let $temp2 = $update2;
        if (!$n29) {
          $update2 = false;
          $n29 = __CreateImage($currentComponentId);
          let $nid29 = $lepusStoreElementRefByLepusID($n29, 29, "image");
          __SetAttribute($n29, 1004, $nid29[1]);
          __SetAttribute($n29, "mode", "aspectFill");
          __SetAttribute($n29, "autoplay", "true");
          __SetAttribute($n29, "loop-count", "0");
          __AppendElement($parent, $n29);
        }
        __SetStyleObject($n29, [29, 30, 22, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          12: (item.img_info.radius != null ? item.img_info.radius : "0px") + ""
        }, {
          26: item.img_info.height * parseFloat($data.control_info.font_scale) + "px"
        }, {
          27: item.img_info.width * parseFloat($data.control_info.font_scale) + "px"
        }]);
        __SetAttribute($n29, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
function $$update_2ef9e10_35($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 1 && item.text_info != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n36 = $update2 ? $lepusGetElementRefByLepusID("text", 36) : null;
        let $temp2 = $update2;
        if (!$n36) {
          $update2 = false;
          $n36 = __CreateText($currentComponentId);
          let $nid36 = $lepusStoreElementRefByLepusID($n36, 36, "text");
          __SetAttribute($n36, 1004, $nid36[1]);
          __AppendElement($parent, $n36);
        }
        __SetStyleObject($n36, [{
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#161823") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false ? "400" : "500"
        }, {
          47: item.text_info.text_size == null ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n36, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp4 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n38 = $update2 ? $lepusGetElementRefByLepusID("if", 38) : null;
      if (!$n38) {
        $update2 = false;
        $n38 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n38, 38, "if");
        __AppendElement($parent, $n38);
      }
      $$update_2ef9e10_38($n38, $data, $update2, index, item);
      $update2 = _$temp4;
    }
  }
}
$$update_2ef9e10_38 = function ($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 2 && item.img_info != null) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n39 = $update2 ? $lepusGetElementRefByLepusID("image", 39) : null;
        let $temp2 = $update2;
        if (!$n39) {
          $update2 = false;
          $n39 = __CreateImage($currentComponentId);
          let $nid39 = $lepusStoreElementRefByLepusID($n39, 39, "image");
          __SetAttribute($n39, 1004, $nid39[1]);
          __AppendElement($parent, $n39);
        }
        __SetStyleObject($n39, [46, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          12: (item.img_info.radius != null ? item.img_info.radius : 0) + "px"
        }, {
          26: item.img_info.height * parseFloat($data.control_info.font_scale) + "px"
        }, {
          27: item.img_info.width * parseFloat($data.control_info.font_scale) + "px"
        }]);
        __SetAttribute($n39, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp5 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      let $n40 = $update2 ? $lepusGetElementRefByLepusID("if", 40) : null;
      if (!$n40) {
        $update2 = false;
        $n40 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n40, 40, "if");
        __AppendElement($parent, $n40);
      }
      $$update_2ef9e10_40($n40, $data, $update2, index, item);
      $update2 = _$temp5;
    }
  }
};
$$update_2ef9e10_40 = function ($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 3 && item.img_info != null) {
      __UpdateIfNodeIndex($parent, 4);
      $conditionNodeIndex[uniqueId] = 4;
      let $temp = $update2;
      if ($ifNodeIndex !== 4) {
        $update2 = false;
      }
      {
        let $n41 = $update2 ? $lepusGetElementRefByLepusID("image", 41) : null;
        let $temp2 = $update2;
        if (!$n41) {
          $update2 = false;
          $n41 = __CreateImage($currentComponentId);
          let $nid41 = $lepusStoreElementRefByLepusID($n41, 41, "image");
          __SetAttribute($n41, 1004, $nid41[1]);
          __SetAttribute($n41, "autoplay", "true");
          __AppendElement($parent, $n41);
        }
        __SetStyleObject($n41, [{
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          12: (item.img_info.radius != null ? item.img_info.radius : "0px") + ""
        }, {
          26: item.img_info.height * parseFloat($data.control_info.font_scale) + "px"
        }, {
          27: item.img_info.width * parseFloat($data.control_info.font_scale) + "px"
        }]);
        __SetAttribute($n41, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
function $$update_2ef9e10_48($parent, $data, $update2, index, item, tagIndex, tagItem) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 1) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n49 = $update2 ? $lepusGetElementRefByLepusID("text", 49) : null;
        let $temp2 = $update2;
        if (!$n49) {
          $update2 = false;
          $n49 = __CreateText($currentComponentId);
          let $nid49 = $lepusStoreElementRefByLepusID($n49, 49, "text");
          __SetAttribute($n49, 1004, $nid49[1]);
          __SetAttribute($n49, "text-maxline", "1");
          __AppendElement($parent, $n49);
        }
        __SetStyleObject($n49, [20, 21, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#FFFFFF") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false ? "400" : "500"
        }, {
          47: item.text_info.text_size == null ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n49, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp6 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n51 = $update2 ? $lepusGetElementRefByLepusID("if", 51) : null;
      if (!$n51) {
        $update2 = false;
        $n51 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n51, 51, "if");
        __AppendElement($parent, $n51);
      }
      $$update_2ef9e10_51($n51, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp6;
    }
  }
}
$$update_2ef9e10_51 = function ($parent, $data, $update2, index, item, tagIndex, tagItem) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 2 && item.img_info != null) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n52 = $update2 ? $lepusGetElementRefByLepusID("image", 52) : null;
        let $temp2 = $update2;
        if (!$n52) {
          $update2 = false;
          $n52 = __CreateImage($currentComponentId);
          let $nid52 = $lepusStoreElementRefByLepusID($n52, 52, "image");
          __SetAttribute($n52, 1004, $nid52[1]);
          __SetAttribute($n52, "mode", "scaleToFill");
          __AppendElement($parent, $n52);
        }
        __SetStyleObject($n52, [22, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          12: (item.img_info.radius != null ? item.img_info.radius : "0px") + ""
        }, {
          26: item.img_info.height * parseFloat($data.control_info.font_scale) + "px"
        }, {
          27: item.img_info.width * parseFloat($data.control_info.font_scale) + "px"
        }]);
        __SetAttribute($n52, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp7 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      let $n53 = $update2 ? $lepusGetElementRefByLepusID("if", 53) : null;
      if (!$n53) {
        $update2 = false;
        $n53 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n53, 53, "if");
        __AppendElement($parent, $n53);
      }
      $$update_2ef9e10_53($n53, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp7;
    }
  }
};
$$update_2ef9e10_53 = function ($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 3) {
      __UpdateIfNodeIndex($parent, 4);
      $conditionNodeIndex[uniqueId] = 4;
      let $temp = $update2;
      if ($ifNodeIndex !== 4) {
        $update2 = false;
      }
      {
        let $n54 = $update2 ? $lepusGetElementRefByLepusID("image", 54) : null;
        let $temp2 = $update2;
        if (!$n54) {
          $update2 = false;
          $n54 = __CreateImage($currentComponentId);
          let $nid54 = $lepusStoreElementRefByLepusID($n54, 54, "image");
          __SetAttribute($n54, 1004, $nid54[1]);
          __SetAttribute($n54, "mode", "aspectFill");
          __SetAttribute($n54, "autoplay", "true");
          __SetAttribute($n54, "loop-count", "0");
          __AppendElement($parent, $n54);
        }
        __SetStyleObject($n54, [22, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          12: (item.img_info.radius != null ? item.img_info.radius : "0px") + ""
        }, {
          26: item.img_info.height * parseFloat($data.control_info.font_scale) + "px"
        }, {
          27: item.img_info.width * parseFloat($data.control_info.font_scale) + "px"
        }]);
        __SetAttribute($n54, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 5);
      $conditionNodeIndex[uniqueId] = 5;
      let _$temp8 = $update2;
      if ($ifNodeIndex !== 5) {
        $update2 = false;
      }
      {
        let $n55 = $update2 ? $lepusGetElementRefByLepusID("view", 55) : null;
        let _$temp9 = $update2;
        if (!$n55) {
          $update2 = false;
          $n55 = __CreateView($currentComponentId);
          let $nid55 = $lepusStoreElementRefByLepusID($n55, 55, "view");
          __SetAttribute($n55, 1004, $nid55[1]);
          __AppendElement($parent, $n55);
        }
        $update2 = _$temp9;
      }
      $update2 = _$temp8;
    }
  }
};
function $$update_100f540_5($parent, $data, $update2) {
  let _a, _b;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_b = (_a = $data.ui_data.cover_area.cover_top_info) == null ? undefined : _a.tags) == null ? undefined : _b.length) > 0) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n6 = $update2 ? $lepusGetElementRefByLepusID("view", 6) : null;
          let $temp2 = $update2;
          if (!$n6) {
            $update2 = false;
            $n6 = __CreateView($currentComponentId);
            let $nid6 = $lepusStoreElementRefByLepusID($n6, 6, "view");
            __SetAttribute($n6, 1004, $nid6[1]);
            __SetStyleObject($n6, [11, 12, 13, 14, 15, 16, 0, 17, 18, 19]);
            __AppendElement($parent, $n6);
          }
          if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
            let $n7 = $update2 ? $lepusGetElementRefByLepusID("for", 7) : null;
            if (!$n7) {
              $n7 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n7, 7, "for");
              __AppendElement($n6, $n7);
            }
            $$update_100f540_7($n7, $data, $update2);
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
$$update_100f540_7 = function ($parent, $data, $update2) {
  let _a, _b, _c, _d, _e, _f;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo2 = $lepusPushFiberForNode($parent, 7, uniqueId),
        $forLepus = _$lepusPushFiberForNo2[0],
        $lastForLepus = _$lepusPushFiberForNo2[1];
    let $object = $data.ui_data.cover_area.cover_top_info.tags;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let tagIndex = 0; tagIndex < $length; ++tagIndex) {
      $update2 = tagIndex < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(tagIndex);
      let tagItem = $object[tagIndex];
      {
        let $n8 = $update2 ? $lepusGetElementRefByLepusID("view", 8) : null;
        let $temp2 = $update2;
        if (!$n8) {
          $update2 = false;
          $n8 = __CreateView($currentComponentId);
          let $nid8 = $lepusStoreElementRefByLepusID($n8, 8, "view");
          __SetAttribute($n8, 1004, $nid8[1]);
          __AppendElement($parent, $n8);
        }
        __SetStyleObject($n8, [{
          38: tagIndex == 0 ? "0px" : "4px"
        }, {
          35: ((_a = tagItem.padding) == null ? undefined : _a[0]) + "px"
        }, {
          36: ((_b = tagItem.padding) == null ? undefined : _b[2]) + "px"
        }, {
          33: ((_c = tagItem.padding) == null ? undefined : _c[3]) + "px"
        }, {
          34: ((_d = tagItem.padding) == null ? undefined : _d[1]) + "px"
        }, {
          7: (tagItem.background != null ? tagItem.background : "#ffffff00") + ""
        }, {
          12: tagItem.background_round + "px"
        }, {
          51: (tagIndex === ((_f = (_e = $data.ui_data.cover_area.cover_top_info) == null ? undefined : _e.tags) == null ? undefined : _f.length) - 1 ? 1 : 0) + ""
        }]);
        if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
          let $n9 = $update2 ? $lepusGetElementRefByLepusID("for", 9) : null;
          if (!$n9) {
            $n9 = __CreateFor($currentComponentId);
            $lepusStoreElementRefByLepusID($n9, 9, "for");
            __AppendElement($n8, $n9);
          }
          $$update_100f540_9($n9, $data, $update2, tagIndex, tagItem);
        }
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
$$update_100f540_9 = function ($parent, $data, $update2, tagIndex, tagItem) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo3 = $lepusPushFiberForNode($parent, 9, uniqueId),
        $forLepus = _$lepusPushFiberForNo3[0],
        $lastForLepus = _$lepusPushFiberForNo3[1];
    let $object = tagItem.items;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      let $n10 = $update2 ? $lepusGetElementRefByLepusID("if", 10) : null;
      if (!$n10) {
        $n10 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n10, 10, "if");
        __AppendElement($parent, $n10);
      }
      $$update_100f540_10($n10, $data, $update2, index, item, tagIndex, tagItem);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
$$update_100f540_10 = function ($parent, $data, $update2, index, item, tagIndex, tagItem) {
  let _a, _b;
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 1) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n11 = $update2 ? $lepusGetElementRefByLepusID("text", 11) : null;
        let $temp2 = $update2;
        if (!$n11) {
          $update2 = false;
          $n11 = __CreateText($currentComponentId);
          let $nid11 = $lepusStoreElementRefByLepusID($n11, 11, "text");
          __SetAttribute($n11, 1004, $nid11[1]);
          __SetAttribute($n11, "text-maxline", "1");
          __AppendElement($parent, $n11);
        }
        __SetStyleObject($n11, [20, 21, {
          51: (tagIndex == ((_b = (_a = $data.ui_data.cover_area.cover_top_info) == null ? undefined : _a.tags) == null ? undefined : _b.length) - 1 ? 1 : 0) + ""
        }, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#FFFFFF") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false ? "400" : "500"
        }, {
          47: item.text_info.text_size == null ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n11, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp10 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n13 = $update2 ? $lepusGetElementRefByLepusID("if", 13) : null;
      if (!$n13) {
        $update2 = false;
        $n13 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n13, 13, "if");
        __AppendElement($parent, $n13);
      }
      $$update_2ef9e10_13($n13, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp10;
    }
  }
};
function $$update_100f540_17($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.ui_data.cover_area.cover_bottom_info.tags != null && ($data.ui_data.cover_area.cover_bottom_info.tags.length > 0 || $data.ui_data.cover_area.cover_bottom_info.has_ad_tag)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n18 = $update2 ? $lepusGetElementRefByLepusID("view", 18) : null;
          let $temp2 = $update2;
          if (!$n18) {
            $update2 = false;
            $n18 = __CreateView($currentComponentId);
            let $nid18 = $lepusStoreElementRefByLepusID($n18, 18, "view");
            __SetAttribute($n18, 1004, $nid18[1]);
            __SetStyleObject($n18, [11, 23, 13, 0, 24, 25]);
            __AppendElement($parent, $n18);
          }
          {
            let $n19 = $update2 ? $lepusGetElementRefByLepusID("view", 19) : null;
            let $temp3 = $update2;
            if (!$n19) {
              $update2 = false;
              $n19 = __CreateView($currentComponentId);
              let $nid19 = $lepusStoreElementRefByLepusID($n19, 19, "view");
              __SetAttribute($n19, 1004, $nid19[1]);
              __AppendElement($n18, $n19);
            }
            if (!$update2 || $varUpdateState[2]) {
              {
                let $value = "padding-right:" + (($data.ui_data.cover_area.cover_bottom_info.has_ad_tag ? "36px" : "8px") + ";") + "position:absolute;bottom:8px;left:0px;linear-orientation:row;width:100%;height:auto;padding-left:8px;padding-right:8px;";
                if (!$update2 || $value !== "padding-right:" + (($cardInstance._data.ui_data.cover_area.cover_bottom_info.has_ad_tag ? "36px" : "8px") + ";") + "position:absolute;bottom:8px;left:0px;linear-orientation:row;width:100%;height:auto;padding-left:8px;padding-right:8px;") {
                  __SetStyleObject($n19, [11, 26, 13, 27, 0, 17, 18, 19, {
                    34: $data.ui_data.cover_area.cover_bottom_info.has_ad_tag ? "36px" : "8px"
                  }]);
                }
              }
            }
            if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
              let $n20 = $update2 ? $lepusGetElementRefByLepusID("for", 20) : null;
              if (!$n20) {
                $n20 = __CreateFor($currentComponentId);
                $lepusStoreElementRefByLepusID($n20, 20, "for");
                __AppendElement($n19, $n20);
              }
              $$update_100f540_20($n20, $data, $update2);
            }
            {
              let $template_update = $update2;
              let $n30 = $update2 ? $lepusGetElementRefByLepusID("if", 30) : null;
              if (!$n30) {
                $update2 = false;
                $n30 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n30, 30, "if");
                __AppendElement($n19, $n30);
              }
              $$update_1a82dd8_30($n30, $data, $update2);
              $update2 = $template_update;
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
}
$$update_100f540_20 = function ($parent, $data, $update2) {
  let _a, _b;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo4 = $lepusPushFiberForNode($parent, 20, uniqueId),
        $forLepus = _$lepusPushFiberForNo4[0],
        $lastForLepus = _$lepusPushFiberForNo4[1];
    let $object = $data.ui_data.cover_area.cover_bottom_info.tags;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let tagIndex = 0; tagIndex < $length; ++tagIndex) {
      $update2 = tagIndex < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(tagIndex);
      let tagItem = $object[tagIndex];
      {
        let $n21 = $update2 ? $lepusGetElementRefByLepusID("view", 21) : null;
        let $temp2 = $update2;
        if (!$n21) {
          $update2 = false;
          $n21 = __CreateView($currentComponentId);
          let $nid21 = $lepusStoreElementRefByLepusID($n21, 21, "view");
          __SetAttribute($n21, 1004, $nid21[1]);
          __AppendElement($parent, $n21);
        }
        __SetStyleObject($n21, [28, {
          38: tagIndex == 0 ? "0px" : "4px"
        }, {
          35: tagItem.padding[0] + "px"
        }, {
          36: tagItem.padding[2] + "px"
        }, {
          33: tagItem.padding[3] + "px"
        }, {
          34: tagItem.padding[1] + "px"
        }, {
          7: (tagItem.background != null ? tagItem.background : "#ffffff00") + ""
        }, {
          12: tagItem.background_round + "px"
        }, {
          51: (tagIndex == ((_b = (_a = $data.ui_data.cover_area.cover_top_info) == null ? undefined : _a.tags) == null ? undefined : _b.length) - 1 ? 1 : 0) + ""
        }]);
        if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
          let $n22 = $update2 ? $lepusGetElementRefByLepusID("for", 22) : null;
          if (!$n22) {
            $n22 = __CreateFor($currentComponentId);
            $lepusStoreElementRefByLepusID($n22, 22, "for");
            __AppendElement($n21, $n22);
          }
          $$update_2ef9e10_22($n22, $data, $update2, tagIndex, tagItem);
        }
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
function $$update_100f540_34($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo5 = $lepusPushFiberForNode($parent, 34, uniqueId),
        $forLepus = _$lepusPushFiberForNo5[0],
        $lastForLepus = _$lepusPushFiberForNo5[1];
    let $object = $data.ui_data.info_area.title_info.items;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      let $n35 = $update2 ? $lepusGetElementRefByLepusID("if", 35) : null;
      if (!$n35) {
        $n35 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n35, 35, "if");
        __AppendElement($parent, $n35);
      }
      $$update_2ef9e10_35($n35, $data, $update2, index, item);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}
function $$update_100f540_42($parent, $data, $update2) {
  let _a, _b;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_b = (_a = $data.ui_data.info_area.recommend_reason) == null ? undefined : _a.tags) == null ? undefined : _b.length) > 0) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n43 = $update2 ? $lepusGetElementRefByLepusID("view", 43) : null;
          let $temp2 = $update2;
          if (!$n43) {
            $update2 = false;
            $n43 = __CreateView($currentComponentId);
            let $nid43 = $lepusStoreElementRefByLepusID($n43, 43, "view");
            __SetAttribute($n43, 1004, $nid43[1]);
            __AppendElement($parent, $n43);
          }
          if (!$update2 || $varUpdateState[2]) {
            {
              let $value = "max-height:" + ($data.ui_data.info_area.recommend_reason.tags_max_height + "px;") + "linear-direction:row;flex-wrap:wrap;overflow:hidden;width:100%;padding-left:8px;padding-right:8px;margin-top:4px;";
              if (!$update2 || $value !== "max-height:" + ($cardInstance._data.ui_data.info_area.recommend_reason.tags_max_height + "px;") + "linear-direction:row;flex-wrap:wrap;overflow:hidden;width:100%;padding-left:8px;padding-right:8px;margin-top:4px;") {
                __SetStyleObject($n43, [28, 16, 20, 0, 18, 19, 47, {
                  30: $data.ui_data.info_area.recommend_reason.tags_max_height + "px"
                }]);
              }
            }
          }
          if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
            let $n44 = $update2 ? $lepusGetElementRefByLepusID("for", 44) : null;
            if (!$n44) {
              $n44 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n44, 44, "for");
              __AppendElement($n43, $n44);
            }
            $$update_100f540_44($n44, $data, $update2);
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
$$update_100f540_44 = function ($parent, $data, $update2) {
  let _a, _b;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo6 = $lepusPushFiberForNode($parent, 44, uniqueId),
        $forLepus = _$lepusPushFiberForNo6[0],
        $lastForLepus = _$lepusPushFiberForNo6[1];
    let $object = $data.ui_data.info_area.recommend_reason.tags;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let tagIndex = 0; tagIndex < $length; ++tagIndex) {
      $update2 = tagIndex < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(tagIndex);
      let tagItem = $object[tagIndex];
      {
        let $n45 = $update2 ? $lepusGetElementRefByLepusID("view", 45) : null;
        let $temp2 = $update2;
        if (!$n45) {
          $update2 = false;
          $n45 = __CreateView($currentComponentId);
          let $nid45 = $lepusStoreElementRefByLepusID($n45, 45, "view");
          __SetAttribute($n45, 1004, $nid45[1]);
          __AppendElement($parent, $n45);
        }
        __SetStyleObject($n45, [{
          38: tagIndex == 0 ? "0px" : $data.ui_data.info_area.recommend_reason.spacing != null ? "" + $data.ui_data.info_area.recommend_reason.spacing + "px" : "4px"
        }, {
          35: tagItem.padding[0] + "px"
        }, {
          36: tagItem.padding[2] + "px"
        }, {
          33: tagItem.padding[3] + "px"
        }, {
          34: tagItem.padding[1] + "px"
        }, {
          7: (tagItem.background != null ? tagItem.background : "#ffffff00") + ""
        }, {
          12: tagItem.background_round + "px"
        }, {
          51: (tagIndex == ((_b = (_a = $data.ui_data.cover_area.cover_top_info) == null ? undefined : _a.tags) == null ? undefined : _b.length) - 1 ? 1 : 0) + ""
        }]);
        if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
          let $n46 = $update2 ? $lepusGetElementRefByLepusID("for", 46) : null;
          if (!$n46) {
            $n46 = __CreateFor($currentComponentId);
            $lepusStoreElementRefByLepusID($n46, 46, "for");
            __AppendElement($n45, $n46);
          }
          $$update_100f540_46($n46, $data, $update2, tagIndex, tagItem);
        }
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
$$update_100f540_46 = function ($parent, $data, $update2, tagIndex, tagItem) {
  let _a, _b;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo7 = $lepusPushFiberForNode($parent, 46, uniqueId),
        $forLepus = _$lepusPushFiberForNo7[0],
        $lastForLepus = _$lepusPushFiberForNo7[1];
    let $object = tagItem.items;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      {
        let $n47 = $update2 ? $lepusGetElementRefByLepusID("view", 47) : null;
        let $temp2 = $update2;
        if (!$n47) {
          $update2 = false;
          $n47 = __CreateView($currentComponentId);
          let $nid47 = $lepusStoreElementRefByLepusID($n47, 47, "view");
          __SetAttribute($n47, 1004, $nid47[1]);
          __AppendElement($parent, $n47);
        }
        __SetStyleObject($n47, [{
          51: (tagIndex == ((_b = (_a = $data.ui_data.cover_area.recommend_reason) == null ? undefined : _a.tags) == null ? undefined : _b.length) - 1 ? 1 : 0) + ""
        }]);
        let $n48 = $update2 ? $lepusGetElementRefByLepusID("if", 48) : null;
        if (!$n48) {
          $n48 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n48, 48, "if");
          __AppendElement($n47, $n48);
        }
        $$update_2ef9e10_48($n48, $data, $update2, index, item, tagIndex, tagItem);
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
function $$update_100f540_81($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (Object.keys($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo).length > 0) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n82 = $update2 ? $lepusGetElementRefByLepusID("view", 82) : null;
        let $temp2 = $update2;
        if (!$n82) {
          $update2 = false;
          $n82 = __CreateView($currentComponentId);
          let $nid82 = $lepusStoreElementRefByLepusID($n82, 82, "view");
          __SetAttribute($n82, 1004, $nid82[1]);
          __SetStyleObject($n82, [48, 16, 58, 30, 22, 47]);
          __AppendElement($parent, $n82);
        }
        {
          let $n83 = $update2 ? $lepusGetElementRefByLepusID("view", 83) : null;
          let $temp3 = $update2;
          if (!$n83) {
            $update2 = false;
            $n83 = __CreateView($currentComponentId);
            let $nid83 = $lepusStoreElementRefByLepusID($n83, 83, "view");
            __SetAttribute($n83, 1004, $nid83[1]);
            __SetStyleObject($n83, [14, 5, 22, 59, 58]);
            __AppendElement($n82, $n83);
          }
          {
            let $template_update = $update2;
            let $n84 = $update2 ? $lepusGetElementRefByLepusID("if", 84) : null;
            if (!$n84) {
              $update2 = false;
              $n84 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n84, 84, "if");
              __AppendElement($n83, $n84);
            }
            $$update_1a82dd8_84($n84, $data, $update2);
            $update2 = $template_update;
          }
          {
            let $n96 = $update2 ? $lepusGetElementRefByLepusID("image", 96) : null;
            let $temp4 = $update2;
            if (!$n96) {
              $update2 = false;
              $n96 = __CreateImage($currentComponentId);
              let $nid96 = $lepusStoreElementRefByLepusID($n96, 96, "image");
              __SetAttribute($n96, 1004, $nid96[1]);
              __AppendElement($n83, $n96);
            }
            if (!$update2 || $varUpdateState[2]) {
              {
                let $value = "width:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.bgRightStyle.width + ";") + "height:100%;flex-shrink:0;width:7px;";
                if (!$update2 || $value !== "width:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.bgRightStyle.width + ";") + "height:100%;flex-shrink:0;width:7px;") {
                  __SetStyleObject($n96, [59, 22, 67, {
                    27: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.bgRightStyle.width + ""
                  }]);
                }
              }
              {
                let _$value = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.bgRight;
                if (!$update2 || _$value !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.bgRight) {
                  __SetAttribute($n96, "src", _$value);
                }
              }
            }
            $update2 = $temp4;
          }
          $update2 = $temp3;
        }
        {
          let _$template_update = $update2;
          let $n97 = $update2 ? $lepusGetElementRefByLepusID("if", 97) : null;
          if (!$n97) {
            $update2 = false;
            $n97 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n97, 97, "if");
            __AppendElement($n82, $n97);
          }
          $$update_1a82dd8_97($n97, $data, $update2);
          $update2 = _$template_update;
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp11 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let _$template_update2 = $update2;
        let $n111 = $update2 ? $lepusGetElementRefByLepusID("if", 111) : null;
        if (!$n111) {
          $update2 = false;
          $n111 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n111, 111, "if");
          __AppendElement($parent, $n111);
        }
        $$update_100f540_111($n111, $data, $update2);
        $update2 = _$template_update2;
      }
      $update2 = _$temp11;
    }
  }
}
$$update_100f540_111 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (Object.keys($data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo).length > 0) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n112 = $update2 ? $lepusGetElementRefByLepusID("view", 112) : null;
        let $temp2 = $update2;
        if (!$n112) {
          $update2 = false;
          $n112 = __CreateView($currentComponentId);
          let $nid112 = $lepusStoreElementRefByLepusID($n112, 112, "view");
          __SetAttribute($n112, 1004, $nid112[1]);
          __SetStyleObject($n112, [48, 73, 74, 5, 75, 76, 77, 78, 79, 80]);
          __AppendElement($parent, $n112);
        }
        {
          let $template_update = $update2;
          let $n113 = $update2 ? $lepusGetElementRefByLepusID("if", 113) : null;
          if (!$n113) {
            $update2 = false;
            $n113 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n113, 113, "if");
            __AppendElement($n112, $n113);
          }
          $$update_1a82dd8_113($n113, $data, $update2);
          $update2 = $template_update;
        }
        {
          let $n115 = $update2 ? $lepusGetElementRefByLepusID("text", 115) : null;
          let $temp3 = $update2;
          if (!$n115) {
            $update2 = false;
            $n115 = __CreateText($currentComponentId);
            let $nid115 = $lepusStoreElementRefByLepusID($n115, 115, "text");
            __SetAttribute($n115, 1004, $nid115[1]);
            __SetStyleObject($n115, [53, 83, 84, 85]);
            __AppendElement($n112, $n115);
          }
          {
            if (!$update2 || $varUpdateState[2]) {
              let $value = $data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagText;
              if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagText) {
                __SetAttribute($n115, "text", $value);
              }
            }
          }
          $update2 = $temp3;
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp12 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      {
        let _$template_update3 = $update2;
        let $n117 = $update2 ? $lepusGetElementRefByLepusID("if", 117) : null;
        if (!$n117) {
          $update2 = false;
          $n117 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n117, 117, "if");
          __AppendElement($parent, $n117);
        }
        $$update_100f540_117($n117, $data, $update2);
        $update2 = _$template_update3;
      }
      $update2 = _$temp12;
    }
  }
};
$$update_100f540_117 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (Object.keys($data.ui_data.info_area.sale_info.price_info_vo.seckill_info_vo).length > 0 && Date.now() / 1e3 >= $data.ui_data.info_area.sale_info.price_info_vo.seckill_info_vo.start_time && Date.now() / 1e3 <= $data.ui_data.info_area.sale_info.price_info_vo.seckill_info_vo.end_time) {
      __UpdateIfNodeIndex($parent, 4);
      $conditionNodeIndex[uniqueId] = 4;
      let $temp = $update2;
      if ($ifNodeIndex !== 4) {
        $update2 = false;
      }
      {
        let $n118 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 118) : null;
        let $temp2 = $update2;
        if (!$n118) {
          $update2 = false;
          $n118 = __CreateElement("countdown-view", $currentComponentId);
          let $nid118 = $lepusStoreElementRefByLepusID($n118, 118, "countdown-view");
          __SetAttribute($n118, 1004, $nid118[1]);
          __SetStyleObject($n118, [5, 75, 86, 78, 74, 76, 87, 79, 80, 47, 88, 28]);
          __SetAttribute($n118, "gone-after-end", "true");
          __SetAttribute($n118, "unit", "seconds");
          __SetID($n118, "countdown");
          __AppendElement($parent, $n118);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "" + $data.ui_data.info_area.sale_info.price_info_vo.seckill_info_vo.end_time;
            if (!$update2 || $value !== "" + $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.seckill_info_vo.end_time) {
              __SetAttribute($n118, "end-time", $value);
            }
          }
        }
        {
          let $n119 = $update2 ? $lepusGetElementRefByLepusID("text", 119) : null;
          let $temp3 = $update2;
          if (!$n119) {
            $update2 = false;
            $n119 = __CreateText($currentComponentId);
            let $nid119 = $lepusStoreElementRefByLepusID($n119, 119, "text");
            __SetAttribute($n119, 1004, $nid119[1]);
            __SetStyleObject($n119, [84, 53, 85, 83, 89]);
            __SetAttribute($n119, "text-maxline", "1");
            __AppendElement($n118, $n119);
          }
          __SetAttribute($n119, "text", "秒杀中");
          $update2 = $temp3;
        }
        {
          let $n121 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 121) : null;
          let _$temp13 = $update2;
          if (!$n121) {
            $update2 = false;
            $n121 = __CreateElement("countdown-item", $currentComponentId);
            let $nid121 = $lepusStoreElementRefByLepusID($n121, 121, "countdown-item");
            __SetAttribute($n121, 1004, $nid121[1]);
            __SetAttribute($n121, "text-maxline", "1");
            __SetAttribute($n121, "countdown-display", "HH");
            __AppendElement($n118, $n121);
          }
          if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
            {
              let _$value2 = "width:" + ((__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px;") + "justify-content:center;align-items:baseline;font-family:PingFang SC;text-align:center;font-size:10px;font-weight:500;color:#ff1c49;";
              if (!$update2 || _$value2 !== undefined) {
                __SetStyleObject($n121, [75, 51, 84, 90, 53, 40, 83, {
                  27: (__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px"
                }]);
              }
            }
          }
          $update2 = _$temp13;
        }
        {
          let $n122 = $update2 ? $lepusGetElementRefByLepusID("text", 122) : null;
          let _$temp14 = $update2;
          if (!$n122) {
            $update2 = false;
            $n122 = __CreateText($currentComponentId);
            let $nid122 = $lepusStoreElementRefByLepusID($n122, 122, "text");
            __SetAttribute($n122, 1004, $nid122[1]);
            __SetStyleObject($n122, [84, 53, 40, 83]);
            __AppendElement($n118, $n122);
          }
          __SetAttribute($n122, "text", ":");
          $update2 = _$temp14;
        }
        {
          let $n124 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 124) : null;
          let _$temp15 = $update2;
          if (!$n124) {
            $update2 = false;
            $n124 = __CreateElement("countdown-item", $currentComponentId);
            let $nid124 = $lepusStoreElementRefByLepusID($n124, 124, "countdown-item");
            __SetAttribute($n124, 1004, $nid124[1]);
            __SetAttribute($n124, "text-maxline", "1");
            __SetAttribute($n124, "countdown-display", "mm");
            __AppendElement($n118, $n124);
          }
          if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
            {
              let _$value3 = "width:" + ((__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px;") + "justify-content:center;align-items:baseline;font-family:PingFang SC;text-align:center;font-size:10px;font-weight:500;color:#ff1c49;";
              if (!$update2 || _$value3 !== undefined) {
                __SetStyleObject($n124, [75, 51, 84, 90, 53, 40, 83, {
                  27: (__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px"
                }]);
              }
            }
          }
          $update2 = _$temp15;
        }
        {
          let $n125 = $update2 ? $lepusGetElementRefByLepusID("text", 125) : null;
          let _$temp16 = $update2;
          if (!$n125) {
            $update2 = false;
            $n125 = __CreateText($currentComponentId);
            let $nid125 = $lepusStoreElementRefByLepusID($n125, 125, "text");
            __SetAttribute($n125, 1004, $nid125[1]);
            __SetStyleObject($n125, [84, 53, 40, 83]);
            __AppendElement($n118, $n125);
          }
          __SetAttribute($n125, "text", ":");
          $update2 = _$temp16;
        }
        {
          let $n127 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 127) : null;
          let _$temp17 = $update2;
          if (!$n127) {
            $update2 = false;
            $n127 = __CreateElement("countdown-item", $currentComponentId);
            let $nid127 = $lepusStoreElementRefByLepusID($n127, 127, "countdown-item");
            __SetAttribute($n127, 1004, $nid127[1]);
            __SetAttribute($n127, "text-maxline", "1");
            __SetAttribute($n127, "countdown-display", "ss");
            __AppendElement($n118, $n127);
          }
          if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
            {
              let _$value4 = "width:" + ((__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px;") + "justify-content:center;align-items:baseline;font-family:PingFang SC;text-align:center;font-size:10px;font-weight:500;color:#ff1c49;";
              if (!$update2 || _$value4 !== undefined) {
                __SetStyleObject($n127, [75, 51, 84, 90, 53, 40, 83, {
                  27: (__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px"
                }]);
              }
            }
          }
          $update2 = _$temp17;
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
$$update_1a82dd8_30 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.cover_area.cover_bottom_info.has_ad_tag) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n31 = $update2 ? $lepusGetElementRefByLepusID("text", 31) : null;
        let $temp2 = $update2;
        if (!$n31) {
          $update2 = false;
          $n31 = __CreateText($currentComponentId);
          let $nid31 = $lepusStoreElementRefByLepusID($n31, 31, "text");
          __SetAttribute($n31, 1004, $nid31[1]);
          __SetStyleObject($n31, [11, 31, 32, 22, 33, 34, 35, 36, 37, 38, 39, 40, 41]);
          __AppendElement($parent, $n31);
        }
        __SetAttribute($n31, "text", "test11");
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
function $$update_1a82dd8_75($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo8 = $lepusPushFiberForNode($parent, 75, uniqueId),
        $forLepus = _$lepusPushFiberForNo8[0],
        $lastForLepus = _$lepusPushFiberForNo8[1];
    let $object = $data.ui_data.info_area.sale_info.price_info_vo.price_discountInfo_vo;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      {
        let $n76 = $update2 ? $lepusGetElementRefByLepusID("text", 76) : null;
        let $temp2 = $update2;
        if (!$n76) {
          $update2 = false;
          $n76 = __CreateText($currentComponentId);
          let $nid76 = $lepusStoreElementRefByLepusID($n76, 76, "text");
          __SetAttribute($n76, 1004, $nid76[1]);
          __AppendElement($parent, $n76);
        }
        __SetStyleObject($n76, [{
          61: item.style.fontFamily + ""
        }, {
          22: item.style.color + ""
        }, {
          51: item.style.flexShrink + ""
        }, {
          47: item.style.fontSize + ""
        }, {
          48: item.style.fontWeight + ""
        }, {
          38: item.style.marginLeft + ""
        }]);
        __SetAttribute($n76, "text", item.text);
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}
function $$update_1a82dd8_78($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.sold_count != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n79 = $update2 ? $lepusGetElementRefByLepusID("text", 79) : null;
        let $temp2 = $update2;
        if (!$n79) {
          $update2 = false;
          $n79 = __CreateText($currentComponentId);
          let $nid79 = $lepusStoreElementRefByLepusID($n79, 79, "text");
          __SetAttribute($n79, 1004, $nid79[1]);
          __SetStyleObject($n79, [55, 56, 57, 22, 54]);
          __AppendElement($parent, $n79);
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let $value = $data.ui_data.info_area.sale_info.sold_count;
            if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.sold_count) {
              __SetAttribute($n79, "text", $value);
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
$$update_1a82dd8_84 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null || $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixCurrencySign != null || $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixAmount != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n85 = $update2 ? $lepusGetElementRefByLepusID("view", 85) : null;
        let $temp2 = $update2;
        if (!$n85) {
          $update2 = false;
          $n85 = __CreateView($currentComponentId);
          let $nid85 = $lepusStoreElementRefByLepusID($n85, 85, "view");
          __SetAttribute($n85, 1004, $nid85[1]);
          __SetStyleObject($n85, [48, 59, 5, 60]);
          __AppendElement($parent, $n85);
        }
        {
          let $n86 = $update2 ? $lepusGetElementRefByLepusID("image", 86) : null;
          let $temp3 = $update2;
          if (!$n86) {
            $update2 = false;
            $n86 = __CreateImage($currentComponentId);
            let $nid86 = $lepusStoreElementRefByLepusID($n86, 86, "image");
            __SetAttribute($n86, 1004, $nid86[1]);
            __SetStyleObject($n86, [11, 61, 62, 0, 59, 63]);
            __SetAttribute($n86, "cap-insets-scale", "3");
            __SetAttribute($n86, "cap-insets", "0px 10px 0px 20px");
            __AppendElement($n85, $n86);
          }
          if (!$update2 || $varUpdateState[2]) {
            {
              let $value = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.bgCenter;
              if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.bgCenter) {
                __SetAttribute($n86, "src", $value);
              }
            }
          }
          $update2 = $temp3;
        }
        {
          let $n87 = $update2 ? $lepusGetElementRefByLepusID("view", 87) : null;
          let _$temp18 = $update2;
          if (!$n87) {
            $update2 = false;
            $n87 = __CreateView($currentComponentId);
            let $nid87 = $lepusStoreElementRefByLepusID($n87, 87, "view");
            __SetAttribute($n87, 1004, $nid87[1]);
            __AppendElement($n85, $n87);
          }
          if (!$update2 || $varUpdateState[2]) {
            {
              let _$value5 = "margin-left:" + (($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft != null ? $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft : "14px") + ";") + "display:flex;align-items:center;height:100%;";
              if (!$update2 || _$value5 !== "margin-left:" + (($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft != null ? $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft : "14px") + ";") + "display:flex;align-items:center;height:100%;") {
                __SetStyleObject($n87, [48, 5, 59, {
                  38: ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft != null ? $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft : "14px") + ""
                }]);
              }
            }
          }
          {
            let $n88 = $update2 ? $lepusGetElementRefByLepusID("text", 88) : null;
            let $temp4 = $update2;
            if (!$n88) {
              $update2 = false;
              $n88 = __CreateText($currentComponentId);
              let $nid88 = $lepusStoreElementRefByLepusID($n88, 88, "text");
              __SetAttribute($n88, 1004, $nid88[1]);
              __SetStyleObject($n88, [64, 40, 65]);
              __SetAttribute($n88, "include-font-padding", "true");
              __SetAttribute($n88, "text-single-line-vertical-align", "center");
              __AppendElement($n87, $n88);
            }
            {
              if (!$update2 || $varUpdateState[2]) {
                let _$value6 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText;
                if (!$update2 || _$value6 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText) {
                  __SetAttribute($n88, "text", _$value6);
                }
              }
            }
            $update2 = $temp4;
          }
          {
            let $template_update = $update2;
            let $n90 = $update2 ? $lepusGetElementRefByLepusID("if", 90) : null;
            if (!$n90) {
              $update2 = false;
              $n90 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n90, 90, "if");
              __AppendElement($n87, $n90);
            }
            $$update_1a82dd8_90($n90, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update4 = $update2;
            let $n93 = $update2 ? $lepusGetElementRefByLepusID("if", 93) : null;
            if (!$n93) {
              $update2 = false;
              $n93 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n93, 93, "if");
              __AppendElement($n87, $n93);
            }
            $$update_1a82dd8_93($n93, $data, $update2);
            $update2 = _$template_update4;
          }
          $update2 = _$temp18;
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
$$update_1a82dd8_90 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixCurrencySign != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n91 = $update2 ? $lepusGetElementRefByLepusID("text", 91) : null;
        let $temp2 = $update2;
        if (!$n91) {
          $update2 = false;
          $n91 = __CreateText($currentComponentId);
          let $nid91 = $lepusStoreElementRefByLepusID($n91, 91, "text");
          __SetAttribute($n91, 1004, $nid91[1]);
          __SetAttribute($n91, "include-font-padding", "true");
          __SetAttribute($n91, "text-single-line-vertical-align", "center");
          __AppendElement($parent, $n91);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixCurrencySignStyle.color + ";") + "white-space:nowrap;font-size:9px;font-weight:500;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixCurrencySignStyle.color + ";") + "white-space:nowrap;font-size:9px;font-weight:500;") {
              __SetStyleObject($n91, [66, 64, 40, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixCurrencySignStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value7 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixCurrencySign;
            if (!$update2 || _$value7 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixCurrencySign) {
              __SetAttribute($n91, "text", _$value7);
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
};
$$update_1a82dd8_93 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixAmount != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n94 = $update2 ? $lepusGetElementRefByLepusID("text", 94) : null;
        let $temp2 = $update2;
        if (!$n94) {
          $update2 = false;
          $n94 = __CreateText($currentComponentId);
          let $nid94 = $lepusStoreElementRefByLepusID($n94, 94, "text");
          __SetAttribute($n94, 1004, $nid94[1]);
          __SetAttribute($n94, "include-font-padding", "true");
          __SetAttribute($n94, "text-single-line-vertical-align", "center");
          __AppendElement($parent, $n94);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixAmountStyle.color + ";") + "white-space:nowrap;font-size:9px;font-weight:500;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixAmountStyle.color + ";") + "white-space:nowrap;font-size:9px;font-weight:500;") {
              __SetStyleObject($n94, [66, 64, 40, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixAmountStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value8 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixAmount;
            if (!$update2 || _$value8 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixAmount) {
              __SetAttribute($n94, "text", _$value8);
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
};
$$update_1a82dd8_97 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixText != null || $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixCurrencySign != null || $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixAmount != null || $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixSuffixText != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n98 = $update2 ? $lepusGetElementRefByLepusID("view", 98) : null;
        let $temp2 = $update2;
        if (!$n98) {
          $update2 = false;
          $n98 = __CreateView($currentComponentId);
          let $nid98 = $lepusStoreElementRefByLepusID($n98, 98, "view");
          __SetAttribute($n98, 1004, $nid98[1]);
          __AppendElement($parent, $n98);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "padding-left:" + (($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "7px" : "3px") + ";") + ("margin-left:" + (($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "-7px" : "0px") + ";") + ("border-radius:" + (($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "2px" : "0px") + ";"))) + "linear-cross-gravity:center;linear-orientation:horizontal;align-items:center;border-top-right-radius:2px;border-bottom-right-radius:2px;padding-right:3px;background-color:#fe2c5519;";
            if (!$update2 || $value !== undefined) {
              __SetStyleObject($n98, [68, 14, 5, 69, 70, 71, 72, {
                33: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "7px" : "3px"
              }, {
                38: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "-7px" : "0px"
              }, {
                12: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "2px" : "0px"
              }]);
            }
          }
        }
        {
          let $n99 = $update2 ? $lepusGetElementRefByLepusID("view", 99) : null;
          let $temp3 = $update2;
          if (!$n99) {
            $update2 = false;
            $n99 = __CreateView($currentComponentId);
            let $nid99 = $lepusStoreElementRefByLepusID($n99, 99, "view");
            __SetAttribute($n99, 1004, $nid99[1]);
            __SetStyleObject($n99, [48, 58, 5]);
            __AppendElement($n98, $n99);
          }
          {
            let $n100 = $update2 ? $lepusGetElementRefByLepusID("text", 100) : null;
            let $temp4 = $update2;
            if (!$n100) {
              $update2 = false;
              $n100 = __CreateText($currentComponentId);
              let $nid100 = $lepusStoreElementRefByLepusID($n100, 100, "text");
              __SetAttribute($n100, 1004, $nid100[1]);
              __SetAttribute($n100, "text-maxline", "1");
              __SetAttribute($n100, "include-font-padding", "true");
              __SetAttribute($n100, "text-single-line-vertical-align", "center");
              __AppendElement($n99, $n100);
            }
            if (!$update2 || $varUpdateState[2]) {
              {
                let _$value9 = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixTextStyle.color + ";") + "font-weight:500;font-size:9px;flex-shrink:0;";
                if (!$update2 || _$value9 !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixTextStyle.color + ";") + "font-weight:500;font-size:9px;flex-shrink:0;") {
                  __SetStyleObject($n100, [40, 64, 22, {
                    22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixTextStyle.color + ""
                  }]);
                }
              }
            }
            {
              if (!$update2 || $varUpdateState[2]) {
                let _$value10 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixText;
                if (!$update2 || _$value10 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixText) {
                  __SetAttribute($n100, "text", _$value10);
                }
              }
            }
            $update2 = $temp4;
          }
          {
            let $template_update = $update2;
            let $n102 = $update2 ? $lepusGetElementRefByLepusID("if", 102) : null;
            if (!$n102) {
              $update2 = false;
              $n102 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n102, 102, "if");
              __AppendElement($n99, $n102);
            }
            $$update_1a82dd8_102($n102, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update5 = $update2;
            let $n105 = $update2 ? $lepusGetElementRefByLepusID("if", 105) : null;
            if (!$n105) {
              $update2 = false;
              $n105 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n105, 105, "if");
              __AppendElement($n99, $n105);
            }
            $$update_1a82dd8_105($n105, $data, $update2);
            $update2 = _$template_update5;
          }
          {
            let _$template_update6 = $update2;
            let $n108 = $update2 ? $lepusGetElementRefByLepusID("if", 108) : null;
            if (!$n108) {
              $update2 = false;
              $n108 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n108, 108, "if");
              __AppendElement($n99, $n108);
            }
            $$update_1a82dd8_108($n108, $data, $update2);
            $update2 = _$template_update6;
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
};
$$update_1a82dd8_102 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixCurrencySign != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n103 = $update2 ? $lepusGetElementRefByLepusID("text", 103) : null;
        let $temp2 = $update2;
        if (!$n103) {
          $update2 = false;
          $n103 = __CreateText($currentComponentId);
          let $nid103 = $lepusStoreElementRefByLepusID($n103, 103, "text");
          __SetAttribute($n103, 1004, $nid103[1]);
          __SetAttribute($n103, "include-font-padding", "true");
          __SetAttribute($n103, "text-single-line-vertical-align", "center");
          __AppendElement($parent, $n103);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixCurrencySignStyle.color + ";") + "font-weight:500;font-size:9px;flex-shrink:0;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixCurrencySignStyle.color + ";") + "font-weight:500;font-size:9px;flex-shrink:0;") {
              __SetStyleObject($n103, [40, 64, 22, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixCurrencySignStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value11 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixCurrencySign;
            if (!$update2 || _$value11 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixCurrencySign) {
              __SetAttribute($n103, "text", _$value11);
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
};
$$update_1a82dd8_105 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixAmount != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n106 = $update2 ? $lepusGetElementRefByLepusID("text", 106) : null;
        let $temp2 = $update2;
        if (!$n106) {
          $update2 = false;
          $n106 = __CreateText($currentComponentId);
          let $nid106 = $lepusStoreElementRefByLepusID($n106, 106, "text");
          __SetAttribute($n106, 1004, $nid106[1]);
          __SetAttribute($n106, "include-font-padding", "true");
          __SetAttribute($n106, "text-single-line-vertical-align", "center");
          __AppendElement($parent, $n106);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixAmountStyle.color + ";") + "font-size:9px;font-weight:500;flex-shrink:0;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixAmountStyle.color + ";") + "font-size:9px;font-weight:500;flex-shrink:0;") {
              __SetStyleObject($n106, [64, 40, 22, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixAmountStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value12 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixAmount;
            if (!$update2 || _$value12 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixAmount) {
              __SetAttribute($n106, "text", _$value12);
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
};
$$update_1a82dd8_108 = function ($parent, $data, $update2) {
  let _a, _b, _c, _d, _e, _f;
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (((_f = (_e = (_d = (_c = (_b = (_a = $data.ui_data) == null ? undefined : _a.info_area) == null ? undefined : _b.sale_info) == null ? undefined : _c.price_info_vo) == null ? undefined : _d.coupon_info_vo) == null ? undefined : _e.suffixContent) == null ? undefined : _f.suffixSuffixText) != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n109 = $update2 ? $lepusGetElementRefByLepusID("text", 109) : null;
        let $temp2 = $update2;
        if (!$n109) {
          $update2 = false;
          $n109 = __CreateText($currentComponentId);
          let $nid109 = $lepusStoreElementRefByLepusID($n109, 109, "text");
          __SetAttribute($n109, 1004, $nid109[1]);
          __SetAttribute($n109, "text-maxline", "1");
          __SetAttribute($n109, "include-font-padding", "true");
          __SetAttribute($n109, "text-single-line-vertical-align", "center");
          __AppendElement($parent, $n109);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixSuffixTextStyle.color + ";") + "font-weight:500;font-size:9px;flex-shrink:0;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixSuffixTextStyle.color + ";") + "font-weight:500;font-size:9px;flex-shrink:0;") {
              __SetStyleObject($n109, [40, 64, 22, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixSuffixTextStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value13 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixSuffixText;
            if (!$update2 || _$value13 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixSuffixText) {
              __SetAttribute($n109, "text", _$value13);
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
};
$$update_1a82dd8_113 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagIcon != null) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n114 = $update2 ? $lepusGetElementRefByLepusID("image", 114) : null;
        let $temp2 = $update2;
        if (!$n114) {
          $update2 = false;
          $n114 = __CreateImage($currentComponentId);
          let $nid114 = $lepusStoreElementRefByLepusID($n114, 114, "image");
          __SetAttribute($n114, 1004, $nid114[1]);
          __SetStyleObject($n114, [81, 82]);
          __AppendElement($parent, $n114);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = $data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagIcon;
            if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagIcon) {
              __SetAttribute($n114, "src", $value);
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
};
function $$update_d8b490_61($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo9 = $lepusPushFiberForNode($parent, 61, uniqueId),
        $forLepus = _$lepusPushFiberForNo9[0],
        $lastForLepus = _$lepusPushFiberForNo9[1];
    let $object = $data.ui_data.info_area.sale_info.price_info_vo.price_vo;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      {
        let $n62 = $update2 ? $lepusGetElementRefByLepusID("text", 62) : null;
        let $temp2 = $update2;
        if (!$n62) {
          $update2 = false;
          $n62 = __CreateText($currentComponentId);
          let $nid62 = $lepusStoreElementRefByLepusID($n62, 62, "text");
          __SetAttribute($n62, 1004, $nid62[1]);
          __AppendElement($parent, $n62);
        }
        {
          let $n63 = $update2 ? $lepusGetElementRefByLepusID("if", 63) : null;
          if (!$n63) {
            $n63 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n63, 63, "if");
            __AppendElement($n62, $n63);
          }
          let uniqueId2 = __GetElementUniqueID($n63);
          if (!$update2) {
            $conditionNodeIndex[uniqueId2] = -1;
          }
          let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
          if (item.pre_fix.isShow == true && item.pre_fix.renderValue) {
            __UpdateIfNodeIndex($n63, 0);
            $conditionNodeIndex[uniqueId2] = 0;
            let $temp3 = $update2;
            if ($ifNodeIndex !== 0) {
              $update2 = false;
            }
            {
              let $n64 = $update2 ? $lepusGetElementRefByLepusID("text", 64) : null;
              let $temp4 = $update2;
              if (!$n64) {
                $update2 = false;
                $n64 = __CreateText($currentComponentId);
                let $nid64 = $lepusStoreElementRefByLepusID($n64, 64, "text");
                __SetAttribute($n64, 1004, $nid64[1]);
                __SetStyleObject($n64, [52, 53, 40]);
                __AppendElement($n63, $n64);
              }
              __SetAttribute($n64, "text", item.pre_fix.renderValue);
              $update2 = $temp4;
            }
            $update2 = $temp3;
          } else {
            __UpdateIfNodeIndex($n63, -1);
            $conditionNodeIndex[uniqueId2] = -1;
          }
        }
        {
          let $n66 = $update2 ? $lepusGetElementRefByLepusID("if", 66) : null;
          if (!$n66) {
            $n66 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n66, 66, "if");
            __AppendElement($n62, $n66);
          }
          let _uniqueId = __GetElementUniqueID($n66);
          if (!$update2) {
            $conditionNodeIndex[_uniqueId] = -1;
          }
          let _$ifNodeIndex = $conditionNodeIndex[_uniqueId];
          if (item.currency_sign.isShow == true && item.currency_sign.renderValue) {
            __UpdateIfNodeIndex($n66, 0);
            $conditionNodeIndex[_uniqueId] = 0;
            let _$temp19 = $update2;
            if (_$ifNodeIndex !== 0) {
              $update2 = false;
            }
            {
              let $n67 = $update2 ? $lepusGetElementRefByLepusID("text", 67) : null;
              let _$temp20 = $update2;
              if (!$n67) {
                $update2 = false;
                $n67 = __CreateText($currentComponentId);
                let $nid67 = $lepusStoreElementRefByLepusID($n67, 67, "text");
                __SetAttribute($n67, 1004, $nid67[1]);
                __AppendElement($n66, $n67);
              }
              __SetStyleObject($n67, [{
                22: item.currency_sign.renderStyle.color + ""
              }, {
                47: item.currency_sign.renderStyle.fontSize + ""
              }, {
                48: item.currency_sign.renderStyle.fontWeight + ""
              }]);
              __SetAttribute($n67, "text", item.currency_sign.renderValue);
              $update2 = _$temp20;
            }
            $update2 = _$temp19;
          } else {
            __UpdateIfNodeIndex($n66, -1);
            $conditionNodeIndex[_uniqueId] = -1;
          }
        }
        {
          let $n69 = $update2 ? $lepusGetElementRefByLepusID("text", 69) : null;
          let _$temp21 = $update2;
          if (!$n69) {
            $update2 = false;
            $n69 = __CreateText($currentComponentId);
            let $nid69 = $lepusStoreElementRefByLepusID($n69, 69, "text");
            __SetAttribute($n69, 1004, $nid69[1]);
            __AppendElement($n62, $n69);
          }
          __SetStyleObject($n69, [{
            61: $data.control_info.groupon_feeds_marketing_expression_new_text_style == 1 ? "NumberABC-Medium" : "PingFang SC"
          }, {
            22: item.amount.renderStyle.color + ""
          }, {
            47: item.amount.renderStyle.fontSize + ""
          }, {
            48: item.amount.renderStyle.fontWeight + ""
          }]);
          __SetAttribute($n69, "text", item.amount.renderValue);
          $update2 = _$temp21;
        }
        {
          let $n71 = $update2 ? $lepusGetElementRefByLepusID("if", 71) : null;
          if (!$n71) {
            $n71 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n71, 71, "if");
            __AppendElement($n62, $n71);
          }
          let _uniqueId2 = __GetElementUniqueID($n71);
          if (!$update2) {
            $conditionNodeIndex[_uniqueId2] = -1;
          }
          let _$ifNodeIndex2 = $conditionNodeIndex[_uniqueId2];
          if (item.post_fix.isShow == true && item.post_fix.renderValue) {
            __UpdateIfNodeIndex($n71, 0);
            $conditionNodeIndex[_uniqueId2] = 0;
            let _$temp22 = $update2;
            if (_$ifNodeIndex2 !== 0) {
              $update2 = false;
            }
            {
              let $n72 = $update2 ? $lepusGetElementRefByLepusID("text", 72) : null;
              let _$temp23 = $update2;
              if (!$n72) {
                $update2 = false;
                $n72 = __CreateText($currentComponentId);
                let $nid72 = $lepusStoreElementRefByLepusID($n72, 72, "text");
                __SetAttribute($n72, 1004, $nid72[1]);
                __SetStyleObject($n72, [52, 53, 40]);
                __AppendElement($n71, $n72);
              }
              __SetAttribute($n72, "text", item.post_fix.renderValue);
              $update2 = _$temp23;
            }
            $update2 = _$temp22;
          } else {
            __UpdateIfNodeIndex($n71, -1);
            $conditionNodeIndex[_uniqueId2] = -1;
          }
        }
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}
function $$update_3d85448_131($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (Object.keys($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo).length > 0 || Object.keys($data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo).length > 0 || Object.keys($data.ui_data.info_area.sale_info.price_info_vo.seckill_info_vo).length > 0) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n132 = $update2 ? $lepusGetElementRefByLepusID("view", 132) : null;
        let $temp2 = $update2;
        if (!$n132) {
          $update2 = false;
          $n132 = __CreateView($currentComponentId);
          let $nid132 = $lepusStoreElementRefByLepusID($n132, 132, "view");
          __SetAttribute($n132, 1004, $nid132[1]);
          __SetStyleObject($n132, [97, 47, 59, 94, 95, 96]);
          __AppendElement($parent, $n132);
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
function $$update_ea5060_128($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.need_ansync_refresh && !$data.is_cache && Date.now() < $data.extra_data.first_load_time + 2e3) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n129 = $update2 ? $lepusGetElementRefByLepusID("view", 129) : null;
        let $temp2 = $update2;
        if (!$n129) {
          $update2 = false;
          $n129 = __CreateView($currentComponentId);
          let $nid129 = $lepusStoreElementRefByLepusID($n129, 129, "view");
          __SetAttribute($n129, 1004, $nid129[1]);
          __SetStyleObject($n129, [11, 3, 62, 91, 0, 59, 1, 92]);
          __AppendElement($parent, $n129);
        }
        {
          let $n130 = $update2 ? $lepusGetElementRefByLepusID("view", 130) : null;
          let $temp3 = $update2;
          if (!$n130) {
            $update2 = false;
            $n130 = __CreateView($currentComponentId);
            let $nid130 = $lepusStoreElementRefByLepusID($n130, 130, "view");
            __SetAttribute($n130, 1004, $nid130[1]);
            __SetStyleObject($n130, [93, 59, 94, 95, 96]);
            __AppendElement($n129, $n130);
          }
          $update2 = $temp3;
        }
        {
          let $template_update = $update2;
          let $n131 = $update2 ? $lepusGetElementRefByLepusID("if", 131) : null;
          if (!$n131) {
            $update2 = false;
            $n131 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n131, 131, "if");
            __AppendElement($n129, $n131);
          }
          $$update_3d85448_131($n131, $data, $update2);
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
function $$update_360f5b0_56($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5] || $varUpdateState[4] || $varUpdateState[3]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.ui_data.info_area.sale_info != null) {
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
            let $nid57 = $lepusStoreElementRefByLepusID($n57, 57, "view");
            __SetAttribute($n57, 1004, $nid57[1]);
            __SetStyleObject($n57, [0, 18, 19, 47]);
            __AppendElement($parent, $n57);
          }
          {
            let $n58 = $update2 ? $lepusGetElementRefByLepusID("view", 58) : null;
            let $temp3 = $update2;
            if (!$n58) {
              $update2 = false;
              $n58 = __CreateView($currentComponentId);
              let $nid58 = $lepusStoreElementRefByLepusID($n58, 58, "view");
              __SetAttribute($n58, 1004, $nid58[1]);
              __SetStyleObject($n58, [1]);
              __AppendElement($n57, $n58);
            }
            {
              let $n59 = $update2 ? $lepusGetElementRefByLepusID("view", 59) : null;
              let $temp4 = $update2;
              if (!$n59) {
                $update2 = false;
                $n59 = __CreateView($currentComponentId);
                let $nid59 = $lepusStoreElementRefByLepusID($n59, 59, "view");
                __SetAttribute($n59, 1004, $nid59[1]);
                __AppendElement($n58, $n59);
              }
              if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
                {
                  let $value = "max-height:" + (17 * 1.2 * parseFloat($data.control_info.font_scale) + "px;") + "width:100%;display:flex;justify-content:space-between;overflow:hidden;flex-wrap:wrap;align-items:flex-end;";
                  if (!$update2 || $value !== undefined) {
                    __SetStyleObject($n59, [0, 48, 49, 20, 16, 50, {
                      30: 17 * 1.2 * parseFloat($data.control_info.font_scale) + "px"
                    }]);
                  }
                }
              }
              {
                let $n60 = $update2 ? $lepusGetElementRefByLepusID("view", 60) : null;
                let $temp5 = $update2;
                if (!$n60) {
                  $update2 = false;
                  $n60 = __CreateView($currentComponentId);
                  let $nid60 = $lepusStoreElementRefByLepusID($n60, 60, "view");
                  __SetAttribute($n60, 1004, $nid60[1]);
                  __SetStyleObject($n60, [22, 51, 48]);
                  __AppendElement($n59, $n60);
                }
                if (!$update2 || $varUpdateState[2] || $varUpdateState[1]) {
                  let $n61 = $update2 ? $lepusGetElementRefByLepusID("for", 61) : null;
                  if (!$n61) {
                    $n61 = __CreateFor($currentComponentId);
                    $lepusStoreElementRefByLepusID($n61, 61, "for");
                    __AppendElement($n60, $n61);
                  }
                  $$update_d8b490_61($n61, $data, $update2);
                }
                {
                  let $n74 = $update2 ? $lepusGetElementRefByLepusID("view", 74) : null;
                  let $temp6 = $update2;
                  if (!$n74) {
                    $update2 = false;
                    $n74 = __CreateView($currentComponentId);
                    let $nid74 = $lepusStoreElementRefByLepusID($n74, 74, "view");
                    __SetAttribute($n74, 1004, $nid74[1]);
                    __SetStyleObject($n74, [51, 20, 22, 48, 54]);
                    __AppendElement($n60, $n74);
                  }
                  if (!$update2 || $varUpdateState[2]) {
                    let $n75 = $update2 ? $lepusGetElementRefByLepusID("for", 75) : null;
                    if (!$n75) {
                      $n75 = __CreateFor($currentComponentId);
                      $lepusStoreElementRefByLepusID($n75, 75, "for");
                      __AppendElement($n74, $n75);
                    }
                    $$update_1a82dd8_75($n75, $data, $update2);
                  }
                  $update2 = $temp6;
                }
                $update2 = $temp5;
              }
              {
                let $template_update = $update2;
                let $n78 = $update2 ? $lepusGetElementRefByLepusID("if", 78) : null;
                if (!$n78) {
                  $update2 = false;
                  $n78 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n78, 78, "if");
                  __AppendElement($n59, $n78);
                }
                $$update_1a82dd8_78($n78, $data, $update2);
                $update2 = $template_update;
              }
              $update2 = $temp4;
            }
            {
              let _$template_update7 = $update2;
              let $n81 = $update2 ? $lepusGetElementRefByLepusID("if", 81) : null;
              if (!$n81) {
                $update2 = false;
                $n81 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n81, 81, "if");
                __AppendElement($n58, $n81);
              }
              $$update_100f540_81($n81, $data, $update2);
              $update2 = _$template_update7;
            }
            {
              let _$template_update8 = $update2;
              let $n128 = $update2 ? $lepusGetElementRefByLepusID("if", 128) : null;
              if (!$n128) {
                $update2 = false;
                $n128 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n128, 128, "if");
                __AppendElement($n58, $n128);
              }
              $$update_ea5060_128($n128, $data, $update2);
              $update2 = _$template_update8;
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
    let $n2 = $lepusGetElementRefByLepusID("view", 2);
    {
      let $value = $data.biz_data.is_webcasting ? "groupon_feed_product_card_live" : "groupon_feed_product_card_no_live";
      if (!$update || $value !== ($cardInstance._data.biz_data.is_webcasting ? "groupon_feed_product_card_live" : "groupon_feed_product_card_no_live")) {
        __SetAttribute($n2, "lynx-test-tag", $value);
      }
    }
  }
  if ($varUpdateState[1] || $varUpdateState[2]) {
    let $n4 = $lepusGetElementRefByLepusID("image", 4);
    {
      let _$value14 = "aspect-ratio:" + (($data.control_info.cover_ratio == null || $data.control_info.cover_ratio == 0 ? 1 : $data.control_info.cover_ratio) + ";") + "width:100%;border-top-left-radius:8px;border-top-right-radius:8px;border-bottom-right-radius:0px;border-bottom-left-radius:0px;";
      if (!$update || _$value14 !== "aspect-ratio:" + (($cardInstance._data.control_info.cover_ratio == null || $cardInstance._data.control_info.cover_ratio == 0 ? 1 : $cardInstance._data.control_info.cover_ratio) + ";") + "width:100%;border-top-left-radius:8px;border-top-right-radius:8px;border-bottom-right-radius:0px;border-bottom-left-radius:0px;") {
        __SetStyleObject($n4, [0, 7, 8, 9, 10, {
          95: ($data.control_info.cover_ratio == null || $data.control_info.cover_ratio == 0 ? 1 : $data.control_info.cover_ratio) + ""
        }]);
      }
    }
    {
      let _$value15 = $data.ui_data.cover_area.cover.url_list[0];
      if (!$update || _$value15 !== $cardInstance._data.ui_data.cover_area.cover.url_list[0]) {
        __SetAttribute($n4, "src", _$value15);
      }
    }
  }
  let $n5 = $lepusGetElementRefByLepusID("if", 5);
  $$update_100f540_5($n5, $data, $update);
  let $n17 = $lepusGetElementRefByLepusID("if", 17);
  $$update_100f540_17($n17, $data, $update);
  if ($varUpdateState[1]) {
    let $n33 = $lepusGetElementRefByLepusID("text", 33);
    {
      let _$value16 = "text-overflow:" + ((($data.control_info.title_over_length_truncation == null ? 0 : $data.control_info.title_over_length_truncation) ? "clip" : "ellipsis") + ";") + "width:100%;padding-left:8px;padding-right:8px;margin-top:8px;text-align:left;font-size:14px;color:#161823;overflow:hidden;";
      if (!$update || _$value16 !== "text-overflow:" + ((($cardInstance._data.control_info.title_over_length_truncation == null ? 0 : $cardInstance._data.control_info.title_over_length_truncation) ? "clip" : "ellipsis") + ";") + "width:100%;padding-left:8px;padding-right:8px;margin-top:8px;text-align:left;font-size:14px;color:#161823;overflow:hidden;") {
        __SetStyleObject($n33, [0, 18, 19, 42, 43, 44, 45, 20, {
          46: ($data.control_info.title_over_length_truncation == null ? 0 : $data.control_info.title_over_length_truncation) ? "clip" : "ellipsis"
        }]);
      }
    }
    {
      let _$value17 = $data.control_info.title_limit_line == null ? 2 : $data.control_info.title_limit_line;
      if (!$update || _$value17 !== ($cardInstance._data.control_info.title_limit_line == null ? 2 : $cardInstance._data.control_info.title_limit_line)) {
        __SetAttribute($n33, "text-maxline", _$value17);
      }
    }
  }
  let $n34 = $lepusGetElementRefByLepusID("for", 34);
  $$update_100f540_34($n34, $data, $update);
  let $n42 = $lepusGetElementRefByLepusID("if", 42);
  $$update_100f540_42($n42, $data, $update);
  let $n56 = $lepusGetElementRefByLepusID("if", 56);
  $$update_360f5b0_56($n56, $data, $update);
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
  __SetStyleObject($n1, [0, 1, 2]);
  __SetID($n1, "main_card");
  __AddEventListener($n1, "tap", "onCardClick", {
    closure_type: 3,
    bind_type: 1
  });
  __AppendElement($page, $n1);
  let $n2 = __CreateView($currentComponentId);
  $lepusStoreElementRefByLepusID($n2, 2, "view");
  __SetAttribute($n2, 1004, 2);
  __SetStyleObject($n2, [0, 3, 4, 1, 5, 6]);
  __SetAttribute($n2, "lynx-test-tag", $data.biz_data.is_webcasting ? "groupon_feed_product_card_live" : "groupon_feed_product_card_no_live");
  __AppendElement($n1, $n2);
  let $n3 = __CreateView($currentComponentId);
  __SetAttribute($n3, 1004, 3);
  __SetStyleObject($n3, [0]);
  __AddEventListener($n3, "tap", "onCoverClick", {
    closure_type: 3,
    bind_type: 4
  });
  __AppendElement($n2, $n3);
  let $n4 = __CreateImage($currentComponentId);
  $lepusStoreElementRefByLepusID($n4, 4, "image");
  __SetAttribute($n4, 1004, 4);
  __SetStyleObject($n4, [0, 7, 8, 9, 10, {
    95: ($data.control_info.cover_ratio == null || $data.control_info.cover_ratio == 0 ? 1 : $data.control_info.cover_ratio) + ""
  }]);
  __SetAttribute($n4, "mode", "aspectFill");
  __SetAttribute($n4, "src", $data.ui_data.cover_area.cover.url_list[0]);
  __AppendElement($n3, $n4);
  let $n5 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n5, 5, "if");
  __AppendElement($n3, $n5);
  $$update_100f540_5($n5, $data, $update);
  let $n17 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n17, 17, "if");
  __AppendElement($n3, $n17);
  $$update_100f540_17($n17, $data, $update);
  let $n33 = __CreateText($currentComponentId);
  $lepusStoreElementRefByLepusID($n33, 33, "text");
  __SetAttribute($n33, 1004, 33);
  __SetStyleObject($n33, [0, 18, 19, 42, 43, 44, 45, 20, {
    46: ($data.control_info.title_over_length_truncation == null ? 0 : $data.control_info.title_over_length_truncation) ? "clip" : "ellipsis"
  }]);
  __SetAttribute($n33, "text-maxline", $data.control_info.title_limit_line == null ? 2 : $data.control_info.title_limit_line);
  __AppendElement($n2, $n33);
  let $n34 = __CreateFor($currentComponentId);
  $lepusStoreElementRefByLepusID($n34, 34, "for");
  __AppendElement($n33, $n34);
  $$update_100f540_34($n34, $data, $update);
  let $n42 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n42, 42, "if");
  __AppendElement($n2, $n42);
  $$update_100f540_42($n42, $data, $update);
  let $n56 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n56, 56, "if");
  __AppendElement($n2, $n56);
  $$update_360f5b0_56($n56, $data, $update);
  $airFirstScreen = false;
  $cardVariables = ["biz_data", "control_info", "ui_data", "is_cache", "extra_data"];
  return true;
};
//# sourceMappingURL=http://10.91.108.134:8787/poi_tempo/intermediate/lepus.js.map
