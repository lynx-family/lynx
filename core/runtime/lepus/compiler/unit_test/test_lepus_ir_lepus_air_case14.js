let $$update_2ef9e10_15;
let $$update_2ef9e10_31;
let $$update_2ef9e10_42;
let $$update_2ef9e10_44;
let $$update_2ef9e10_51;
let $$update_2ef9e10_53;
let $$update_2ef9e10_64;
let $$update_2ef9e10_66;
let $$update_2ef9e10_68;
let $$update_100f540_7;
let $$update_100f540_9;
let $$update_100f540_10;
let $$update_100f540_21;
let $$update_100f540_25;
let $$update_100f540_26;
let $$update_100f540_38;
let $$update_100f540_47;
let $$update_100f540_57;
let $$update_100f540_59;
let $$update_100f540_149;
let $$update_100f540_155;
let $$update_1a82dd8_33;
let $$update_1a82dd8_116;
let $$update_1a82dd8_122;
let $$update_1a82dd8_128;
let $$update_1a82dd8_131;
let $$update_1a82dd8_135;
let $$update_1a82dd8_140;
let $$update_1a82dd8_143;
let $$update_1a82dd8_146;
let $$update_1a82dd8_151;
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
function __GetElementByUniqueID(a) {}
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
function __GetElementUniqueID(a) {}
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
  data: {}
};
function __UpdateIfNodeIndex(a,b){}
function __CreateImage(a) {}
function __SetAttribute(a,b,c) {}
function __AppendElement(a,b) {}
function __CreateIf(a) {}
function __CreateText(a) {}
function __CreateElement(a, b) {}
function __CreateView(a) {}
function __CreateFor(a) {}
function _GetLength(a) {}
function __UpdateForChildCount(a, b) {}
let lynx = {}
function __AddEvent(a,b,c,d) {}
function __AddDataset(a,b,c) {}
function __SetID(a,b) {}
function __GetDiffData(a,b,c) {}
function __FlushElementTree(a) {}
function __CreatePage(a,b) {}
function __AddEventListener(a,b,c,d) {}
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
        __SetStyleObject($n14, [18, {
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
        __SetStyleObject($n16, [18, 5, {
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
function $$update_2ef9e10_29($parent, $data, $update2, index, item, tagIndex, tagItem) {
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
        let $n30 = $update2 ? $lepusGetElementRefByLepusID("image", 30) : null;
        let $temp2 = $update2;
        if (!$n30) {
          $update2 = false;
          $n30 = __CreateImage($currentComponentId);
          let $nid30 = $lepusStoreElementRefByLepusID($n30, 30, "image");
          __SetAttribute($n30, 1004, $nid30[1]);
          __SetAttribute($n30, "mode", "scaleToFill");
          __AppendElement($parent, $n30);
        }
        __SetStyleObject($n30, [29, 30, 18, {
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
        __SetAttribute($n30, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp2 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      let $n31 = $update2 ? $lepusGetElementRefByLepusID("if", 31) : null;
      if (!$n31) {
        $update2 = false;
        $n31 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n31, 31, "if");
        __AppendElement($parent, $n31);
      }
      $$update_2ef9e10_31($n31, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp2;
    }
  }
}
$$update_2ef9e10_31 = function ($parent, $data, $update2, index, item) {
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
        let $n32 = $update2 ? $lepusGetElementRefByLepusID("image", 32) : null;
        let $temp2 = $update2;
        if (!$n32) {
          $update2 = false;
          $n32 = __CreateImage($currentComponentId);
          let $nid32 = $lepusStoreElementRefByLepusID($n32, 32, "image");
          __SetAttribute($n32, 1004, $nid32[1]);
          __SetAttribute($n32, "mode", "aspectFill");
          __SetAttribute($n32, "autoplay", "true");
          __SetAttribute($n32, "loop-count", "0");
          __AppendElement($parent, $n32);
        }
        __SetStyleObject($n32, [29, 30, 18, {
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
        __SetAttribute($n32, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
function $$update_2ef9e10_39($parent, $data, $update2, index, item) {
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
        let $n40 = $update2 ? $lepusGetElementRefByLepusID("text", 40) : null;
        let $temp2 = $update2;
        if (!$n40) {
          $update2 = false;
          $n40 = __CreateText($currentComponentId);
          let $nid40 = $lepusStoreElementRefByLepusID($n40, 40, "text");
          __SetAttribute($n40, 1004, $nid40[1]);
          __AppendElement($parent, $n40);
        }
        __SetStyleObject($n40, [47, {
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#161823") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false || item.text_info.is_bold == 0 ? "400" : "500"
        }, {
          47: item.text_info.text_size == null || item.text_info.text_size == 0 ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n40, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp3 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n42 = $update2 ? $lepusGetElementRefByLepusID("if", 42) : null;
      if (!$n42) {
        $update2 = false;
        $n42 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n42, 42, "if");
        __AppendElement($parent, $n42);
      }
      $$update_2ef9e10_42($n42, $data, $update2, index, item);
      $update2 = _$temp3;
    }
  }
}
$$update_2ef9e10_42 = function ($parent, $data, $update2, index, item) {
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
        let $n43 = $update2 ? $lepusGetElementRefByLepusID("image", 43) : null;
        let $temp2 = $update2;
        if (!$n43) {
          $update2 = false;
          $n43 = __CreateImage($currentComponentId);
          let $nid43 = $lepusStoreElementRefByLepusID($n43, 43, "image");
          __SetAttribute($n43, 1004, $nid43[1]);
          __AppendElement($parent, $n43);
        }
        __SetStyleObject($n43, [48, {
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
        __SetAttribute($n43, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp4 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      let $n44 = $update2 ? $lepusGetElementRefByLepusID("if", 44) : null;
      if (!$n44) {
        $update2 = false;
        $n44 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n44, 44, "if");
        __AppendElement($parent, $n44);
      }
      $$update_2ef9e10_44($n44, $data, $update2, index, item);
      $update2 = _$temp4;
    }
  }
};
$$update_2ef9e10_44 = function ($parent, $data, $update2, index, item) {
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
        let $n45 = $update2 ? $lepusGetElementRefByLepusID("image", 45) : null;
        let $temp2 = $update2;
        if (!$n45) {
          $update2 = false;
          $n45 = __CreateImage($currentComponentId);
          let $nid45 = $lepusStoreElementRefByLepusID($n45, 45, "image");
          __SetAttribute($n45, 1004, $nid45[1]);
          __SetAttribute($n45, "autoplay", "true");
          __AppendElement($parent, $n45);
        }
        __SetStyleObject($n45, [{
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
        __SetAttribute($n45, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
function $$update_2ef9e10_48($parent, $data, $update2, index, item) {
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
        let $n49 = $update2 ? $lepusGetElementRefByLepusID("text", 49) : null;
        let $temp2 = $update2;
        if (!$n49) {
          $update2 = false;
          $n49 = __CreateText($currentComponentId);
          let $nid49 = $lepusStoreElementRefByLepusID($n49, 49, "text");
          __SetAttribute($n49, 1004, $nid49[1]);
          __AppendElement($parent, $n49);
        }
        __SetStyleObject($n49, [47, {
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#161823") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false || item.text_info.is_bold == 0 ? "400" : "500"
        }, {
          47: item.text_info.text_size == null || item.text_info.text_size == 0 ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n49, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp5 = $update2;
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
      $$update_2ef9e10_51($n51, $data, $update2, index, item);
      $update2 = _$temp5;
    }
  }
}
$$update_2ef9e10_51 = function ($parent, $data, $update2, index, item) {
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
          __AppendElement($parent, $n52);
        }
        __SetStyleObject($n52, [48, {
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
        __SetAttribute($n52, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp6 = $update2;
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
      $$update_2ef9e10_53($n53, $data, $update2, index, item);
      $update2 = _$temp6;
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
    if (item.type == 3 && item.img_info != null) {
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
          __SetAttribute($n54, "autoplay", "true");
          __AppendElement($parent, $n54);
        }
        __SetStyleObject($n54, [{
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
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
function $$update_2ef9e10_61($parent, $data, $update2, index, item, tagIndex, tagItem) {
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
        let $n62 = $update2 ? $lepusGetElementRefByLepusID("text", 62) : null;
        let $temp2 = $update2;
        if (!$n62) {
          $update2 = false;
          $n62 = __CreateText($currentComponentId);
          let $nid62 = $lepusStoreElementRefByLepusID($n62, 62, "text");
          __SetAttribute($n62, 1004, $nid62[1]);
          __SetAttribute($n62, "text-maxline", "1");
          __AppendElement($parent, $n62);
        }
        __SetStyleObject($n62, [52, 5, 17, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#FFFFFF") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false || item.text_info.is_bold == 0 ? "400" : "500"
        }, {
          47: item.text_info.text_size == null || item.text_info.text_size == 0 ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n62, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp7 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n64 = $update2 ? $lepusGetElementRefByLepusID("if", 64) : null;
      if (!$n64) {
        $update2 = false;
        $n64 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n64, 64, "if");
        __AppendElement($parent, $n64);
      }
      $$update_2ef9e10_64($n64, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp7;
    }
  }
}
$$update_2ef9e10_64 = function ($parent, $data, $update2, index, item, tagIndex, tagItem) {
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
        let $n65 = $update2 ? $lepusGetElementRefByLepusID("image", 65) : null;
        let $temp2 = $update2;
        if (!$n65) {
          $update2 = false;
          $n65 = __CreateImage($currentComponentId);
          let $nid65 = $lepusStoreElementRefByLepusID($n65, 65, "image");
          __SetAttribute($n65, 1004, $nid65[1]);
          __SetAttribute($n65, "mode", "scaleToFill");
          __AppendElement($parent, $n65);
        }
        __SetStyleObject($n65, [18, {
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
        __SetAttribute($n65, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp8 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      let $n66 = $update2 ? $lepusGetElementRefByLepusID("if", 66) : null;
      if (!$n66) {
        $update2 = false;
        $n66 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n66, 66, "if");
        __AppendElement($parent, $n66);
      }
      $$update_2ef9e10_66($n66, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp8;
    }
  }
};
$$update_2ef9e10_66 = function ($parent, $data, $update2, index, item, tagIndex, tagItem) {
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
        let $n67 = $update2 ? $lepusGetElementRefByLepusID("image", 67) : null;
        let $temp2 = $update2;
        if (!$n67) {
          $update2 = false;
          $n67 = __CreateImage($currentComponentId);
          let $nid67 = $lepusStoreElementRefByLepusID($n67, 67, "image");
          __SetAttribute($n67, 1004, $nid67[1]);
          __SetAttribute($n67, "mode", "aspectFill");
          __SetAttribute($n67, "autoplay", "true");
          __SetAttribute($n67, "loop-count", "0");
          __AppendElement($parent, $n67);
        }
        __SetStyleObject($n67, [18, {
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
        __SetAttribute($n67, "src", item.img_info.url_list[0]);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 5);
      $conditionNodeIndex[uniqueId] = 5;
      let _$temp9 = $update2;
      if ($ifNodeIndex !== 5) {
        $update2 = false;
      }
      let $n68 = $update2 ? $lepusGetElementRefByLepusID("if", 68) : null;
      if (!$n68) {
        $update2 = false;
        $n68 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n68, 68, "if");
        __AppendElement($parent, $n68);
      }
      $$update_2ef9e10_68($n68, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp9;
    }
  }
};
$$update_2ef9e10_68 = function ($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.type == 5 && item.countdown_info != null && Date.now() / 1e3 >= item.countdown_info.start_time && Date.now() / 1e3 <= item.countdown_info.end_time) {
      __UpdateIfNodeIndex($parent, 6);
      $conditionNodeIndex[uniqueId] = 6;
      let $temp = $update2;
      if ($ifNodeIndex !== 6) {
        $update2 = false;
      }
      {
        let $n69 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 69) : null;
        let $temp2 = $update2;
        if (!$n69) {
          $update2 = false;
          $n69 = __CreateElement("countdown-view", $currentComponentId);
          let $nid69 = $lepusStoreElementRefByLepusID($n69, 69, "countdown-view");
          __SetAttribute($n69, 1004, $nid69[1]);
          __SetAttribute($n69, "gone-after-end", false);
          __SetAttribute($n69, "unit", "seconds");
          __AppendElement($parent, $n69);
        }
        __SetStyleObject($n69, [6, 53, 54, 30, 55, 56, 25, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }]);
        __SetAttribute($n69, "end-time", "" + item.countdown_info.end_time);
        {
          let $n70 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 70) : null;
          let $temp3 = $update2;
          if (!$n70) {
            $update2 = false;
            $n70 = __CreateElement("countdown-item", $currentComponentId);
            let $nid70 = $lepusStoreElementRefByLepusID($n70, 70, "countdown-item");
            __SetAttribute($n70, 1004, $nid70[1]);
            __SetAttribute($n70, "text-maxline", "1");
            __SetAttribute($n70, "countdown-display", "HH");
            __AppendElement($n69, $n70);
          }
          __SetStyleObject($n70, [47, 53, 57, 58, {
            47: (item.countdown_info.text_size != null ? item.countdown_info.text_size : 12) + "px"
          }, {
            48: "400"
          }, {
            22: (item.countdown_info.text_color != null ? item.countdown_info.text_color : "#ff1c49") + ""
          }, {
            27: (__globalProps.os == "ios" ? 15 : 16) * parseFloat($data.control_info.font_scale) + "px"
          }]);
          $update2 = $temp3;
        }
        {
          let $n71 = $update2 ? $lepusGetElementRefByLepusID("text", 71) : null;
          let _$temp10 = $update2;
          if (!$n71) {
            $update2 = false;
            $n71 = __CreateText($currentComponentId);
            let $nid71 = $lepusStoreElementRefByLepusID($n71, 71, "text");
            __SetAttribute($n71, 1004, $nid71[1]);
            __AppendElement($n69, $n71);
          }
          __SetStyleObject($n71, [{
            47: (item.countdown_info.text_size != null ? item.countdown_info.text_size : 12) + "px"
          }, {
            48: "400"
          }, {
            22: (item.countdown_info.text_color != null ? item.countdown_info.text_color : "#ff1c49") + ""
          }]);
          __SetAttribute($n71, "text", ":");
          $update2 = _$temp10;
        }
        {
          let $n73 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 73) : null;
          let _$temp11 = $update2;
          if (!$n73) {
            $update2 = false;
            $n73 = __CreateElement("countdown-item", $currentComponentId);
            let $nid73 = $lepusStoreElementRefByLepusID($n73, 73, "countdown-item");
            __SetAttribute($n73, 1004, $nid73[1]);
            __SetAttribute($n73, "text-maxline", "1");
            __SetAttribute($n73, "countdown-display", "mm");
            __AppendElement($n69, $n73);
          }
          __SetStyleObject($n73, [53, 57, 58, 47, {
            47: (item.countdown_info.text_size != null ? item.countdown_info.text_size : 12) + "px"
          }, {
            48: "400"
          }, {
            22: (item.countdown_info.text_color != null ? item.countdown_info.text_color : "#ff1c49") + ""
          }, {
            27: (__globalProps.os == "ios" ? 15 : 16) * parseFloat($data.control_info.font_scale) + "px"
          }]);
          $update2 = _$temp11;
        }
        {
          let $n74 = $update2 ? $lepusGetElementRefByLepusID("text", 74) : null;
          let _$temp12 = $update2;
          if (!$n74) {
            $update2 = false;
            $n74 = __CreateText($currentComponentId);
            let $nid74 = $lepusStoreElementRefByLepusID($n74, 74, "text");
            __SetAttribute($n74, 1004, $nid74[1]);
            __AppendElement($n69, $n74);
          }
          __SetStyleObject($n74, [{
            47: (item.countdown_info.text_size != null ? item.countdown_info.text_size : 12) + "px"
          }, {
            48: "400"
          }, {
            22: (item.countdown_info.text_color != null ? item.countdown_info.text_color : "#ff1c49") + ""
          }]);
          __SetAttribute($n74, "text", ":");
          $update2 = _$temp12;
        }
        {
          let $n76 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 76) : null;
          let _$temp13 = $update2;
          if (!$n76) {
            $update2 = false;
            $n76 = __CreateElement("countdown-item", $currentComponentId);
            let $nid76 = $lepusStoreElementRefByLepusID($n76, 76, "countdown-item");
            __SetAttribute($n76, 1004, $nid76[1]);
            __SetAttribute($n76, "text-maxline", "1");
            __SetAttribute($n76, "countdown-display", "ss");
            __AppendElement($n69, $n76);
          }
          __SetStyleObject($n76, [53, 57, 47, 58, {
            47: (item.countdown_info.text_size != null ? item.countdown_info.text_size : 12) + "px"
          }, {
            48: "400"
          }, {
            22: (item.countdown_info.text_color != null ? item.countdown_info.text_color : "#ff1c49") + ""
          }, {
            27: (__globalProps.os == "ios" ? 15 : 16) * parseFloat($data.control_info.font_scale) + "px"
          }]);
          $update2 = _$temp13;
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 7);
      $conditionNodeIndex[uniqueId] = 7;
      let _$temp14 = $update2;
      if ($ifNodeIndex !== 7) {
        $update2 = false;
      }
      {
        let $n77 = $update2 ? $lepusGetElementRefByLepusID("view", 77) : null;
        let _$temp15 = $update2;
        if (!$n77) {
          $update2 = false;
          $n77 = __CreateView($currentComponentId);
          let $nid77 = $lepusStoreElementRefByLepusID($n77, 77, "view");
          __SetAttribute($n77, 1004, $nid77[1]);
          __AppendElement($parent, $n77);
        }
        $update2 = _$temp15;
      }
      $update2 = _$temp14;
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
            __SetStyleObject($n6, [8, 9, 10, 11, 12, 13, 0, 14, 15, 16]);
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
    let _$lepusPushFiberForNo = $lepusPushFiberForNode($parent, 7, uniqueId),
        $forLepus = _$lepusPushFiberForNo[0],
        $lastForLepus = _$lepusPushFiberForNo[1];
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
          __SetAttribute($n8, "flatten", "false");
          __AppendElement($parent, $n8);
        }
        __SetStyleObject($n8, [5, {
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
    let _$lepusPushFiberForNo2 = $lepusPushFiberForNode($parent, 9, uniqueId),
        $forLepus = _$lepusPushFiberForNo2[0],
        $lastForLepus = _$lepusPushFiberForNo2[1];
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
        __SetStyleObject($n11, [5, 17, {
          51: (tagIndex == ((_b = (_a = $data.ui_data.cover_area.cover_top_info) == null ? undefined : _a.tags) == null ? undefined : _b.length) - 1 ? 1 : 0) + ""
        }, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#FFFFFF") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false || item.text_info.is_bold == 0 ? "400" : "500"
        }, {
          47: item.text_info.text_size == null || item.text_info.text_size == 0 ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n11, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp16 = $update2;
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
      $update2 = _$temp16;
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
            __SetStyleObject($n18, [8, 19, 10, 0, 20, 21]);
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
                  __SetStyleObject($n19, [8, 22, 10, 23, 0, 14, 15, 16, {
                    34: $data.ui_data.cover_area.cover_bottom_info.has_ad_tag ? "36px" : "8px"
                  }]);
                }
              }
            }
            {
              let $n20 = $update2 ? $lepusGetElementRefByLepusID("view", 20) : null;
              let $temp4 = $update2;
              if (!$n20) {
                $update2 = false;
                $n20 = __CreateView($currentComponentId);
                let $nid20 = $lepusStoreElementRefByLepusID($n20, 20, "view");
                __SetAttribute($n20, 1004, $nid20[1]);
                __SetAttribute($n20, "clip-radius", true);
                __AppendElement($n19, $n20);
              }
              if (!$update2 || $varUpdateState[2]) {
                {
                  let _$value = "border-radius:" + ($data.ui_data.cover_area.cover_bottom_info.background_round + "px;") + "overflow:hidden;linear-orientation:row;width:auto;height:auto;";
                  if (!$update2 || _$value !== "border-radius:" + ($cardInstance._data.ui_data.cover_area.cover_bottom_info.background_round + "px;") + "overflow:hidden;linear-orientation:row;width:auto;height:auto;") {
                    __SetStyleObject($n20, [5, 23, 24, 14, {
                      12: $data.ui_data.cover_area.cover_bottom_info.background_round + "px"
                    }]);
                  }
                }
              }
              if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
                let $n21 = $update2 ? $lepusGetElementRefByLepusID("for", 21) : null;
                if (!$n21) {
                  $n21 = __CreateFor($currentComponentId);
                  $lepusStoreElementRefByLepusID($n21, 21, "for");
                  __AppendElement($n20, $n21);
                }
                $$update_100f540_21($n21, $data, $update2);
              }
              $update2 = $temp4;
            }
            {
              let $template_update = $update2;
              let $n33 = $update2 ? $lepusGetElementRefByLepusID("if", 33) : null;
              if (!$n33) {
                $update2 = false;
                $n33 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n33, 33, "if");
                __AppendElement($n19, $n33);
              }
              $$update_1a82dd8_33($n33, $data, $update2);
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
$$update_100f540_21 = function ($parent, $data, $update2) {
  let _a, _b;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo3 = $lepusPushFiberForNode($parent, 21, uniqueId),
        $forLepus = _$lepusPushFiberForNo3[0],
        $lastForLepus = _$lepusPushFiberForNo3[1];
    let $object = $data.ui_data.cover_area.cover_bottom_info.tags;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let tagIndex = 0; tagIndex < $length; ++tagIndex) {
      $update2 = tagIndex < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(tagIndex);
      let tagItem = $object[tagIndex];
      {
        let $n22 = $update2 ? $lepusGetElementRefByLepusID("view", 22) : null;
        let $temp2 = $update2;
        if (!$n22) {
          $update2 = false;
          $n22 = __CreateView($currentComponentId);
          let $nid22 = $lepusStoreElementRefByLepusID($n22, 22, "view");
          __SetAttribute($n22, 1004, $nid22[1]);
          __AppendElement($parent, $n22);
        }
        __SetStyleObject($n22, [25, {
          38: tagIndex == 0 ? tagItem.margin_left != null ? tagItem.margin_left + "px" : "0px" : tagItem.margin_left != null ? tagItem.margin_left + "px" : "4px"
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
        }, {
          55: tagItem.align_items + ""
        }]);
        {
          let $n23 = $update2 ? $lepusGetElementRefByLepusID("if", 23) : null;
          if (!$n23) {
            $n23 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n23, 23, "if");
            __AppendElement($n22, $n23);
          }
          let uniqueId2 = __GetElementUniqueID($n23);
          if (!$update2) {
            $conditionNodeIndex[uniqueId2] = -1;
          }
          let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
          if (tagItem.background_image.url_list[0] != null) {
            __UpdateIfNodeIndex($n23, 0);
            $conditionNodeIndex[uniqueId2] = 0;
            let $temp3 = $update2;
            if ($ifNodeIndex !== 0) {
              $update2 = false;
            }
            {
              let $n24 = $update2 ? $lepusGetElementRefByLepusID("image", 24) : null;
              let $temp4 = $update2;
              if (!$n24) {
                $update2 = false;
                $n24 = __CreateImage($currentComponentId);
                let $nid24 = $lepusStoreElementRefByLepusID($n24, 24, "image");
                __SetAttribute($n24, 1004, $nid24[1]);
                __SetAttribute($n24, "mode", "aspectFill");
                __AppendElement($n23, $n24);
              }
              __SetStyleObject($n24, [8, 0, 26, {
                12: tagItem.background_round + "px"
              }]);
              __SetAttribute($n24, "src", tagItem.background_image.url_list[0]);
              $update2 = $temp4;
            }
            $update2 = $temp3;
          } else {
            __UpdateIfNodeIndex($n23, -1);
            $conditionNodeIndex[uniqueId2] = -1;
          }
        }
        if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
          let $n25 = $update2 ? $lepusGetElementRefByLepusID("for", 25) : null;
          if (!$n25) {
            $n25 = __CreateFor($currentComponentId);
            $lepusStoreElementRefByLepusID($n25, 25, "for");
            __AppendElement($n22, $n25);
          }
          $$update_100f540_25($n25, $data, $update2, tagIndex, tagItem);
        }
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
$$update_100f540_25 = function ($parent, $data, $update2, tagIndex, tagItem) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo4 = $lepusPushFiberForNode($parent, 25, uniqueId),
        $forLepus = _$lepusPushFiberForNo4[0],
        $lastForLepus = _$lepusPushFiberForNo4[1];
    let $object = tagItem.items;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      let $n26 = $update2 ? $lepusGetElementRefByLepusID("if", 26) : null;
      if (!$n26) {
        $n26 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n26, 26, "if");
        __AppendElement($parent, $n26);
      }
      $$update_100f540_26($n26, $data, $update2, index, item, tagIndex, tagItem);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
$$update_100f540_26 = function ($parent, $data, $update2, index, item, tagIndex, tagItem) {
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
        let $n27 = $update2 ? $lepusGetElementRefByLepusID("text", 27) : null;
        let $temp2 = $update2;
        if (!$n27) {
          $update2 = false;
          $n27 = __CreateText($currentComponentId);
          let $nid27 = $lepusStoreElementRefByLepusID($n27, 27, "text");
          __SetAttribute($n27, 1004, $nid27[1]);
          __SetAttribute($n27, "text-maxline", "1");
          __SetAttribute($n27, "text-single-line-vertical-align", "center");
          __AppendElement($parent, $n27);
        }
        __SetStyleObject($n27, [27, 28, 5, 17, {
          51: (tagIndex == $data.ui_data.cover_area.cover_bottom_info.tags.length - 1 ? 1 : 0) + ""
        }, {
          39: item.right_margin + "px"
        }, {
          38: item.left_margin + "px"
        }, {
          22: (item.text_info.text_color != null ? item.text_info.text_color : "#FFFFFF") + ""
        }, {
          48: item.text_info.is_bold == null || item.text_info.is_bold == false || item.text_info.is_bold == 0 ? "400" : "500"
        }, {
          47: item.text_info.text_size == null || item.text_info.text_size == 0 ? "12px" : item.text_info.text_size + "px"
        }]);
        __SetAttribute($n27, "text", item.text_info.text);
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp17 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n29 = $update2 ? $lepusGetElementRefByLepusID("if", 29) : null;
      if (!$n29) {
        $update2 = false;
        $n29 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n29, 29, "if");
        __AppendElement($parent, $n29);
      }
      $$update_2ef9e10_29($n29, $data, $update2, index, item, tagIndex, tagItem);
      $update2 = _$temp17;
    }
  }
};
function $$update_100f540_36($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[1] || $varUpdateState[2] || $varUpdateState[5]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (lynx.__globalProps.os != "ios") {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n37 = $update2 ? $lepusGetElementRefByLepusID("text", 37) : null;
          let $temp2 = $update2;
          if (!$n37) {
            $update2 = false;
            $n37 = __CreateText($currentComponentId);
            let $nid37 = $lepusStoreElementRefByLepusID($n37, 37, "text");
            __SetAttribute($n37, 1004, $nid37[1]);
            __AppendElement($parent, $n37);
          }
          if (!$update2 || $varUpdateState[1]) {
            {
              let $value = "text-overflow:" + (($data.control_info.title_over_length_truncation == null || $data.control_info.title_over_length_truncation == 0 ? "clip" : "ellipsis") + ";") + "width:100%;padding-left:8px;padding-right:8px;margin-top:8px;text-align:left;font-size:14px;color:#161823;overflow:hidden;line-height:20px;";
              if (!$update2 || $value !== "text-overflow:" + (($cardInstance._data.control_info.title_over_length_truncation == null || $cardInstance._data.control_info.title_over_length_truncation == 0 ? "clip" : "ellipsis") + ";") + "width:100%;padding-left:8px;padding-right:8px;margin-top:8px;text-align:left;font-size:14px;color:#161823;overflow:hidden;line-height:20px;") {
                __SetStyleObject($n37, [0, 15, 16, 42, 43, 44, 45, 5, 46, {
                  46: $data.control_info.title_over_length_truncation == null || $data.control_info.title_over_length_truncation == 0 ? "clip" : "ellipsis"
                }]);
              }
            }
            {
              let _$value2 = $data.control_info.title_limit_line == null || $data.control_info.title_limit_line == 0 ? 2 : $data.control_info.title_limit_line;
              if (!$update2 || _$value2 !== ($cardInstance._data.control_info.title_limit_line == null || $cardInstance._data.control_info.title_limit_line == 0 ? 2 : $cardInstance._data.control_info.title_limit_line)) {
                __SetAttribute($n37, "text-maxline", _$value2);
              }
            }
          }
          if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
            let $n38 = $update2 ? $lepusGetElementRefByLepusID("for", 38) : null;
            if (!$n38) {
              $n38 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n38, 38, "for");
              __AppendElement($n37, $n38);
            }
            $$update_100f540_38($n38, $data, $update2);
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp18 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n46 = $update2 ? $lepusGetElementRefByLepusID("text", 46) : null;
          let _$temp19 = $update2;
          if (!$n46) {
            $update2 = false;
            $n46 = __CreateText($currentComponentId);
            let $nid46 = $lepusStoreElementRefByLepusID($n46, 46, "text");
            __SetAttribute($n46, 1004, $nid46[1]);
            __AppendElement($parent, $n46);
          }
          if (!$update2 || $varUpdateState[1]) {
            {
              let _$value3 = "text-overflow:" + (($data.control_info.title_over_length_truncation == null || $data.control_info.title_over_length_truncation == 0 ? "clip" : "ellipsis") + ";") + "width:100%;padding-left:8px;padding-right:8px;margin-top:8px;text-align:left;font-size:14px;color:#161823;overflow:hidden;line-height:20px;";
              if (!$update2 || _$value3 !== "text-overflow:" + (($cardInstance._data.control_info.title_over_length_truncation == null || $cardInstance._data.control_info.title_over_length_truncation == 0 ? "clip" : "ellipsis") + ";") + "width:100%;padding-left:8px;padding-right:8px;margin-top:8px;text-align:left;font-size:14px;color:#161823;overflow:hidden;line-height:20px;") {
                __SetStyleObject($n46, [0, 15, 16, 42, 43, 44, 45, 5, 46, {
                  46: $data.control_info.title_over_length_truncation == null || $data.control_info.title_over_length_truncation == 0 ? "clip" : "ellipsis"
                }]);
              }
            }
            {
              let _$value4 = $data.control_info.title_limit_line == null || $data.control_info.title_limit_line == 0 ? 2 : $data.control_info.title_limit_line;
              if (!$update2 || _$value4 !== ($cardInstance._data.control_info.title_limit_line == null || $cardInstance._data.control_info.title_limit_line == 0 ? 2 : $cardInstance._data.control_info.title_limit_line)) {
                __SetAttribute($n46, "text-maxline", _$value4);
              }
            }
          }
          if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
            let $n47 = $update2 ? $lepusGetElementRefByLepusID("for", 47) : null;
            if (!$n47) {
              $n47 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n47, 47, "for");
              __AppendElement($n46, $n47);
            }
            $$update_100f540_47($n47, $data, $update2);
          }
          $update2 = _$temp19;
        }
        $update2 = _$temp18;
      }
    }
  }
}
$$update_100f540_38 = function ($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo5 = $lepusPushFiberForNode($parent, 38, uniqueId),
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
      let $n39 = $update2 ? $lepusGetElementRefByLepusID("if", 39) : null;
      if (!$n39) {
        $n39 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n39, 39, "if");
        __AppendElement($parent, $n39);
      }
      $$update_2ef9e10_39($n39, $data, $update2, index, item);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
$$update_100f540_47 = function ($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo6 = $lepusPushFiberForNode($parent, 47, uniqueId),
        $forLepus = _$lepusPushFiberForNo6[0],
        $lastForLepus = _$lepusPushFiberForNo6[1];
    let $object = $data.ui_data.info_area.title_info.items;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      let $n48 = $update2 ? $lepusGetElementRefByLepusID("if", 48) : null;
      if (!$n48) {
        $n48 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n48, 48, "if");
        __AppendElement($parent, $n48);
      }
      $$update_2ef9e10_48($n48, $data, $update2, index, item);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
function $$update_100f540_55($parent, $data, $update2) {
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
          let $n56 = $update2 ? $lepusGetElementRefByLepusID("view", 56) : null;
          let $temp2 = $update2;
          if (!$n56) {
            $update2 = false;
            $n56 = __CreateView($currentComponentId);
            let $nid56 = $lepusStoreElementRefByLepusID($n56, 56, "view");
            __SetAttribute($n56, 1004, $nid56[1]);
            __AppendElement($parent, $n56);
          }
          if (!$update2 || $varUpdateState[2]) {
            {
              let $value = "max-height:" + ($data.ui_data.info_area.recommend_reason.tags_max_height + "px;") + "display:flex;flex-direction:row;flex-wrap:wrap;overflow:hidden;width:100%;padding-left:8px;padding-right:8px;margin-top:4px;";
              if (!$update2 || $value !== "max-height:" + ($cardInstance._data.ui_data.info_area.recommend_reason.tags_max_height + "px;") + "display:flex;flex-direction:row;flex-wrap:wrap;overflow:hidden;width:100%;padding-left:8px;padding-right:8px;margin-top:4px;") {
                __SetStyleObject($n56, [49, 50, 13, 5, 0, 15, 16, 51, {
                  30: $data.ui_data.info_area.recommend_reason.tags_max_height + "px"
                }]);
              }
            }
          }
          if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
            let $n57 = $update2 ? $lepusGetElementRefByLepusID("for", 57) : null;
            if (!$n57) {
              $n57 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n57, 57, "for");
              __AppendElement($n56, $n57);
            }
            $$update_100f540_57($n57, $data, $update2);
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
$$update_100f540_57 = function ($parent, $data, $update2) {
  let _a, _b;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo7 = $lepusPushFiberForNode($parent, 57, uniqueId),
        $forLepus = _$lepusPushFiberForNo7[0],
        $lastForLepus = _$lepusPushFiberForNo7[1];
    let $object = $data.ui_data.info_area.recommend_reason.tags;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let tagIndex = 0; tagIndex < $length; ++tagIndex) {
      $update2 = tagIndex < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(tagIndex);
      let tagItem = $object[tagIndex];
      {
        let $n58 = $update2 ? $lepusGetElementRefByLepusID("view", 58) : null;
        let $temp2 = $update2;
        if (!$n58) {
          $update2 = false;
          $n58 = __CreateView($currentComponentId);
          let $nid58 = $lepusStoreElementRefByLepusID($n58, 58, "view");
          __SetAttribute($n58, 1004, $nid58[1]);
          __AddEvent($n58, "bindEvent", "layoutchange", "onLayoutChange");
          __AppendElement($parent, $n58);
        }
        __SetStyleObject($n58, [49, 50, 6, {
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
          51: (tagIndex == ((_b = (_a = $data.ui_data.info_area.recommend_reason) == null ? undefined : _a.tags) == null ? undefined : _b.length) - 1 ? 1 : 0) + ""
        }]);
        __AddDataset($n58, "index", tagIndex);
        if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
          let $n59 = $update2 ? $lepusGetElementRefByLepusID("for", 59) : null;
          if (!$n59) {
            $n59 = __CreateFor($currentComponentId);
            $lepusStoreElementRefByLepusID($n59, 59, "for");
            __AppendElement($n58, $n59);
          }
          $$update_100f540_59($n59, $data, $update2, tagIndex, tagItem);
        }
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
$$update_100f540_59 = function ($parent, $data, $update2, tagIndex, tagItem) {
  let _a, _b;
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1] || $varUpdateState[5]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo8 = $lepusPushFiberForNode($parent, 59, uniqueId),
        $forLepus = _$lepusPushFiberForNo8[0],
        $lastForLepus = _$lepusPushFiberForNo8[1];
    let $object = tagItem.items;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      {
        let $n60 = $update2 ? $lepusGetElementRefByLepusID("view", 60) : null;
        let $temp2 = $update2;
        if (!$n60) {
          $update2 = false;
          $n60 = __CreateView($currentComponentId);
          let $nid60 = $lepusStoreElementRefByLepusID($n60, 60, "view");
          __SetAttribute($n60, 1004, $nid60[1]);
          __AppendElement($parent, $n60);
        }
        __SetStyleObject($n60, [49, 50, {
          51: (tagIndex == ((_b = (_a = $data.ui_data.info_area.recommend_reason) == null ? undefined : _a.tags) == null ? undefined : _b.length) - 1 ? 1 : 0) + ""
        }]);
        let $n61 = $update2 ? $lepusGetElementRefByLepusID("if", 61) : null;
        if (!$n61) {
          $n61 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n61, 61, "if");
          __AppendElement($n60, $n61);
        }
        $$update_2ef9e10_61($n61, $data, $update2, index, item, tagIndex, tagItem);
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
function $$update_100f540_119($parent, $data, $update2) {
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
        let $n120 = $update2 ? $lepusGetElementRefByLepusID("view", 120) : null;
        let $temp2 = $update2;
        if (!$n120) {
          $update2 = false;
          $n120 = __CreateView($currentComponentId);
          let $nid120 = $lepusStoreElementRefByLepusID($n120, 120, "view");
          __SetAttribute($n120, 1004, $nid120[1]);
          __SetStyleObject($n120, [49, 13, 72, 30, 18, 51]);
          __AppendElement($parent, $n120);
        }
        {
          let $n121 = $update2 ? $lepusGetElementRefByLepusID("view", 121) : null;
          let $temp3 = $update2;
          if (!$n121) {
            $update2 = false;
            $n121 = __CreateView($currentComponentId);
            let $nid121 = $lepusStoreElementRefByLepusID($n121, 121, "view");
            __SetAttribute($n121, 1004, $nid121[1]);
            __SetStyleObject($n121, [11, 6, 18, 26, 72]);
            __AppendElement($n120, $n121);
          }
          {
            let $template_update = $update2;
            let $n122 = $update2 ? $lepusGetElementRefByLepusID("if", 122) : null;
            if (!$n122) {
              $update2 = false;
              $n122 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n122, 122, "if");
              __AppendElement($n121, $n122);
            }
            $$update_1a82dd8_122($n122, $data, $update2);
            $update2 = $template_update;
          }
          {
            let $n134 = $update2 ? $lepusGetElementRefByLepusID("image", 134) : null;
            let $temp4 = $update2;
            if (!$n134) {
              $update2 = false;
              $n134 = __CreateImage($currentComponentId);
              let $nid134 = $lepusStoreElementRefByLepusID($n134, 134, "image");
              __SetAttribute($n134, 1004, $nid134[1]);
              __AppendElement($n121, $n134);
            }
            if (!$update2 || $varUpdateState[2]) {
              {
                let $value = "width:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.bgRightStyle.width + ";") + "height:100%;flex-shrink:0;width:7px;";
                if (!$update2 || $value !== "width:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.bgRightStyle.width + ";") + "height:100%;flex-shrink:0;width:7px;") {
                  __SetStyleObject($n134, [26, 18, 79, {
                    27: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.bgRightStyle.width + ""
                  }]);
                }
              }
              {
                let _$value5 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.bgRight;
                if (!$update2 || _$value5 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.bgRight) {
                  __SetAttribute($n134, "src", _$value5);
                }
              }
            }
            $update2 = $temp4;
          }
          $update2 = $temp3;
        }
        {
          let _$template_update = $update2;
          let $n135 = $update2 ? $lepusGetElementRefByLepusID("if", 135) : null;
          if (!$n135) {
            $update2 = false;
            $n135 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n135, 135, "if");
            __AppendElement($n120, $n135);
          }
          $$update_1a82dd8_135($n135, $data, $update2);
          $update2 = _$template_update;
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp20 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let _$template_update2 = $update2;
        let $n149 = $update2 ? $lepusGetElementRefByLepusID("if", 149) : null;
        if (!$n149) {
          $update2 = false;
          $n149 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n149, 149, "if");
          __AppendElement($parent, $n149);
        }
        $$update_100f540_149($n149, $data, $update2);
        $update2 = _$template_update2;
      }
      $update2 = _$temp20;
    }
  }
}
$$update_100f540_149 = function ($parent, $data, $update2) {
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
        let $n150 = $update2 ? $lepusGetElementRefByLepusID("view", 150) : null;
        let $temp2 = $update2;
        if (!$n150) {
          $update2 = false;
          $n150 = __CreateView($currentComponentId);
          let $nid150 = $lepusStoreElementRefByLepusID($n150, 150, "view");
          __SetAttribute($n150, 1004, $nid150[1]);
          __SetStyleObject($n150, [49, 85, 55, 6, 53, 86, 87, 88, 89, 90]);
          __AppendElement($parent, $n150);
        }
        {
          let $template_update = $update2;
          let $n151 = $update2 ? $lepusGetElementRefByLepusID("if", 151) : null;
          if (!$n151) {
            $update2 = false;
            $n151 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n151, 151, "if");
            __AppendElement($n150, $n151);
          }
          $$update_1a82dd8_151($n151, $data, $update2);
          $update2 = $template_update;
        }
        {
          let $n153 = $update2 ? $lepusGetElementRefByLepusID("text", 153) : null;
          let $temp3 = $update2;
          if (!$n153) {
            $update2 = false;
            $n153 = __CreateText($currentComponentId);
            let $nid153 = $lepusStoreElementRefByLepusID($n153, 153, "text");
            __SetAttribute($n153, 1004, $nid153[1]);
            __SetStyleObject($n153, [63, 93, 94, 95]);
            __AppendElement($n150, $n153);
          }
          {
            if (!$update2 || $varUpdateState[2]) {
              let $value = $data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagText;
              if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagText) {
                __SetAttribute($n153, "text", $value);
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
      let _$temp21 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      {
        let _$template_update3 = $update2;
        let $n155 = $update2 ? $lepusGetElementRefByLepusID("if", 155) : null;
        if (!$n155) {
          $update2 = false;
          $n155 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n155, 155, "if");
          __AppendElement($parent, $n155);
        }
        $$update_100f540_155($n155, $data, $update2);
        $update2 = _$template_update3;
      }
      $update2 = _$temp21;
    }
  }
};
$$update_100f540_155 = function ($parent, $data, $update2) {
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
        let $n156 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 156) : null;
        let $temp2 = $update2;
        if (!$n156) {
          $update2 = false;
          $n156 = __CreateElement("countdown-view", $currentComponentId);
          let $nid156 = $lepusStoreElementRefByLepusID($n156, 156, "countdown-view");
          __SetAttribute($n156, 1004, $nid156[1]);
          __SetStyleObject($n156, [6, 53, 54, 88, 55, 86, 96, 89, 90, 51, 56, 25]);
          __SetAttribute($n156, "gone-after-end", "true");
          __SetAttribute($n156, "unit", "seconds");
          __SetID($n156, "countdown");
          __AddEvent($n156, "bindEvent", "countdownend", "onCountDownEnd");
          __AppendElement($parent, $n156);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "" + $data.ui_data.info_area.sale_info.price_info_vo.seckill_info_vo.end_time;
            if (!$update2 || $value !== "" + $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.seckill_info_vo.end_time) {
              __SetAttribute($n156, "end-time", $value);
            }
          }
        }
        {
          let $n157 = $update2 ? $lepusGetElementRefByLepusID("text", 157) : null;
          let $temp3 = $update2;
          if (!$n157) {
            $update2 = false;
            $n157 = __CreateText($currentComponentId);
            let $nid157 = $lepusStoreElementRefByLepusID($n157, 157, "text");
            __SetAttribute($n157, 1004, $nid157[1]);
            __SetStyleObject($n157, [47, 63, 95, 93, 97]);
            __AppendElement($n156, $n157);
          }
          __SetAttribute($n157, "text", "Flash Sale");
          $update2 = $temp3;
        }
        {
          let $n159 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 159) : null;
          let _$temp22 = $update2;
          if (!$n159) {
            $update2 = false;
            $n159 = __CreateElement("countdown-item", $currentComponentId);
            let $nid159 = $lepusStoreElementRefByLepusID($n159, 159, "countdown-item");
            __SetAttribute($n159, 1004, $nid159[1]);
            __SetAttribute($n159, "text-maxline", "1");
            __SetAttribute($n159, "countdown-display", "HH");
            __AppendElement($n156, $n159);
          }
          if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
            {
              let _$value6 = "width:" + ((__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px;") + "white-space:nowrap;justify-content:center;align-items:baseline;text-align:center;font-size:10px;font-weight:500;color:#ff1c49;";
              if (!$update2 || _$value6 !== undefined) {
                __SetStyleObject($n159, [47, 53, 57, 58, 63, 40, 93, {
                  27: (__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px"
                }]);
              }
            }
          }
          $update2 = _$temp22;
        }
        {
          let $n160 = $update2 ? $lepusGetElementRefByLepusID("text", 160) : null;
          let _$temp23 = $update2;
          if (!$n160) {
            $update2 = false;
            $n160 = __CreateText($currentComponentId);
            let $nid160 = $lepusStoreElementRefByLepusID($n160, 160, "text");
            __SetAttribute($n160, 1004, $nid160[1]);
            __SetStyleObject($n160, [63, 40, 93]);
            __AppendElement($n156, $n160);
          }
          __SetAttribute($n160, "text", ":");
          $update2 = _$temp23;
        }
        {
          let $n162 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 162) : null;
          let _$temp24 = $update2;
          if (!$n162) {
            $update2 = false;
            $n162 = __CreateElement("countdown-item", $currentComponentId);
            let $nid162 = $lepusStoreElementRefByLepusID($n162, 162, "countdown-item");
            __SetAttribute($n162, 1004, $nid162[1]);
            __SetAttribute($n162, "text-maxline", "1");
            __SetAttribute($n162, "countdown-display", "mm");
            __AppendElement($n156, $n162);
          }
          if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
            {
              let _$value7 = "width:" + ((__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px;") + "justify-content:center;align-items:baseline;text-align:center;font-size:10px;font-weight:500;color:#ff1c49;white-space:nowrap;";
              if (!$update2 || _$value7 !== undefined) {
                __SetStyleObject($n162, [53, 57, 58, 63, 40, 93, 47, {
                  27: (__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px"
                }]);
              }
            }
          }
          $update2 = _$temp24;
        }
        {
          let $n163 = $update2 ? $lepusGetElementRefByLepusID("text", 163) : null;
          let _$temp25 = $update2;
          if (!$n163) {
            $update2 = false;
            $n163 = __CreateText($currentComponentId);
            let $nid163 = $lepusStoreElementRefByLepusID($n163, 163, "text");
            __SetAttribute($n163, 1004, $nid163[1]);
            __SetStyleObject($n163, [63, 40, 93]);
            __AppendElement($n156, $n163);
          }
          __SetAttribute($n163, "text", ":");
          $update2 = _$temp25;
        }
        {
          let $n165 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 165) : null;
          let _$temp26 = $update2;
          if (!$n165) {
            $update2 = false;
            $n165 = __CreateElement("countdown-item", $currentComponentId);
            let $nid165 = $lepusStoreElementRefByLepusID($n165, 165, "countdown-item");
            __SetAttribute($n165, 1004, $nid165[1]);
            __SetAttribute($n165, "text-maxline", "1");
            __SetAttribute($n165, "countdown-display", "ss");
            __AppendElement($n156, $n165);
          }
          if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
            {
              let _$value8 = "width:" + ((__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px;") + "justify-content:center;align-items:baseline;white-space:nowrap;text-align:center;font-size:10px;font-weight:500;color:#ff1c49;";
              if (!$update2 || _$value8 !== undefined) {
                __SetStyleObject($n165, [53, 57, 47, 58, 63, 40, 93, {
                  27: (__globalProps.os == "ios" ? 14 : 15) * parseFloat($data.control_info.font_scale) + "px"
                }]);
              }
            }
          }
          $update2 = _$temp26;
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
$$update_1a82dd8_33 = function ($parent, $data, $update2) {
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
        let $n34 = $update2 ? $lepusGetElementRefByLepusID("text", 34) : null;
        let $temp2 = $update2;
        if (!$n34) {
          $update2 = false;
          $n34 = __CreateText($currentComponentId);
          let $nid34 = $lepusStoreElementRefByLepusID($n34, 34, "text");
          __SetAttribute($n34, 1004, $nid34[1]);
          __SetStyleObject($n34, [8, 31, 32, 18, 33, 34, 35, 36, 37, 38, 39, 40, 41]);
          __AppendElement($parent, $n34);
        }
        __SetAttribute($n34, "text", "ad");
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
};
function $$update_1a82dd8_110($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo9 = $lepusPushFiberForNode($parent, 110, uniqueId),
        $forLepus = _$lepusPushFiberForNo9[0],
        $lastForLepus = _$lepusPushFiberForNo9[1];
    let $object = $data.ui_data.info_area.sale_info.price_info_vo.price_discountInfo_vo;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      {
        let $n111 = $update2 ? $lepusGetElementRefByLepusID("text", 111) : null;
        let $temp2 = $update2;
        if (!$n111) {
          $update2 = false;
          $n111 = __CreateText($currentComponentId);
          let $nid111 = $lepusStoreElementRefByLepusID($n111, 111, "text");
          __SetAttribute($n111, 1004, $nid111[1]);
          __AppendElement($parent, $n111);
        }
        __SetStyleObject($n111, [47, {
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
        __SetAttribute($n111, "text", item.text);
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}
function $$update_1a82dd8_113($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.sold_count != null && lynx.__globalProps.os != "ios") {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n114 = $update2 ? $lepusGetElementRefByLepusID("text", 114) : null;
        let $temp2 = $update2;
        if (!$n114) {
          $update2 = false;
          $n114 = __CreateText($currentComponentId);
          let $nid114 = $lepusStoreElementRefByLepusID($n114, 114, "text");
          __SetAttribute($n114, 1004, $nid114[1]);
          __SetStyleObject($n114, [64, 47, 68, 18, 67, 69]);
          __AppendElement($parent, $n114);
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let $value = $data.ui_data.info_area.sale_info.sold_count;
            if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.sold_count) {
              __SetAttribute($n114, "text", $value);
            }
          }
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp27 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let $template_update = $update2;
        let $n116 = $update2 ? $lepusGetElementRefByLepusID("if", 116) : null;
        if (!$n116) {
          $update2 = false;
          $n116 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n116, 116, "if");
          __AppendElement($parent, $n116);
        }
        $$update_1a82dd8_116($n116, $data, $update2);
        $update2 = $template_update;
      }
      $update2 = _$temp27;
    }
  }
}
$$update_1a82dd8_116 = function ($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.ui_data.info_area.sale_info.sold_count != null) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n117 = $update2 ? $lepusGetElementRefByLepusID("text", 117) : null;
        let $temp2 = $update2;
        if (!$n117) {
          $update2 = false;
          $n117 = __CreateText($currentComponentId);
          let $nid117 = $lepusStoreElementRefByLepusID($n117, 117, "text");
          __SetAttribute($n117, 1004, $nid117[1]);
          __SetStyleObject($n117, [64, 47, 68, 18, 67, 70, 71]);
          __AppendElement($parent, $n117);
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let $value = $data.ui_data.info_area.sale_info.sold_count;
            if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.sold_count) {
              __SetAttribute($n117, "text", $value);
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
$$update_1a82dd8_122 = function ($parent, $data, $update2) {
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
        let $n123 = $update2 ? $lepusGetElementRefByLepusID("view", 123) : null;
        let $temp2 = $update2;
        if (!$n123) {
          $update2 = false;
          $n123 = __CreateView($currentComponentId);
          let $nid123 = $lepusStoreElementRefByLepusID($n123, 123, "view");
          __SetAttribute($n123, 1004, $nid123[1]);
          __SetStyleObject($n123, [49, 26, 6, 73]);
          __AppendElement($parent, $n123);
        }
        {
          let $n124 = $update2 ? $lepusGetElementRefByLepusID("image", 124) : null;
          let $temp3 = $update2;
          if (!$n124) {
            $update2 = false;
            $n124 = __CreateImage($currentComponentId);
            let $nid124 = $lepusStoreElementRefByLepusID($n124, 124, "image");
            __SetAttribute($n124, 1004, $nid124[1]);
            __SetStyleObject($n124, [8, 74, 75, 0, 26, 76]);
            __AppendElement($n123, $n124);
          }
          if (!$update2 || $varUpdateState[2]) {
            {
              let $value = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.bgCenter;
              if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.bgCenter) {
                __SetAttribute($n124, "src", $value);
              }
            }
          }
          $update2 = $temp3;
        }
        {
          let $n125 = $update2 ? $lepusGetElementRefByLepusID("view", 125) : null;
          let _$temp28 = $update2;
          if (!$n125) {
            $update2 = false;
            $n125 = __CreateView($currentComponentId);
            let $nid125 = $lepusStoreElementRefByLepusID($n125, 125, "view");
            __SetAttribute($n125, 1004, $nid125[1]);
            __AppendElement($n123, $n125);
          }
          if (!$update2 || $varUpdateState[2]) {
            {
              let _$value9 = "margin-left:" + (($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft != null ? $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft : "14px") + ";") + "display:flex;align-items:center;height:100%;";
              if (!$update2 || _$value9 !== "margin-left:" + (($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft != null ? $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft : "14px") + ";") + "display:flex;align-items:center;height:100%;") {
                __SetStyleObject($n125, [49, 6, 26, {
                  38: ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft != null ? $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixContentStyle.paddingLeft : "14px") + ""
                }]);
              }
            }
          }
          {
            let $n126 = $update2 ? $lepusGetElementRefByLepusID("text", 126) : null;
            let $temp4 = $update2;
            if (!$n126) {
              $update2 = false;
              $n126 = __CreateText($currentComponentId);
              let $nid126 = $lepusStoreElementRefByLepusID($n126, 126, "text");
              __SetAttribute($n126, 1004, $nid126[1]);
              __SetStyleObject($n126, [77, 40, 78]);
              __SetAttribute($n126, "include-font-padding", "true");
              __SetAttribute($n126, "text-single-line-vertical-align", "center");
              __AppendElement($n125, $n126);
            }
            {
              if (!$update2 || $varUpdateState[2]) {
                let _$value10 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText;
                if (!$update2 || _$value10 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText) {
                  __SetAttribute($n126, "text", _$value10);
                }
              }
            }
            $update2 = $temp4;
          }
          {
            let $template_update = $update2;
            let $n128 = $update2 ? $lepusGetElementRefByLepusID("if", 128) : null;
            if (!$n128) {
              $update2 = false;
              $n128 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n128, 128, "if");
              __AppendElement($n125, $n128);
            }
            $$update_1a82dd8_128($n128, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update4 = $update2;
            let $n131 = $update2 ? $lepusGetElementRefByLepusID("if", 131) : null;
            if (!$n131) {
              $update2 = false;
              $n131 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n131, 131, "if");
              __AppendElement($n125, $n131);
            }
            $$update_1a82dd8_131($n131, $data, $update2);
            $update2 = _$template_update4;
          }
          $update2 = _$temp28;
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
$$update_1a82dd8_128 = function ($parent, $data, $update2) {
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
        let $n129 = $update2 ? $lepusGetElementRefByLepusID("text", 129) : null;
        let $temp2 = $update2;
        if (!$n129) {
          $update2 = false;
          $n129 = __CreateText($currentComponentId);
          let $nid129 = $lepusStoreElementRefByLepusID($n129, 129, "text");
          __SetAttribute($n129, 1004, $nid129[1]);
          __SetAttribute($n129, "include-font-padding", "true");
          __SetAttribute($n129, "text-single-line-vertical-align", "center");
          __AppendElement($parent, $n129);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixCurrencySignStyle.color + ";") + "white-space:nowrap;font-size:9px;font-weight:500;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixCurrencySignStyle.color + ";") + "white-space:nowrap;font-size:9px;font-weight:500;") {
              __SetStyleObject($n129, [47, 77, 40, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixCurrencySignStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value11 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixCurrencySign;
            if (!$update2 || _$value11 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixCurrencySign) {
              __SetAttribute($n129, "text", _$value11);
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
$$update_1a82dd8_131 = function ($parent, $data, $update2) {
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
        let $n132 = $update2 ? $lepusGetElementRefByLepusID("text", 132) : null;
        let $temp2 = $update2;
        if (!$n132) {
          $update2 = false;
          $n132 = __CreateText($currentComponentId);
          let $nid132 = $lepusStoreElementRefByLepusID($n132, 132, "text");
          __SetAttribute($n132, 1004, $nid132[1]);
          __SetAttribute($n132, "include-font-padding", "true");
          __SetAttribute($n132, "text-single-line-vertical-align", "center");
          __AppendElement($parent, $n132);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixAmountStyle.color + ";") + "white-space:nowrap;font-size:9px;font-weight:500;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixAmountStyle.color + ";") + "white-space:nowrap;font-size:9px;font-weight:500;") {
              __SetStyleObject($n132, [47, 77, 40, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixStyles.prefixAmountStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value12 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixAmount;
            if (!$update2 || _$value12 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixAmount) {
              __SetAttribute($n132, "text", _$value12);
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
$$update_1a82dd8_135 = function ($parent, $data, $update2) {
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
        let $n136 = $update2 ? $lepusGetElementRefByLepusID("view", 136) : null;
        let $temp2 = $update2;
        if (!$n136) {
          $update2 = false;
          $n136 = __CreateView($currentComponentId);
          let $nid136 = $lepusStoreElementRefByLepusID($n136, 136, "view");
          __SetAttribute($n136, 1004, $nid136[1]);
          __AppendElement($parent, $n136);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "padding-left:" + (($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "7px" : "3px") + ";") + ("margin-left:" + (($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "-7px" : "0px") + ";") + ("border-radius:" + (($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.prefixContent.prefixText != null ? "2px" : "0px") + ";"))) + "linear-cross-gravity:center;linear-orientation:horizontal;align-items:center;border-top-right-radius:2px;border-bottom-right-radius:2px;padding-right:3px;background-color:#fe2c5519;";
            if (!$update2 || $value !== undefined) {
              __SetStyleObject($n136, [80, 11, 6, 81, 82, 83, 84, {
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
          let $n137 = $update2 ? $lepusGetElementRefByLepusID("text", 137) : null;
          let $temp3 = $update2;
          if (!$n137) {
            $update2 = false;
            $n137 = __CreateText($currentComponentId);
            let $nid137 = $lepusStoreElementRefByLepusID($n137, 137, "text");
            __SetAttribute($n137, 1004, $nid137[1]);
            __SetStyleObject($n137, [40, 77, 18, 47]);
            __SetAttribute($n137, "include-font-padding", "true");
            __AppendElement($n136, $n137);
          }
          {
            let $n138 = $update2 ? $lepusGetElementRefByLepusID("text", 138) : null;
            let $temp4 = $update2;
            if (!$n138) {
              $update2 = false;
              $n138 = __CreateText($currentComponentId);
              let $nid138 = $lepusStoreElementRefByLepusID($n138, 138, "text");
              __SetAttribute($n138, 1004, $nid138[1]);
              __AppendElement($n137, $n138);
            }
            if (!$update2 || $varUpdateState[2]) {
              {
                let _$value13 = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixTextStyle.color + ";") + "font-weight:500;font-size:9px;";
                if (!$update2 || _$value13 !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixTextStyle.color + ";") + "font-weight:500;font-size:9px;") {
                  __SetStyleObject($n138, [40, 77, {
                    22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixTextStyle.color + ""
                  }]);
                }
              }
            }
            {
              if (!$update2 || $varUpdateState[2]) {
                let _$value14 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixText;
                if (!$update2 || _$value14 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixText) {
                  __SetAttribute($n138, "text", _$value14);
                }
              }
            }
            $update2 = $temp4;
          }
          {
            let $template_update = $update2;
            let $n140 = $update2 ? $lepusGetElementRefByLepusID("if", 140) : null;
            if (!$n140) {
              $update2 = false;
              $n140 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n140, 140, "if");
              __AppendElement($n137, $n140);
            }
            $$update_1a82dd8_140($n140, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update5 = $update2;
            let $n143 = $update2 ? $lepusGetElementRefByLepusID("if", 143) : null;
            if (!$n143) {
              $update2 = false;
              $n143 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n143, 143, "if");
              __AppendElement($n137, $n143);
            }
            $$update_1a82dd8_143($n143, $data, $update2);
            $update2 = _$template_update5;
          }
          {
            let _$template_update6 = $update2;
            let $n146 = $update2 ? $lepusGetElementRefByLepusID("if", 146) : null;
            if (!$n146) {
              $update2 = false;
              $n146 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n146, 146, "if");
              __AppendElement($n137, $n146);
            }
            $$update_1a82dd8_146($n146, $data, $update2);
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
$$update_1a82dd8_140 = function ($parent, $data, $update2) {
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
        let $n141 = $update2 ? $lepusGetElementRefByLepusID("text", 141) : null;
        let $temp2 = $update2;
        if (!$n141) {
          $update2 = false;
          $n141 = __CreateText($currentComponentId);
          let $nid141 = $lepusStoreElementRefByLepusID($n141, 141, "text");
          __SetAttribute($n141, 1004, $nid141[1]);
          __AppendElement($parent, $n141);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixCurrencySignStyle.color + ";") + "font-weight:500;font-size:9px;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixCurrencySignStyle.color + ";") + "font-weight:500;font-size:9px;") {
              __SetStyleObject($n141, [40, 77, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixCurrencySignStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value15 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixCurrencySign;
            if (!$update2 || _$value15 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixCurrencySign) {
              __SetAttribute($n141, "text", _$value15);
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
$$update_1a82dd8_143 = function ($parent, $data, $update2) {
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
        let $n144 = $update2 ? $lepusGetElementRefByLepusID("text", 144) : null;
        let $temp2 = $update2;
        if (!$n144) {
          $update2 = false;
          $n144 = __CreateText($currentComponentId);
          let $nid144 = $lepusStoreElementRefByLepusID($n144, 144, "text");
          __SetAttribute($n144, 1004, $nid144[1]);
          __AppendElement($parent, $n144);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixAmountStyle.color + ";") + "font-size:9px;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixAmountStyle.color + ";") + "font-size:9px;") {
              __SetStyleObject($n144, [77, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixAmountStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value16 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixAmount;
            if (!$update2 || _$value16 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixAmount) {
              __SetAttribute($n144, "text", _$value16);
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
$$update_1a82dd8_146 = function ($parent, $data, $update2) {
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
        let $n147 = $update2 ? $lepusGetElementRefByLepusID("text", 147) : null;
        let $temp2 = $update2;
        if (!$n147) {
          $update2 = false;
          $n147 = __CreateText($currentComponentId);
          let $nid147 = $lepusStoreElementRefByLepusID($n147, 147, "text");
          __SetAttribute($n147, 1004, $nid147[1]);
          __AppendElement($parent, $n147);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = "color:" + ($data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixSuffixTextStyle.color + ";") + "font-size:9px;";
            if (!$update2 || $value !== "color:" + ($cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixSuffixTextStyle.color + ";") + "font-size:9px;") {
              __SetStyleObject($n147, [77, {
                22: $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixStyles.suffixSuffixTextStyle.color + ""
              }]);
            }
          }
        }
        {
          if (!$update2 || $varUpdateState[2]) {
            let _$value17 = $data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixSuffixText;
            if (!$update2 || _$value17 !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.coupon_info_vo.suffixContent.suffixSuffixText) {
              __SetAttribute($n147, "text", _$value17);
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
$$update_1a82dd8_151 = function ($parent, $data, $update2) {
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
        let $n152 = $update2 ? $lepusGetElementRefByLepusID("image", 152) : null;
        let $temp2 = $update2;
        if (!$n152) {
          $update2 = false;
          $n152 = __CreateImage($currentComponentId);
          let $nid152 = $lepusStoreElementRefByLepusID($n152, 152, "image");
          __SetAttribute($n152, 1004, $nid152[1]);
          __SetStyleObject($n152, [91, 92]);
          __AppendElement($parent, $n152);
        }
        if (!$update2 || $varUpdateState[2]) {
          {
            let $value = $data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagIcon;
            if (!$update2 || $value !== $cardInstance._data.ui_data.info_area.sale_info.price_info_vo.discount_tag_info_vo.marketingCustomTagIcon) {
              __SetAttribute($n152, "src", $value);
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
function $$update_29fefa0_84($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (lynx.__globalProps.os == "ios") {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n85 = $update2 ? $lepusGetElementRefByLepusID("text", 85) : null;
        let $temp2 = $update2;
        if (!$n85) {
          $update2 = false;
          $n85 = __CreateText($currentComponentId);
          let $nid85 = $lepusStoreElementRefByLepusID($n85, 85, "text");
          __SetAttribute($n85, 1004, $nid85[1]);
          __SetStyleObject($n85, [47, 61]);
          __AppendElement($parent, $n85);
        }
        {
          let $n86 = $update2 ? $lepusGetElementRefByLepusID("if", 86) : null;
          if (!$n86) {
            $update2 = false;
            $n86 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n86, 86, "if");
            __AppendElement($n85, $n86);
          }
          let uniqueId2 = __GetElementUniqueID($n86);
          if (!$update2) {
            $conditionNodeIndex[uniqueId2] = -1;
          }
          let $ifNodeIndex2 = $conditionNodeIndex[uniqueId2];
          if ((item.pre_fix.isShow == true || item.pre_fix.isShow == 1) && item.pre_fix.renderValue) {
            __UpdateIfNodeIndex($n86, 0);
            $conditionNodeIndex[uniqueId2] = 0;
            let $temp3 = $update2;
            if ($ifNodeIndex2 !== 0) {
              $update2 = false;
            }
            {
              let $n87 = $update2 ? $lepusGetElementRefByLepusID("text", 87) : null;
              let $temp4 = $update2;
              if (!$n87) {
                $update2 = false;
                $n87 = __CreateText($currentComponentId);
                let $nid87 = $lepusStoreElementRefByLepusID($n87, 87, "text");
                __SetAttribute($n87, 1004, $nid87[1]);
                __SetStyleObject($n87, [62, 63, 40]);
                __AppendElement($n86, $n87);
              }
              __SetAttribute($n87, "text", item.pre_fix.renderValue);
              $update2 = $temp4;
            }
            $update2 = $temp3;
          } else {
            __UpdateIfNodeIndex($n86, -1);
            $conditionNodeIndex[uniqueId2] = -1;
          }
        }
        {
          let $n89 = $update2 ? $lepusGetElementRefByLepusID("if", 89) : null;
          if (!$n89) {
            $update2 = false;
            $n89 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n89, 89, "if");
            __AppendElement($n85, $n89);
          }
          let _uniqueId = __GetElementUniqueID($n89);
          if (!$update2) {
            $conditionNodeIndex[_uniqueId] = -1;
          }
          let _$ifNodeIndex = $conditionNodeIndex[_uniqueId];
          if ((item.currency_sign.isShow == true || item.currency_sign.isShow == 1) && item.currency_sign.renderValue) {
            __UpdateIfNodeIndex($n89, 0);
            $conditionNodeIndex[_uniqueId] = 0;
            let _$temp29 = $update2;
            if (_$ifNodeIndex !== 0) {
              $update2 = false;
            }
            {
              let $n90 = $update2 ? $lepusGetElementRefByLepusID("text", 90) : null;
              let _$temp30 = $update2;
              if (!$n90) {
                $update2 = false;
                $n90 = __CreateText($currentComponentId);
                let $nid90 = $lepusStoreElementRefByLepusID($n90, 90, "text");
                __SetAttribute($n90, 1004, $nid90[1]);
                __AppendElement($n89, $n90);
              }
              __SetStyleObject($n90, [{
                22: item.currency_sign.renderStyle.color + ""
              }, {
                47: item.currency_sign.renderStyle.fontSize + ""
              }, {
                48: item.currency_sign.renderStyle.fontWeight + ""
              }]);
              __SetAttribute($n90, "text", item.currency_sign.renderValue);
              $update2 = _$temp30;
            }
            $update2 = _$temp29;
          } else {
            __UpdateIfNodeIndex($n89, -1);
            $conditionNodeIndex[_uniqueId] = -1;
          }
        }
        {
          let $n92 = $update2 ? $lepusGetElementRefByLepusID("text", 92) : null;
          let _$temp31 = $update2;
          if (!$n92) {
            $update2 = false;
            $n92 = __CreateText($currentComponentId);
            let $nid92 = $lepusStoreElementRefByLepusID($n92, 92, "text");
            __SetAttribute($n92, 1004, $nid92[1]);
            __AppendElement($n85, $n92);
          }
          __SetStyleObject($n92, [{
            61: $data.control_info.groupon_feeds_marketing_expression_new_text_style == 1 ? "test" : "PingFang SC"
          }, {
            22: item.amount.renderStyle.color + ""
          }, {
            47: item.amount.renderStyle.fontSize + ""
          }, {
            48: item.amount.renderStyle.fontWeight + ""
          }]);
          __SetAttribute($n92, "text", item.amount.renderValue);
          $update2 = _$temp31;
        }
        {
          let $n94 = $update2 ? $lepusGetElementRefByLepusID("if", 94) : null;
          if (!$n94) {
            $update2 = false;
            $n94 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n94, 94, "if");
            __AppendElement($n85, $n94);
          }
          let _uniqueId2 = __GetElementUniqueID($n94);
          if (!$update2) {
            $conditionNodeIndex[_uniqueId2] = -1;
          }
          let _$ifNodeIndex2 = $conditionNodeIndex[_uniqueId2];
          if ((item.post_fix.isShow == true || item.post_fix.isShow == 1) && item.post_fix.renderValue) {
            __UpdateIfNodeIndex($n94, 0);
            $conditionNodeIndex[_uniqueId2] = 0;
            let _$temp32 = $update2;
            if (_$ifNodeIndex2 !== 0) {
              $update2 = false;
            }
            {
              let $n95 = $update2 ? $lepusGetElementRefByLepusID("text", 95) : null;
              let _$temp33 = $update2;
              if (!$n95) {
                $update2 = false;
                $n95 = __CreateText($currentComponentId);
                let $nid95 = $lepusStoreElementRefByLepusID($n95, 95, "text");
                __SetAttribute($n95, 1004, $nid95[1]);
                __SetStyleObject($n95, [62, 64, 65, 66]);
                __AppendElement($n94, $n95);
              }
              __SetAttribute($n95, "text", item.post_fix.renderValue);
              $update2 = _$temp33;
            }
            $update2 = _$temp32;
          } else {
            __UpdateIfNodeIndex($n94, -1);
            $conditionNodeIndex[_uniqueId2] = -1;
          }
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp34 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let $n97 = $update2 ? $lepusGetElementRefByLepusID("text", 97) : null;
        let _$temp35 = $update2;
        if (!$n97) {
          $update2 = false;
          $n97 = __CreateText($currentComponentId);
          let $nid97 = $lepusStoreElementRefByLepusID($n97, 97, "text");
          __SetAttribute($n97, 1004, $nid97[1]);
          __SetStyleObject($n97, [47]);
          __AppendElement($parent, $n97);
        }
        {
          let $n98 = $update2 ? $lepusGetElementRefByLepusID("if", 98) : null;
          if (!$n98) {
            $update2 = false;
            $n98 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n98, 98, "if");
            __AppendElement($n97, $n98);
          }
          let _uniqueId3 = __GetElementUniqueID($n98);
          if (!$update2) {
            $conditionNodeIndex[_uniqueId3] = -1;
          }
          let _$ifNodeIndex3 = $conditionNodeIndex[_uniqueId3];
          if ((item.pre_fix.isShow == true || item.pre_fix.isShow == 1) && item.pre_fix.renderValue) {
            __UpdateIfNodeIndex($n98, 0);
            $conditionNodeIndex[_uniqueId3] = 0;
            let _$temp36 = $update2;
            if (_$ifNodeIndex3 !== 0) {
              $update2 = false;
            }
            {
              let $n99 = $update2 ? $lepusGetElementRefByLepusID("text", 99) : null;
              let _$temp37 = $update2;
              if (!$n99) {
                $update2 = false;
                $n99 = __CreateText($currentComponentId);
                let $nid99 = $lepusStoreElementRefByLepusID($n99, 99, "text");
                __SetAttribute($n99, 1004, $nid99[1]);
                __SetStyleObject($n99, [62, 63, 40]);
                __AppendElement($n98, $n99);
              }
              __SetAttribute($n99, "text", item.pre_fix.renderValue);
              $update2 = _$temp37;
            }
            $update2 = _$temp36;
          } else {
            __UpdateIfNodeIndex($n98, -1);
            $conditionNodeIndex[_uniqueId3] = -1;
          }
        }
        {
          let $n101 = $update2 ? $lepusGetElementRefByLepusID("if", 101) : null;
          if (!$n101) {
            $update2 = false;
            $n101 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n101, 101, "if");
            __AppendElement($n97, $n101);
          }
          let _uniqueId4 = __GetElementUniqueID($n101);
          if (!$update2) {
            $conditionNodeIndex[_uniqueId4] = -1;
          }
          let _$ifNodeIndex4 = $conditionNodeIndex[_uniqueId4];
          if ((item.currency_sign.isShow == true || item.currency_sign.isShow == 1) && item.currency_sign.renderValue) {
            __UpdateIfNodeIndex($n101, 0);
            $conditionNodeIndex[_uniqueId4] = 0;
            let _$temp38 = $update2;
            if (_$ifNodeIndex4 !== 0) {
              $update2 = false;
            }
            {
              let $n102 = $update2 ? $lepusGetElementRefByLepusID("text", 102) : null;
              let _$temp39 = $update2;
              if (!$n102) {
                $update2 = false;
                $n102 = __CreateText($currentComponentId);
                let $nid102 = $lepusStoreElementRefByLepusID($n102, 102, "text");
                __SetAttribute($n102, 1004, $nid102[1]);
                __AppendElement($n101, $n102);
              }
              __SetStyleObject($n102, [{
                22: item.currency_sign.renderStyle.color + ""
              }, {
                47: item.currency_sign.renderStyle.fontSize + ""
              }, {
                48: item.currency_sign.renderStyle.fontWeight + ""
              }]);
              __SetAttribute($n102, "text", item.currency_sign.renderValue);
              $update2 = _$temp39;
            }
            $update2 = _$temp38;
          } else {
            __UpdateIfNodeIndex($n101, -1);
            $conditionNodeIndex[_uniqueId4] = -1;
          }
        }
        {
          let $n104 = $update2 ? $lepusGetElementRefByLepusID("text", 104) : null;
          let _$temp40 = $update2;
          if (!$n104) {
            $update2 = false;
            $n104 = __CreateText($currentComponentId);
            let $nid104 = $lepusStoreElementRefByLepusID($n104, 104, "text");
            __SetAttribute($n104, 1004, $nid104[1]);
            __AppendElement($n97, $n104);
          }
          __SetStyleObject($n104, [{
            61: $data.control_info.groupon_feeds_marketing_expression_new_text_style == 1 ? "test" : "PingFang SC"
          }, {
            22: item.amount.renderStyle.color + ""
          }, {
            47: item.amount.renderStyle.fontSize + ""
          }, {
            48: item.amount.renderStyle.fontWeight + ""
          }]);
          __SetAttribute($n104, "text", item.amount.renderValue);
          $update2 = _$temp40;
        }
        {
          let $n106 = $update2 ? $lepusGetElementRefByLepusID("if", 106) : null;
          if (!$n106) {
            $update2 = false;
            $n106 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n106, 106, "if");
            __AppendElement($n97, $n106);
          }
          let _uniqueId5 = __GetElementUniqueID($n106);
          if (!$update2) {
            $conditionNodeIndex[_uniqueId5] = -1;
          }
          let _$ifNodeIndex5 = $conditionNodeIndex[_uniqueId5];
          if ((item.post_fix.isShow == true || item.post_fix.isShow == 1) && item.post_fix.renderValue) {
            __UpdateIfNodeIndex($n106, 0);
            $conditionNodeIndex[_uniqueId5] = 0;
            let _$temp41 = $update2;
            if (_$ifNodeIndex5 !== 0) {
              $update2 = false;
            }
            {
              let $n107 = $update2 ? $lepusGetElementRefByLepusID("text", 107) : null;
              let _$temp42 = $update2;
              if (!$n107) {
                $update2 = false;
                $n107 = __CreateText($currentComponentId);
                let $nid107 = $lepusStoreElementRefByLepusID($n107, 107, "text");
                __SetAttribute($n107, 1004, $nid107[1]);
                __SetStyleObject($n107, [62, 64, 65]);
                __AppendElement($n106, $n107);
              }
              __SetAttribute($n107, "text", item.post_fix.renderValue);
              $update2 = _$temp42;
            }
            $update2 = _$temp41;
          } else {
            __UpdateIfNodeIndex($n106, -1);
            $conditionNodeIndex[_uniqueId5] = -1;
          }
        }
        $update2 = _$temp35;
      }
      $update2 = _$temp34;
    }
  }
}
function $$update_d8b490_83($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[2] || $varUpdateState[1]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo10 = $lepusPushFiberForNode($parent, 83, uniqueId),
        $forLepus = _$lepusPushFiberForNo10[0],
        $lastForLepus = _$lepusPushFiberForNo10[1];
    let $object = $data.ui_data.info_area.sale_info.price_info_vo.price_vo;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      let $n84 = $update2 ? $lepusGetElementRefByLepusID("if", 84) : null;
      if (!$n84) {
        $n84 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n84, 84, "if");
        __AppendElement($parent, $n84);
      }
      $$update_29fefa0_84($n84, $data, $update2, index, item);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}
function $$update_3d85448_169($parent, $data, $update2) {
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
        let $n170 = $update2 ? $lepusGetElementRefByLepusID("view", 170) : null;
        let $temp2 = $update2;
        if (!$n170) {
          $update2 = false;
          $n170 = __CreateView($currentComponentId);
          let $nid170 = $lepusStoreElementRefByLepusID($n170, 170, "view");
          __SetAttribute($n170, 1004, $nid170[1]);
          __SetStyleObject($n170, [104, 51, 26, 101, 102, 103]);
          __AppendElement($parent, $n170);
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
function $$update_ea5060_166($parent, $data, $update2) {
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
        let $n167 = $update2 ? $lepusGetElementRefByLepusID("view", 167) : null;
        let $temp2 = $update2;
        if (!$n167) {
          $update2 = false;
          $n167 = __CreateView($currentComponentId);
          let $nid167 = $lepusStoreElementRefByLepusID($n167, 167, "view");
          __SetAttribute($n167, 1004, $nid167[1]);
          __SetStyleObject($n167, [8, 3, 75, 98, 0, 26, 1, 99]);
          __AppendElement($parent, $n167);
        }
        {
          let $n168 = $update2 ? $lepusGetElementRefByLepusID("view", 168) : null;
          let $temp3 = $update2;
          if (!$n168) {
            $update2 = false;
            $n168 = __CreateView($currentComponentId);
            let $nid168 = $lepusStoreElementRefByLepusID($n168, 168, "view");
            __SetAttribute($n168, 1004, $nid168[1]);
            __SetStyleObject($n168, [100, 26, 101, 102, 103]);
            __AppendElement($n167, $n168);
          }
          $update2 = $temp3;
        }
        {
          let $template_update = $update2;
          let $n169 = $update2 ? $lepusGetElementRefByLepusID("if", 169) : null;
          if (!$n169) {
            $update2 = false;
            $n169 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n169, 169, "if");
            __AppendElement($n167, $n169);
          }
          $$update_3d85448_169($n169, $data, $update2);
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
function $$update_360f5b0_78($parent, $data, $update2) {
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
          let $n79 = $update2 ? $lepusGetElementRefByLepusID("view", 79) : null;
          let $temp2 = $update2;
          if (!$n79) {
            $update2 = false;
            $n79 = __CreateView($currentComponentId);
            let $nid79 = $lepusStoreElementRefByLepusID($n79, 79, "view");
            __SetAttribute($n79, 1004, $nid79[1]);
            __SetStyleObject($n79, [0, 15, 16, 51]);
            __AppendElement($parent, $n79);
          }
          {
            let $n80 = $update2 ? $lepusGetElementRefByLepusID("view", 80) : null;
            let $temp3 = $update2;
            if (!$n80) {
              $update2 = false;
              $n80 = __CreateView($currentComponentId);
              let $nid80 = $lepusStoreElementRefByLepusID($n80, 80, "view");
              __SetAttribute($n80, 1004, $nid80[1]);
              __SetStyleObject($n80, [1]);
              __AppendElement($n79, $n80);
            }
            {
              let $n81 = $update2 ? $lepusGetElementRefByLepusID("view", 81) : null;
              let $temp4 = $update2;
              if (!$n81) {
                $update2 = false;
                $n81 = __CreateView($currentComponentId);
                let $nid81 = $lepusStoreElementRefByLepusID($n81, 81, "view");
                __SetAttribute($n81, 1004, $nid81[1]);
                __AppendElement($n80, $n81);
              }
              if (!$update2 || $varUpdateState[1] || $varUpdateState[5]) {
                {
                  let $value = "max-height:" + (17 * 1.1 * 1.1 * parseFloat($data.control_info.font_scale) + "px;") + "width:100%;display:flex;justify-content:space-between;overflow:hidden;flex-wrap:wrap;align-items:flex-end;";
                  if (!$update2 || $value !== undefined) {
                    __SetStyleObject($n81, [0, 49, 59, 5, 13, 60, {
                      30: 17 * 1.1 * 1.1 * parseFloat($data.control_info.font_scale) + "px"
                    }]);
                  }
                }
              }
              {
                let $n82 = $update2 ? $lepusGetElementRefByLepusID("view", 82) : null;
                let $temp5 = $update2;
                if (!$n82) {
                  $update2 = false;
                  $n82 = __CreateView($currentComponentId);
                  let $nid82 = $lepusStoreElementRefByLepusID($n82, 82, "view");
                  __SetAttribute($n82, 1004, $nid82[1]);
                  __SetStyleObject($n82, [18, 57, 49]);
                  __AppendElement($n81, $n82);
                }
                if (!$update2 || $varUpdateState[2] || $varUpdateState[1]) {
                  let $n83 = $update2 ? $lepusGetElementRefByLepusID("for", 83) : null;
                  if (!$n83) {
                    $n83 = __CreateFor($currentComponentId);
                    $lepusStoreElementRefByLepusID($n83, 83, "for");
                    __AppendElement($n82, $n83);
                  }
                  $$update_d8b490_83($n83, $data, $update2);
                }
                {
                  let $n109 = $update2 ? $lepusGetElementRefByLepusID("view", 109) : null;
                  let $temp6 = $update2;
                  if (!$n109) {
                    $update2 = false;
                    $n109 = __CreateView($currentComponentId);
                    let $nid109 = $lepusStoreElementRefByLepusID($n109, 109, "view");
                    __SetAttribute($n109, 1004, $nid109[1]);
                    __SetStyleObject($n109, [57, 5, 18, 49, 67]);
                    __AppendElement($n82, $n109);
                  }
                  if (!$update2 || $varUpdateState[2]) {
                    let $n110 = $update2 ? $lepusGetElementRefByLepusID("for", 110) : null;
                    if (!$n110) {
                      $n110 = __CreateFor($currentComponentId);
                      $lepusStoreElementRefByLepusID($n110, 110, "for");
                      __AppendElement($n109, $n110);
                    }
                    $$update_1a82dd8_110($n110, $data, $update2);
                  }
                  $update2 = $temp6;
                }
                $update2 = $temp5;
              }
              {
                let $template_update = $update2;
                let $n113 = $update2 ? $lepusGetElementRefByLepusID("if", 113) : null;
                if (!$n113) {
                  $update2 = false;
                  $n113 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n113, 113, "if");
                  __AppendElement($n81, $n113);
                }
                $$update_1a82dd8_113($n113, $data, $update2);
                $update2 = $template_update;
              }
              $update2 = $temp4;
            }
            {
              let _$template_update7 = $update2;
              let $n119 = $update2 ? $lepusGetElementRefByLepusID("if", 119) : null;
              if (!$n119) {
                $update2 = false;
                $n119 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n119, 119, "if");
                __AppendElement($n80, $n119);
              }
              $$update_100f540_119($n119, $data, $update2);
              $update2 = _$template_update7;
            }
            {
              let _$template_update8 = $update2;
              let $n166 = $update2 ? $lepusGetElementRefByLepusID("if", 166) : null;
              if (!$n166) {
                $update2 = false;
                $n166 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n166, 166, "if");
                __AppendElement($n80, $n166);
              }
              $$update_ea5060_166($n166, $data, $update2);
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
      let _$value18 = "aspect-ratio:" + (($data.control_info.cover_ratio == null || $data.control_info.cover_ratio == 0 ? 1 : $data.control_info.cover_ratio) + ";") + "width:100%;";
      if (!$update || _$value18 !== "aspect-ratio:" + (($cardInstance._data.control_info.cover_ratio == null || $cardInstance._data.control_info.cover_ratio == 0 ? 1 : $cardInstance._data.control_info.cover_ratio) + ";") + "width:100%;") {
        __SetStyleObject($n4, [0, {
          95: ($data.control_info.cover_ratio == null || $data.control_info.cover_ratio == 0 ? 1 : $data.control_info.cover_ratio) + ""
        }]);
      }
    }
    {
      let _$value19 = $data.ui_data.cover_area.cover.url_list[0];
      if (!$update || _$value19 !== $cardInstance._data.ui_data.cover_area.cover.url_list[0]) {
        __SetAttribute($n4, "src", _$value19);
      }
    }
  }
  let $n5 = $lepusGetElementRefByLepusID("if", 5);
  $$update_100f540_5($n5, $data, $update);
  let $n17 = $lepusGetElementRefByLepusID("if", 17);
  $$update_100f540_17($n17, $data, $update);
  let $n36 = $lepusGetElementRefByLepusID("if", 36);
  $$update_100f540_36($n36, $data, $update);
  let $n55 = $lepusGetElementRefByLepusID("if", 55);
  $$update_100f540_55($n55, $data, $update);
  let $n78 = $lepusGetElementRefByLepusID("if", 78);
  $$update_360f5b0_78($n78, $data, $update);
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
  __SetStyleObject($n2, [0, 3, 4, 5, 1, 6, 7]);
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
  __SetStyleObject($n4, [0, {
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
  let $n36 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n36, 36, "if");
  __AppendElement($n2, $n36);
  $$update_100f540_36($n36, $data, $update);
  let $n55 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n55, 55, "if");
  __AppendElement($n2, $n55);
  $$update_100f540_55($n55, $data, $update);
  let $n78 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n78, 78, "if");
  __AppendElement($n2, $n78);
  $$update_360f5b0_78($n78, $data, $update);
  $airFirstScreen = false;
  $cardVariables = ["biz_data", "control_info", "ui_data", "is_cache", "extra_data"];
  return true;
};