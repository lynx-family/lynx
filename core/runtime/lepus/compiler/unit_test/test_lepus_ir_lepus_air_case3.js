// [TEST_TARGET: IR]
let $$update_3d4f50_844;
let $$update_a91910_894;
let $$update_a91910_896;
let $$update_a91910_898;
let lynx = {}
let __defProp = Object.defineProperty;
let __defProps = Object.defineProperties;
let __getOwnPropDescs = Object.getOwnPropertyDescriptors;
let __getOwnPropSymbols = Object.getOwnPropertySymbols;
let __hasOwnProp = Object.prototype.hasOwnProperty;
let __propIsEnum = Object.prototype.propertyIsEnumerable;
function __defNormalProp(obj, key, value) {
  return Object.keys(obj).includes(key) ? __defProp(obj, key, {
    enumerable: true,
    configurable: true,
    writable: true,
    value: value
  }) : obj[key] = value;
}
function __spreadValues(a, b) {
  for (let _i2 = 0, _Object$keys = Object.keys(b || (b = {})); _i2 < _Object$keys.length; _i2++) {
    let prop = _Object$keys[_i2];
    if (__hasOwnProp(b, prop)) {
      __defNormalProp(a, prop, b[prop]);
    }
  }
  if (__getOwnPropSymbols) for (let _i3 = 0, _getOwnPropSymbols = __getOwnPropSymbols(b); _i3 < _getOwnPropSymbols.length; _i3++) {
    let prop = _getOwnPropSymbols[_i3];
    if (__propIsEnum(b, prop)) {
      __defNormalProp(a, prop, b[prop]);
    }
  }
  return a;
}
function __spreadProps(a, b) {
  return __defProps(a, __getOwnPropDescs(b));
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
function getCssPropertyIDObj(obj) {
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
function __GetElementByUniqueID(a){}
function __GetElementUniqueID(a){}
function __CreateView(a) {}
function __SetAttribute(a, b, c){}
function __AppendElement(a, b) {}
function __CreateFor(a) {}
function _GetLength(a) {}
function __UpdateForChildCount(a, b){}
function __CreateText(a) {}
function __CreateRawText(a) {}
function __CreateIf(a) {}
function __UpdateIfNodeIndex(a, b) {}
function __CreateImage(a) {}
function __AddDataset(a, b, c) {}
function __AddEvent(a, b, c, d) {};
function __SetClasses(a, b) {}
function __CreateWrapperElement(a) {}
function __SetID(a, b) {}
function __CreateElement(a, b) {}
function __GetDiffData(a, b, c) {}
function __FlushElementTree(a) {}
function __CreatePage(a) {}

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
function $lepusPushFiberTemplateNode(templateElement) {
  if (templateElement) {
    let lastTemplate = $cardInstance._currentTemplateElement;
    $cardInstance._currentTemplateElement = templateElement;
    return lastTemplate;
  } else {
    $cardInstance._currentTemplateElement = undefined;
    return undefined;
  }
}
function $lepusUpdateFiberForNodeIndex(index) {
  $cardInstance._currentForElement.activeIndex = index;
}
$lepusStoreElementRefByLepusID = function (elementRef, lepusId, tag) {
  let _$getKeyForCreatedEle2 = $getKeyForCreatedElement(lepusId, tag),
      lepusUniqueId = _$getKeyForCreatedEle2[0],
      uniqId = _$getKeyForCreatedEle2[1];
  let uniqueId = __GetElementUniqueID(elementRef);
  $lepusElementLepusIdMap[$getLepusHash(uniqId, lepusId)] = uniqueId;
  return [uniqueId, lepusUniqueId];
};
let $renderTemplates = {};
let $templateInfo = {};
let $updatePropsSet = [];
function $templateConstructor(parentUniqId, parent) {
  let template = {
    _templateId: parentUniqId,
    _parentUniqId: parentUniqId,
    _parent: parent,
    data: {},
    _data: {}
  };
  template.setData = function (key, value, update) {
    if (!update || value !== template.data[key]) {
      $updatePropsSet.push(key);
      template._data[key] = template.data[key];
      template.data[key] = $deepClone(value);
      return true;
    }
  };
  $templateInfo[parentUniqId] = template;
  return template;
}
function $templateUpdate($lepusTemplate, $path, $array) {
  $renderTemplates[$path].variables.forEach(function (it, index) {
    $renderTemplates[$path].varUpdateState[index] = $array.includes(it);
  });
  let globalIndex = $renderTemplates[$path].variables.length;
  if ($array.includes("__globalProps")) {
    $renderTemplates[$path].varUpdateState[globalIndex] = true;
  } else {
    $renderTemplates[$path].varUpdateState[globalIndex] = false;
  }
  return $lepusPushFiberTemplateNode($lepusTemplate);
}
let renderPage = null;
let updatePage = null;
let $cardVariables = [];
let $varUpdateState = [];
let $conditionNodeIndex = {};
$cardOptions = {
};
$renderTemplates["CombSubSpuInfo"] = {
  variables: ["subSpuInfoList"],
  varUpdateState: [],
  update_34b0098_2: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    if (!$update2 || $renderTemplates["CombSubSpuInfo"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo = $lepusPushFiberForNode($parent, 2, uniqueId),
          $forLepus = _$lepusPushFiberForNo[0],
          $lastForLepus = _$lepusPushFiberForNo[1];
      let $object = $data.subSpuInfoList;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n3 = $update2 ? $lepusGetElementRefByLepusID("view", 3) : null;
          let $temp2 = $update2;
          if (!$n3) {
            $update2 = false;
            $n3 = __CreateView($currentComponentId);
            let $nid3 = $lepusStoreElementRefByLepusID($n3, 3, "view");
            __SetAttribute($n3, 1004, $nid3[1]);
            __AppendElement($parent, $n3);
          }
          __SetStyleObject($n3, [3, 4, {
            41: index != $data.subSpuInfoList.length - 1 ? "4px" : "2px"
          }]);
          __SetAttribute($n3, "key", item.spu_id);
          {
            let $n4 = $update2 ? $lepusGetElementRefByLepusID("view", 4) : null;
            let $temp3 = $update2;
            if (!$n4) {
              $update2 = false;
              $n4 = __CreateView($currentComponentId);
              let $nid4 = $lepusStoreElementRefByLepusID($n4, 4, "view");
              __SetAttribute($n4, 1004, $nid4[1]);
              __SetStyleObject($n4, [5, 6, 7, 3, 4, 8, 9, 10]);
              __AppendElement($n3, $n4);
            }
            {
              let $n5 = $update2 ? $lepusGetElementRefByLepusID("text", 5) : null;
              let $temp4 = $update2;
              if (!$n5) {
                $update2 = false;
                $n5 = __CreateText($currentComponentId);
                let $nid5 = $lepusStoreElementRefByLepusID($n5, 5, "text");
                __SetAttribute($n5, 1004, $nid5[1]);
                __SetStyleObject($n5, [11, 12, 13]);
                __AppendElement($n4, $n5);
              }
              __SetAttribute($n5, "text", ((_a = item == null ? undefined : item.icon) == null ? undefined : _a.content) || "享");
              $update2 = $temp4;
            }
            $update2 = $temp3;
          }
          {
            let $n7 = $update2 ? $lepusGetElementRefByLepusID("view", 7) : null;
            let _$temp = $update2;
            if (!$n7) {
              $update2 = false;
              $n7 = __CreateView($currentComponentId);
              let $nid7 = $lepusStoreElementRefByLepusID($n7, 7, "view");
              __SetAttribute($n7, 1004, $nid7[1]);
              __SetStyleObject($n7, [0, 6, 14, 3, 4]);
              __AppendElement($n3, $n7);
            }
            {
              let $n8 = $update2 ? $lepusGetElementRefByLepusID("text", 8) : null;
              let _$temp2 = $update2;
              if (!$n8) {
                $update2 = false;
                $n8 = __CreateText($currentComponentId);
                let $nid8 = $lepusStoreElementRefByLepusID($n8, 8, "text");
                __SetAttribute($n8, 1004, $nid8[1]);
                __SetStyleObject($n8, [15, 16, 17, 18, 19, 20]);
                __SetAttribute($n8, "text-maxline", "1");
                __AppendElement($n7, $n8);
              }
              {
                let $n9 = $update2 ? $lepusGetElementRefByLepusID("text", 9) : null;
                let $temp5 = $update2;
                if (!$n9) {
                  $update2 = false;
                  $n9 = __CreateText($currentComponentId);
                  let $nid9 = $lepusStoreElementRefByLepusID($n9, 9, "text");
                  __SetAttribute($n9, 1004, $nid9[1]);
                  __SetStyleObject($n9, [17, 21, 19, 22]);
                  __AppendElement($n8, $n9);
                }
                __SetAttribute($n9, "text", item.brand_name || item.poi_name);
                $update2 = $temp5;
              }
              {
                let $n11 = $update2 ? $lepusGetElementRefByLepusID("raw-text", 11) : null;
                if (!$n11) {
                  $n11 = __CreateRawText("·" + item.spu_name);
                  $lepusStoreElementRefByLepusID($n11, 11, "raw-text");
                  __AppendElement($n8, $n11);
                } else {
                  __SetAttribute($n11, "text", "·" + item.spu_name);
                }
              }
              $update2 = _$temp2;
            }
            $update2 = _$temp;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "CombSubSpuInfo";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n2 = $lepusGetElementRefByLepusID("for", 2);
    $renderTemplates[$path].update_34b0098_2($lepusTemplate, $n2, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n1 = __CreateView($currentComponentId);
    __SetAttribute($n1, 1004, 1);
    __SetStyleObject($n1, [0, 1, 2]);
    __AppendElement($template, $n1);
    let $n2 = __CreateFor($currentComponentId);
    $lepusStoreElementRefByLepusID($n2, 2, "for");
    __AppendElement($n1, $n2);
    $renderTemplates["CombSubSpuInfo"].update_34b0098_2($lepusTemplate, $n2, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["SpuCombinationInfoList"] = {
  variables: ["subCombinationInfoList"],
  varUpdateState: [],
  update_8b290_13: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e;
    if (!$update2 || $renderTemplates["SpuCombinationInfoList"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo2 = $lepusPushFiberForNode($parent, 13, uniqueId),
          $forLepus = _$lepusPushFiberForNo2[0],
          $lastForLepus = _$lepusPushFiberForNo2[1];
      let $object = $data.subCombinationInfoList;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n14 = $update2 ? $lepusGetElementRefByLepusID("view", 14) : null;
          let $temp2 = $update2;
          if (!$n14) {
            $update2 = false;
            $n14 = __CreateView($currentComponentId);
            let $nid14 = $lepusStoreElementRefByLepusID($n14, 14, "view");
            __SetAttribute($n14, 1004, $nid14[1]);
            __AppendElement($parent, $n14);
          }
          __SetStyleObject($n14, [3, 4, {
            41: index != $data.subCombinationInfoList.length - 1 ? "4px" : "2px"
          }]);
          __SetAttribute($n14, "key", "sub-combination-info-list-item-wrapper-" + index);
          {
            let $n15 = $update2 ? $lepusGetElementRefByLepusID("if", 15) : null;
            if (!$n15) {
              $n15 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n15, 15, "if");
              __AppendElement($n14, $n15);
            }
            let uniqueId2 = __GetElementUniqueID($n15);
            if (!$update2) {
              $conditionNodeIndex[uniqueId2] = -1;
            }
            let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
            if ((item == null ? undefined : item.icon) && ((_a = item == null ? undefined : item.icon) == null ? undefined : _a.type) != 2) {
              __UpdateIfNodeIndex($n15, 0);
              $conditionNodeIndex[uniqueId2] = 0;
              let $temp3 = $update2;
              if ($ifNodeIndex !== 0) {
                $update2 = false;
              }
              {
                let $n16 = $update2 ? $lepusGetElementRefByLepusID("view", 16) : null;
                let $temp4 = $update2;
                if (!$n16) {
                  $update2 = false;
                  $n16 = __CreateView($currentComponentId);
                  let $nid16 = $lepusStoreElementRefByLepusID($n16, 16, "view");
                  __SetAttribute($n16, 1004, $nid16[1]);
                  __SetStyleObject($n16, [5, 6, 7, 3, 4, 8, 9, 10]);
                  __AppendElement($n15, $n16);
                }
                {
                  let $n17 = $update2 ? $lepusGetElementRefByLepusID("text", 17) : null;
                  let $temp5 = $update2;
                  if (!$n17) {
                    $update2 = false;
                    $n17 = __CreateText($currentComponentId);
                    let $nid17 = $lepusStoreElementRefByLepusID($n17, 17, "text");
                    __SetAttribute($n17, 1004, $nid17[1]);
                    __SetStyleObject($n17, [11, 12, 13]);
                    __AppendElement($n16, $n17);
                  }
                  __SetAttribute($n17, "text", ((_b = item == null ? undefined : item.icon) == null ? undefined : _b.content) || "享");
                  $update2 = $temp5;
                }
                $update2 = $temp4;
              }
              $update2 = $temp3;
            } else {
              __UpdateIfNodeIndex($n15, 1);
              $conditionNodeIndex[uniqueId2] = 1;
              let _$temp3 = $update2;
              if ($ifNodeIndex !== 1) {
                $update2 = false;
              }
              {
                let $n19 = $update2 ? $lepusGetElementRefByLepusID("image", 19) : null;
                let _$temp4 = $update2;
                if (!$n19) {
                  $update2 = false;
                  $n19 = __CreateImage($currentComponentId);
                  let $nid19 = $lepusStoreElementRefByLepusID($n19, 19, "image");
                  __SetAttribute($n19, 1004, $nid19[1]);
                  __SetStyleObject($n19, [5, 6, 7, 3, 4, 8]);
                  __SetAttribute($n19, "skip-redirection", true);
                  __AppendElement($n15, $n19);
                }
                __SetAttribute($n19, "src", (_c = item == null ? undefined : item.icon) == null ? undefined : _c.content);
                $update2 = _$temp4;
              }
              $update2 = _$temp3;
            }
          }
          {
            let $n20 = $update2 ? $lepusGetElementRefByLepusID("if", 20) : null;
            if (!$n20) {
              $n20 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n20, 20, "if");
              __AppendElement($n14, $n20);
            }
            let _uniqueId = __GetElementUniqueID($n20);
            if (!$update2) {
              $conditionNodeIndex[_uniqueId] = -1;
            }
            let _$ifNodeIndex = $conditionNodeIndex[_uniqueId];
            if ((_d = item == null ? undefined : item.spu_combination_info_tag_list) == null ? undefined : _d.length) {
              __UpdateIfNodeIndex($n20, 0);
              $conditionNodeIndex[_uniqueId] = 0;
              let _$temp5 = $update2;
              if (_$ifNodeIndex !== 0) {
                $update2 = false;
              }
              {
                let $n21 = $update2 ? $lepusGetElementRefByLepusID("view", 21) : null;
                let _$temp6 = $update2;
                if (!$n21) {
                  $update2 = false;
                  $n21 = __CreateView($currentComponentId);
                  let $nid21 = $lepusStoreElementRefByLepusID($n21, 21, "view");
                  __SetAttribute($n21, 1004, $nid21[1]);
                  __SetStyleObject($n21, [0, 6, 19, 14, 3, 4]);
                  __AppendElement($n20, $n21);
                }
                {
                  let $n22 = $update2 ? $lepusGetElementRefByLepusID("text", 22) : null;
                  let _$temp7 = $update2;
                  if (!$n22) {
                    $update2 = false;
                    $n22 = __CreateText($currentComponentId);
                    let $nid22 = $lepusStoreElementRefByLepusID($n22, 22, "text");
                    __SetAttribute($n22, 1004, $nid22[1]);
                    __SetStyleObject($n22, [14, 16, 19, 4]);
                    __AppendElement($n21, $n22);
                  }
                  let $n23 = $update2 ? $lepusGetElementRefByLepusID("for", 23) : null;
                  if (!$n23) {
                    $n23 = __CreateFor($currentComponentId);
                    $lepusStoreElementRefByLepusID($n23, 23, "for");
                    __AppendElement($n22, $n23);
                  }
                  {
                    let _$lepusPushFiberForNo3 = $lepusPushFiberForNode($n23, 23, undefined),
                        $forLepus2 = _$lepusPushFiberForNo3[0],
                        $lastForLepus2 = _$lepusPushFiberForNo3[1];
                    let $object2 = item == null ? undefined : item.spu_combination_info_tag_list;
                    let $length2 = _GetLength($object2);
                    __UpdateForChildCount($n23, $length2);
                    let $temp6 = $update2;
                    for (let index2 = 0; index2 < $length2; ++index2) {
                      $lepusUpdateFiberForNodeIndex(index2);
                      let item2 = $object2[index2];
                      {
                        let $n24 = $update2 ? $lepusGetElementRefByLepusID("if", 24) : null;
                        if (!$n24) {
                          $n24 = __CreateIf($currentComponentId);
                          $lepusStoreElementRefByLepusID($n24, 24, "if");
                          __AppendElement($n23, $n24);
                        }
                        let uniqueId3 = __GetElementUniqueID($n24);
                        if (!$update2) {
                          $conditionNodeIndex[uniqueId3] = -1;
                        }
                        let $ifNodeIndex2 = $conditionNodeIndex[uniqueId3];
                        if ((item2 == null ? undefined : item2.tag_type) === 1 || (item2 == null ? undefined : item2.tag_type) === 2 || (item2 == null ? undefined : item2.tag_type) === 3) {
                          __UpdateIfNodeIndex($n24, 0);
                          $conditionNodeIndex[uniqueId3] = 0;
                          let $temp7 = $update2;
                          if ($ifNodeIndex2 !== 0) {
                            $update2 = false;
                          }
                          {
                            let $n25 = $update2 ? $lepusGetElementRefByLepusID("text", 25) : null;
                            let $temp8 = $update2;
                            if (!$n25) {
                              $update2 = false;
                              $n25 = __CreateText($currentComponentId);
                              let $nid25 = $lepusStoreElementRefByLepusID($n25, 25, "text");
                              __SetAttribute($n25, 1004, $nid25[1]);
                              __SetAttribute($n25, "text-maxline", "1");
                              __AppendElement($n24, $n25);
                            }
                            __SetStyleObject($n25, [17, 19, {
                              48: (item2 == null ? undefined : item2.tag_type) === 2 ? "400" : "500"
                            }, {
                              22: (item2 == null ? undefined : item2.tag_type) === 3 ? "rgba(166, 98, 55, 1)" : "rgba(22, 24, 35, 0.85)"
                            }]);
                            __SetAttribute($n25, "text", (_e = item2 == null ? undefined : item2.tag_text) == null ? undefined : _e.content);
                            $update2 = $temp8;
                          }
                          $update2 = $temp7;
                        } else {
                          __UpdateIfNodeIndex($n24, -1);
                          $conditionNodeIndex[uniqueId3] = -1;
                        }
                      }
                      {
                        let $n27 = $update2 ? $lepusGetElementRefByLepusID("if", 27) : null;
                        if (!$n27) {
                          $n27 = __CreateIf($currentComponentId);
                          $lepusStoreElementRefByLepusID($n27, 27, "if");
                          __AppendElement($n20, $n27);
                        }
                        let _uniqueId2 = __GetElementUniqueID($n27);
                        if (!$update2) {
                          $conditionNodeIndex[_uniqueId2] = -1;
                        }
                        let _$ifNodeIndex2 = $conditionNodeIndex[_uniqueId2];
                        if ((item2 == null ? undefined : item2.tag_split_type) != 3) {
                          __UpdateIfNodeIndex($n27, 0);
                          $conditionNodeIndex[_uniqueId2] = 0;
                          let _$temp8 = $update2;
                          if (_$ifNodeIndex2 !== 0) {
                            $update2 = false;
                          }
                          {
                            let $n28 = $update2 ? $lepusGetElementRefByLepusID("text", 28) : null;
                            let _$temp9 = $update2;
                            if (!$n28) {
                              $update2 = false;
                              $n28 = __CreateText($currentComponentId);
                              let $nid28 = $lepusStoreElementRefByLepusID($n28, 28, "text");
                              __SetAttribute($n28, 1004, $nid28[1]);
                              __SetStyleObject($n28, [17, 21, 19, 23, 24]);
                              __SetAttribute($n28, "text-maxline", "1");
                              __AppendElement($n27, $n28);
                            }
                            __SetAttribute($n28, "text", (item2 == null ? undefined : item2.tag_split_type) === 1 ? "·" : " ");
                            $update2 = _$temp9;
                          }
                          $update2 = _$temp8;
                        } else {
                          __UpdateIfNodeIndex($n27, -1);
                          $conditionNodeIndex[_uniqueId2] = -1;
                        }
                      }
                    }
                    $update2 = $temp6;
                    $lepusPushFiberForNode($lastForLepus2, undefined, undefined);
                  }
                  $update2 = _$temp7;
                }
                $update2 = _$temp6;
              }
              $update2 = _$temp5;
            } else {
              __UpdateIfNodeIndex($n20, -1);
              $conditionNodeIndex[_uniqueId] = -1;
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "SpuCombinationInfoList";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n13 = $lepusGetElementRefByLepusID("for", 13);
    $renderTemplates[$path].update_8b290_13($lepusTemplate, $n13, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n12 = __CreateView($currentComponentId);
    __SetAttribute($n12, 1004, 12);
    __SetStyleObject($n12, [0, 1, 2]);
    __AppendElement($template, $n12);
    let $n13 = __CreateFor($currentComponentId);
    $lepusStoreElementRefByLepusID($n13, 13, "for");
    __AppendElement($n12, $n13);
    $renderTemplates["SpuCombinationInfoList"].update_8b290_13($lepusTemplate, $n13, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["SpuDecisionText"] = {
  variables: ["decisionText"],
  varUpdateState: [],
  update: function ($lepusTemplate, $data, $array) {
    let $path = "SpuDecisionText";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $value = $data.decisionText || "";
      if ($value !== ($lepusTemplate._data.decisionText || "")) {
        let $n31 = $lepusGetElementRefByLepusID("text", 31);
        __SetAttribute($n31, "text", $value);
      }
    }
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n30 = __CreateView($currentComponentId);
    __SetAttribute($n30, 1004, 30);
    __SetStyleObject($n30, [25]);
    __AppendElement($template, $n30);
    let $n31 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n31, 31, "text");
    __SetAttribute($n31, 1004, 31);
    __SetStyleObject($n31, [26, 27, 18, 16, 28]);
    __SetAttribute($n31, "text-maxline", "1");
    __AppendElement($n30, $n31);
    __SetAttribute($n31, "text", $data.decisionText || "");
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["SpuTag"] = {
  variables: ["saleTagVO"],
  varUpdateState: [],
  update_21ef030_34: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "SpuTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.saleTagVO.prefixImageUrl) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n35 = $update2 ? $lepusGetElementRefByLepusID("image", 35) : null;
            let $temp2 = $update2;
            if (!$n35) {
              $update2 = false;
              $n35 = __CreateImage($currentComponentId);
              let $nid35 = $lepusStoreElementRefByLepusID($n35, 35, "image");
              __SetAttribute($n35, 1004, $nid35[1]);
              __SetAttribute($n35, "mode", "aspectFill");
              __SetAttribute($n35, "skip-redirection", true);
              __AppendElement($parent, $n35);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.saleTagVO.prefixStyle;
                if (!$update2 || $value !== $lepusTemplate._data.saleTagVO.prefixStyle) {
                  __SetStyleObject($n35, [38, 39, 40, getCssPropertyIDObj($data.saleTagVO.prefixStyle)]);
                }
              }
              {
                let _$value = $data.saleTagVO.prefixImageUrl;
                if (!$update2 || _$value !== $lepusTemplate._data.saleTagVO.prefixImageUrl) {
                  __SetAttribute($n35, "src", _$value);
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
  update_21ef030_36: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "SpuTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.saleTagVO.content) {
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
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.saleTagVO.contentStyle;
                if (!$update2 || $value !== $lepusTemplate._data.saleTagVO.contentStyle) {
                  __SetStyleObject($n37, [11, 41, 42, getCssPropertyIDObj($data.saleTagVO.contentStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value2 = $data.saleTagVO.content;
                if (!$update2 || _$value2 !== $lepusTemplate._data.saleTagVO.content) {
                  __SetAttribute($n37, "text", _$value2);
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
  update_21ef030_39: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "SpuTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.saleTagVO.suffixImageUrl) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n40 = $update2 ? $lepusGetElementRefByLepusID("image", 40) : null;
            let $temp2 = $update2;
            if (!$n40) {
              $update2 = false;
              $n40 = __CreateImage($currentComponentId);
              let $nid40 = $lepusStoreElementRefByLepusID($n40, 40, "image");
              __SetAttribute($n40, 1004, $nid40[1]);
              __SetAttribute($n40, "mode", "aspectFill");
              __SetAttribute($n40, "skip-redirection", true);
              __AppendElement($parent, $n40);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.saleTagVO.suffixStyle;
                if (!$update2 || $value !== $lepusTemplate._data.saleTagVO.suffixStyle) {
                  __SetStyleObject($n40, [38, 39, 43, getCssPropertyIDObj($data.saleTagVO.suffixStyle)]);
                }
              }
              {
                let _$value3 = $data.saleTagVO.suffixImageUrl;
                if (!$update2 || _$value3 !== $lepusTemplate._data.saleTagVO.suffixImageUrl) {
                  __SetAttribute($n40, "src", _$value3);
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
  update_21ef030_41: function ($lepusTemplate, $parent, $data, $update2) {
    if (!$update2 || $renderTemplates["SpuTag"].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.saleTagVO.clickable && $data.saleTagVO.tagType != 23) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n42 = $update2 ? $lepusGetElementRefByLepusID("image", 42) : null;
            let $temp2 = $update2;
            if (!$n42) {
              $update2 = false;
              $n42 = __CreateImage($currentComponentId);
              let $nid42 = $lepusStoreElementRefByLepusID($n42, 42, "image");
              __SetAttribute($n42, 1004, $nid42[1]);
              __SetStyleObject($n42, [38, 39]);
              __SetAttribute($n42, "src", "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAABYlAAAWJQFJUiTwAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAHDSURBVHgBpVNNb9NAEJ2Zta2SQJ0qFQlyQE1AUaERCHLjx3PiwiFCqoIEEakT1Mo2brCrkrSKvbv1OE3kpB+q1Dnszr6dfZqPtwCPNNwEut2u6Z6ctUmrJgKW8yDCSGoZG3r2IwiC6Z0E9freXqLoE/tkkDuXScy+JcyKlsohDZZS0D89PRrcINjdbbVR4EcQOHj1wu73er1kM7ta7XVHARwIlN98fzRiTCwuamWF1meJ+HPiDw89z1NL0lJppzqbRRM+T6fR33KpamXuu/391m+OI75I4UkHEeZRMOyv1WfCc0XULGJhaPdltnOf8lJ50dqw0RDHmymH3tHX5kv7yzraS0iTy01eEQjUO8l1w4pWbbSd8ThqbuJzSuLlhAjuszRxwEDnvhAjLwH01MxGBQ80S5kViRCvMjCQXExki0X0EAJNykGS0YrA9+0Bd3b456xTDNwSGAsNYRGr198c6ExQrEo+iwXsqe2n25eE9H7rWRUu/v/LH52fRxOePfucXZqWP2jUb0HBYRge+4yvSdlxWu3LBHJNoKCTopRVuhibgfJ7EIzd5Zsbn4lVycJibfB485qzJgvAUaNR+XWbxB9lV9EKxA72ECw2AAAAAElFTkSuQmCC");
              __SetAttribute($n42, "skip-redirection", true);
              __AppendElement($parent, $n42);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "SpuTag";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n33 = $lepusGetElementRefByLepusID("view", 33);
      {
        let $value = $data.saleTagVO.tagBox;
        if (!$update2 || $value !== $lepusTemplate._data.saleTagVO.tagBox) {
          __SetStyleObject($n33, [3, 6, 4, 8, 29, 30, 31, 32, 33, 34, 35, 36, 37, getCssPropertyIDObj($data.saleTagVO.tagBox)]);
        }
      }
      {
        let _$value4 = {
          clickable: $data.saleTagVO.clickable,
          tag_type: $data.saleTagVO.tagType,
          explanation: $data.saleTagVO.explanation
        };
        if (!$update2 || _$value4 !== {
          clickable: $lepusTemplate._data.saleTagVO.clickable,
          tag_type: $lepusTemplate._data.saleTagVO.tagType,
          explanation: $lepusTemplate._data.saleTagVO.explanation
        }) {
          __AddDataset($n33, "item", _$value4);
        }
      }
    }
    let $n34 = $lepusGetElementRefByLepusID("if", 34);
    $renderTemplates[$path].update_21ef030_34($lepusTemplate, $n34, $data, $update2);
    let $n36 = $lepusGetElementRefByLepusID("if", 36);
    $renderTemplates[$path].update_21ef030_36($lepusTemplate, $n36, $data, $update2);
    let $n39 = $lepusGetElementRefByLepusID("if", 39);
    $renderTemplates[$path].update_21ef030_39($lepusTemplate, $n39, $data, $update2);
    let $n41 = $lepusGetElementRefByLepusID("if", 41);
    $renderTemplates[$path].update_21ef030_41($lepusTemplate, $n41, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $path = "SpuTag";
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n33 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n33, 33, "view");
    __SetAttribute($n33, 1004, 33);
    __SetStyleObject($n33, [3, 6, 4, 8, 29, 30, 31, 32, 33, 34, 35, 36, 37, getCssPropertyIDObj($data.saleTagVO.tagBox)]);
    __SetAttribute($n33, "implicit-animation", false);
    __AddDataset($n33, "item", {
      clickable: $data.saleTagVO.clickable,
      tag_type: $data.saleTagVO.tagType,
      explanation: $data.saleTagVO.explanation
    });
    __AddEvent($n33, "bindEvent", "tap", "handleTagTap");
    __AppendElement($template, $n33);
    let $n34 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n34, 34, "if");
    __AppendElement($n33, $n34);
    $renderTemplates[$path].update_21ef030_34($lepusTemplate, $n34, $data, $update2);
    let $n36 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n36, 36, "if");
    __AppendElement($n33, $n36);
    $renderTemplates[$path].update_21ef030_36($lepusTemplate, $n36, $data, $update2);
    let $n39 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n39, 39, "if");
    __AppendElement($n33, $n39);
    $renderTemplates[$path].update_21ef030_39($lepusTemplate, $n39, $data, $update2);
    let $n41 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n41, 41, "if");
    __AppendElement($n33, $n41);
    $renderTemplates[$path].update_21ef030_41($lepusTemplate, $n41, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["ImpressionTag"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_377e4c8_44: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ImpressionTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.vo.content_list == null || $data.vo.content_list == undefined || $data.vo.contentListSize <= 0) {
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
              let $nid45 = $lepusStoreElementRefByLepusID($n45, 45, "view");
              __SetAttribute($n45, 1004, $nid45[1]);
              __AppendElement($parent, $n45);
            }
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
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            let $n46 = $update2 ? $lepusGetElementRefByLepusID("for", 46) : null;
            if (!$n46) {
              $n46 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n46, 46, "for");
              __AppendElement($parent, $n46);
            }
            $renderTemplates[$path].update_377e4c8_46($lepusTemplate, $n46, $data, $update2);
          }
          $update2 = _$temp10;
        }
      }
    }
  },
  update_377e4c8_46: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ImpressionTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo4 = $lepusPushFiberForNode($parent, 46, uniqueId),
          $forLepus = _$lepusPushFiberForNo4[0],
          $lastForLepus = _$lepusPushFiberForNo4[1];
      let $object = $data.vo.content_list;
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
            __SetStyleObject($n47, [7, 3, 4]);
            __AppendElement($parent, $n47);
          }
          {
            let $n48 = $update2 ? $lepusGetElementRefByLepusID("text", 48) : null;
            let $temp3 = $update2;
            if (!$n48) {
              $update2 = false;
              $n48 = __CreateText($currentComponentId);
              let $nid48 = $lepusStoreElementRefByLepusID($n48, 48, "text");
              __SetAttribute($n48, 1004, $nid48[1]);
              __SetClasses($n48, "tag-text-text-no-shrink");
              __AppendElement($n47, $n48);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.content_style;
                if (!$update2 || $value !== $lepusTemplate._data.vo.content_style) {
                  __SetStyleObject($n48, [11, 46, 47, 48, 31, getCssPropertyIDObj($data.vo.content_style)]);
                }
              }
            }
            __SetAttribute($n48, "text", "“");
            $update2 = $temp3;
          }
          {
            let $n50 = $update2 ? $lepusGetElementRefByLepusID("text", 50) : null;
            let _$temp11 = $update2;
            if (!$n50) {
              $update2 = false;
              $n50 = __CreateText($currentComponentId);
              let $nid50 = $lepusStoreElementRefByLepusID($n50, 50, "text");
              __SetAttribute($n50, 1004, $nid50[1]);
              __SetClasses($n50, "tag-text-text-ellipsis");
              __AppendElement($n47, $n50);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value5 = $data.vo.content_style;
                if (!$update2 || _$value5 !== $lepusTemplate._data.vo.content_style) {
                  __SetStyleObject($n50, [11, 46, 47, 48, 41, 16, 49, getCssPropertyIDObj($data.vo.content_style)]);
                }
              }
            }
            __SetAttribute($n50, "text", item);
            $update2 = _$temp11;
          }
          {
            let $n52 = $update2 ? $lepusGetElementRefByLepusID("text", 52) : null;
            let _$temp12 = $update2;
            if (!$n52) {
              $update2 = false;
              $n52 = __CreateText($currentComponentId);
              let $nid52 = $lepusStoreElementRefByLepusID($n52, 52, "text");
              __SetAttribute($n52, 1004, $nid52[1]);
              __SetClasses($n52, "tag-text-text-no-shrink");
              __AppendElement($n47, $n52);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value6 = $data.vo.content_style;
                if (!$update2 || _$value6 !== $lepusTemplate._data.vo.content_style) {
                  __SetStyleObject($n52, [11, 46, 47, 48, 31, getCssPropertyIDObj($data.vo.content_style)]);
                }
              }
            }
            __SetAttribute($n52, "text", "”");
            $update2 = _$temp12;
          }
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "ImpressionTag";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n44 = $lepusGetElementRefByLepusID("if", 44);
    $renderTemplates[$path].update_377e4c8_44($lepusTemplate, $n44, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n43 = __CreateView($currentComponentId);
    __SetAttribute($n43, 1004, 43);
    __SetStyleObject($n43, [3, 44, 45, 0]);
    __AppendElement($template, $n43);
    let $n44 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n44, 44, "if");
    __AppendElement($n43, $n44);
    $renderTemplates["ImpressionTag"].update_377e4c8_44($lepusTemplate, $n44, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["ProductPointTag"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_377e4c8_55: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ProductPointTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.vo.prefixImageUrl) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n56 = $update2 ? $lepusGetElementRefByLepusID("image", 56) : null;
            let $temp2 = $update2;
            if (!$n56) {
              $update2 = false;
              $n56 = __CreateImage($currentComponentId);
              let $nid56 = $lepusStoreElementRefByLepusID($n56, 56, "image");
              __SetAttribute($n56, 1004, $nid56[1]);
              __SetAttribute($n56, "mode", "aspectFill");
              __SetAttribute($n56, "skip-redirection", true);
              __AppendElement($parent, $n56);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.prefixStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.prefixStyle) {
                  __SetStyleObject($n56, [38, 39, 40, getCssPropertyIDObj($data.vo.prefixStyle)]);
                }
              }
              {
                let _$value7 = $data.vo.prefixImageUrl;
                if (!$update2 || _$value7 !== $lepusTemplate._data.vo.prefixImageUrl) {
                  __SetAttribute($n56, "src", _$value7);
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
  update_377e4c8_59: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ProductPointTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.vo.suffixImageUrl) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n60 = $update2 ? $lepusGetElementRefByLepusID("image", 60) : null;
            let $temp2 = $update2;
            if (!$n60) {
              $update2 = false;
              $n60 = __CreateImage($currentComponentId);
              let $nid60 = $lepusStoreElementRefByLepusID($n60, 60, "image");
              __SetAttribute($n60, 1004, $nid60[1]);
              __SetAttribute($n60, "mode", "aspectFill");
              __SetAttribute($n60, "skip-redirection", true);
              __AppendElement($parent, $n60);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.suffixStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.suffixStyle) {
                  __SetStyleObject($n60, [38, 39, 43, getCssPropertyIDObj($data.vo.suffixStyle)]);
                }
              }
              {
                let _$value8 = $data.vo.suffixImageUrl;
                if (!$update2 || _$value8 !== $lepusTemplate._data.vo.suffixImageUrl) {
                  __SetAttribute($n60, "src", _$value8);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "ProductPointTag";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n55 = $lepusGetElementRefByLepusID("if", 55);
    $renderTemplates[$path].update_377e4c8_55($lepusTemplate, $n55, $data, $update2);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n57 = $lepusGetElementRefByLepusID("text", 57);
      {
        let $value = $data.vo.content_style;
        if (!$update2 || $value !== $lepusTemplate._data.vo.content_style) {
          __SetStyleObject($n57, [11, 46, 47, 48, getCssPropertyIDObj($data.vo.content_style)]);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let _$value9 = $data.vo.content_list[0];
      if (_$value9 !== $lepusTemplate._data.vo.content_list[0]) {
        let _$n = $lepusGetElementRefByLepusID("text", 57);
        __SetAttribute(_$n, "text", _$value9);
      }
    }
    let $n59 = $lepusGetElementRefByLepusID("if", 59);
    $renderTemplates[$path].update_377e4c8_59($lepusTemplate, $n59, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $path = "ProductPointTag";
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n54 = __CreateView($currentComponentId);
    __SetAttribute($n54, 1004, 54);
    __SetStyleObject($n54, [7, 3, 4]);
    __AppendElement($template, $n54);
    let $n55 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n55, 55, "if");
    __AppendElement($n54, $n55);
    $renderTemplates[$path].update_377e4c8_55($lepusTemplate, $n55, $data, $update2);
    let $n57 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n57, 57, "text");
    __SetAttribute($n57, 1004, 57);
    __SetStyleObject($n57, [11, 46, 47, 48, getCssPropertyIDObj($data.vo.content_style)]);
    __AppendElement($n54, $n57);
    __SetAttribute($n57, "text", $data.vo.content_list[0]);
    let $n59 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n59, 59, "if");
    __AppendElement($n54, $n59);
    $renderTemplates[$path].update_377e4c8_59($lepusTemplate, $n59, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["RecommendDishTag"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_377e4c8_61: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "RecommendDishTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.vo.content_list == null || $data.vo.content_list == undefined || $data.vo.contentListSize <= 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp13 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n62 = $update2 ? $lepusGetElementRefByLepusID("view", 62) : null;
            let $temp2 = $update2;
            if (!$n62) {
              $update2 = false;
              $n62 = __CreateView($currentComponentId);
              let $nid62 = $lepusStoreElementRefByLepusID($n62, 62, "view");
              __SetAttribute($n62, 1004, $nid62[1]);
              __SetStyleObject($n62, [7, 3, 4]);
              __AppendElement($parent, $n62);
            }
            {
              let $template_update = $update2;
              let $n63 = $update2 ? $lepusGetElementRefByLepusID("if", 63) : null;
              if (!$n63) {
                $update2 = false;
                $n63 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n63, 63, "if");
                __AppendElement($n62, $n63);
              }
              $renderTemplates[$path].update_377e4c8_63($lepusTemplate, $n63, $data, $update2);
              $update2 = $template_update;
            }
            $update2 = $temp2;
          }
          $update2 = _$temp13;
        }
      }
    }
  },
  update_377e4c8_63: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "RecommendDishTag";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.contentListSize == 1) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n64 = $update2 ? $lepusGetElementRefByLepusID("text", 64) : null;
          let $temp2 = $update2;
          if (!$n64) {
            $update2 = false;
            $n64 = __CreateText($currentComponentId);
            let $nid64 = $lepusStoreElementRefByLepusID($n64, 64, "text");
            __SetAttribute($n64, 1004, $nid64[1]);
            __AppendElement($parent, $n64);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.content_style;
              if (!$update2 || $value !== $lepusTemplate._data.vo.content_style) {
                __SetStyleObject($n64, [11, 46, 47, 48, 31, getCssPropertyIDObj($data.vo.content_style)]);
              }
            }
          }
          __SetAttribute($n64, "text", "含推荐菜「");
          $update2 = $temp2;
        }
        {
          let $n66 = $update2 ? $lepusGetElementRefByLepusID("text", 66) : null;
          let _$temp14 = $update2;
          if (!$n66) {
            $update2 = false;
            $n66 = __CreateText($currentComponentId);
            let $nid66 = $lepusStoreElementRefByLepusID($n66, 66, "text");
            __SetAttribute($n66, 1004, $nid66[1]);
            __SetAttribute($n66, "text-maxline", "1");
            __AppendElement($parent, $n66);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let _$value10 = $data.vo.content_style;
              if (!$update2 || _$value10 !== $lepusTemplate._data.vo.content_style) {
                __SetStyleObject($n66, [11, 46, 47, 48, 41, 16, 49, getCssPropertyIDObj($data.vo.content_style)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value11 = $data.vo.content_list[0];
              if (!$update2 || _$value11 !== $lepusTemplate._data.vo.content_list[0]) {
                __SetAttribute($n66, "text", _$value11);
              }
            }
          }
          $update2 = _$temp14;
        }
        {
          let $n68 = $update2 ? $lepusGetElementRefByLepusID("text", 68) : null;
          let _$temp15 = $update2;
          if (!$n68) {
            $update2 = false;
            $n68 = __CreateText($currentComponentId);
            let $nid68 = $lepusStoreElementRefByLepusID($n68, 68, "text");
            __SetAttribute($n68, 1004, $nid68[1]);
            __AppendElement($parent, $n68);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let _$value12 = $data.vo.content_style;
              if (!$update2 || _$value12 !== $lepusTemplate._data.vo.content_style) {
                __SetStyleObject($n68, [11, 46, 47, 48, 31, getCssPropertyIDObj($data.vo.content_style)]);
              }
            }
          }
          __SetAttribute($n68, "text", "」");
          $update2 = _$temp15;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp16 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n70 = $update2 ? $lepusGetElementRefByLepusID("text", 70) : null;
          let _$temp17 = $update2;
          if (!$n70) {
            $update2 = false;
            $n70 = __CreateText($currentComponentId);
            let $nid70 = $lepusStoreElementRefByLepusID($n70, 70, "text");
            __SetAttribute($n70, 1004, $nid70[1]);
            __AppendElement($parent, $n70);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let _$value13 = $data.vo.content_style;
              if (!$update2 || _$value13 !== $lepusTemplate._data.vo.content_style) {
                __SetStyleObject($n70, [11, 46, 47, 48, 31, getCssPropertyIDObj($data.vo.content_style)]);
              }
            }
          }
          __SetAttribute($n70, "text", "含「");
          $update2 = _$temp17;
        }
        {
          let $n72 = $update2 ? $lepusGetElementRefByLepusID("text", 72) : null;
          let _$temp18 = $update2;
          if (!$n72) {
            $update2 = false;
            $n72 = __CreateText($currentComponentId);
            let $nid72 = $lepusStoreElementRefByLepusID($n72, 72, "text");
            __SetAttribute($n72, 1004, $nid72[1]);
            __SetAttribute($n72, "text-maxline", "1");
            __AppendElement($parent, $n72);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let _$value14 = $data.vo.content_style;
              if (!$update2 || _$value14 !== $lepusTemplate._data.vo.content_style) {
                __SetStyleObject($n72, [11, 46, 47, 48, 31, 3, 4, getCssPropertyIDObj($data.vo.content_style)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value15 = $data.vo.content_list[0];
              if (!$update2 || _$value15 !== $lepusTemplate._data.vo.content_list[0]) {
                __SetAttribute($n72, "text", _$value15);
              }
            }
          }
          $update2 = _$temp18;
        }
        {
          let $n74 = $update2 ? $lepusGetElementRefByLepusID("text", 74) : null;
          let _$temp19 = $update2;
          if (!$n74) {
            $update2 = false;
            $n74 = __CreateText($currentComponentId);
            let $nid74 = $lepusStoreElementRefByLepusID($n74, 74, "text");
            __SetAttribute($n74, 1004, $nid74[1]);
            __AppendElement($parent, $n74);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let _$value16 = $data.vo.content_style;
              if (!$update2 || _$value16 !== $lepusTemplate._data.vo.content_style) {
                __SetStyleObject($n74, [11, 46, 47, 48, 31, getCssPropertyIDObj($data.vo.content_style)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value17 = "」等" + $data.vo.contentListSize + "个推荐菜";
              if (!$update2 || _$value17 !== "」等" + $lepusTemplate._data.vo.contentListSize + "个推荐菜") {
                __SetAttribute($n74, "text", _$value17);
              }
            }
          }
          $update2 = _$temp19;
        }
        $update2 = _$temp16;
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "RecommendDishTag";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n61 = $lepusGetElementRefByLepusID("if", 61);
    $renderTemplates[$path].update_377e4c8_61($lepusTemplate, $n61, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n61 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n61, 61, "if");
    __AppendElement($template, $n61);
    $renderTemplates["RecommendDishTag"].update_377e4c8_61($lepusTemplate, $n61, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["SpuTitle"] = {
  variables: ["spuCardTitleVO"],
  varUpdateState: [],
  update_2a72360_79: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "SpuTitle";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.spuCardTitleVO.tagUrl != undefined && $data.spuCardTitleVO.tagUrl && ($data.spuCardTitleVO.tagLoadError ? $data.spuCardTitleVO.tagLoadError : false) == false) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n80 = $update2 ? $lepusGetElementRefByLepusID("image", 80) : null;
            let $temp2 = $update2;
            if (!$n80) {
              $update2 = false;
              $n80 = __CreateImage($currentComponentId);
              let $nid80 = $lepusStoreElementRefByLepusID($n80, 80, "image");
              __SetAttribute($n80, 1004, $nid80[1]);
              __SetAttribute($n80, "skip-redirection", true);
              __SetAttribute($n80, "android-simple-cache-key", true);
              __AddEvent($n80, "bindEvent", "error", "handleTagLoadError");
              __AddEvent($n80, "bindEvent", "load", "handleTagLoaded");
              __AppendElement($parent, $n80);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = "width:" + ((($data.spuCardTitleVO.imgWidth ? $data.spuCardTitleVO.imgWidth : 0) || ($data.spuCardTitleVO.tagWidth ? $data.spuCardTitleVO.tagWidth : 1)) + "px;") + ("margin-right:" + ((($data.spuCardTitleVO.imgWidth ? $data.spuCardTitleVO.imgWidth : 0) > 1 || ($data.spuCardTitleVO.tagWidth ? $data.spuCardTitleVO.tagWidth : 1) > 1 ? 6 : 0) + "px;") + ("opacity:" + (((($data.spuCardTitleVO.imgWidth ? $data.spuCardTitleVO.imgWidth : 0) || ($data.spuCardTitleVO.tagWidth ? $data.spuCardTitleVO.tagWidth : 1)) > 1 ? 1 : 0) + ";"))) + "height:14px;border-radius:2px;";
                if (!$update2 || $value !== undefined) {
                  __SetStyleObject($n80, [52, 32, {
                    27: (($data.spuCardTitleVO.imgWidth ? $data.spuCardTitleVO.imgWidth : 0) || ($data.spuCardTitleVO.tagWidth ? $data.spuCardTitleVO.tagWidth : 1)) + "px"
                  }, {
                    39: (($data.spuCardTitleVO.imgWidth ? $data.spuCardTitleVO.imgWidth : 0) > 1 || ($data.spuCardTitleVO.tagWidth ? $data.spuCardTitleVO.tagWidth : 1) > 1 ? 6 : 0) + "px"
                  }, {
                    23: ((($data.spuCardTitleVO.imgWidth ? $data.spuCardTitleVO.imgWidth : 0) || ($data.spuCardTitleVO.tagWidth ? $data.spuCardTitleVO.tagWidth : 1)) > 1 ? 1 : 0) + ""
                  }]);
                }
              }
              {
                let _$value18 = $data.spuCardTitleVO.tagUrl ? $data.spuCardTitleVO.tagUrl : "";
                if (!$update2 || _$value18 !== ($lepusTemplate._data.spuCardTitleVO.tagUrl ? $lepusTemplate._data.spuCardTitleVO.tagUrl : "")) {
                  __SetAttribute($n80, "src", _$value18);
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
  update_2a72360_81: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "SpuTitle";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.spuCardTitleVO.spuNameHighLightInfo != undefined && $data.spuCardTitleVO.spuNameHighLightInfo != null) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n82 = $update2 ? $lepusGetElementRefByLepusID("text", 82) : null;
            let $temp2 = $update2;
            if (!$n82) {
              $update2 = false;
              $n82 = __CreateText($currentComponentId);
              let $nid82 = $lepusStoreElementRefByLepusID($n82, 82, "text");
              __SetAttribute($n82, 1004, $nid82[1]);
              __SetStyleObject($n82, [53, 21, 54]);
              __AppendElement($parent, $n82);
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let $value = $data.spuCardTitleVO.spuNameHighLightInfo.highlightBeforeText || "";
                if (!$update2 || $value !== ($lepusTemplate._data.spuCardTitleVO.spuNameHighLightInfo.highlightBeforeText || "")) {
                  __SetAttribute($n82, "text", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          {
            let $n84 = $update2 ? $lepusGetElementRefByLepusID("text", 84) : null;
            let _$temp20 = $update2;
            if (!$n84) {
              $update2 = false;
              $n84 = __CreateText($currentComponentId);
              let $nid84 = $lepusStoreElementRefByLepusID($n84, 84, "text");
              __SetAttribute($n84, 1004, $nid84[1]);
              __AppendElement($parent, $n84);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value19 = "color:" + (($data.spuCardTitleVO.spuNameHighLightInfo.hightlightColor || "") + ";") + "font-size:14px;font-weight:500;color:#161823ff;";
                if (!$update2 || _$value19 !== "color:" + (($lepusTemplate._data.spuCardTitleVO.spuNameHighLightInfo.hightlightColor || "") + ";") + "font-size:14px;font-weight:500;color:#161823ff;") {
                  __SetStyleObject($n84, [53, 21, 54, {
                    22: ($data.spuCardTitleVO.spuNameHighLightInfo.hightlightColor || "") + ""
                  }]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value20 = $data.spuCardTitleVO.spuNameHighLightInfo.highlightText || "";
                if (!$update2 || _$value20 !== ($lepusTemplate._data.spuCardTitleVO.spuNameHighLightInfo.highlightText || "")) {
                  __SetAttribute($n84, "text", _$value20);
                }
              }
            }
            $update2 = _$temp20;
          }
          {
            let $n86 = $update2 ? $lepusGetElementRefByLepusID("text", 86) : null;
            let _$temp21 = $update2;
            if (!$n86) {
              $update2 = false;
              $n86 = __CreateText($currentComponentId);
              let $nid86 = $lepusStoreElementRefByLepusID($n86, 86, "text");
              __SetAttribute($n86, 1004, $nid86[1]);
              __SetStyleObject($n86, [53, 21, 54]);
              __AppendElement($parent, $n86);
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value21 = $data.spuCardTitleVO.spuNameHighLightInfo.highlightAfterText || "";
                if (!$update2 || _$value21 !== ($lepusTemplate._data.spuCardTitleVO.spuNameHighLightInfo.highlightAfterText || "")) {
                  __SetAttribute($n86, "text", _$value21);
                }
              }
            }
            $update2 = _$temp21;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp22 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n88 = $update2 ? $lepusGetElementRefByLepusID("text", 88) : null;
            let _$temp23 = $update2;
            if (!$n88) {
              $update2 = false;
              $n88 = __CreateText($currentComponentId);
              let $nid88 = $lepusStoreElementRefByLepusID($n88, 88, "text");
              __SetAttribute($n88, 1004, $nid88[1]);
              __SetStyleObject($n88, [53, 21, 54]);
              __AppendElement($parent, $n88);
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value22 = $data.spuCardTitleVO.spuName || "";
                if (!$update2 || _$value22 !== ($lepusTemplate._data.spuCardTitleVO.spuName || "")) {
                  __SetAttribute($n88, "text", _$value22);
                }
              }
            }
            $update2 = _$temp23;
          }
          $update2 = _$temp22;
        }
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "SpuTitle";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n77 = $lepusGetElementRefByLepusID("text", 77);
      {
        let $value = "line-height:" + (($data.spuCardTitleVO.titleShowAll ? "20px" : "unset") + ";") + "text-overflow:ellipsis;";
        if (!$update2 || $value !== "line-height:" + (($lepusTemplate._data.spuCardTitleVO.titleShowAll ? "20px" : "unset") + ";") + "text-overflow:ellipsis;") {
          __SetStyleObject($n77, [16, {
            45: $data.spuCardTitleVO.titleShowAll ? "20px" : "unset"
          }]);
        }
      }
      {
        let _$value23 = $data.spuCardTitleVO.titleShowAll ? "2" : "1";
        if (!$update2 || _$value23 !== ($lepusTemplate._data.spuCardTitleVO.titleShowAll ? "2" : "1")) {
          __SetAttribute($n77, "text-maxline", _$value23);
        }
      }
    }
    let $n79 = $lepusGetElementRefByLepusID("if", 79);
    $renderTemplates[$path].update_2a72360_79($lepusTemplate, $n79, $data, $update2);
    let $n81 = $lepusGetElementRefByLepusID("if", 81);
    $renderTemplates[$path].update_2a72360_81($lepusTemplate, $n81, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $path = "SpuTitle";
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n76 = __CreateView($currentComponentId);
    __SetAttribute($n76, 1004, 76);
    __SetStyleObject($n76, [50]);
    __AppendElement($template, $n76);
    let $n77 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n77, 77, "text");
    __SetAttribute($n77, 1004, 77);
    __SetStyleObject($n77, [16, {
      45: $data.spuCardTitleVO.titleShowAll ? "20px" : "unset"
    }]);
    __SetAttribute($n77, "accessibility-element", false);
    __SetAttribute($n77, "text-maxline", $data.spuCardTitleVO.titleShowAll ? "2" : "1");
    __AppendElement($n76, $n77);
    let $n78 = __CreateView($currentComponentId);
    __SetAttribute($n78, 1004, 78);
    __SetStyleObject($n78, [51, 48]);
    __AppendElement($n77, $n78);
    let $n79 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n79, 79, "if");
    __AppendElement($n78, $n79);
    $renderTemplates[$path].update_2a72360_79($lepusTemplate, $n79, $data, $update2);
    let $n81 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n81, 81, "if");
    __AppendElement($n77, $n81);
    $renderTemplates[$path].update_2a72360_81($lepusTemplate, $n81, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["ImageTag"] = {
  variables: ["spuCardImageTagVO"],
  varUpdateState: [],
  update_36f5948_92: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ImageTag";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.spuCardImageTagVO.showRecommend) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n93 = $update2 ? $lepusGetElementRefByLepusID("image", 93) : null;
          let $temp2 = $update2;
          if (!$n93) {
            $update2 = false;
            $n93 = __CreateImage($currentComponentId);
            let $nid93 = $lepusStoreElementRefByLepusID($n93, 93, "image");
            __SetAttribute($n93, 1004, $nid93[1]);
            __SetStyleObject($n93, [55, 56, 57, 6, 58]);
            __AppendElement($parent, $n93);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.spuCardImageTagVO.imageUrl;
              if (!$update2 || $value !== $lepusTemplate._data.spuCardImageTagVO.imageUrl) {
                __SetAttribute($n93, "src", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp24 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n94 = $update2 ? $lepusGetElementRefByLepusID("if", 94) : null;
          if (!$n94) {
            $update2 = false;
            $n94 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n94, 94, "if");
            __AppendElement($parent, $n94);
          }
          $renderTemplates[$path].update_36f5948_94($lepusTemplate, $n94, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp24;
      }
    }
  },
  update_36f5948_94: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.spuCardImageTagVO.videoSameStyle == "independent") {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n95 = $update2 ? $lepusGetElementRefByLepusID("if", 95) : null;
          if (!$n95) {
            $update2 = false;
            $n95 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n95, 95, "if");
            __AppendElement($parent, $n95);
          }
          $renderTemplates["ImageTag"].update_36f5948_95($lepusTemplate, $n95, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_36f5948_95: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ImageTag";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.spuCardImageTagVO.imageUrl != null && $data.spuCardImageTagVO.imageUrl != "" && $data.spuCardImageTagVO.imageUrl != undefined) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n96 = $update2 ? $lepusGetElementRefByLepusID("image", 96) : null;
          let $temp2 = $update2;
          if (!$n96) {
            $update2 = false;
            $n96 = __CreateImage($currentComponentId);
            let $nid96 = $lepusStoreElementRefByLepusID($n96, 96, "image");
            __SetAttribute($n96, 1004, $nid96[1]);
            __SetStyleObject($n96, [55, 56, 57, 6, 58]);
            __AppendElement($parent, $n96);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.spuCardImageTagVO.imageUrl;
              if (!$update2 || $value !== $lepusTemplate._data.spuCardImageTagVO.imageUrl) {
                __SetAttribute($n96, "src", $value);
              }
            }
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
          let $n97 = $update2 ? $lepusGetElementRefByLepusID("view", 97) : null;
          let _$temp26 = $update2;
          if (!$n97) {
            $update2 = false;
            $n97 = __CreateView($currentComponentId);
            let $nid97 = $lepusStoreElementRefByLepusID($n97, 97, "view");
            __SetAttribute($n97, 1004, $nid97[1]);
            __SetStyleObject($n97, [55, 56, 57]);
            __AppendElement($parent, $n97);
          }
          {
            let $n98 = $update2 ? $lepusGetElementRefByLepusID("view", 98) : null;
            let $temp3 = $update2;
            if (!$n98) {
              $update2 = false;
              $n98 = __CreateView($currentComponentId);
              let $nid98 = $lepusStoreElementRefByLepusID($n98, 98, "view");
              __SetAttribute($n98, 1004, $nid98[1]);
              __SetStyleObject($n98, [3, 4, 59, 6, 60, 61, 62, 45, 8]);
              __AppendElement($n97, $n98);
            }
            {
              let $template_update = $update2;
              let $n99 = $update2 ? $lepusGetElementRefByLepusID("if", 99) : null;
              if (!$n99) {
                $update2 = false;
                $n99 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n99, 99, "if");
                __AppendElement($n98, $n99);
              }
              $renderTemplates[$path].update_36f5948_99($lepusTemplate, $n99, $data, $update2);
              $update2 = $template_update;
            }
            $update2 = $temp3;
          }
          $update2 = _$temp26;
        }
        $update2 = _$temp25;
      }
    }
  },
  update_36f5948_99: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.spuCardImageTagVO.videoSameTagTextStyle == 1) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n100 = $update2 ? $lepusGetElementRefByLepusID("text", 100) : null;
          let $temp2 = $update2;
          if (!$n100) {
            $update2 = false;
            $n100 = __CreateText($currentComponentId);
            let $nid100 = $lepusStoreElementRefByLepusID($n100, 100, "text");
            __SetAttribute($n100, 1004, $nid100[1]);
            __SetStyleObject($n100, [63, 11, 64, 65]);
            __AppendElement($parent, $n100);
          }
          __SetAttribute($n100, "text", "跟TA买");
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
          let $n102 = $update2 ? $lepusGetElementRefByLepusID("text", 102) : null;
          let _$temp28 = $update2;
          if (!$n102) {
            $update2 = false;
            $n102 = __CreateText($currentComponentId);
            let $nid102 = $lepusStoreElementRefByLepusID($n102, 102, "text");
            __SetAttribute($n102, 1004, $nid102[1]);
            __SetStyleObject($n102, [63, 11, 65]);
            __AppendElement($parent, $n102);
          }
          __SetAttribute($n102, "text", "视频同款");
          $update2 = _$temp28;
        }
        $update2 = _$temp27;
      }
    }
  },
  update_3b11d38_91: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ImageTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.spuCardImageTagVO.tag_type == 23) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $template_update = $update2;
            let $n92 = $update2 ? $lepusGetElementRefByLepusID("if", 92) : null;
            if (!$n92) {
              $update2 = false;
              $n92 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n92, 92, "if");
              __AppendElement($parent, $n92);
            }
            $renderTemplates[$path].update_36f5948_92($lepusTemplate, $n92, $data, $update2);
            $update2 = $template_update;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp29 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let _$template_update = $update2;
            let $n104 = $update2 ? $lepusGetElementRefByLepusID("if", 104) : null;
            if (!$n104) {
              $update2 = false;
              $n104 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n104, 104, "if");
              __AppendElement($parent, $n104);
            }
            $renderTemplates[$path].update_3b11d38_104($lepusTemplate, $n104, $data, $update2);
            $update2 = _$template_update;
          }
          $update2 = _$temp29;
        }
      }
    }
  },
  update_3b11d38_104: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.spuCardImageTagVO.imageUrl) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n105 = $update2 ? $lepusGetElementRefByLepusID("if", 105) : null;
          if (!$n105) {
            $update2 = false;
            $n105 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n105, 105, "if");
            __AppendElement($parent, $n105);
          }
          $renderTemplates["ImageTag"].update_3b11d38_105($lepusTemplate, $n105, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_3b11d38_105: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ImageTag";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.spuCardImageTagVO.tag_type == 301 && $data.spuCardImageTagVO.showRecommend) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n106 = $update2 ? $lepusGetElementRefByLepusID("image", 106) : null;
          let $temp2 = $update2;
          if (!$n106) {
            $update2 = false;
            $n106 = __CreateImage($currentComponentId);
            let $nid106 = $lepusStoreElementRefByLepusID($n106, 106, "image");
            __SetAttribute($n106, 1004, $nid106[1]);
            __AppendElement($parent, $n106);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
            {
              let $value = "left:" + (($data.spuCardImageTagVO.tag_box && _GetLength($data.spuCardImageTagVO.tag_box) > 0 ? "-2px" : "") + ";") + "height:18px;width:54px;position:absolute;top:-2px;animation-name:image-expand;animation-duration:800ms;animation-delay:200ms;animation-timing-function:linear;animation-fill-mode:both;";
              if (!$update2 || $value !== undefined) {
                __SetStyleObject($n106, [66, 67, 55, 68, 69, 70, 71, 72, 73, {
                  2: $data.spuCardImageTagVO.tag_box && _GetLength($data.spuCardImageTagVO.tag_box) > 0 ? "-2px" : ""
                }]);
              }
            }
            {
              let _$value24 = $data.spuCardImageTagVO.imageUrl;
              if (!$update2 || _$value24 !== $lepusTemplate._data.spuCardImageTagVO.imageUrl) {
                __SetAttribute($n106, "src", _$value24);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp30 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n107 = $update2 ? $lepusGetElementRefByLepusID("if", 107) : null;
          if (!$n107) {
            $update2 = false;
            $n107 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n107, 107, "if");
            __AppendElement($parent, $n107);
          }
          $renderTemplates[$path].update_3b11d38_107($lepusTemplate, $n107, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp30;
      }
    }
  },
  update_3b11d38_107: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ImageTag";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.spuCardImageTagVO.tag_type == 301 && !$data.spuCardImageTagVO.showRecommend) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n108 = $update2 ? $lepusGetElementRefByLepusID("image", 108) : null;
          let $temp2 = $update2;
          if (!$n108) {
            $update2 = false;
            $n108 = __CreateImage($currentComponentId);
            let $nid108 = $lepusStoreElementRefByLepusID($n108, 108, "image");
            __SetAttribute($n108, 1004, $nid108[1]);
            __AppendElement($parent, $n108);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
            {
              let $value = "left:" + (($data.spuCardImageTagVO.tag_box && _GetLength($data.spuCardImageTagVO.tag_box) > 0 ? "-2px" : "") + ";") + "height:18px;width:54px;position:absolute;top:-2px;";
              if (!$update2 || $value !== undefined) {
                __SetStyleObject($n108, [66, 67, 55, 68, {
                  2: $data.spuCardImageTagVO.tag_box && _GetLength($data.spuCardImageTagVO.tag_box) > 0 ? "-2px" : ""
                }]);
              }
            }
            {
              let _$value25 = $data.spuCardImageTagVO.imageUrl;
              if (!$update2 || _$value25 !== $lepusTemplate._data.spuCardImageTagVO.imageUrl) {
                __SetAttribute($n108, "src", _$value25);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 3);
        $conditionNodeIndex[uniqueId] = 3;
        let _$temp31 = $update2;
        if ($ifNodeIndex !== 3) {
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
          $renderTemplates[$path].update_3b11d38_109($lepusTemplate, $n109, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp31;
      }
    }
  },
  update_3b11d38_109: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ImageTag";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.spuCardImageTagVO.showRecommend) {
        __UpdateIfNodeIndex($parent, 4);
        $conditionNodeIndex[uniqueId] = 4;
        let $temp = $update2;
        if ($ifNodeIndex !== 4) {
          $update2 = false;
        }
        {
          let $n110 = $update2 ? $lepusGetElementRefByLepusID("image", 110) : null;
          let $temp2 = $update2;
          if (!$n110) {
            $update2 = false;
            $n110 = __CreateImage($currentComponentId);
            let $nid110 = $lepusStoreElementRefByLepusID($n110, 110, "image");
            __SetAttribute($n110, 1004, $nid110[1]);
            __AppendElement($parent, $n110);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
            {
              let $value = "left:" + (($data.spuCardImageTagVO.tag_box && _GetLength($data.spuCardImageTagVO.tag_box) > 0 ? "-2px" : "4px") + ";") + "height:16px;width:48px;position:absolute;top:4px;animation-name:image-expand;animation-duration:800ms;animation-delay:200ms;animation-timing-function:linear;animation-fill-mode:both;";
              if (!$update2 || $value !== undefined) {
                __SetStyleObject($n110, [6, 58, 55, 56, 69, 70, 71, 72, 73, {
                  2: $data.spuCardImageTagVO.tag_box && _GetLength($data.spuCardImageTagVO.tag_box) > 0 ? "-2px" : "4px"
                }]);
              }
            }
            {
              let _$value26 = $data.spuCardImageTagVO.imageUrl;
              if (!$update2 || _$value26 !== $lepusTemplate._data.spuCardImageTagVO.imageUrl) {
                __SetAttribute($n110, "src", _$value26);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 5);
        $conditionNodeIndex[uniqueId] = 5;
        let _$temp32 = $update2;
        if ($ifNodeIndex !== 5) {
          $update2 = false;
        }
        {
          let $n111 = $update2 ? $lepusGetElementRefByLepusID("image", 111) : null;
          let _$temp33 = $update2;
          if (!$n111) {
            $update2 = false;
            $n111 = __CreateImage($currentComponentId);
            let $nid111 = $lepusStoreElementRefByLepusID($n111, 111, "image");
            __SetAttribute($n111, 1004, $nid111[1]);
            __AppendElement($parent, $n111);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
            {
              let _$value27 = "left:" + (($data.spuCardImageTagVO.tag_box && _GetLength($data.spuCardImageTagVO.tag_box) > 0 ? "-2px" : "4px") + ";") + "height:16px;width:48px;position:absolute;top:4px;left:4px;";
              if (!$update2 || _$value27 !== undefined) {
                __SetStyleObject($n111, [6, 58, 55, 56, 57, {
                  2: $data.spuCardImageTagVO.tag_box && _GetLength($data.spuCardImageTagVO.tag_box) > 0 ? "-2px" : "4px"
                }]);
              }
            }
            {
              let _$value28 = $data.spuCardImageTagVO.imageUrl;
              if (!$update2 || _$value28 !== $lepusTemplate._data.spuCardImageTagVO.imageUrl) {
                __SetAttribute($n111, "src", _$value28);
              }
            }
          }
          $update2 = _$temp33;
        }
        $update2 = _$temp32;
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "ImageTag";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n90 = $lepusGetElementRefByLepusID("view", 90);
      {
        let $value = $data.spuCardImageTagVO.tag_box;
        if (!$update2 || $value !== $lepusTemplate._data.spuCardImageTagVO.tag_box) {
          __SetStyleObject($n90, [3, 44, 55, 0, getCssPropertyIDObj($data.spuCardImageTagVO.tag_box)]);
        }
      }
    }
    let $n91 = $lepusGetElementRefByLepusID("if", 91);
    $renderTemplates[$path].update_3b11d38_91($lepusTemplate, $n91, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n90 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n90, 90, "view");
    __SetAttribute($n90, 1004, 90);
    __SetStyleObject($n90, [3, 44, 55, 0, getCssPropertyIDObj($data.spuCardImageTagVO.tag_box)]);
    __AppendElement($template, $n90);
    let $n91 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n91, 91, "if");
    __AppendElement($n90, $n91);
    $renderTemplates["ImageTag"].update_3b11d38_91($lepusTemplate, $n91, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["VoucherCover"] = {
  variables: ["vo"],
  varUpdateState: [],
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "VoucherCover";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n115 = $lepusGetElementRefByLepusID("text", 115);
      {
        let $value = "font-size:" + ($data.vo.fontSize + "px;") + "font-family:'Arial Black';color:rgba(255, 57, 81, 1);font-weight:900;vertical-align:baseline;line-height:26px;";
        if (!$update2 || $value !== undefined) {
          __SetStyleObject($n115, [80, 81, 82, 83, 84, {
            47: $data.vo.fontSize + "px"
          }]);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let _$value29 = $data.vo.price || "";
      if (_$value29 !== ($lepusTemplate._data.vo.price || "")) {
        let $n116 = $lepusGetElementRefByLepusID("raw-text", 116);
        __SetAttribute($n116, "text", _$value29);
      }
    }
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n112 = __CreateView($currentComponentId);
    __SetAttribute($n112, 1004, 112);
    __SetStyleObject($n112, [74, 1, 0, 45]);
    __AppendElement($template, $n112);
    let $n113 = __CreateImage($currentComponentId);
    __SetAttribute($n113, 1004, 113);
    __SetStyleObject($n113, [55, 75, 76, 0, 1]);
    __SetAttribute($n113, "src", "https://test.com/obj/eden-cn/lm_pvw_lmps/ljhwZthlaukjlkulzlp/poi_shelf/voucher-cover-bg.png");
    __AppendElement($n112, $n113);
    let $n114 = __CreateView($currentComponentId);
    __SetAttribute($n114, 1004, 114);
    __SetStyleObject($n114, [55, 3, 77, 0, 78, 79, 8]);
    __AppendElement($n112, $n114);
    let $n115 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n115, 115, "text");
    __SetAttribute($n115, 1004, 115);
    __SetStyleObject($n115, [80, 81, 82, 83, 84, {
      47: $data.vo.fontSize + "px"
    }]);
    __AppendElement($n114, $n115);
    let $n116 = __CreateRawText($data.vo.price || "");
    $lepusStoreElementRefByLepusID($n116, 116, "raw-text");
    __AppendElement($n115, $n116);
    let $n117 = __CreateImage($currentComponentId);
    __SetAttribute($n117, 1004, 117);
    __SetStyleObject($n117, [85, 86, 83]);
    __SetAttribute($n117, "skip-redirection", true);
    __SetAttribute($n117, "src", "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACQAAAAkCAYAAADhAJiYAAAACXBIWXMAACE4AAAhOAFFljFgAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAJBSURBVHgB7ZdRbtNAEIb/WacSjzmCuYGRGolHcwDacAKSE5TegJ6g6gnIDTAniB8RrRRzAswJMEIVvHiH2XVK4ni3Nq6UppK/Byda745/7/w73gUODNpt4JfTEPviGQpKk2K7adToVOqlXEPsg9+0kOt8u0nhwBgEtXFwgpqmZnUFpcfYB6wzDDx1bKXm42kMKkM8KkFG10lWmVrpGZje4jEhfSHXbKhDbQyC2qhMrdVCVlnq76YuZS06qrd6J/uVn9V/iuRy5hgs1Ziv0AYHtmoTOsCT0x9OQYF6Tp+T3PaxpcPupXaglK4/vUJHOqaM9/NtQwdBPJlG3pt/UKAHHE+9LzhCK2XkyWxBWdJLEG75I09OxtZbQZDepd3QIWXKU8HpAVsHawHzoh9kD/9NPLrkyetZq6DqBMKx5+5X9IbCnVixFdcmaH0CcROoBD3gyPjHtUiqGfcKkmk0ikPnTaJc8p6iD0e+mPzd/DRMbVeAmM6fKtOJLtAXLdsc5VwkzRkSz8S41at7xchA2bcs0BmOqjStUerU3U9ZQXaG1rNyKZ6Z4T4kVfJ2b/B/jHGkV2KBvPIOu+uawkaQiFl6O9YVnW/XjBoj5Ci9A0OJH/rDWk/mlS4D0zlaUXP6knhXVhWQ+hVKRvrvKTbYjawYhtuoJk1QLzr5po/ZTfxgM65md1Mx64amVDrPvWlyaTo+eS8POev2QW7GrwsylbnklbQW0NLxpl+tsYvkF2Q7It7ZPQVrVcjeq7g7ZXQLNrDhLyudzp+D7LarAAAAAElFTkSuQmCC");
    __AppendElement($n115, $n117);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["DeliveryPurchaseInfo"] = {
  variables: ["deliveryPurchaseInfoVO"],
  varUpdateState: [],
  update_1632210_119: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "DeliveryPurchaseInfo";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.deliveryPurchaseInfoVO.showPrice) {
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
              __SetStyleObject($n120, [88, 3, 79, 44]);
              __AppendElement($parent, $n120);
            }
            {
              let $n121 = $update2 ? $lepusGetElementRefByLepusID("text", 121) : null;
              let $temp3 = $update2;
              if (!$n121) {
                $update2 = false;
                $n121 = __CreateText($currentComponentId);
                let $nid121 = $lepusStoreElementRefByLepusID($n121, 121, "text");
                __SetAttribute($n121, 1004, $nid121[1]);
                __SetStyleObject($n121, [17, 89, 21, 30]);
                __AppendElement($n120, $n121);
              }
              __SetAttribute($n121, "text", "¥");
              $update2 = $temp3;
            }
            {
              let $n123 = $update2 ? $lepusGetElementRefByLepusID("text", 123) : null;
              let _$temp34 = $update2;
              if (!$n123) {
                $update2 = false;
                $n123 = __CreateText($currentComponentId);
                let $nid123 = $lepusStoreElementRefByLepusID($n123, 123, "text");
                __SetAttribute($n123, 1004, $nid123[1]);
                __SetStyleObject($n123, [21, 90, 89]);
                __AppendElement($n120, $n123);
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let $value = $data.deliveryPurchaseInfoVO.price || "";
                  if (!$update2 || $value !== ($lepusTemplate._data.deliveryPurchaseInfoVO.price || "")) {
                    __SetAttribute($n123, "text", $value);
                  }
                }
              }
              $update2 = _$temp34;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp35 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n125 = $update2 ? $lepusGetElementRefByLepusID("view", 125) : null;
            let _$temp36 = $update2;
            if (!$n125) {
              $update2 = false;
              $n125 = __CreateView($currentComponentId);
              let $nid125 = $lepusStoreElementRefByLepusID($n125, 125, "view");
              __SetAttribute($n125, 1004, $nid125[1]);
              __SetStyleObject($n125, [0, 3, 91, 4, 92, 93, 44]);
              __AppendElement($parent, $n125);
            }
            {
              let $n126 = $update2 ? $lepusGetElementRefByLepusID("text", 126) : null;
              let _$temp37 = $update2;
              if (!$n126) {
                $update2 = false;
                $n126 = __CreateText($currentComponentId);
                let $nid126 = $lepusStoreElementRefByLepusID($n126, 126, "text");
                __SetAttribute($n126, 1004, $nid126[1]);
                __SetStyleObject($n126, [94, 26, 95]);
                __AppendElement($n125, $n126);
              }
              __SetAttribute($n126, "text", "暂无报价");
              $update2 = _$temp37;
            }
            $update2 = _$temp36;
          }
          $update2 = _$temp35;
        }
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "DeliveryPurchaseInfo";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n119 = $lepusGetElementRefByLepusID("if", 119);
    $renderTemplates[$path].update_1632210_119($lepusTemplate, $n119, $data, $update2);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n128 = $lepusGetElementRefByLepusID("view", 128);
      {
        let $value = "background-color:" + (($data.deliveryPurchaseInfoVO.noStock ? "#FE2C5557" : "#fe2c55") + ";") + "display:flex;align-items:center;justify-content:center;width:64px;height:28px;border-radius:6px;";
        if (!$update2 || $value !== "background-color:" + (($lepusTemplate._data.deliveryPurchaseInfoVO.noStock ? "#FE2C5557" : "#fe2c55") + ";") + "display:flex;align-items:center;justify-content:center;width:64px;height:28px;border-radius:6px;") {
          __SetStyleObject($n128, [3, 4, 8, 96, 97, 98, {
            7: $data.deliveryPurchaseInfoVO.noStock ? "#FE2C5557" : "#fe2c55"
          }]);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n129 = $lepusGetElementRefByLepusID("text", 129);
      {
        let _$value30 = "font-size:" + (($data.deliveryPurchaseInfoVO.showSmallTitle ? "12px" : "13px") + ";") + "color:#ffffff;font-weight:500;";
        if (!$update2 || _$value30 !== "font-size:" + (($lepusTemplate._data.deliveryPurchaseInfoVO.showSmallTitle ? "12px" : "13px") + ";") + "color:#ffffff;font-weight:500;") {
          __SetStyleObject($n129, [99, 21, {
            47: $data.deliveryPurchaseInfoVO.showSmallTitle ? "12px" : "13px"
          }]);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let _$value31 = $data.deliveryPurchaseInfoVO.title;
      if (_$value31 !== $lepusTemplate._data.deliveryPurchaseInfoVO.title) {
        let _$n2 = $lepusGetElementRefByLepusID("text", 129);
        __SetAttribute(_$n2, "text", _$value31);
      }
    }
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n118 = __CreateView($currentComponentId);
    __SetAttribute($n118, 1004, 118);
    __SetStyleObject($n118, [0, 1, 3, 87, 79, 44]);
    __AppendElement($template, $n118);
    let $n119 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n119, 119, "if");
    __AppendElement($n118, $n119);
    $renderTemplates["DeliveryPurchaseInfo"].update_1632210_119($lepusTemplate, $n119, $data, $update2);
    let $n128 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n128, 128, "view");
    __SetAttribute($n128, 1004, 128);
    __SetStyleObject($n128, [3, 4, 8, 96, 97, 98, {
      7: $data.deliveryPurchaseInfoVO.noStock ? "#FE2C5557" : "#fe2c55"
    }]);
    __AddEvent($n128, "catchEvent", "tap", "handleClickPurchase");
    __AppendElement($n118, $n128);
    let $n129 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n129, 129, "text");
    __SetAttribute($n129, 1004, 129);
    __SetStyleObject($n129, [99, 21, {
      47: $data.deliveryPurchaseInfoVO.showSmallTitle ? "12px" : "13px"
    }]);
    __AppendElement($n128, $n129);
    __SetAttribute($n129, "text", $data.deliveryPurchaseInfoVO.title);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["ProcessComp"] = {
  variables: ["marketingInfoVO", "isCouponOverflow", "isSiblingOverflow", "seckill"],
  varUpdateState: [],
  update_1387060_138: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.marketingInfoVO.lowPriceTagVO.hidden) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n139 = $update2 ? $lepusGetElementRefByLepusID("template", 139) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n139) {
            $templateCreated = false;
            $n139 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n139, 139, "template");
            __AppendElement($parent, $n139);
            $templateId = __GetElementUniqueID($n139);
            $childLepusTemplate = $templateConstructor($templateId, $n139);
          } else {
            $templateId = __GetElementUniqueID($n139);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["LowPriceTag"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["LowPriceTag"].entry($n139, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1387060_144: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.marketingInfoVO.discountVO.hidden) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n145 = $update2 ? $lepusGetElementRefByLepusID("view", 145) : null;
          let $temp2 = $update2;
          if (!$n145) {
            $update2 = false;
            $n145 = __CreateView($currentComponentId);
            let $nid145 = $lepusStoreElementRefByLepusID($n145, 145, "view");
            __SetAttribute($n145, 1004, $nid145[1]);
            __SetID($n145, "sec-kill-layout_tag-discount-tag");
            __AppendElement($parent, $n145);
          }
          {
            let $n146 = $update2 ? $lepusGetElementRefByLepusID("template", 146) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n146) {
              $templateCreated = false;
              $n146 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n146, 146, "template");
              __AppendElement($n145, $n146);
              $templateId = __GetElementUniqueID($n146);
              $childLepusTemplate = $templateConstructor($templateId, $n146);
            } else {
              $templateId = __GetElementUniqueID($n146);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["Discount"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["Discount"].entry($n146, $childLepusTemplate, $childLepusTemplate.data, false);
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
  update_1387060_190: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.marketingInfoVO.lowPriceTagVO.hidden) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n191 = $update2 ? $lepusGetElementRefByLepusID("template", 191) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n191) {
            $templateCreated = false;
            $n191 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n191, 191, "template");
            __AppendElement($parent, $n191);
            $templateId = __GetElementUniqueID($n191);
            $childLepusTemplate = $templateConstructor($templateId, $n191);
          } else {
            $templateId = __GetElementUniqueID($n191);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["LowPriceTag"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["LowPriceTag"].entry($n191, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1387060_194: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.marketingInfoVO.discountVO.hidden) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n195 = $update2 ? $lepusGetElementRefByLepusID("template", 195) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n195) {
            $templateCreated = false;
            $n195 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n195, 195, "template");
            __AppendElement($parent, $n195);
            $templateId = __GetElementUniqueID($n195);
            $childLepusTemplate = $templateConstructor($templateId, $n195);
          } else {
            $templateId = __GetElementUniqueID($n195);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Discount"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Discount"].entry($n195, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_284ed90_141: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.marketingInfoVO.couponVO.hidden) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n142 = $update2 ? $lepusGetElementRefByLepusID("view", 142) : null;
          let $temp2 = $update2;
          if (!$n142) {
            $update2 = false;
            $n142 = __CreateView($currentComponentId);
            let $nid142 = $lepusStoreElementRefByLepusID($n142, 142, "view");
            __SetAttribute($n142, 1004, $nid142[1]);
            __SetStyleObject($n142, [31]);
            __SetID($n142, "sec-kill-layout_tag-coupon-tag");
            __AppendElement($parent, $n142);
          }
          {
            let $n143 = $update2 ? $lepusGetElementRefByLepusID("template", 143) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n143) {
              $templateCreated = false;
              $n143 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n143, 143, "template");
              __AppendElement($n142, $n143);
              $templateId = __GetElementUniqueID($n143);
              $childLepusTemplate = $templateConstructor($templateId, $n143);
            } else {
              $templateId = __GetElementUniqueID($n143);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
            $childLepusTemplate.setData("isCouponOverflow", $data.isCouponOverflow, $update2);
            $childLepusTemplate.setData("isSiblingOverflow", $data.isSiblingOverflow, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["Coupon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["Coupon"].entry($n143, $childLepusTemplate, $childLepusTemplate.data, false);
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
  update_393d908_150: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_c = (_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.secKillInfo) == null ? undefined : _c.status) == 1 && $data.seckill.curSecKillStatus != 2) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n151 = $update2 ? $lepusGetElementRefByLepusID("view", 151) : null;
          let $temp2 = $update2;
          if (!$n151) {
            $update2 = false;
            $n151 = __CreateView($currentComponentId);
            let $nid151 = $lepusStoreElementRefByLepusID($n151, 151, "view");
            __SetAttribute($n151, 1004, $nid151[1]);
            __AppendElement($parent, $n151);
          }
          {
            let $n152 = $update2 ? $lepusGetElementRefByLepusID("template", 152) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n152) {
              $templateCreated = false;
              $n152 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n152, 152, "template");
              __AppendElement($n151, $n152);
              $templateId = __GetElementUniqueID($n152);
              $childLepusTemplate = $templateConstructor($templateId, $n152);
            } else {
              $templateId = __GetElementUniqueID($n152);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["SecKillPre"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["SecKillPre"].entry($n152, $childLepusTemplate, $childLepusTemplate.data, false);
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp38 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n153 = $update2 ? $lepusGetElementRefByLepusID("if", 153) : null;
          if (!$n153) {
            $update2 = false;
            $n153 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n153, 153, "if");
            __AppendElement($parent, $n153);
          }
          $renderTemplates["ProcessComp"].update_393d908_153($lepusTemplate, $n153, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp38;
      }
    }
  },
  update_393d908_153: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "ProcessComp";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.showCountdownInProgressBar) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n154 = $update2 ? $lepusGetElementRefByLepusID("view", 154) : null;
          let $temp2 = $update2;
          if (!$n154) {
            $update2 = false;
            $n154 = __CreateView($currentComponentId);
            let $nid154 = $lepusStoreElementRefByLepusID($n154, 154, "view");
            __SetAttribute($n154, 1004, $nid154[1]);
            __AppendElement($parent, $n154);
          }
          {
            let $n155 = $update2 ? $lepusGetElementRefByLepusID("view", 155) : null;
            let $temp3 = $update2;
            if (!$n155) {
              $update2 = false;
              $n155 = __CreateView($currentComponentId);
              let $nid155 = $lepusStoreElementRefByLepusID($n155, 155, "view");
              __SetAttribute($n155, 1004, $nid155[1]);
              __SetStyleObject($n155, [31, 3, 87, 4, 52, 74, 0, 107, 108, 109, 110, 111, 112, 113]);
              __AppendElement($n154, $n155);
            }
            {
              let $n156 = $update2 ? $lepusGetElementRefByLepusID("view", 156) : null;
              let $temp4 = $update2;
              if (!$n156) {
                $update2 = false;
                $n156 = __CreateView($currentComponentId);
                let $nid156 = $lepusStoreElementRefByLepusID($n156, 156, "view");
                __SetAttribute($n156, 1004, $nid156[1]);
                __SetStyleObject($n156, [0, 55, 1, 76, 75, 45]);
                __AppendElement($n155, $n156);
              }
              {
                let $n157 = $update2 ? $lepusGetElementRefByLepusID("view", 157) : null;
                let $temp5 = $update2;
                if (!$n157) {
                  $update2 = false;
                  $n157 = __CreateView($currentComponentId);
                  let $nid157 = $lepusStoreElementRefByLepusID($n157, 157, "view");
                  __SetAttribute($n157, 1004, $nid157[1]);
                  __SetStyleObject($n157, [0, 1, 108, 114, 115, 111, 45]);
                  __AppendElement($n156, $n157);
                }
                {
                  let $n158 = $update2 ? $lepusGetElementRefByLepusID("image", 158) : null;
                  let $temp6 = $update2;
                  if (!$n158) {
                    $update2 = false;
                    $n158 = __CreateImage($currentComponentId);
                    let $nid158 = $lepusStoreElementRefByLepusID($n158, 158, "image");
                    __SetAttribute($n158, 1004, $nid158[1]);
                    __SetStyleObject($n158, [1, 0]);
                    __SetAttribute($n158, "cap-insets", "0px 15px 0px 15px");
                    __SetAttribute($n158, "cap-insets-scale", "3");
                    __AppendElement($n157, $n158);
                  }
                  if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                    {
                      let $value = (_d = (_c = $data.marketingInfoVO) == null ? undefined : _c.secKillProgressVO) == null ? undefined : _d.progressBarBgImage;
                      if (!$update2 || $value !== ((_f = (_e = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _e.secKillProgressVO) == null ? undefined : _f.progressBarBgImage)) {
                        __SetAttribute($n158, "src", $value);
                      }
                    }
                  }
                  $update2 = $temp6;
                }
                $update2 = $temp5;
              }
              $update2 = $temp4;
            }
            {
              let $n159 = $update2 ? $lepusGetElementRefByLepusID("view", 159) : null;
              let _$temp39 = $update2;
              if (!$n159) {
                $update2 = false;
                $n159 = __CreateView($currentComponentId);
                let $nid159 = $lepusStoreElementRefByLepusID($n159, 159, "view");
                __SetAttribute($n159, 1004, $nid159[1]);
                __SetStyleObject($n159, [3, 1, 4, 31, 101]);
                __AppendElement($n155, $n159);
              }
              {
                let $template_update = $update2;
                let $n160 = $update2 ? $lepusGetElementRefByLepusID("if", 160) : null;
                if (!$n160) {
                  $update2 = false;
                  $n160 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n160, 160, "if");
                  __AppendElement($n159, $n160);
                }
                $renderTemplates[$path].update_393d908_160($lepusTemplate, $n160, $data, $update2);
                $update2 = $template_update;
              }
              $update2 = _$temp39;
            }
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 3);
        $conditionNodeIndex[uniqueId] = 3;
        let _$temp40 = $update2;
        if ($ifNodeIndex !== 3) {
          $update2 = false;
        }
        {
          let _$template_update2 = $update2;
          let $n174 = $update2 ? $lepusGetElementRefByLepusID("if", 174) : null;
          if (!$n174) {
            $update2 = false;
            $n174 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n174, 174, "if");
            __AppendElement($parent, $n174);
          }
          $renderTemplates[$path].update_393d908_174($lepusTemplate, $n174, $data, $update2);
          $update2 = _$template_update2;
        }
        $update2 = _$temp40;
      }
    }
  },
  update_393d908_160: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_c = (_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.subText) == null ? undefined : _c.length) > 0) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n161 = $update2 ? $lepusGetElementRefByLepusID("if", 161) : null;
          if (!$n161) {
            $update2 = false;
            $n161 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n161, 161, "if");
            __AppendElement($parent, $n161);
          }
          $renderTemplates["ProcessComp"].update_393d908_161($lepusTemplate, $n161, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_393d908_161: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
    let $path = "ProcessComp";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!((_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.secKillInfo)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n162 = $update2 ? $lepusGetElementRefByLepusID("text", 162) : null;
          let $temp2 = $update2;
          if (!$n162) {
            $update2 = false;
            $n162 = __CreateText($currentComponentId);
            let $nid162 = $lepusStoreElementRefByLepusID($n162, 162, "text");
            __SetAttribute($n162, 1004, $nid162[1]);
            __AppendElement($parent, $n162);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = (_d = (_c = $data.marketingInfoVO) == null ? undefined : _c.secKillProgressVO) == null ? undefined : _d.subTextStyle;
              if (!$update2 || $value !== ((_f = (_e = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _e.secKillProgressVO) == null ? undefined : _f.subTextStyle)) {
                __SetStyleObject($n162, [63, 116, 18, 117, 118, getCssPropertyIDObj((_h = (_g = $data.marketingInfoVO) == null ? undefined : _g.secKillProgressVO) == null ? undefined : _h.subTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value32 = (_j = (_i = $data.marketingInfoVO) == null ? undefined : _i.secKillProgressVO) == null ? undefined : _j.subText;
              if (!$update2 || _$value32 !== ((_l = (_k = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _k.secKillProgressVO) == null ? undefined : _l.subText)) {
                __SetAttribute($n162, "text", _$value32);
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
          let $n164 = $update2 ? $lepusGetElementRefByLepusID("if", 164) : null;
          if (!$n164) {
            $update2 = false;
            $n164 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n164, 164, "if");
            __AppendElement($parent, $n164);
          }
          $renderTemplates[$path].update_393d908_164($lepusTemplate, $n164, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp41;
      }
    }
  },
  update_393d908_164: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s, _t, _u, _v, _w, _x, _y, _z, _A;
    let $path = "ProcessComp";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_c = (_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.secKillInfo) == null ? undefined : _c.status) == 1 && $data.seckill.curSecKillStatus != 2) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n165 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 165) : null;
          let $temp2 = $update2;
          if (!$n165) {
            $update2 = false;
            $n165 = __CreateElement("countdown-view", $currentComponentId);
            let $nid165 = $lepusStoreElementRefByLepusID($n165, 165, "countdown-view");
            __SetAttribute($n165, 1004, $nid165[1]);
            __SetAttribute($n165, "unit", "seconds");
            __SetID($n165, "countdown-pre");
            __AddEvent($n165, "bindEvent", "countdownend", "onPreCountDownEnd");
            __AppendElement($parent, $n165);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = (_e = (_d = $data.marketingInfoVO) == null ? undefined : _d.secKillProgressVO) == null ? undefined : _e.subTextStyle;
              if (!$update2 || $value !== ((_g = (_f = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _f.secKillProgressVO) == null ? undefined : _g.subTextStyle)) {
                __SetStyleObject($n165, [4, 8, 101, 119, 120, getCssPropertyIDObj((_i = (_h = $data.marketingInfoVO) == null ? undefined : _h.secKillProgressVO) == null ? undefined : _i.subTextStyle)]);
              }
            }
            {
              let _$value33 = "" + ((_l = (_k = (_j = $data.marketingInfoVO) == null ? undefined : _j.secKillProgressVO) == null ? undefined : _k.secKillInfo) == null ? undefined : _l.start_time);
              if (!$update2 || _$value33 !== "" + ((_o = (_n = (_m = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _m.secKillProgressVO) == null ? undefined : _n.secKillInfo) == null ? undefined : _o.start_time)) {
                __SetAttribute($n165, "end-time", _$value33);
              }
            }
          }
          {
            let $n166 = $update2 ? $lepusGetElementRefByLepusID("text", 166) : null;
            let $temp3 = $update2;
            if (!$n166) {
              $update2 = false;
              $n166 = __CreateText($currentComponentId);
              let $nid166 = $lepusStoreElementRefByLepusID($n166, 166, "text");
              __SetAttribute($n166, 1004, $nid166[1]);
              __AppendElement($n165, $n166);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value34 = (_q = (_p = $data.marketingInfoVO) == null ? undefined : _p.secKillProgressVO) == null ? undefined : _q.subTextStyle;
                if (!$update2 || _$value34 !== ((_s = (_r = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _r.secKillProgressVO) == null ? undefined : _s.subTextStyle)) {
                  __SetStyleObject($n166, [63, 116, 18, 117, 118, getCssPropertyIDObj((_u = (_t = $data.marketingInfoVO) == null ? undefined : _t.secKillProgressVO) == null ? undefined : _u.subTextStyle)]);
                }
              }
            }
            __SetAttribute($n166, "text", "剩 ");
            $update2 = $temp3;
          }
          {
            let $n168 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 168) : null;
            let _$temp42 = $update2;
            if (!$n168) {
              $update2 = false;
              $n168 = __CreateElement("countdown-item", $currentComponentId);
              let $nid168 = $lepusStoreElementRefByLepusID($n168, 168, "countdown-item");
              __SetAttribute($n168, 1004, $nid168[1]);
              __SetAttribute($n168, "text-maxline", "1");
              __SetAttribute($n168, "countdown-display", "HH:mm:ss");
              __AppendElement($n165, $n168);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value35 = (_w = (_v = $data.marketingInfoVO) == null ? undefined : _v.secKillProgressVO) == null ? undefined : _w.subTextStyle;
                if (!$update2 || _$value35 !== ((_y = (_x = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _x.secKillProgressVO) == null ? undefined : _y.subTextStyle)) {
                  __SetStyleObject($n168, [63, 116, 18, 117, 118, getCssPropertyIDObj((_A = (_z = $data.marketingInfoVO) == null ? undefined : _z.secKillProgressVO) == null ? undefined : _A.subTextStyle)]);
                }
              }
            }
            $update2 = _$temp42;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp43 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n169 = $update2 ? $lepusGetElementRefByLepusID("if", 169) : null;
          if (!$n169) {
            $update2 = false;
            $n169 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n169, 169, "if");
            __AppendElement($parent, $n169);
          }
          $renderTemplates[$path].update_393d908_169($lepusTemplate, $n169, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp43;
      }
    }
  },
  update_393d908_169: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, _r, _s, _t, _u, _v, _w, _x, _y, _z, _A;
    let $path = "ProcessComp";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_c = (_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.secKillInfo) == null ? undefined : _c.status) == 2 || $data.seckill.curSecKillStatus == 2) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n170 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 170) : null;
          let $temp2 = $update2;
          if (!$n170) {
            $update2 = false;
            $n170 = __CreateElement("countdown-view", $currentComponentId);
            let $nid170 = $lepusStoreElementRefByLepusID($n170, 170, "countdown-view");
            __SetAttribute($n170, 1004, $nid170[1]);
            __SetAttribute($n170, "unit", "seconds");
            __SetID($n170, "countdown-seckill");
            __AddEvent($n170, "bindEvent", "countdownend", "onSecKillCountDownEnd");
            __AppendElement($parent, $n170);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = (_e = (_d = $data.marketingInfoVO) == null ? undefined : _d.secKillProgressVO) == null ? undefined : _e.subTextStyle;
              if (!$update2 || $value !== ((_g = (_f = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _f.secKillProgressVO) == null ? undefined : _g.subTextStyle)) {
                __SetStyleObject($n170, [4, 8, 101, 119, 120, getCssPropertyIDObj((_i = (_h = $data.marketingInfoVO) == null ? undefined : _h.secKillProgressVO) == null ? undefined : _i.subTextStyle)]);
              }
            }
            {
              let _$value36 = "" + ((_l = (_k = (_j = $data.marketingInfoVO) == null ? undefined : _j.secKillProgressVO) == null ? undefined : _k.secKillInfo) == null ? undefined : _l.end_time);
              if (!$update2 || _$value36 !== "" + ((_o = (_n = (_m = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _m.secKillProgressVO) == null ? undefined : _n.secKillInfo) == null ? undefined : _o.end_time)) {
                __SetAttribute($n170, "end-time", _$value36);
              }
            }
          }
          {
            let $n171 = $update2 ? $lepusGetElementRefByLepusID("text", 171) : null;
            let $temp3 = $update2;
            if (!$n171) {
              $update2 = false;
              $n171 = __CreateText($currentComponentId);
              let $nid171 = $lepusStoreElementRefByLepusID($n171, 171, "text");
              __SetAttribute($n171, 1004, $nid171[1]);
              __AppendElement($n170, $n171);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value37 = (_q = (_p = $data.marketingInfoVO) == null ? undefined : _p.secKillProgressVO) == null ? undefined : _q.subTextStyle;
                if (!$update2 || _$value37 !== ((_s = (_r = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _r.secKillProgressVO) == null ? undefined : _s.subTextStyle)) {
                  __SetStyleObject($n171, [63, 116, 18, 117, 118, getCssPropertyIDObj((_u = (_t = $data.marketingInfoVO) == null ? undefined : _t.secKillProgressVO) == null ? undefined : _u.subTextStyle)]);
                }
              }
            }
            __SetAttribute($n171, "text", "剩 ");
            $update2 = $temp3;
          }
          {
            let $n173 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 173) : null;
            let _$temp44 = $update2;
            if (!$n173) {
              $update2 = false;
              $n173 = __CreateElement("countdown-item", $currentComponentId);
              let $nid173 = $lepusStoreElementRefByLepusID($n173, 173, "countdown-item");
              __SetAttribute($n173, 1004, $nid173[1]);
              __SetAttribute($n173, "text-maxline", "1");
              __SetAttribute($n173, "countdown-display", "HH:mm:ss");
              __AppendElement($n170, $n173);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value38 = (_w = (_v = $data.marketingInfoVO) == null ? undefined : _v.secKillProgressVO) == null ? undefined : _w.subTextStyle;
                if (!$update2 || _$value38 !== ((_y = (_x = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _x.secKillProgressVO) == null ? undefined : _y.subTextStyle)) {
                  __SetStyleObject($n173, [63, 116, 18, 117, 118, getCssPropertyIDObj((_A = (_z = $data.marketingInfoVO) == null ? undefined : _z.secKillProgressVO) == null ? undefined : _A.subTextStyle)]);
                }
              }
            }
            $update2 = _$temp44;
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
  update_393d908_174: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h;
    let $path = "ProcessComp";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.isOldModule) {
        __UpdateIfNodeIndex($parent, 4);
        $conditionNodeIndex[uniqueId] = 4;
        let $temp = $update2;
        if ($ifNodeIndex !== 4) {
          $update2 = false;
        }
        {
          let $n175 = $update2 ? $lepusGetElementRefByLepusID("view", 175) : null;
          let $temp2 = $update2;
          if (!$n175) {
            $update2 = false;
            $n175 = __CreateView($currentComponentId);
            let $nid175 = $lepusStoreElementRefByLepusID($n175, 175, "view");
            __SetAttribute($n175, 1004, $nid175[1]);
            __AppendElement($parent, $n175);
          }
          {
            let $n176 = $update2 ? $lepusGetElementRefByLepusID("view", 176) : null;
            let $temp3 = $update2;
            if (!$n176) {
              $update2 = false;
              $n176 = __CreateView($currentComponentId);
              let $nid176 = $lepusStoreElementRefByLepusID($n176, 176, "view");
              __SetAttribute($n176, 1004, $nid176[1]);
              __SetStyleObject($n176, [31, 3, 87, 4, 52, 74, 0, 107, 108, 109, 110, 111, 112, 113]);
              __AppendElement($n175, $n176);
            }
            {
              let $template_update = $update2;
              let $n177 = $update2 ? $lepusGetElementRefByLepusID("if", 177) : null;
              if (!$n177) {
                $update2 = false;
                $n177 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n177, 177, "if");
                __AppendElement($n176, $n177);
              }
              $renderTemplates[$path].update_393d908_177($lepusTemplate, $n177, $data, $update2);
              $update2 = $template_update;
            }
            {
              let $n181 = $update2 ? $lepusGetElementRefByLepusID("view", 181) : null;
              let $temp4 = $update2;
              if (!$n181) {
                $update2 = false;
                $n181 = __CreateView($currentComponentId);
                let $nid181 = $lepusStoreElementRefByLepusID($n181, 181, "view");
                __SetAttribute($n181, 1004, $nid181[1]);
                __SetStyleObject($n181, [3, 1, 4, 31, 101]);
                __AppendElement($n176, $n181);
              }
              {
                let $n182 = $update2 ? $lepusGetElementRefByLepusID("text", 182) : null;
                let $temp5 = $update2;
                if (!$n182) {
                  $update2 = false;
                  $n182 = __CreateText($currentComponentId);
                  let $nid182 = $lepusStoreElementRefByLepusID($n182, 182, "text");
                  __SetAttribute($n182, 1004, $nid182[1]);
                  __SetStyleObject($n182, [121, 122, 123, 116, 21, 31]);
                  __AppendElement($n181, $n182);
                }
                {
                  if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                    let $value = (_e = (_d = (_c = $data.marketingInfoVO) == null ? undefined : _c.secKillProgressVO) == null ? undefined : _d.secKillInfo) == null ? undefined : _e.progress_bar_text;
                    if (!$update2 || $value !== ((_h = (_g = (_f = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _f.secKillProgressVO) == null ? undefined : _g.secKillInfo) == null ? undefined : _h.progress_bar_text)) {
                      __SetAttribute($n182, "text", $value);
                    }
                  }
                }
                $update2 = $temp5;
              }
              $update2 = $temp4;
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
  },
  update_393d908_177: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q;
    let $path = "ProcessComp";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.seckill.curSecKillStatus == 2 || ((_c = (_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.secKillInfo) == null ? undefined : _c.status) == 2) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n178 = $update2 ? $lepusGetElementRefByLepusID("view", 178) : null;
          let $temp2 = $update2;
          if (!$n178) {
            $update2 = false;
            $n178 = __CreateView($currentComponentId);
            let $nid178 = $lepusStoreElementRefByLepusID($n178, 178, "view");
            __SetAttribute($n178, 1004, $nid178[1]);
            __AppendElement($parent, $n178);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = "width:" + (((_e = (_d = $data.marketingInfoVO) == null ? undefined : _d.secKillProgressVO) == null ? undefined : _e.soldPercent) + "%;") + ("min-width:" + ((((_h = (_g = (_f = $data.marketingInfoVO) == null ? undefined : _f.secKillProgressVO) == null ? undefined : _g.secKillInfo) == null ? undefined : _h.show_progress_bar) ? "48px" : undefined) + ";")) + "position:absolute;height:100%;left:0;top:0;overflow:hidden;";
              if (!$update2 || $value !== undefined) {
                __SetStyleObject($n178, [55, 1, 76, 75, 45, {
                  27: ((_j = (_i = $data.marketingInfoVO) == null ? undefined : _i.secKillProgressVO) == null ? undefined : _j.soldPercent) + "%"
                }, {
                  29: (((_m = (_l = (_k = $data.marketingInfoVO) == null ? undefined : _k.secKillProgressVO) == null ? undefined : _l.secKillInfo) == null ? undefined : _m.show_progress_bar) ? "48px" : undefined) + ""
                }]);
              }
            }
          }
          {
            let $n179 = $update2 ? $lepusGetElementRefByLepusID("view", 179) : null;
            let $temp3 = $update2;
            if (!$n179) {
              $update2 = false;
              $n179 = __CreateView($currentComponentId);
              let $nid179 = $lepusStoreElementRefByLepusID($n179, 179, "view");
              __SetAttribute($n179, 1004, $nid179[1]);
              __SetStyleObject($n179, [0, 1, 108, 114, 115, 111, 45]);
              __AppendElement($n178, $n179);
            }
            {
              let $n180 = $update2 ? $lepusGetElementRefByLepusID("image", 180) : null;
              let $temp4 = $update2;
              if (!$n180) {
                $update2 = false;
                $n180 = __CreateImage($currentComponentId);
                let $nid180 = $lepusStoreElementRefByLepusID($n180, 180, "image");
                __SetAttribute($n180, 1004, $nid180[1]);
                __SetStyleObject($n180, [1, 0]);
                __SetAttribute($n180, "cap-insets", "0px 15px 0px 15px");
                __SetAttribute($n180, "cap-insets-scale", "3");
                __AppendElement($n179, $n180);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value39 = (_o = (_n = $data.marketingInfoVO) == null ? undefined : _n.secKillProgressVO) == null ? undefined : _o.progressBarBgImage;
                  if (!$update2 || _$value39 !== ((_q = (_p = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _p.secKillProgressVO) == null ? undefined : _q.progressBarBgImage)) {
                    __SetAttribute($n180, "src", _$value39);
                  }
                }
              }
              $update2 = $temp4;
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
  },
  update_393d908_197: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ProcessComp";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingInfoVO.isShowDualBtn) {
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
            let $nid198 = $lepusStoreElementRefByLepusID($n198, 198, "view");
            __SetAttribute($n198, 1004, $nid198[1]);
            __SetStyleObject($n198, [3, 4, 44]);
            __AppendElement($parent, $n198);
          }
          {
            let $n199 = $update2 ? $lepusGetElementRefByLepusID("view", 199) : null;
            let $temp3 = $update2;
            if (!$n199) {
              $update2 = false;
              $n199 = __CreateView($currentComponentId);
              let $nid199 = $lepusStoreElementRefByLepusID($n199, 199, "view");
              __SetAttribute($n199, 1004, $nid199[1]);
              __SetStyleObject($n199, [3, 4, 97, 128, 129, 130, 131, 132]);
              __AddEvent($n199, "catchEvent", "tap", "handleLeftBtnClick");
              __AppendElement($n198, $n199);
            }
            {
              let $n200 = $update2 ? $lepusGetElementRefByLepusID("text", 200) : null;
              let $temp4 = $update2;
              if (!$n200) {
                $update2 = false;
                $n200 = __CreateText($currentComponentId);
                let $nid200 = $lepusStoreElementRefByLepusID($n200, 200, "text");
                __SetAttribute($n200, 1004, $nid200[1]);
                __SetStyleObject($n200, [118, 133, 134, 21, 121]);
                __AppendElement($n199, $n200);
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let $value = $data.marketingInfoVO.dualButtonVO.leftBtnText;
                  if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.dualButtonVO.leftBtnText) {
                    __SetAttribute($n200, "text", $value);
                  }
                }
              }
              $update2 = $temp4;
            }
            $update2 = $temp3;
          }
          {
            let $n202 = $update2 ? $lepusGetElementRefByLepusID("view", 202) : null;
            let _$temp45 = $update2;
            if (!$n202) {
              $update2 = false;
              $n202 = __CreateView($currentComponentId);
              let $nid202 = $lepusStoreElementRefByLepusID($n202, 202, "view");
              __SetAttribute($n202, 1004, $nid202[1]);
              __SetStyleObject($n202, [3, 4, 97, 128, 135, 136, 137]);
              __AddEvent($n202, "catchEvent", "tap", "handleClickRightBtn");
              __AppendElement($n198, $n202);
            }
            {
              let $n203 = $update2 ? $lepusGetElementRefByLepusID("text", 203) : null;
              let _$temp46 = $update2;
              if (!$n203) {
                $update2 = false;
                $n203 = __CreateText($currentComponentId);
                let $nid203 = $lepusStoreElementRefByLepusID($n203, 203, "text");
                __SetAttribute($n203, 1004, $nid203[1]);
                __SetStyleObject($n203, [118, 133, 134, 21, 138]);
                __AppendElement($n202, $n203);
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let _$value40 = $data.marketingInfoVO.dualButtonVO.rightBtnText;
                  if (!$update2 || _$value40 !== $lepusTemplate._data.marketingInfoVO.dualButtonVO.rightBtnText) {
                    __SetAttribute($n203, "text", _$value40);
                  }
                }
              }
              $update2 = _$temp46;
            }
            $update2 = _$temp45;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp47 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n205 = $update2 ? $lepusGetElementRefByLepusID("template", 205) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n205) {
            $templateCreated = false;
            $n205 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n205, 205, "template");
            __AppendElement($parent, $n205);
            $templateId = __GetElementUniqueID($n205);
            $childLepusTemplate = $templateConstructor($templateId, $n205);
          } else {
            $templateId = __GetElementUniqueID($n205);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
          $childLepusTemplate.setData("seckill", $data.seckill, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Button"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Button"].entry($n205, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = _$temp47;
      }
    }
  },
  update_201d6f8_147: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b;
    let $path = "ProcessComp";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!((_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.hidden) && !$data.isSiblingOverflow) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n148 = $update2 ? $lepusGetElementRefByLepusID("view", 148) : null;
          let $temp2 = $update2;
          if (!$n148) {
            $update2 = false;
            $n148 = __CreateView($currentComponentId);
            let $nid148 = $lepusStoreElementRefByLepusID($n148, 148, "view");
            __SetAttribute($n148, 1004, $nid148[1]);
            __SetID($n148, "sec-kill_tag-stock-progress");
            __AppendElement($parent, $n148);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.marketingInfoVO.layoutVO.secKillStyle.progressStyle;
              if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.layoutVO.secKillStyle.progressStyle) {
                __SetStyleObject($n148, [31, 102, getCssPropertyIDObj($data.marketingInfoVO.layoutVO.secKillStyle.progressStyle)]);
              }
            }
          }
          {
            let $n149 = $update2 ? $lepusGetElementRefByLepusID("view", 149) : null;
            let $temp3 = $update2;
            if (!$n149) {
              $update2 = false;
              $n149 = __CreateView($currentComponentId);
              let $nid149 = $lepusStoreElementRefByLepusID($n149, 149, "view");
              __SetAttribute($n149, 1004, $nid149[1]);
              __AppendElement($n148, $n149);
            }
            {
              let $template_update = $update2;
              let $n150 = $update2 ? $lepusGetElementRefByLepusID("if", 150) : null;
              if (!$n150) {
                $update2 = false;
                $n150 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n150, 150, "if");
                __AppendElement($n149, $n150);
              }
              $renderTemplates[$path].update_393d908_150($lepusTemplate, $n150, $data, $update2);
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
  },
  update_1bd09b0_132: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ProcessComp";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[3]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.sceneType == "seckill_layer") {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n133 = $update2 ? $lepusGetElementRefByLepusID("view", 133) : null;
            let $temp2 = $update2;
            if (!$n133) {
              $update2 = false;
              $n133 = __CreateView($currentComponentId);
              let $nid133 = $lepusStoreElementRefByLepusID($n133, 133, "view");
              __SetAttribute($n133, 1004, $nid133[1]);
              __AppendElement($parent, $n133);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.layoutVO.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.layoutVO.containerStyle) {
                  __SetStyleObject($n133, [0, 3, 44, 79, 62, getCssPropertyIDObj($data.marketingInfoVO.layoutVO.containerStyle)]);
                }
              }
            }
            {
              let $n134 = $update2 ? $lepusGetElementRefByLepusID("view", 134) : null;
              let $temp3 = $update2;
              if (!$n134) {
                $update2 = false;
                $n134 = __CreateView($currentComponentId);
                let $nid134 = $lepusStoreElementRefByLepusID($n134, 134, "view");
                __SetAttribute($n134, 1004, $nid134[1]);
                __AppendElement($n133, $n134);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value41 = $data.marketingInfoVO.layoutVO.secKillStyle.infoRightStyle;
                  if (!$update2 || _$value41 !== $lepusTemplate._data.marketingInfoVO.layoutVO.secKillStyle.infoRightStyle) {
                    __SetStyleObject($n134, [102, 3, 100, 103, getCssPropertyIDObj($data.marketingInfoVO.layoutVO.secKillStyle.infoRightStyle)]);
                  }
                }
              }
              {
                let $n135 = $update2 ? $lepusGetElementRefByLepusID("view", 135) : null;
                let $temp4 = $update2;
                if (!$n135) {
                  $update2 = false;
                  $n135 = __CreateView($currentComponentId);
                  let $nid135 = $lepusStoreElementRefByLepusID($n135, 135, "view");
                  __SetAttribute($n135, 1004, $nid135[1]);
                  __SetStyleObject($n135, [3, 44, 104, 45, 103, 66]);
                  __AppendElement($n134, $n135);
                }
                {
                  let $n136 = $update2 ? $lepusGetElementRefByLepusID("template", 136) : null;
                  let $templateCreated = true;
                  let $childLepusTemplate = null;
                  let $templateId = null;
                  if (!$n136) {
                    $templateCreated = false;
                    $n136 = __CreateWrapperElement($currentComponentId);
                    $lepusStoreElementRefByLepusID($n136, 136, "template");
                    __AppendElement($n135, $n136);
                    $templateId = __GetElementUniqueID($n136);
                    $childLepusTemplate = $templateConstructor($templateId, $n136);
                  } else {
                    $templateId = __GetElementUniqueID($n136);
                    $childLepusTemplate = $templateInfo[$templateId];
                  }
                  $updatePropsSet = [];
                  $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
                  if ($templateCreated) {
                    let $update_keys = $updatePropsSet;
                    if ($update_keys.length > 0) {
                      $renderTemplates["Price"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
                    }
                  } else {
                    $renderTemplates["Price"].entry($n136, $childLepusTemplate, $childLepusTemplate.data, false);
                  }
                }
                {
                  let $n137 = $update2 ? $lepusGetElementRefByLepusID("view", 137) : null;
                  let $temp5 = $update2;
                  if (!$n137) {
                    $update2 = false;
                    $n137 = __CreateView($currentComponentId);
                    let $nid137 = $lepusStoreElementRefByLepusID($n137, 137, "view");
                    __SetAttribute($n137, 1004, $nid137[1]);
                    __SetStyleObject($n137, [31, 3, 104, 105]);
                    __AppendElement($n135, $n137);
                  }
                  {
                    let $template_update = $update2;
                    let $n138 = $update2 ? $lepusGetElementRefByLepusID("if", 138) : null;
                    if (!$n138) {
                      $update2 = false;
                      $n138 = __CreateIf($currentComponentId);
                      $lepusStoreElementRefByLepusID($n138, 138, "if");
                      __AppendElement($n137, $n138);
                    }
                    $renderTemplates[$path].update_1387060_138($lepusTemplate, $n138, $data, $update2);
                    $update2 = $template_update;
                  }
                  $update2 = $temp5;
                }
                $update2 = $temp4;
              }
              {
                let $n140 = $update2 ? $lepusGetElementRefByLepusID("view", 140) : null;
                let _$temp48 = $update2;
                if (!$n140) {
                  $update2 = false;
                  $n140 = __CreateView($currentComponentId);
                  let $nid140 = $lepusStoreElementRefByLepusID($n140, 140, "view");
                  __SetAttribute($n140, 1004, $nid140[1]);
                  __AppendElement($n134, $n140);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value42 = $data.marketingInfoVO.layoutVO.secKillStyle.tagRightStyle;
                    if (!$update2 || _$value42 !== $lepusTemplate._data.marketingInfoVO.layoutVO.secKillStyle.tagRightStyle) {
                      __SetStyleObject($n140, [3, 44, 4, 45, 103, 106, 0, getCssPropertyIDObj($data.marketingInfoVO.layoutVO.secKillStyle.tagRightStyle)]);
                    }
                  }
                }
                {
                  let _$template_update3 = $update2;
                  let $n141 = $update2 ? $lepusGetElementRefByLepusID("if", 141) : null;
                  if (!$n141) {
                    $update2 = false;
                    $n141 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n141, 141, "if");
                    __AppendElement($n140, $n141);
                  }
                  $renderTemplates[$path].update_284ed90_141($lepusTemplate, $n141, $data, $update2);
                  $update2 = _$template_update3;
                }
                {
                  let _$template_update4 = $update2;
                  let $n144 = $update2 ? $lepusGetElementRefByLepusID("if", 144) : null;
                  if (!$n144) {
                    $update2 = false;
                    $n144 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n144, 144, "if");
                    __AppendElement($n140, $n144);
                  }
                  $renderTemplates[$path].update_1387060_144($lepusTemplate, $n144, $data, $update2);
                  $update2 = _$template_update4;
                }
                {
                  let _$template_update5 = $update2;
                  let $n147 = $update2 ? $lepusGetElementRefByLepusID("if", 147) : null;
                  if (!$n147) {
                    $update2 = false;
                    $n147 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n147, 147, "if");
                    __AppendElement($n140, $n147);
                  }
                  $renderTemplates[$path].update_201d6f8_147($lepusTemplate, $n147, $data, $update2);
                  $update2 = _$template_update5;
                }
                $update2 = _$temp48;
              }
              $update2 = $temp3;
            }
            {
              let $n184 = $update2 ? $lepusGetElementRefByLepusID("view", 184) : null;
              let _$temp49 = $update2;
              if (!$n184) {
                $update2 = false;
                $n184 = __CreateView($currentComponentId);
                let $nid184 = $lepusStoreElementRefByLepusID($n184, 184, "view");
                __SetAttribute($n184, 1004, $nid184[1]);
                __SetStyleObject($n184, [124, 125, 31, 126]);
                __AppendElement($n133, $n184);
              }
              {
                let $n185 = $update2 ? $lepusGetElementRefByLepusID("template", 185) : null;
                let _$templateCreated = true;
                let _$childLepusTemplate = null;
                let _$templateId = null;
                if (!$n185) {
                  _$templateCreated = false;
                  $n185 = __CreateWrapperElement($currentComponentId);
                  $lepusStoreElementRefByLepusID($n185, 185, "template");
                  __AppendElement($n184, $n185);
                  _$templateId = __GetElementUniqueID($n185);
                  _$childLepusTemplate = $templateConstructor(_$templateId, $n185);
                } else {
                  _$templateId = __GetElementUniqueID($n185);
                  _$childLepusTemplate = $templateInfo[_$templateId];
                }
                $updatePropsSet = [];
                _$childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
                _$childLepusTemplate.setData("seckill", $data.seckill, $update2);
                if (_$templateCreated) {
                  let _$update_keys = $updatePropsSet;
                  if (_$update_keys.length > 0) {
                    $renderTemplates["Button"].update(_$childLepusTemplate, _$childLepusTemplate.data, _$update_keys, true);
                  }
                } else {
                  $renderTemplates["Button"].entry($n185, _$childLepusTemplate, _$childLepusTemplate.data, false);
                }
              }
              $update2 = _$temp49;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp50 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n186 = $update2 ? $lepusGetElementRefByLepusID("view", 186) : null;
            let _$temp51 = $update2;
            if (!$n186) {
              $update2 = false;
              $n186 = __CreateView($currentComponentId);
              let $nid186 = $lepusStoreElementRefByLepusID($n186, 186, "view");
              __SetAttribute($n186, 1004, $nid186[1]);
              __AppendElement($parent, $n186);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value43 = $data.marketingInfoVO.layoutVO.containerStyle;
                if (!$update2 || _$value43 !== $lepusTemplate._data.marketingInfoVO.layoutVO.containerStyle) {
                  __SetStyleObject($n186, [0, 3, 44, 79, 62, 102, getCssPropertyIDObj($data.marketingInfoVO.layoutVO.containerStyle)]);
                }
              }
            }
            {
              let $n187 = $update2 ? $lepusGetElementRefByLepusID("view", 187) : null;
              let _$temp52 = $update2;
              if (!$n187) {
                $update2 = false;
                $n187 = __CreateView($currentComponentId);
                let $nid187 = $lepusStoreElementRefByLepusID($n187, 187, "view");
                __SetAttribute($n187, 1004, $nid187[1]);
                __AppendElement($n186, $n187);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value44 = "flex-direction:" + (($data.marketingInfoVO.layoutVO.showPriceOneLine ? "row" : "column") + ";") + ("align-items:" + (($data.marketingInfoVO.layoutVO.showPriceOneLine ? "center" : "") + ";") + ("overflow:" + (($data.marketingInfoVO.layoutVO.showPriceOneLine ? "hidden" : "") + ";") + ("max-height:" + (($data.marketingInfoVO.layoutVO.showPriceOneLine ? "18px" : "100px") + ";")))) + "flex-grow:1;display:flex;flex-wrap:wrap;";
                  if (!$update2 || _$value44 !== undefined) {
                    __SetStyleObject($n187, [102, 3, 103, {
                      53: $data.marketingInfoVO.layoutVO.showPriceOneLine ? "row" : "column"
                    }, {
                      55: $data.marketingInfoVO.layoutVO.showPriceOneLine ? "center" : ""
                    }, {
                      25: $data.marketingInfoVO.layoutVO.showPriceOneLine ? "hidden" : ""
                    }, {
                      30: $data.marketingInfoVO.layoutVO.showPriceOneLine ? "18px" : "100px"
                    }]);
                  }
                }
              }
              {
                let $n188 = $update2 ? $lepusGetElementRefByLepusID("view", 188) : null;
                let _$temp53 = $update2;
                if (!$n188) {
                  $update2 = false;
                  $n188 = __CreateView($currentComponentId);
                  let $nid188 = $lepusStoreElementRefByLepusID($n188, 188, "view");
                  __SetAttribute($n188, 1004, $nid188[1]);
                  __SetStyleObject($n188, [3, 44, 104, 31, 45, 103, 66]);
                  __AppendElement($n187, $n188);
                }
                {
                  let $n189 = $update2 ? $lepusGetElementRefByLepusID("template", 189) : null;
                  let _$templateCreated2 = true;
                  let _$childLepusTemplate2 = null;
                  let _$templateId2 = null;
                  if (!$n189) {
                    _$templateCreated2 = false;
                    $n189 = __CreateWrapperElement($currentComponentId);
                    $lepusStoreElementRefByLepusID($n189, 189, "template");
                    __AppendElement($n188, $n189);
                    _$templateId2 = __GetElementUniqueID($n189);
                    _$childLepusTemplate2 = $templateConstructor(_$templateId2, $n189);
                  } else {
                    _$templateId2 = __GetElementUniqueID($n189);
                    _$childLepusTemplate2 = $templateInfo[_$templateId2];
                  }
                  $updatePropsSet = [];
                  _$childLepusTemplate2.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
                  if (_$templateCreated2) {
                    let _$update_keys2 = $updatePropsSet;
                    if (_$update_keys2.length > 0) {
                      $renderTemplates["Price"].update(_$childLepusTemplate2, _$childLepusTemplate2.data, _$update_keys2, true);
                    }
                  } else {
                    $renderTemplates["Price"].entry($n189, _$childLepusTemplate2, _$childLepusTemplate2.data, false);
                  }
                }
                {
                  let _$template_update6 = $update2;
                  let $n190 = $update2 ? $lepusGetElementRefByLepusID("if", 190) : null;
                  if (!$n190) {
                    $update2 = false;
                    $n190 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n190, 190, "if");
                    __AppendElement($n188, $n190);
                  }
                  $renderTemplates[$path].update_1387060_190($lepusTemplate, $n190, $data, $update2);
                  $update2 = _$template_update6;
                }
                $update2 = _$temp53;
              }
              {
                let $n192 = $update2 ? $lepusGetElementRefByLepusID("view", 192) : null;
                let _$temp54 = $update2;
                if (!$n192) {
                  $update2 = false;
                  $n192 = __CreateView($currentComponentId);
                  let $nid192 = $lepusStoreElementRefByLepusID($n192, 192, "view");
                  __SetAttribute($n192, 1004, $nid192[1]);
                  __AppendElement($n187, $n192);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value45 = "width:" + (($data.marketingInfoVO.layoutVO.showPriceOneLine ? "auto" : "100%") + ";") + "max-height:15px;display:flex;flex-direction:row;align-items:center;overflow:hidden;flex-wrap:wrap;";
                    if (!$update2 || _$value45 !== "width:" + (($lepusTemplate._data.marketingInfoVO.layoutVO.showPriceOneLine ? "auto" : "100%") + ";") + "max-height:15px;display:flex;flex-direction:row;align-items:center;overflow:hidden;flex-wrap:wrap;") {
                      __SetStyleObject($n192, [127, 3, 44, 4, 45, 103, {
                        27: $data.marketingInfoVO.layoutVO.showPriceOneLine ? "auto" : "100%"
                      }]);
                    }
                  }
                }
                {
                  let $n193 = $update2 ? $lepusGetElementRefByLepusID("template", 193) : null;
                  let _$templateCreated3 = true;
                  let _$childLepusTemplate3 = null;
                  let _$templateId3 = null;
                  if (!$n193) {
                    _$templateCreated3 = false;
                    $n193 = __CreateWrapperElement($currentComponentId);
                    $lepusStoreElementRefByLepusID($n193, 193, "template");
                    __AppendElement($n192, $n193);
                    _$templateId3 = __GetElementUniqueID($n193);
                    _$childLepusTemplate3 = $templateConstructor(_$templateId3, $n193);
                  } else {
                    _$templateId3 = __GetElementUniqueID($n193);
                    _$childLepusTemplate3 = $templateInfo[_$templateId3];
                  }
                  $updatePropsSet = [];
                  _$childLepusTemplate3.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
                  _$childLepusTemplate3.setData("isCouponOverflow", $data.isCouponOverflow, $update2);
                  _$childLepusTemplate3.setData("isSiblingOverflow", $data.isSiblingOverflow, $update2);
                  if (_$templateCreated3) {
                    let _$update_keys3 = $updatePropsSet;
                    if (_$update_keys3.length > 0) {
                      $renderTemplates["Coupon"].update(_$childLepusTemplate3, _$childLepusTemplate3.data, _$update_keys3, true);
                    }
                  } else {
                    $renderTemplates["Coupon"].entry($n193, _$childLepusTemplate3, _$childLepusTemplate3.data, false);
                  }
                }
                {
                  let _$template_update7 = $update2;
                  let $n194 = $update2 ? $lepusGetElementRefByLepusID("if", 194) : null;
                  if (!$n194) {
                    $update2 = false;
                    $n194 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n194, 194, "if");
                    __AppendElement($n192, $n194);
                  }
                  $renderTemplates[$path].update_1387060_194($lepusTemplate, $n194, $data, $update2);
                  $update2 = _$template_update7;
                }
                $update2 = _$temp54;
              }
              $update2 = _$temp52;
            }
            {
              let $n196 = $update2 ? $lepusGetElementRefByLepusID("view", 196) : null;
              let _$temp55 = $update2;
              if (!$n196) {
                $update2 = false;
                $n196 = __CreateView($currentComponentId);
                let $nid196 = $lepusStoreElementRefByLepusID($n196, 196, "view");
                __SetAttribute($n196, 1004, $nid196[1]);
                __SetStyleObject($n196, [31, 124, 125, 126]);
                __AppendElement($n186, $n196);
              }
              {
                let _$template_update8 = $update2;
                let $n197 = $update2 ? $lepusGetElementRefByLepusID("if", 197) : null;
                if (!$n197) {
                  $update2 = false;
                  $n197 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n197, 197, "if");
                  __AppendElement($n196, $n197);
                }
                $renderTemplates[$path].update_393d908_197($lepusTemplate, $n197, $data, $update2);
                $update2 = _$template_update8;
              }
              $update2 = _$temp55;
            }
            $update2 = _$temp51;
          }
          $update2 = _$temp50;
        }
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "ProcessComp";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n132 = $lepusGetElementRefByLepusID("if", 132);
    $renderTemplates[$path].update_1bd09b0_132($lepusTemplate, $n132, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n131 = __CreateView($currentComponentId);
    __SetAttribute($n131, 1004, 131);
    __SetStyleObject($n131, [3, 100, 0, 101]);
    __AppendElement($template, $n131);
    let $n132 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n132, 132, "if");
    __AppendElement($n131, $n132);
    $renderTemplates["ProcessComp"].update_1bd09b0_132($lepusTemplate, $n132, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["Price"] = {
  variables: ["marketingInfoVO"],
  varUpdateState: [],
  update_1387060_261: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Price";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.priceVO.finalPrice.prefix) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n262 = $update2 ? $lepusGetElementRefByLepusID("text", 262) : null;
            let $temp2 = $update2;
            if (!$n262) {
              $update2 = false;
              $n262 = __CreateText($currentComponentId);
              let $nid262 = $lepusStoreElementRefByLepusID($n262, 262, "text");
              __SetAttribute($n262, 1004, $nid262[1]);
              __SetAttribute($n262, "text-maxline", "1");
              __AppendElement($parent, $n262);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.priceVO.prefixFontStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.priceVO.prefixFontStyle) {
                  __SetStyleObject($n262, [117, 139, 21, 140, 31, 89, getCssPropertyIDObj($data.marketingInfoVO.priceVO.prefixFontStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value46 = $data.marketingInfoVO.priceVO.finalPrice.prefix || "";
                if (!$update2 || _$value46 !== ($lepusTemplate._data.marketingInfoVO.priceVO.finalPrice.prefix || "")) {
                  __SetAttribute($n262, "text", _$value46);
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
  update_1387060_264: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Price";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.priceVO.finalPrice.currency) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n265 = $update2 ? $lepusGetElementRefByLepusID("text", 265) : null;
            let $temp2 = $update2;
            if (!$n265) {
              $update2 = false;
              $n265 = __CreateText($currentComponentId);
              let $nid265 = $lepusStoreElementRefByLepusID($n265, 265, "text");
              __SetAttribute($n265, 1004, $nid265[1]);
              __SetAttribute($n265, "text-maxline", "1");
              __AppendElement($parent, $n265);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.priceVO.finalPrice.currencyFontStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.priceVO.finalPrice.currencyFontStyle) {
                  __SetStyleObject($n265, [31, 40, 89, 133, 17, 12, getCssPropertyIDObj($data.marketingInfoVO.priceVO.finalPrice.currencyFontStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value47 = $data.marketingInfoVO.priceVO.finalPrice.currency || "";
                if (!$update2 || _$value47 !== ($lepusTemplate._data.marketingInfoVO.priceVO.finalPrice.currency || "")) {
                  __SetAttribute($n265, "text", _$value47);
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
  update_1387060_267: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Price";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.priceVO.finalPrice.amountYuan.length > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n268 = $update2 ? $lepusGetElementRefByLepusID("text", 268) : null;
            let $temp2 = $update2;
            if (!$n268) {
              $update2 = false;
              $n268 = __CreateText($currentComponentId);
              let $nid268 = $lepusStoreElementRefByLepusID($n268, 268, "text");
              __SetAttribute($n268, 1004, $nid268[1]);
              __SetAttribute($n268, "text-maxline", "1");
              __AppendElement($parent, $n268);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.priceVO.finalPrice.amountFontStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.priceVO.finalPrice.amountFontStyle) {
                  __SetStyleObject($n268, [89, 133, 53, 12, 31, getCssPropertyIDObj($data.marketingInfoVO.priceVO.finalPrice.amountFontStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value48 = $data.marketingInfoVO.priceVO.finalPrice.amountYuan || "";
                if (!$update2 || _$value48 !== ($lepusTemplate._data.marketingInfoVO.priceVO.finalPrice.amountYuan || "")) {
                  __SetAttribute($n268, "text", _$value48);
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
  update_1387060_270: function ($lepusTemplate, $parent, $data, $update2) {
    if (!$update2 || $renderTemplates["Price"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo5 = $lepusPushFiberForNode($parent, 270, uniqueId),
          $forLepus = _$lepusPushFiberForNo5[0],
          $lastForLepus = _$lepusPushFiberForNo5[1];
      let $object = $data.marketingInfoVO.priceVO.finalPrice.postfixItems;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n271 = $update2 ? $lepusGetElementRefByLepusID("text", 271) : null;
          let $temp2 = $update2;
          if (!$n271) {
            $update2 = false;
            $n271 = __CreateText($currentComponentId);
            let $nid271 = $lepusStoreElementRefByLepusID($n271, 271, "text");
            __SetAttribute($n271, 1004, $nid271[1]);
            __SetAttribute($n271, "text-maxline", "1");
            __AppendElement($parent, $n271);
          }
          __SetStyleObject($n271, [117, 11, 21, 31, 89, getCssPropertyIDObj(item.style)]);
          __SetAttribute($n271, "text", item.str || "");
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_1387060_273: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Price";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.priceVO.comparePrice.text) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n274 = $update2 ? $lepusGetElementRefByLepusID("view", 274) : null;
            let $temp2 = $update2;
            if (!$n274) {
              $update2 = false;
              $n274 = __CreateView($currentComponentId);
              let $nid274 = $lepusStoreElementRefByLepusID($n274, 274, "view");
              __SetAttribute($n274, 1004, $nid274[1]);
              __SetStyleObject($n274, [3, 105]);
              __SetID($n274, "compare-price");
              __AppendElement($parent, $n274);
            }
            {
              let $template_update = $update2;
              let $n275 = $update2 ? $lepusGetElementRefByLepusID("if", 275) : null;
              if (!$n275) {
                $update2 = false;
                $n275 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n275, 275, "if");
                __AppendElement($n274, $n275);
              }
              $renderTemplates[$path].update_1387060_275($lepusTemplate, $n275, $data, $update2);
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
  update_1387060_275: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Price";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingInfoVO.priceVO.comparePrice.text.length > 0) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n276 = $update2 ? $lepusGetElementRefByLepusID("text", 276) : null;
          let $temp2 = $update2;
          if (!$n276) {
            $update2 = false;
            $n276 = __CreateText($currentComponentId);
            let $nid276 = $lepusStoreElementRefByLepusID($n276, 276, "text");
            __SetAttribute($n276, 1004, $nid276[1]);
            __SetAttribute($n276, "text-maxline", "1");
            __AppendElement($parent, $n276);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.marketingInfoVO.priceVO.comparePrice.textFontStyle;
              if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.priceVO.comparePrice.textFontStyle) {
                __SetStyleObject($n276, [89, 117, 11, 18, getCssPropertyIDObj($data.marketingInfoVO.priceVO.comparePrice.textFontStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value49 = $data.marketingInfoVO.priceVO.comparePrice.text || "";
              if (!$update2 || _$value49 !== ($lepusTemplate._data.marketingInfoVO.priceVO.comparePrice.text || "")) {
                __SetAttribute($n276, "text", _$value49);
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
  update_1387060_278: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Price";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.priceVO.introducePrice.text) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n279 = $update2 ? $lepusGetElementRefByLepusID("view", 279) : null;
            let $temp2 = $update2;
            if (!$n279) {
              $update2 = false;
              $n279 = __CreateView($currentComponentId);
              let $nid279 = $lepusStoreElementRefByLepusID($n279, 279, "view");
              __SetAttribute($n279, 1004, $nid279[1]);
              __SetStyleObject($n279, [3, 105]);
              __SetID($n279, "introduce-price");
              __AppendElement($parent, $n279);
            }
            {
              let $template_update = $update2;
              let $n280 = $update2 ? $lepusGetElementRefByLepusID("if", 280) : null;
              if (!$n280) {
                $update2 = false;
                $n280 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n280, 280, "if");
                __AppendElement($n279, $n280);
              }
              $renderTemplates[$path].update_1387060_280($lepusTemplate, $n280, $data, $update2);
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
  update_1387060_280: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Price";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingInfoVO.priceVO.introducePrice.text.length > 0) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n281 = $update2 ? $lepusGetElementRefByLepusID("text", 281) : null;
          let $temp2 = $update2;
          if (!$n281) {
            $update2 = false;
            $n281 = __CreateText($currentComponentId);
            let $nid281 = $lepusStoreElementRefByLepusID($n281, 281, "text");
            __SetAttribute($n281, 1004, $nid281[1]);
            __SetAttribute($n281, "text-maxline", "1");
            __AppendElement($parent, $n281);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.marketingInfoVO.priceVO.introducePrice.textFontStyle;
              if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.priceVO.introducePrice.textFontStyle) {
                __SetStyleObject($n281, [117, 11, 18, 31, 89, getCssPropertyIDObj($data.marketingInfoVO.priceVO.introducePrice.textFontStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value50 = $data.marketingInfoVO.priceVO.introducePrice.text || "";
              if (!$update2 || _$value50 !== ($lepusTemplate._data.marketingInfoVO.priceVO.introducePrice.text || "")) {
                __SetAttribute($n281, "text", _$value50);
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
  update_1387060_284: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Price";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.priceVO.originPrice.text.length > 0) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n285 = $update2 ? $lepusGetElementRefByLepusID("text", 285) : null;
            let $temp2 = $update2;
            if (!$n285) {
              $update2 = false;
              $n285 = __CreateText($currentComponentId);
              let $nid285 = $lepusStoreElementRefByLepusID($n285, 285, "text");
              __SetAttribute($n285, 1004, $nid285[1]);
              __SetAttribute($n285, "text-maxline", "1");
              __AppendElement($parent, $n285);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.priceVO.originPrice.textFontStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.priceVO.originPrice.textFontStyle) {
                  __SetStyleObject($n285, [117, 11, 18, 141, 142, getCssPropertyIDObj($data.marketingInfoVO.priceVO.originPrice.textFontStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value51 = $data.marketingInfoVO.priceVO.originPrice.text || "";
                if (!$update2 || _$value51 !== ($lepusTemplate._data.marketingInfoVO.priceVO.originPrice.text || "")) {
                  __SetAttribute($n285, "text", _$value51);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "Price";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n261 = $lepusGetElementRefByLepusID("if", 261);
    $renderTemplates[$path].update_1387060_261($lepusTemplate, $n261, $data, $update2);
    let $n264 = $lepusGetElementRefByLepusID("if", 264);
    $renderTemplates[$path].update_1387060_264($lepusTemplate, $n264, $data, $update2);
    let $n267 = $lepusGetElementRefByLepusID("if", 267);
    $renderTemplates[$path].update_1387060_267($lepusTemplate, $n267, $data, $update2);
    let $n270 = $lepusGetElementRefByLepusID("for", 270);
    $renderTemplates[$path].update_1387060_270($lepusTemplate, $n270, $data, $update2);
    let $n273 = $lepusGetElementRefByLepusID("if", 273);
    $renderTemplates[$path].update_1387060_273($lepusTemplate, $n273, $data, $update2);
    let $n278 = $lepusGetElementRefByLepusID("if", 278);
    $renderTemplates[$path].update_1387060_278($lepusTemplate, $n278, $data, $update2);
    let $n284 = $lepusGetElementRefByLepusID("if", 284);
    $renderTemplates[$path].update_1387060_284($lepusTemplate, $n284, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $path = "Price";
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n259 = __CreateView($currentComponentId);
    __SetAttribute($n259, 1004, 259);
    __SetStyleObject($n259, [3, 44, 104, 103]);
    __AppendElement($template, $n259);
    let $n260 = __CreateView($currentComponentId);
    __SetAttribute($n260, 1004, 260);
    __SetStyleObject($n260, [3, 44, 104, 105]);
    __AppendElement($n259, $n260);
    let $n261 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n261, 261, "if");
    __AppendElement($n260, $n261);
    $renderTemplates[$path].update_1387060_261($lepusTemplate, $n261, $data, $update2);
    let $n264 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n264, 264, "if");
    __AppendElement($n260, $n264);
    $renderTemplates[$path].update_1387060_264($lepusTemplate, $n264, $data, $update2);
    let $n267 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n267, 267, "if");
    __AppendElement($n260, $n267);
    $renderTemplates[$path].update_1387060_267($lepusTemplate, $n267, $data, $update2);
    let $n270 = __CreateFor($currentComponentId);
    $lepusStoreElementRefByLepusID($n270, 270, "for");
    __AppendElement($n260, $n270);
    $renderTemplates[$path].update_1387060_270($lepusTemplate, $n270, $data, $update2);
    let $n273 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n273, 273, "if");
    __AppendElement($n259, $n273);
    $renderTemplates[$path].update_1387060_273($lepusTemplate, $n273, $data, $update2);
    let $n278 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n278, 278, "if");
    __AppendElement($n259, $n278);
    $renderTemplates[$path].update_1387060_278($lepusTemplate, $n278, $data, $update2);
    let $n283 = __CreateView($currentComponentId);
    __SetAttribute($n283, 1004, 283);
    __SetStyleObject($n283, [3, 105]);
    __SetID($n283, "origin-price");
    __AppendElement($n259, $n283);
    let $n284 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n284, 284, "if");
    __AppendElement($n283, $n284);
    $renderTemplates[$path].update_1387060_284($lepusTemplate, $n284, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["LowPriceTag"] = {
  variables: ["marketingInfoVO"],
  varUpdateState: [],
  update_1387060_288: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "LowPriceTag";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.lowPriceTagVO.prefixIcon.url) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n289 = $update2 ? $lepusGetElementRefByLepusID("image", 289) : null;
            let $temp2 = $update2;
            if (!$n289) {
              $update2 = false;
              $n289 = __CreateImage($currentComponentId);
              let $nid289 = $lepusStoreElementRefByLepusID($n289, 289, "image");
              __SetAttribute($n289, 1004, $nid289[1]);
              __AppendElement($parent, $n289);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.lowPriceTagVO.prefixIconStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.lowPriceTagVO.prefixIconStyle) {
                  __SetStyleObject($n289, [145, 146, 140, getCssPropertyIDObj($data.marketingInfoVO.lowPriceTagVO.prefixIconStyle)]);
                }
              }
              {
                let _$value52 = $data.marketingInfoVO.lowPriceTagVO.prefixIcon.url;
                if (!$update2 || _$value52 !== $lepusTemplate._data.marketingInfoVO.lowPriceTagVO.prefixIcon.url) {
                  __SetAttribute($n289, "src", _$value52);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "LowPriceTag";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n287 = $lepusGetElementRefByLepusID("view", 287);
      {
        let $value = $data.marketingInfoVO.lowPriceTagVO.containerStyle;
        if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.lowPriceTagVO.containerStyle) {
          __SetStyleObject($n287, [3, 44, 4, 31, 52, 143, 144, getCssPropertyIDObj($data.marketingInfoVO.lowPriceTagVO.containerStyle)]);
        }
      }
    }
    let $n288 = $lepusGetElementRefByLepusID("if", 288);
    $renderTemplates[$path].update_1387060_288($lepusTemplate, $n288, $data, $update2);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n290 = $lepusGetElementRefByLepusID("text", 290);
      {
        let _$value53 = $data.marketingInfoVO.lowPriceTagVO.textFontStyle;
        if (!$update2 || _$value53 !== $lepusTemplate._data.marketingInfoVO.lowPriceTagVO.textFontStyle) {
          __SetStyleObject($n290, [11, 21, 89, getCssPropertyIDObj($data.marketingInfoVO.lowPriceTagVO.textFontStyle)]);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let _$value54 = $data.marketingInfoVO.lowPriceTagVO.text;
      if (_$value54 !== $lepusTemplate._data.marketingInfoVO.lowPriceTagVO.text) {
        let _$n3 = $lepusGetElementRefByLepusID("text", 290);
        __SetAttribute(_$n3, "text", _$value54);
      }
    }
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n287 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n287, 287, "view");
    __SetAttribute($n287, 1004, 287);
    __SetStyleObject($n287, [3, 44, 4, 31, 52, 143, 144, getCssPropertyIDObj($data.marketingInfoVO.lowPriceTagVO.containerStyle)]);
    __AppendElement($template, $n287);
    let $n288 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n288, 288, "if");
    __AppendElement($n287, $n288);
    $renderTemplates["LowPriceTag"].update_1387060_288($lepusTemplate, $n288, $data, $update2);
    let $n290 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n290, 290, "text");
    __SetAttribute($n290, 1004, 290);
    __SetStyleObject($n290, [11, 21, 89, getCssPropertyIDObj($data.marketingInfoVO.lowPriceTagVO.textFontStyle)]);
    __SetAttribute($n290, "text-maxline", "1");
    __AppendElement($n287, $n290);
    __SetAttribute($n290, "text", $data.marketingInfoVO.lowPriceTagVO.text);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["Coupon"] = {
  variables: ["marketingInfoVO", "isCouponOverflow", "isSiblingOverflow"],
  varUpdateState: [],
  update_45ea70_297: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!($data.marketingInfoVO.couponVO.displayType == "bySide" && !$data.isSiblingOverflow) && !$data.marketingInfoVO.couponVO.prefixBackgroundImage.hidden) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n298 = $update2 ? $lepusGetElementRefByLepusID("image", 298) : null;
          let $temp2 = $update2;
          if (!$n298) {
            $update2 = false;
            $n298 = __CreateImage($currentComponentId);
            let $nid298 = $lepusStoreElementRefByLepusID($n298, 298, "image");
            __SetAttribute($n298, 1004, $nid298[1]);
            __SetStyleObject($n298, [55, 75, 76, 153, 77]);
            __SetAttribute($n298, "cap-insets-scale", "3");
            __AppendElement($parent, $n298);
          }
          if (!$update2 || $renderTemplates["Coupon"].varUpdateState[0]) {
            {
              let $value = $data.marketingInfoVO.couponVO.prefixBackgroundImage.url;
              if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.couponVO.prefixBackgroundImage.url) {
                __SetAttribute($n298, "src", $value);
              }
            }
            {
              let _$value55 = $data.marketingInfoVO.couponVO.prefixBackgroundImage.capInsets;
              if (!$update2 || _$value55 !== $lepusTemplate._data.marketingInfoVO.couponVO.prefixBackgroundImage.capInsets) {
                __SetAttribute($n298, "cap-insets", _$value55);
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
  update_45ea70_299: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingInfoVO.couponVO.displayType == "bySide" && !$data.isSiblingOverflow && !$data.marketingInfoVO.couponVO.prefixBackgroundImageBySide.hidden) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n300 = $update2 ? $lepusGetElementRefByLepusID("image", 300) : null;
          let $temp2 = $update2;
          if (!$n300) {
            $update2 = false;
            $n300 = __CreateImage($currentComponentId);
            let $nid300 = $lepusStoreElementRefByLepusID($n300, 300, "image");
            __SetAttribute($n300, 1004, $nid300[1]);
            __SetStyleObject($n300, [55, 75, 76, 153, 77]);
            __SetAttribute($n300, "cap-insets-scale", "3");
            __AppendElement($parent, $n300);
          }
          if (!$update2 || $renderTemplates["Coupon"].varUpdateState[0]) {
            {
              let $value = $data.marketingInfoVO.couponVO.prefixBackgroundImageBySide.url;
              if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.couponVO.prefixBackgroundImageBySide.url) {
                __SetAttribute($n300, "src", $value);
              }
            }
            {
              let _$value56 = $data.marketingInfoVO.couponVO.prefixBackgroundImageBySide.capInsets;
              if (!$update2 || _$value56 !== $lepusTemplate._data.marketingInfoVO.couponVO.prefixBackgroundImageBySide.capInsets) {
                __SetAttribute($n300, "cap-insets", _$value56);
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
  update_256250_301: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Coupon";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.marketingInfoVO.couponVO.prefixLeftImage.hidden) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n302 = $update2 ? $lepusGetElementRefByLepusID("image", 302) : null;
          let $temp2 = $update2;
          if (!$n302) {
            $update2 = false;
            $n302 = __CreateImage($currentComponentId);
            let $nid302 = $lepusStoreElementRefByLepusID($n302, 302, "image");
            __SetAttribute($n302, 1004, $nid302[1]);
            __SetAttribute($n302, "cap-insets-scale", "3");
            __AppendElement($parent, $n302);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[3]) {
            {
              __SetStyleObject($n302, [74, 1, {
                27: !!$data.marketingInfoVO.couponVO.prefixLeftImage.ratio ? 15 * $data.marketingInfoVO.couponVO.prefixLeftImage.ratio + "px" : "auto"
              }, getCssPropertyIDObj($data.marketingInfoVO.couponVO.prefixLeftImageStyle)]);
            }
            {
              let $value = $data.marketingInfoVO.couponVO.prefixLeftImage.url;
              if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.couponVO.prefixLeftImage.url) {
                __SetAttribute($n302, "src", $value);
              }
            }
            {
              let _$value57 = $data.marketingInfoVO.couponVO.prefixLeftImageStyleCapInsets;
              if (!$update2 || _$value57 !== $lepusTemplate._data.marketingInfoVO.couponVO.prefixLeftImageStyleCapInsets) {
                __SetAttribute($n302, "cap-insets", _$value57);
              }
            }
            {
              let _$value58 = !!!$data.marketingInfoVO.couponVO.prefixLeftImage.ratio;
              if (!$update2 || _$value58 !== !!!$lepusTemplate._data.marketingInfoVO.couponVO.prefixLeftImage.ratio) {
                __SetAttribute($n302, "auto-size", _$value58);
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
  update_221e9e8_293: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Coupon";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[3]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!$data.marketingInfoVO.couponVO.hidden && ($data.marketingInfoVO.couponVO.hasPrefix || ((__globalProps.screenWidth || 385) > ($data.marketingInfoVO.couponVO.suffixTextVO.threshold || 0) ? $data.marketingInfoVO.couponVO.suffixTextVO.text : $data.marketingInfoVO.couponVO.suffixTextVO.clippedText))) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n294 = $update2 ? $lepusGetElementRefByLepusID("view", 294) : null;
            let $temp2 = $update2;
            if (!$n294) {
              $update2 = false;
              $n294 = __CreateView($currentComponentId);
              let $nid294 = $lepusStoreElementRefByLepusID($n294, 294, "view");
              __SetAttribute($n294, 1004, $nid294[1]);
              __AppendElement($parent, $n294);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.couponVO.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.couponVO.containerStyle) {
                  __SetStyleObject($n294, [147, 3, 44, 4, 32, 45, 103, 148, getCssPropertyIDObj($data.marketingInfoVO.couponVO.containerStyle)]);
                }
              }
            }
            {
              let $template_update = $update2;
              let $n295 = $update2 ? $lepusGetElementRefByLepusID("if", 295) : null;
              if (!$n295) {
                $update2 = false;
                $n295 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n295, 295, "if");
                __AppendElement($n294, $n295);
              }
              $renderTemplates[$path].update_221e9e8_295($lepusTemplate, $n295, $data, $update2);
              $update2 = $template_update;
            }
            {
              let _$template_update9 = $update2;
              let $n305 = $update2 ? $lepusGetElementRefByLepusID("if", 305) : null;
              if (!$n305) {
                $update2 = false;
                $n305 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n305, 305, "if");
                __AppendElement($n294, $n305);
              }
              $renderTemplates[$path].update_2552718_305($lepusTemplate, $n305, $data, $update2);
              $update2 = _$template_update9;
            }
            {
              let _$template_update10 = $update2;
              let $n308 = $update2 ? $lepusGetElementRefByLepusID("if", 308) : null;
              if (!$n308) {
                $update2 = false;
                $n308 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n308, 308, "if");
                __AppendElement($n294, $n308);
              }
              $renderTemplates[$path].update_2552718_308($lepusTemplate, $n308, $data, $update2);
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
  update_221e9e8_295: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Coupon";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingInfoVO.couponVO.hasPrefix) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n296 = $update2 ? $lepusGetElementRefByLepusID("view", 296) : null;
          let $temp2 = $update2;
          if (!$n296) {
            $update2 = false;
            $n296 = __CreateView($currentComponentId);
            let $nid296 = $lepusStoreElementRefByLepusID($n296, 296, "view");
            __SetAttribute($n296, 1004, $nid296[1]);
            __SetID($n296, "prefix-wrapper");
            __AppendElement($parent, $n296);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
            {
              __SetStyleObject($n296, [1, 149, 150, 151, 152, 3, 44, 4, 33, 74, 31, {
                12: ((__globalProps.screenWidth || 385) > ($data.marketingInfoVO.couponVO.suffixTextVO.threshold || 0) ? $data.marketingInfoVO.couponVO.suffixTextVO.text : $data.marketingInfoVO.couponVO.suffixTextVO.clippedText) || $data.isCouponOverflow ? "2px" : "0px"
              }, {
                34: ((__globalProps.screenWidth || 385) > ($data.marketingInfoVO.couponVO.suffixTextVO.threshold || 0) ? $data.marketingInfoVO.couponVO.suffixTextVO.text : $data.marketingInfoVO.couponVO.suffixTextVO.clippedText) || $data.isCouponOverflow ? "4px" : "0px"
              }, getCssPropertyIDObj($data.marketingInfoVO.couponVO.prefixContainerStyle)]);
            }
          }
          {
            let $template_update = $update2;
            let $n297 = $update2 ? $lepusGetElementRefByLepusID("if", 297) : null;
            if (!$n297) {
              $update2 = false;
              $n297 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n297, 297, "if");
              __AppendElement($n296, $n297);
            }
            $renderTemplates[$path].update_45ea70_297($lepusTemplate, $n297, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update11 = $update2;
            let $n299 = $update2 ? $lepusGetElementRefByLepusID("if", 299) : null;
            if (!$n299) {
              $update2 = false;
              $n299 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n299, 299, "if");
              __AppendElement($n296, $n299);
            }
            $renderTemplates[$path].update_45ea70_299($lepusTemplate, $n299, $data, $update2);
            $update2 = _$template_update11;
          }
          {
            let _$template_update12 = $update2;
            let $n301 = $update2 ? $lepusGetElementRefByLepusID("if", 301) : null;
            if (!$n301) {
              $update2 = false;
              $n301 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n301, 301, "if");
              __AppendElement($n296, $n301);
            }
            $renderTemplates[$path].update_256250_301($lepusTemplate, $n301, $data, $update2);
            $update2 = _$template_update12;
          }
          {
            let $n303 = $update2 ? $lepusGetElementRefByLepusID("text", 303) : null;
            let $temp3 = $update2;
            if (!$n303) {
              $update2 = false;
              $n303 = __CreateText($currentComponentId);
              let $nid303 = $lepusStoreElementRefByLepusID($n303, 303, "text");
              __SetAttribute($n303, 1004, $nid303[1]);
              __SetAttribute($n303, "text-maxline", "1");
              __AppendElement($n296, $n303);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.couponVO.prefixTextFontStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.couponVO.prefixTextFontStyle) {
                  __SetStyleObject($n303, [117, 139, 21, 89, getCssPropertyIDObj($data.marketingInfoVO.couponVO.prefixTextFontStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value59 = $data.marketingInfoVO.couponVO.prefixText;
                if (!$update2 || _$value59 !== $lepusTemplate._data.marketingInfoVO.couponVO.prefixText) {
                  __SetAttribute($n303, "text", _$value59);
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
  },
  update_2552718_305: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!$data.isCouponOverflow && $data.marketingInfoVO.couponVO.hasPrefix && ((__globalProps.screenWidth || 385) > ($data.marketingInfoVO.couponVO.suffixTextVO.threshold || 0) ? $data.marketingInfoVO.couponVO.suffixTextVO.text : $data.marketingInfoVO.couponVO.suffixTextVO.clippedText) && $data.marketingInfoVO.couponVO.connectImage) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n306 = $update2 ? $lepusGetElementRefByLepusID("view", 306) : null;
          let $temp2 = $update2;
          if (!$n306) {
            $update2 = false;
            $n306 = __CreateView($currentComponentId);
            let $nid306 = $lepusStoreElementRefByLepusID($n306, 306, "view");
            __SetAttribute($n306, 1004, $nid306[1]);
            __SetStyleObject($n306, [154, 147]);
            __AppendElement($parent, $n306);
          }
          {
            let $n307 = $update2 ? $lepusGetElementRefByLepusID("image", 307) : null;
            let $temp3 = $update2;
            if (!$n307) {
              $update2 = false;
              $n307 = __CreateImage($currentComponentId);
              let $nid307 = $lepusStoreElementRefByLepusID($n307, 307, "image");
              __SetAttribute($n307, 1004, $nid307[1]);
              __SetStyleObject($n307, [154, 147]);
              __AppendElement($n306, $n307);
            }
            if (!$update2 || $renderTemplates["Coupon"].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.couponVO.connectImage;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.couponVO.connectImage) {
                  __SetAttribute($n307, "src", $value);
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
  },
  update_2552718_308: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Coupon";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((__globalProps.screenWidth || 385) > ($data.marketingInfoVO.couponVO.suffixTextVO.threshold || 0) ? $data.marketingInfoVO.couponVO.suffixTextVO.text : $data.marketingInfoVO.couponVO.suffixTextVO.clippedText) && !$data.isCouponOverflow) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n309 = $update2 ? $lepusGetElementRefByLepusID("view", 309) : null;
          let $temp2 = $update2;
          if (!$n309) {
            $update2 = false;
            $n309 = __CreateView($currentComponentId);
            let $nid309 = $lepusStoreElementRefByLepusID($n309, 309, "view");
            __SetAttribute($n309, 1004, $nid309[1]);
            __SetID($n309, "suffix-wrapper");
            __AppendElement($parent, $n309);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.marketingInfoVO.couponVO.suffixContainerStyle;
              if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.couponVO.suffixContainerStyle) {
                __SetStyleObject($n309, [1, 108, 155, 156, 111, 3, 44, 4, 157, 34, 158, 159, 160, getCssPropertyIDObj($data.marketingInfoVO.couponVO.suffixContainerStyle)]);
              }
            }
          }
          {
            let $n310 = $update2 ? $lepusGetElementRefByLepusID("text", 310) : null;
            let $temp3 = $update2;
            if (!$n310) {
              $update2 = false;
              $n310 = __CreateText($currentComponentId);
              let $nid310 = $lepusStoreElementRefByLepusID($n310, 310, "text");
              __SetAttribute($n310, 1004, $nid310[1]);
              __SetAttribute($n310, "text-maxline", "1");
              __AppendElement($n309, $n310);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value60 = $data.marketingInfoVO.couponVO.suffixTextFontStyle;
                if (!$update2 || _$value60 !== $lepusTemplate._data.marketingInfoVO.couponVO.suffixTextFontStyle) {
                  __SetStyleObject($n310, [117, 139, 21, 89, getCssPropertyIDObj($data.marketingInfoVO.couponVO.suffixTextFontStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                __SetAttribute($n310, "text", (__globalProps.screenWidth || 385) > ($data.marketingInfoVO.couponVO.suffixTextVO.threshold || 0) ? $data.marketingInfoVO.couponVO.suffixTextVO.text : $data.marketingInfoVO.couponVO.suffixTextVO.clippedText);
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
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "Coupon";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n293 = $lepusGetElementRefByLepusID("if", 293);
    $renderTemplates[$path].update_221e9e8_293($lepusTemplate, $n293, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n292 = __CreateView($currentComponentId);
    __SetAttribute($n292, 1004, 292);
    __AppendElement($template, $n292);
    let $n293 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n293, 293, "if");
    __AppendElement($n292, $n293);
    $renderTemplates["Coupon"].update_221e9e8_293($lepusTemplate, $n293, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["Discount"] = {
  variables: ["marketingInfoVO"],
  varUpdateState: [],
  update_256250_313: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Discount";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.discountVO.prefixIconUrl) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n314 = $update2 ? $lepusGetElementRefByLepusID("image", 314) : null;
            let $temp2 = $update2;
            if (!$n314) {
              $update2 = false;
              $n314 = __CreateImage($currentComponentId);
              let $nid314 = $lepusStoreElementRefByLepusID($n314, 314, "image");
              __SetAttribute($n314, 1004, $nid314[1]);
              __AppendElement($parent, $n314);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
              {
                __SetStyleObject($n314, [39, {
                  27: !!$data.marketingInfoVO.discountVO.ratio ? 9 * $data.marketingInfoVO.discountVO.ratio + "px" : "auto"
                }, getCssPropertyIDObj($data.marketingInfoVO.discountVO.prefixImageStyle)]);
              }
              {
                let $value = $data.marketingInfoVO.discountVO.prefixIconUrl;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.discountVO.prefixIconUrl) {
                  __SetAttribute($n314, "src", $value);
                }
              }
              {
                let _$value61 = !!!$data.marketingInfoVO.discountVO.ratio;
                if (!$update2 || _$value61 !== !!!$lepusTemplate._data.marketingInfoVO.discountVO.ratio) {
                  __SetAttribute($n314, "auto-size", _$value61);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp56 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n315 = $update2 ? $lepusGetElementRefByLepusID("image", 315) : null;
            let _$temp57 = $update2;
            if (!$n315) {
              $update2 = false;
              $n315 = __CreateImage($currentComponentId);
              let $nid315 = $lepusStoreElementRefByLepusID($n315, 315, "image");
              __SetAttribute($n315, 1004, $nid315[1]);
              __SetStyleObject($n315, [163, 39]);
              __SetAttribute($n315, "src", "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABkAAAAcCAYAAACUJBTQAAAACXBIWXMAAC4jAAAuIwF4pT92AAAAGXRFWHRTb2Z0d2FyZQB3d3cuaW5rc2NhcGUub3Jnm+48GgAAAmVJREFUSIm11ktIVFEcx/Hv745SQbTIMKiwRQ9E7oxgECVJ4S7CoNDxUVCbFoEtpL3kuoIww1oVET1mrE2U7dzkqqDSSYqix6La6CxCIVPPv4Vv595xrtp/d+/5n/u553/OuefKzPjfUbRmT6psOuCmppKS9grrZ8vwZfr6JlePJOrLnVOLpBZglyQADB2zkdJSD9oAFLlc5c3bKHKNJjsF7MuTmdVgqiQakmg4aqY2oBaIFdJFRgmZVLawcvmNZ0y6DaiwN5qLMiDrLQ8ky0zWtQJgFmFZxEQXsHEFAHiFIH5jM1AX0voLlAb6gcCJdTaNhM+Jn9xs4lpgm9Gr8dETfHo+DkC84aShFDkLwnZCnpE46QpQGkTIc61zAMBg+gni2dJEoTzl8pO1ws6G+JMM9HzJkR2fczIVNifVyQ0mbhG+moqJN1QvfpgkcTgn09gaiLjftAO7Q4CZvrpHZVMVAHtOb3J+sovg3T+GpMU7Pl6fMLzXQHE+ZEGMML281wW+jOyGN5BunR+JOjzDuxkBACgJA4Bhb7z4EiwsV3zoOHAwApA3ZHaBj/eHFyHOWc1aARi9ZNIPZy/nEM/jxxoRo3JT5xfemC+X4w7wdbWCTO0MPf4ejGRSWcWoQbxYhfGKCuvMhYMOLb+xzmTdwPYIwKSc28/7njdLG4I/K5lHT1U04RvWCbhCBDO7GgRAIcevnzw085mpCBf4pth6n3d3x4Kalz8ZM6mX+jNaZagDGA/IMHk6FwZA1L+VRH25mdcNHJm5MyHsIoPp6/m6Rf8lAojXJ1BsB3+9t3x48HO59JUhEeMfBwvUKWZFh3oAAAAASUVORK5CYII=");
              __AppendElement($parent, $n315);
            }
            $update2 = _$temp57;
          }
          $update2 = _$temp56;
        }
      }
    }
  },
  update_1387060_318: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Discount";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.discountVO.suffixText) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n319 = $update2 ? $lepusGetElementRefByLepusID("image", 319) : null;
            let $temp2 = $update2;
            if (!$n319) {
              $update2 = false;
              $n319 = __CreateImage($currentComponentId);
              let $nid319 = $lepusStoreElementRefByLepusID($n319, 319, "image");
              __SetAttribute($n319, 1004, $nid319[1]);
              __SetStyleObject($n319, [164, 165, 105, 31]);
              __SetAttribute($n319, "src", "https://test.com/obj/test/life/marketing/lynx/life_marketing_resource/marketing_expression_tool/super-discount/coupon-separator.png");
              __AppendElement($parent, $n319);
            }
            $update2 = $temp2;
          }
          {
            let $n320 = $update2 ? $lepusGetElementRefByLepusID("text", 320) : null;
            let _$temp58 = $update2;
            if (!$n320) {
              $update2 = false;
              $n320 = __CreateText($currentComponentId);
              let $nid320 = $lepusStoreElementRefByLepusID($n320, 320, "text");
              __SetAttribute($n320, 1004, $nid320[1]);
              __SetStyleObject($n320, [21, 117, 105, 139, 89]);
              __AppendElement($parent, $n320);
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let $value = $data.marketingInfoVO.discountVO.suffixText;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.discountVO.suffixText) {
                  __SetAttribute($n320, "text", $value);
                }
              }
            }
            $update2 = _$temp58;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, -1);
          $conditionNodeIndex[uniqueId] = -1;
        }
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "Discount";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n312 = $lepusGetElementRefByLepusID("view", 312);
      {
        let $value = $data.marketingInfoVO.discountVO.containerStyle;
        if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.discountVO.containerStyle) {
          __SetStyleObject($n312, [3, 4, 32, 161, 52, 31, 162, getCssPropertyIDObj($data.marketingInfoVO.discountVO.containerStyle)]);
        }
      }
    }
    let $n313 = $lepusGetElementRefByLepusID("if", 313);
    $renderTemplates[$path].update_256250_313($lepusTemplate, $n313, $data, $update2);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let _$value62 = $data.marketingInfoVO.discountVO.discountText;
      if (_$value62 !== $lepusTemplate._data.marketingInfoVO.discountVO.discountText) {
        let $n316 = $lepusGetElementRefByLepusID("text", 316);
        __SetAttribute($n316, "text", _$value62);
      }
    }
    let $n318 = $lepusGetElementRefByLepusID("if", 318);
    $renderTemplates[$path].update_1387060_318($lepusTemplate, $n318, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $path = "Discount";
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n312 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n312, 312, "view");
    __SetAttribute($n312, 1004, 312);
    __SetStyleObject($n312, [3, 4, 32, 161, 52, 31, 162, getCssPropertyIDObj($data.marketingInfoVO.discountVO.containerStyle)]);
    __AppendElement($template, $n312);
    let $n313 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n313, 313, "if");
    __AppendElement($n312, $n313);
    $renderTemplates[$path].update_256250_313($lepusTemplate, $n313, $data, $update2);
    let $n316 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n316, 316, "text");
    __SetAttribute($n316, 1004, 316);
    __SetStyleObject($n316, [21, 117, 105, 139, 89]);
    __AppendElement($n312, $n316);
    __SetAttribute($n316, "text", $data.marketingInfoVO.discountVO.discountText);
    let $n318 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n318, 318, "if");
    __AppendElement($n312, $n318);
    $renderTemplates[$path].update_1387060_318($lepusTemplate, $n318, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["Button"] = {
  variables: ["marketingInfoVO", "seckill"],
  varUpdateState: [],
  update_1387060_324: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Button";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.buttonVO.btnBackgroundImageUrl) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n325 = $update2 ? $lepusGetElementRefByLepusID("image", 325) : null;
            let $temp2 = $update2;
            if (!$n325) {
              $update2 = false;
              $n325 = __CreateImage($currentComponentId);
              let $nid325 = $lepusStoreElementRefByLepusID($n325, 325, "image");
              __SetAttribute($n325, 1004, $nid325[1]);
              __AppendElement($parent, $n325);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.buttonVO.btnBackgroundImageStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.buttonVO.btnBackgroundImageStyle) {
                  __SetStyleObject($n325, [168, 153, 55, 75, 1, 108, 135, 136, 111, getCssPropertyIDObj($data.marketingInfoVO.buttonVO.btnBackgroundImageStyle)]);
                }
              }
              {
                let _$value63 = $data.marketingInfoVO.buttonVO.btnBackgroundImageUrl;
                if (!$update2 || _$value63 !== $lepusTemplate._data.marketingInfoVO.buttonVO.btnBackgroundImageUrl) {
                  __SetAttribute($n325, "src", _$value63);
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
  update_1387060_333: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Button";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingInfoVO.buttonVO.btnSubText.length > 0) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n334 = $update2 ? $lepusGetElementRefByLepusID("text", 334) : null;
          let $temp2 = $update2;
          if (!$n334) {
            $update2 = false;
            $n334 = __CreateText($currentComponentId);
            let $nid334 = $lepusStoreElementRefByLepusID($n334, 334, "text");
            __SetAttribute($n334, 1004, $nid334[1]);
            __AppendElement($parent, $n334);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.marketingInfoVO.buttonVO.subTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.buttonVO.subTextStyle) {
                __SetStyleObject($n334, [63, 116, 18, 117, 118, getCssPropertyIDObj($data.marketingInfoVO.buttonVO.subTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value64 = $data.marketingInfoVO.buttonVO.subText;
              if (!$update2 || _$value64 !== $lepusTemplate._data.marketingInfoVO.buttonVO.subText) {
                __SetAttribute($n334, "text", _$value64);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp59 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n336 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 336) : null;
          let _$temp60 = $update2;
          if (!$n336) {
            $update2 = false;
            $n336 = __CreateElement("countdown-view", $currentComponentId);
            let $nid336 = $lepusStoreElementRefByLepusID($n336, 336, "countdown-view");
            __SetAttribute($n336, 1004, $nid336[1]);
            __SetAttribute($n336, "unit", "seconds");
            __SetID($n336, "countdown-seckill");
            __AppendElement($parent, $n336);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let _$value65 = $data.marketingInfoVO.buttonVO.subTextStyle;
              if (!$update2 || _$value65 !== $lepusTemplate._data.marketingInfoVO.buttonVO.subTextStyle) {
                __SetStyleObject($n336, [4, 8, 101, 119, 120, getCssPropertyIDObj($data.marketingInfoVO.buttonVO.subTextStyle)]);
              }
            }
            {
              let _$value66 = "" + $data.marketingInfoVO.buttonVO.secKillForSubText.end_time;
              if (!$update2 || _$value66 !== "" + $lepusTemplate._data.marketingInfoVO.buttonVO.secKillForSubText.end_time) {
                __SetAttribute($n336, "end-time", _$value66);
              }
            }
          }
          {
            let $n337 = $update2 ? $lepusGetElementRefByLepusID("text", 337) : null;
            let $temp3 = $update2;
            if (!$n337) {
              $update2 = false;
              $n337 = __CreateText($currentComponentId);
              let $nid337 = $lepusStoreElementRefByLepusID($n337, 337, "text");
              __SetAttribute($n337, 1004, $nid337[1]);
              __AppendElement($n336, $n337);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value67 = $data.marketingInfoVO.buttonVO.subTextStyle;
                if (!$update2 || _$value67 !== $lepusTemplate._data.marketingInfoVO.buttonVO.subTextStyle) {
                  __SetStyleObject($n337, [63, 116, 18, 117, 118, getCssPropertyIDObj($data.marketingInfoVO.buttonVO.subTextStyle)]);
                }
              }
            }
            __SetAttribute($n337, "text", "剩 ");
            $update2 = $temp3;
          }
          {
            let $n339 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 339) : null;
            let _$temp61 = $update2;
            if (!$n339) {
              $update2 = false;
              $n339 = __CreateElement("countdown-item", $currentComponentId);
              let $nid339 = $lepusStoreElementRefByLepusID($n339, 339, "countdown-item");
              __SetAttribute($n339, 1004, $nid339[1]);
              __SetAttribute($n339, "text-maxline", "1");
              __SetAttribute($n339, "countdown-display", "HH:mm:ss");
              __AppendElement($n336, $n339);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value68 = $data.marketingInfoVO.buttonVO.subTextStyle;
                if (!$update2 || _$value68 !== $lepusTemplate._data.marketingInfoVO.buttonVO.subTextStyle) {
                  __SetStyleObject($n339, [63, 116, 18, 117, 118, getCssPropertyIDObj($data.marketingInfoVO.buttonVO.subTextStyle)]);
                }
              }
            }
            $update2 = _$temp61;
          }
          $update2 = _$temp60;
        }
        $update2 = _$temp59;
      }
    }
  },
  update_393d908_327: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Button";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingInfoVO.buttonVO.textImageUrl) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n328 = $update2 ? $lepusGetElementRefByLepusID("image", 328) : null;
            let $temp2 = $update2;
            if (!$n328) {
              $update2 = false;
              $n328 = __CreateImage($currentComponentId);
              let $nid328 = $lepusStoreElementRefByLepusID($n328, 328, "image");
              __SetAttribute($n328, 1004, $nid328[1]);
              __SetStyleObject($n328, [169, 66]);
              __AppendElement($parent, $n328);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingInfoVO.buttonVO.textImageUrl;
                if (!$update2 || $value !== $lepusTemplate._data.marketingInfoVO.buttonVO.textImageUrl) {
                  __SetAttribute($n328, "src", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp62 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n329 = $update2 ? $lepusGetElementRefByLepusID("view", 329) : null;
            let _$temp63 = $update2;
            if (!$n329) {
              $update2 = false;
              $n329 = __CreateView($currentComponentId);
              let $nid329 = $lepusStoreElementRefByLepusID($n329, 329, "view");
              __SetAttribute($n329, 1004, $nid329[1]);
              __AppendElement($parent, $n329);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value69 = $data.marketingInfoVO.buttonVO.mainTextContainerStyle;
                if (!$update2 || _$value69 !== $lepusTemplate._data.marketingInfoVO.buttonVO.mainTextContainerStyle) {
                  __SetStyleObject($n329, [getCssPropertyIDObj($data.marketingInfoVO.buttonVO.mainTextContainerStyle)]);
                }
              }
            }
            {
              let $n330 = $update2 ? $lepusGetElementRefByLepusID("text", 330) : null;
              let $temp3 = $update2;
              if (!$n330) {
                $update2 = false;
                $n330 = __CreateText($currentComponentId);
                let $nid330 = $lepusStoreElementRefByLepusID($n330, 330, "text");
                __SetAttribute($n330, 1004, $nid330[1]);
                __AppendElement($n329, $n330);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value70 = $data.marketingInfoVO.buttonVO.mainTextStyle;
                  if (!$update2 || _$value70 !== $lepusTemplate._data.marketingInfoVO.buttonVO.mainTextStyle) {
                    __SetStyleObject($n330, [63, 117, 134, 21, getCssPropertyIDObj($data.marketingInfoVO.buttonVO.mainTextStyle)]);
                  }
                }
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let _$value71 = $data.marketingInfoVO.buttonVO.mainText;
                  if (!$update2 || _$value71 !== $lepusTemplate._data.marketingInfoVO.buttonVO.mainText) {
                    __SetAttribute($n330, "text", _$value71);
                  }
                }
              }
              $update2 = $temp3;
            }
            $update2 = _$temp63;
          }
          {
            let $template_update = $update2;
            let $n332 = $update2 ? $lepusGetElementRefByLepusID("if", 332) : null;
            if (!$n332) {
              $update2 = false;
              $n332 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n332, 332, "if");
              __AppendElement($parent, $n332);
            }
            $renderTemplates[$path].update_393d908_332($lepusTemplate, $n332, $data, $update2);
            $update2 = $template_update;
          }
          $update2 = _$temp62;
        }
      }
    }
  },
  update_393d908_332: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingInfoVO.buttonVO.showCountdown && ($data.seckill.curSecKillStatus == 2 || $data.marketingInfoVO.buttonVO.secKillForSubText.status == 2) && !$data.marketingInfoVO.buttonVO.isSoldOut || $data.marketingInfoVO.buttonVO.btnSubText.length > 0) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n333 = $update2 ? $lepusGetElementRefByLepusID("if", 333) : null;
          if (!$n333) {
            $update2 = false;
            $n333 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n333, 333, "if");
            __AppendElement($parent, $n333);
          }
          $renderTemplates["Button"].update_1387060_333($lepusTemplate, $n333, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "Button";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n322 = $lepusGetElementRefByLepusID("view", 322);
      {
        __SetStyleObject($n322, [4, 3, 97, 8, 166, {
          23: ($data.marketingInfoVO.buttonVO.showSoldOut && $data.marketingInfoVO.buttonVO.isSoldOut ? 0.5 : 1) + ""
        }, getCssPropertyIDObj($data.marketingInfoVO.buttonVO.containerStyle)]);
      }
    }
    let $n324 = $lepusGetElementRefByLepusID("if", 324);
    $renderTemplates[$path].update_1387060_324($lepusTemplate, $n324, $data, $update2);
    let $n327 = $lepusGetElementRefByLepusID("if", 327);
    $renderTemplates[$path].update_393d908_327($lepusTemplate, $n327, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $path = "Button";
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n322 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n322, 322, "view");
    __SetAttribute($n322, 1004, 322);
    __SetStyleObject($n322, [4, 3, 97, 8, 166, {
      23: ($data.marketingInfoVO.buttonVO.showSoldOut && $data.marketingInfoVO.buttonVO.isSoldOut ? 0.5 : 1) + ""
    }, getCssPropertyIDObj($data.marketingInfoVO.buttonVO.containerStyle)]);
    __AddEvent($n322, "catchEvent", "tap", "handleBuyButtonClick");
    __AppendElement($template, $n322);
    let $n323 = __CreateView($currentComponentId);
    __SetAttribute($n323, 1004, 323);
    __SetStyleObject($n323, [76, 75, 1, 0, 55, 167, 98]);
    __AppendElement($n322, $n323);
    let $n324 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n324, 324, "if");
    __AppendElement($n322, $n324);
    $renderTemplates[$path].update_1387060_324($lepusTemplate, $n324, $data, $update2);
    let $n326 = __CreateView($currentComponentId);
    __SetAttribute($n326, 1004, 326);
    __SetStyleObject($n326, [3, 4, 8, 100]);
    __AppendElement($n322, $n326);
    let $n327 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n327, 327, "if");
    __AppendElement($n326, $n327);
    $renderTemplates[$path].update_393d908_327($lepusTemplate, $n327, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["SecKillPre"] = {
  variables: ["marketingInfoVO"],
  varUpdateState: [],
  update: function ($lepusTemplate, $data, $array, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n;
    let $path = "SecKillPre";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n342 = $lepusGetElementRefByLepusID("image", 342);
      {
        let $value = (_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.prefixImage;
        if (!$update2 || $value !== ((_d = (_c = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _c.secKillProgressVO) == null ? undefined : _d.prefixImage)) {
          __SetAttribute($n342, "src", $value);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let _$value72 = (_f = (_e = $data.marketingInfoVO) == null ? undefined : _e.secKillProgressVO) == null ? undefined : _f.secKillTips;
      if (_$value72 !== ((_h = (_g = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _g.secKillProgressVO) == null ? undefined : _h.secKillTips)) {
        let $n343 = $lepusGetElementRefByLepusID("text", 343);
        __SetAttribute($n343, "text", _$value72);
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n346 = $lepusGetElementRefByLepusID("countdown-view", 346);
      {
        let _$value73 = "" + ((_k = (_j = (_i = $data.marketingInfoVO) == null ? undefined : _i.secKillProgressVO) == null ? undefined : _j.secKillInfo) == null ? undefined : _k.start_time);
        if (!$update2 || _$value73 !== "" + ((_n = (_m = (_l = $lepusTemplate._data.marketingInfoVO) == null ? undefined : _l.secKillProgressVO) == null ? undefined : _m.secKillInfo) == null ? undefined : _n.start_time)) {
          __SetAttribute($n346, "end-time", _$value73);
        }
      }
    }
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data) {
    let _a, _b, _c, _d, _e, _f, _g;
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n340 = __CreateView($currentComponentId);
    __SetAttribute($n340, 1004, 340);
    __SetStyleObject($n340, [31, 170, 171, 3, 87, 4, 52, 74, 172, 173]);
    __AppendElement($template, $n340);
    let $n341 = __CreateView($currentComponentId);
    __SetAttribute($n341, 1004, 341);
    __SetStyleObject($n341, [3, 1, 4, 31, 101]);
    __AppendElement($n340, $n341);
    let $n342 = __CreateImage($currentComponentId);
    $lepusStoreElementRefByLepusID($n342, 342, "image");
    __SetAttribute($n342, 1004, 342);
    __SetStyleObject($n342, [174, 52, 7, 31]);
    __SetAttribute($n342, "src", (_b = (_a = $data.marketingInfoVO) == null ? undefined : _a.secKillProgressVO) == null ? undefined : _b.prefixImage);
    __AppendElement($n341, $n342);
    let $n343 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n343, 343, "text");
    __SetAttribute($n343, 1004, 343);
    __SetStyleObject($n343, [89, 117, 139, 21, 31]);
    __AppendElement($n341, $n343);
    __SetAttribute($n343, "text", (_d = (_c = $data.marketingInfoVO) == null ? undefined : _c.secKillProgressVO) == null ? undefined : _d.secKillTips);
    let $n345 = __CreateView($currentComponentId);
    __SetAttribute($n345, 1004, 345);
    __SetStyleObject($n345, [175, 105, 31]);
    __AppendElement($n340, $n345);
    let $n346 = __CreateElement("countdown-view", $currentComponentId);
    $lepusStoreElementRefByLepusID($n346, 346, "countdown-view");
    __SetAttribute($n346, 1004, 346);
    __SetStyleObject($n346, [4, 8, 101, 119, 120]);
    __SetAttribute($n346, "unit", "seconds");
    __SetAttribute($n346, "end-time", "" + ((_g = (_f = (_e = $data.marketingInfoVO) == null ? undefined : _e.secKillProgressVO) == null ? undefined : _f.secKillInfo) == null ? undefined : _g.start_time));
    __SetID($n346, "countdown-pre");
    __AddEvent($n346, "bindEvent", "countdownend", "onPreCountDownEnd");
    __AppendElement($n345, $n346);
    let $n347 = __CreateElement("countdown-item", $currentComponentId);
    __SetAttribute($n347, 1004, 347);
    __SetStyleObject($n347, [176, 117, 139, 177, 41]);
    __SetAttribute($n347, "text-maxline", "1");
    __SetAttribute($n347, "countdown-display", "HH:mm:ss");
    __AppendElement($n346, $n347);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["ProcessCompV2"] = {
  variables: ["marketingV2VO", "priceInfoState", "tagCountdownState", "tagMoreIconState", "tagAnimateProgressState", "tagSecKillCountDownState"],
  varUpdateState: [],
  update_10a53b0_383: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showRegularPrice) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n384 = $update2 ? $lepusGetElementRefByLepusID("template", 384) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n384) {
            $templateCreated = false;
            $n384 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n384, 384, "template");
            __AppendElement($parent, $n384);
            $templateId = __GetElementUniqueID($n384);
            $childLepusTemplate = $templateConstructor($templateId, $n384);
          } else {
            $templateId = __GetElementUniqueID($n384);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.regularPriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceFinal"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceFinal"].entry($n384, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        {
          let $n385 = $update2 ? $lepusGetElementRefByLepusID("template", 385) : null;
          let _$templateCreated4 = true;
          let _$childLepusTemplate4 = null;
          let _$templateId4 = null;
          if (!$n385) {
            _$templateCreated4 = false;
            $n385 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n385, 385, "template");
            __AppendElement($parent, $n385);
            _$templateId4 = __GetElementUniqueID($n385);
            _$childLepusTemplate4 = $templateConstructor(_$templateId4, $n385);
          } else {
            _$templateId4 = __GetElementUniqueID($n385);
            _$childLepusTemplate4 = $templateInfo[_$templateId4];
          }
          $updatePropsSet = [];
          _$childLepusTemplate4.setData("vo", $data.marketingV2VO.priceInfoVO.regularPriceBorderVO, $update2);
          if (_$templateCreated4) {
            let _$update_keys4 = $updatePropsSet;
            if (_$update_keys4.length > 0) {
              $renderTemplates["PriceBorder"].update(_$childLepusTemplate4, _$childLepusTemplate4.data, _$update_keys4, true);
            }
          } else {
            $renderTemplates["PriceBorder"].entry($n385, _$childLepusTemplate4, _$childLepusTemplate4.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp64 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n386 = $update2 ? $lepusGetElementRefByLepusID("template", 386) : null;
          let _$templateCreated5 = true;
          let _$childLepusTemplate5 = null;
          let _$templateId5 = null;
          if (!$n386) {
            _$templateCreated5 = false;
            $n386 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n386, 386, "template");
            __AppendElement($parent, $n386);
            _$templateId5 = __GetElementUniqueID($n386);
            _$childLepusTemplate5 = $templateConstructor(_$templateId5, $n386);
          } else {
            _$templateId5 = __GetElementUniqueID($n386);
            _$childLepusTemplate5 = $templateInfo[_$templateId5];
          }
          $updatePropsSet = [];
          _$childLepusTemplate5.setData("vo", $data.marketingV2VO.priceInfoVO.finalPriceVO, $update2);
          if (_$templateCreated5) {
            let _$update_keys5 = $updatePropsSet;
            if (_$update_keys5.length > 0) {
              $renderTemplates["PriceFinal"].update(_$childLepusTemplate5, _$childLepusTemplate5.data, _$update_keys5, true);
            }
          } else {
            $renderTemplates["PriceFinal"].entry($n386, _$childLepusTemplate5, _$childLepusTemplate5.data, false);
          }
        }
        $update2 = _$temp64;
      }
    }
  },
  update_10a53b0_387: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showDeductionPrice) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n388 = $update2 ? $lepusGetElementRefByLepusID("template", 388) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n388) {
            $templateCreated = false;
            $n388 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n388, 388, "template");
            __AppendElement($parent, $n388);
            $templateId = __GetElementUniqueID($n388);
            $childLepusTemplate = $templateConstructor($templateId, $n388);
          } else {
            $templateId = __GetElementUniqueID($n388);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.deductionPriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceDeduction"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceDeduction"].entry($n388, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_10a53b0_398: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.layoutVO.showShelfButton) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n399 = $update2 ? $lepusGetElementRefByLepusID("view", 399) : null;
          let $temp2 = $update2;
          if (!$n399) {
            $update2 = false;
            $n399 = __CreateView($currentComponentId);
            let $nid399 = $lepusStoreElementRefByLepusID($n399, 399, "view");
            __SetAttribute($n399, 1004, $nid399[1]);
            __SetStyleObject($n399, [31, 124, 31, 126]);
            __AppendElement($parent, $n399);
          }
          {
            let $n400 = $update2 ? $lepusGetElementRefByLepusID("template", 400) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n400) {
              $templateCreated = false;
              $n400 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n400, 400, "template");
              __AppendElement($n399, $n400);
              $templateId = __GetElementUniqueID($n400);
              $childLepusTemplate = $templateConstructor($templateId, $n400);
            } else {
              $templateId = __GetElementUniqueID($n400);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("vo", $data.marketingV2VO.shelfButtonVO, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["ShelfButton"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["ShelfButton"].entry($n400, $childLepusTemplate, $childLepusTemplate.data, false);
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
  update_10a53b0_408: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showRegularPrice) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n409 = $update2 ? $lepusGetElementRefByLepusID("template", 409) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n409) {
            $templateCreated = false;
            $n409 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n409, 409, "template");
            __AppendElement($parent, $n409);
            $templateId = __GetElementUniqueID($n409);
            $childLepusTemplate = $templateConstructor($templateId, $n409);
          } else {
            $templateId = __GetElementUniqueID($n409);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.regularPriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceFinal"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceFinal"].entry($n409, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        {
          let $n410 = $update2 ? $lepusGetElementRefByLepusID("template", 410) : null;
          let _$templateCreated6 = true;
          let _$childLepusTemplate6 = null;
          let _$templateId6 = null;
          if (!$n410) {
            _$templateCreated6 = false;
            $n410 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n410, 410, "template");
            __AppendElement($parent, $n410);
            _$templateId6 = __GetElementUniqueID($n410);
            _$childLepusTemplate6 = $templateConstructor(_$templateId6, $n410);
          } else {
            _$templateId6 = __GetElementUniqueID($n410);
            _$childLepusTemplate6 = $templateInfo[_$templateId6];
          }
          $updatePropsSet = [];
          _$childLepusTemplate6.setData("vo", $data.marketingV2VO.priceInfoVO.regularPriceBorderVO, $update2);
          if (_$templateCreated6) {
            let _$update_keys6 = $updatePropsSet;
            if (_$update_keys6.length > 0) {
              $renderTemplates["PriceBorder"].update(_$childLepusTemplate6, _$childLepusTemplate6.data, _$update_keys6, true);
            }
          } else {
            $renderTemplates["PriceBorder"].entry($n410, _$childLepusTemplate6, _$childLepusTemplate6.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp65 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n411 = $update2 ? $lepusGetElementRefByLepusID("template", 411) : null;
          let _$templateCreated7 = true;
          let _$childLepusTemplate7 = null;
          let _$templateId7 = null;
          if (!$n411) {
            _$templateCreated7 = false;
            $n411 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n411, 411, "template");
            __AppendElement($parent, $n411);
            _$templateId7 = __GetElementUniqueID($n411);
            _$childLepusTemplate7 = $templateConstructor(_$templateId7, $n411);
          } else {
            _$templateId7 = __GetElementUniqueID($n411);
            _$childLepusTemplate7 = $templateInfo[_$templateId7];
          }
          $updatePropsSet = [];
          _$childLepusTemplate7.setData("vo", $data.marketingV2VO.priceInfoVO.finalPriceVO, $update2);
          if (_$templateCreated7) {
            let _$update_keys7 = $updatePropsSet;
            if (_$update_keys7.length > 0) {
              $renderTemplates["PriceFinal"].update(_$childLepusTemplate7, _$childLepusTemplate7.data, _$update_keys7, true);
            }
          } else {
            $renderTemplates["PriceFinal"].entry($n411, _$childLepusTemplate7, _$childLepusTemplate7.data, false);
          }
        }
        $update2 = _$temp65;
      }
    }
  },
  update_10a53b0_412: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showDeductionPrice) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n413 = $update2 ? $lepusGetElementRefByLepusID("template", 413) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n413) {
            $templateCreated = false;
            $n413 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n413, 413, "template");
            __AppendElement($parent, $n413);
            $templateId = __GetElementUniqueID($n413);
            $childLepusTemplate = $templateConstructor($templateId, $n413);
          } else {
            $templateId = __GetElementUniqueID($n413);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.deductionPriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceDeduction"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceDeduction"].entry($n413, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_10a53b0_426: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.layoutVO.showShelfButton) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n427 = $update2 ? $lepusGetElementRefByLepusID("view", 427) : null;
          let $temp2 = $update2;
          if (!$n427) {
            $update2 = false;
            $n427 = __CreateView($currentComponentId);
            let $nid427 = $lepusStoreElementRefByLepusID($n427, 427, "view");
            __SetAttribute($n427, 1004, $nid427[1]);
            __SetStyleObject($n427, [31, 124, 31, 126]);
            __AppendElement($parent, $n427);
          }
          {
            let $n428 = $update2 ? $lepusGetElementRefByLepusID("template", 428) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n428) {
              $templateCreated = false;
              $n428 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n428, 428, "template");
              __AppendElement($n427, $n428);
              $templateId = __GetElementUniqueID($n428);
              $childLepusTemplate = $templateConstructor($templateId, $n428);
            } else {
              $templateId = __GetElementUniqueID($n428);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("vo", $data.marketingV2VO.shelfButtonVO, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["ShelfButton"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["ShelfButton"].entry($n428, $childLepusTemplate, $childLepusTemplate.data, false);
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
  update_1ee56c8_379: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c;
    let $path = "ProcessCompV2";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.layoutVO.showPriceInfo) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n380 = $update2 ? $lepusGetElementRefByLepusID("view", 380) : null;
          let $temp2 = $update2;
          if (!$n380) {
            $update2 = false;
            $n380 = __CreateView($currentComponentId);
            let $nid380 = $lepusStoreElementRefByLepusID($n380, 380, "view");
            __SetAttribute($n380, 1004, $nid380[1]);
            __SetStyleObject($n380, [3, 104, 31, 105]);
            __AppendElement($parent, $n380);
          }
          {
            let $n381 = $update2 ? $lepusGetElementRefByLepusID("view", 381) : null;
            let $temp3 = $update2;
            if (!$n381) {
              $update2 = false;
              $n381 = __CreateView($currentComponentId);
              let $nid381 = $lepusStoreElementRefByLepusID($n381, 381, "view");
              __SetAttribute($n381, 1004, $nid381[1]);
              __SetStyleObject($n381, [101, 119, 1]);
              __SetID($n381, "priceInfoWrapper");
              __AppendElement($n380, $n381);
            }
            {
              let $n382 = $update2 ? $lepusGetElementRefByLepusID("view", 382) : null;
              let $temp4 = $update2;
              if (!$n382) {
                $update2 = false;
                $n382 = __CreateView($currentComponentId);
                let $nid382 = $lepusStoreElementRefByLepusID($n382, 382, "view");
                __SetAttribute($n382, 1004, $nid382[1]);
                __SetID($n382, "priceInfoInner");
                __AppendElement($n381, $n382);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[6]) {
                {
                  let $value = "width:" + ((!!((_a = $data.priceInfoState) == null ? undefined : _a.resetInnerWidth) ? "100%" : "max-content") + ";") + "display:flex;flex-direction:row;align-items:baseline;overflow:visible;flex-wrap:wrap;";
                  if (!$update2 || $value !== "width:" + ((!!((_b = $lepusTemplate._data.priceInfoState) == null ? undefined : _b.resetInnerWidth) ? "100%" : "max-content") + ";") + "display:flex;flex-direction:row;align-items:baseline;overflow:visible;flex-wrap:wrap;") {
                    __SetStyleObject($n382, [3, 44, 104, 101, 103, {
                      27: !!((_c = $data.priceInfoState) == null ? undefined : _c.resetInnerWidth) ? "100%" : "max-content"
                    }]);
                  }
                }
              }
              {
                let $template_update = $update2;
                let $n383 = $update2 ? $lepusGetElementRefByLepusID("if", 383) : null;
                if (!$n383) {
                  $update2 = false;
                  $n383 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n383, 383, "if");
                  __AppendElement($n382, $n383);
                }
                $renderTemplates[$path].update_10a53b0_383($lepusTemplate, $n383, $data, $update2);
                $update2 = $template_update;
              }
              {
                let _$template_update13 = $update2;
                let $n387 = $update2 ? $lepusGetElementRefByLepusID("if", 387) : null;
                if (!$n387) {
                  $update2 = false;
                  $n387 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n387, 387, "if");
                  __AppendElement($n382, $n387);
                }
                $renderTemplates[$path].update_10a53b0_387($lepusTemplate, $n387, $data, $update2);
                $update2 = _$template_update13;
              }
              {
                let _$template_update14 = $update2;
                let $n389 = $update2 ? $lepusGetElementRefByLepusID("if", 389) : null;
                if (!$n389) {
                  $update2 = false;
                  $n389 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n389, 389, "if");
                  __AppendElement($n382, $n389);
                }
                $renderTemplates[$path].update_1ee56c8_389($lepusTemplate, $n389, $data, $update2);
                $update2 = _$template_update14;
              }
              {
                let _$template_update15 = $update2;
                let $n391 = $update2 ? $lepusGetElementRefByLepusID("if", 391) : null;
                if (!$n391) {
                  $update2 = false;
                  $n391 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n391, 391, "if");
                  __AppendElement($n382, $n391);
                }
                $renderTemplates[$path].update_1ee56c8_391($lepusTemplate, $n391, $data, $update2);
                $update2 = _$template_update15;
              }
              {
                let _$template_update16 = $update2;
                let $n393 = $update2 ? $lepusGetElementRefByLepusID("if", 393) : null;
                if (!$n393) {
                  $update2 = false;
                  $n393 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n393, 393, "if");
                  __AppendElement($n382, $n393);
                }
                $renderTemplates[$path].update_1ee56c8_393($lepusTemplate, $n393, $data, $update2);
                $update2 = _$template_update16;
              }
              $update2 = $temp4;
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
  },
  update_1ee56c8_389: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showComparePrice && !!!((_a = $data.priceInfoState) == null ? undefined : _a.hiddenComparePrice)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n390 = $update2 ? $lepusGetElementRefByLepusID("template", 390) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n390) {
            $templateCreated = false;
            $n390 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n390, 390, "template");
            __AppendElement($parent, $n390);
            $templateId = __GetElementUniqueID($n390);
            $childLepusTemplate = $templateConstructor($templateId, $n390);
          } else {
            $templateId = __GetElementUniqueID($n390);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.comparePriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceText"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceText"].entry($n390, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1ee56c8_391: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showIntroducePrice && !!!((_a = $data.priceInfoState) == null ? undefined : _a.hiddenIntroducePrice)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n392 = $update2 ? $lepusGetElementRefByLepusID("template", 392) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n392) {
            $templateCreated = false;
            $n392 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n392, 392, "template");
            __AppendElement($parent, $n392);
            $templateId = __GetElementUniqueID($n392);
            $childLepusTemplate = $templateConstructor($templateId, $n392);
          } else {
            $templateId = __GetElementUniqueID($n392);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.introducePriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceText"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceText"].entry($n392, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1ee56c8_393: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showOriginPrice && !!!((_a = $data.priceInfoState) == null ? undefined : _a.hiddenOriginPrice)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n394 = $update2 ? $lepusGetElementRefByLepusID("template", 394) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n394) {
            $templateCreated = false;
            $n394 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n394, 394, "template");
            __AppendElement($parent, $n394);
            $templateId = __GetElementUniqueID($n394);
            $childLepusTemplate = $templateConstructor($templateId, $n394);
          } else {
            $templateId = __GetElementUniqueID($n394);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.originPriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceText"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceText"].entry($n394, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1ee56c8_404: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c;
    let $path = "ProcessCompV2";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.layoutVO.showPriceInfo) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n405 = $update2 ? $lepusGetElementRefByLepusID("view", 405) : null;
          let $temp2 = $update2;
          if (!$n405) {
            $update2 = false;
            $n405 = __CreateView($currentComponentId);
            let $nid405 = $lepusStoreElementRefByLepusID($n405, 405, "view");
            __SetAttribute($n405, 1004, $nid405[1]);
            __SetStyleObject($n405, [3, 104, 31, 105]);
            __AppendElement($parent, $n405);
          }
          {
            let $n406 = $update2 ? $lepusGetElementRefByLepusID("view", 406) : null;
            let $temp3 = $update2;
            if (!$n406) {
              $update2 = false;
              $n406 = __CreateView($currentComponentId);
              let $nid406 = $lepusStoreElementRefByLepusID($n406, 406, "view");
              __SetAttribute($n406, 1004, $nid406[1]);
              __SetStyleObject($n406, [101, 119, 1]);
              __SetID($n406, "priceInfoWrapper");
              __AppendElement($n405, $n406);
            }
            {
              let $n407 = $update2 ? $lepusGetElementRefByLepusID("view", 407) : null;
              let $temp4 = $update2;
              if (!$n407) {
                $update2 = false;
                $n407 = __CreateView($currentComponentId);
                let $nid407 = $lepusStoreElementRefByLepusID($n407, 407, "view");
                __SetAttribute($n407, 1004, $nid407[1]);
                __SetID($n407, "priceInfoInner");
                __AppendElement($n406, $n407);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[6]) {
                {
                  let $value = "width:" + ((!!((_a = $data.priceInfoState) == null ? undefined : _a.resetInnerWidth) ? "100%" : "max-content") + ";") + "display:flex;flex-direction:row;align-items:baseline;overflow:visible;flex-wrap:wrap;";
                  if (!$update2 || $value !== "width:" + ((!!((_b = $lepusTemplate._data.priceInfoState) == null ? undefined : _b.resetInnerWidth) ? "100%" : "max-content") + ";") + "display:flex;flex-direction:row;align-items:baseline;overflow:visible;flex-wrap:wrap;") {
                    __SetStyleObject($n407, [3, 44, 104, 101, 103, {
                      27: !!((_c = $data.priceInfoState) == null ? undefined : _c.resetInnerWidth) ? "100%" : "max-content"
                    }]);
                  }
                }
              }
              {
                let $template_update = $update2;
                let $n408 = $update2 ? $lepusGetElementRefByLepusID("if", 408) : null;
                if (!$n408) {
                  $update2 = false;
                  $n408 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n408, 408, "if");
                  __AppendElement($n407, $n408);
                }
                $renderTemplates[$path].update_10a53b0_408($lepusTemplate, $n408, $data, $update2);
                $update2 = $template_update;
              }
              {
                let _$template_update17 = $update2;
                let $n412 = $update2 ? $lepusGetElementRefByLepusID("if", 412) : null;
                if (!$n412) {
                  $update2 = false;
                  $n412 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n412, 412, "if");
                  __AppendElement($n407, $n412);
                }
                $renderTemplates[$path].update_10a53b0_412($lepusTemplate, $n412, $data, $update2);
                $update2 = _$template_update17;
              }
              {
                let _$template_update18 = $update2;
                let $n414 = $update2 ? $lepusGetElementRefByLepusID("if", 414) : null;
                if (!$n414) {
                  $update2 = false;
                  $n414 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n414, 414, "if");
                  __AppendElement($n407, $n414);
                }
                $renderTemplates[$path].update_1ee56c8_414($lepusTemplate, $n414, $data, $update2);
                $update2 = _$template_update18;
              }
              {
                let _$template_update19 = $update2;
                let $n416 = $update2 ? $lepusGetElementRefByLepusID("if", 416) : null;
                if (!$n416) {
                  $update2 = false;
                  $n416 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n416, 416, "if");
                  __AppendElement($n407, $n416);
                }
                $renderTemplates[$path].update_1ee56c8_416($lepusTemplate, $n416, $data, $update2);
                $update2 = _$template_update19;
              }
              {
                let _$template_update20 = $update2;
                let $n418 = $update2 ? $lepusGetElementRefByLepusID("if", 418) : null;
                if (!$n418) {
                  $update2 = false;
                  $n418 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n418, 418, "if");
                  __AppendElement($n407, $n418);
                }
                $renderTemplates[$path].update_1ee56c8_418($lepusTemplate, $n418, $data, $update2);
                $update2 = _$template_update20;
              }
              $update2 = $temp4;
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
  },
  update_1ee56c8_414: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showComparePrice && !!!((_a = $data.priceInfoState) == null ? undefined : _a.hiddenComparePrice)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n415 = $update2 ? $lepusGetElementRefByLepusID("template", 415) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n415) {
            $templateCreated = false;
            $n415 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n415, 415, "template");
            __AppendElement($parent, $n415);
            $templateId = __GetElementUniqueID($n415);
            $childLepusTemplate = $templateConstructor($templateId, $n415);
          } else {
            $templateId = __GetElementUniqueID($n415);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.comparePriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceText"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceText"].entry($n415, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1ee56c8_416: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showIntroducePrice && !!!((_a = $data.priceInfoState) == null ? undefined : _a.hiddenIntroducePrice)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n417 = $update2 ? $lepusGetElementRefByLepusID("template", 417) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n417) {
            $templateCreated = false;
            $n417 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n417, 417, "template");
            __AppendElement($parent, $n417);
            $templateId = __GetElementUniqueID($n417);
            $childLepusTemplate = $templateConstructor($templateId, $n417);
          } else {
            $templateId = __GetElementUniqueID($n417);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.introducePriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceText"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceText"].entry($n417, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1ee56c8_418: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.priceInfoVO.showOriginPrice && !!!((_a = $data.priceInfoState) == null ? undefined : _a.hiddenOriginPrice)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n419 = $update2 ? $lepusGetElementRefByLepusID("template", 419) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n419) {
            $templateCreated = false;
            $n419 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n419, 419, "template");
            __AppendElement($parent, $n419);
            $templateId = __GetElementUniqueID($n419);
            $childLepusTemplate = $templateConstructor($templateId, $n419);
          } else {
            $templateId = __GetElementUniqueID($n419);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.marketingV2VO.priceInfoVO.originPriceVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["PriceText"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["PriceText"].entry($n419, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_1cfa368_395: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.layoutVO.showTagInfo) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n396 = $update2 ? $lepusGetElementRefByLepusID("view", 396) : null;
          let $temp2 = $update2;
          if (!$n396) {
            $update2 = false;
            $n396 = __CreateView($currentComponentId);
            let $nid396 = $lepusStoreElementRefByLepusID($n396, 396, "view");
            __SetAttribute($n396, 1004, $nid396[1]);
            __SetStyleObject($n396, [31, 3, 179, 180, 181]);
            __AppendElement($parent, $n396);
          }
          if (!$update2 || $renderTemplates["ProcessCompV2"].varUpdateState[0]) {
            {
              let $value = "tag-info-container-" + $data.marketingV2VO.tagInfoVO.type;
              if (!$update2 || $value !== "tag-info-container-" + $lepusTemplate._data.marketingV2VO.tagInfoVO.type) {
                __SetID($n396, $value);
              }
            }
          }
          {
            let $n397 = $update2 ? $lepusGetElementRefByLepusID("template", 397) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n397) {
              $templateCreated = false;
              $n397 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n397, 397, "template");
              __AppendElement($n396, $n397);
              $templateId = __GetElementUniqueID($n397);
              $childLepusTemplate = $templateConstructor($templateId, $n397);
            } else {
              $templateId = __GetElementUniqueID($n397);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("vo", $data.marketingV2VO.tagInfoVO, $update2);
            $childLepusTemplate.setData("tagCountdownState", $data.tagCountdownState, $update2);
            $childLepusTemplate.setData("tagMoreIconState", $data.tagMoreIconState, $update2);
            $childLepusTemplate.setData("tagAnimateProgressState", $data.tagAnimateProgressState, $update2);
            $childLepusTemplate.setData("tagSecKillCountDownState", $data.tagSecKillCountDownState, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["TagInfo"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["TagInfo"].entry($n397, $childLepusTemplate, $childLepusTemplate.data, false);
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
  update_1cfa368_420: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.layoutVO.showLowPriceTag) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n421 = $update2 ? $lepusGetElementRefByLepusID("view", 421) : null;
          let $temp2 = $update2;
          if (!$n421) {
            $update2 = false;
            $n421 = __CreateView($currentComponentId);
            let $nid421 = $lepusStoreElementRefByLepusID($n421, 421, "view");
            __SetAttribute($n421, 1004, $nid421[1]);
            __SetStyleObject($n421, [31, 3, 179, 180, 181]);
            __AppendElement($parent, $n421);
          }
          if (!$update2 || $renderTemplates["ProcessCompV2"].varUpdateState[0]) {
            {
              let $value = "low-price-container-" + $data.marketingV2VO.lowPriceVO.type;
              if (!$update2 || $value !== "low-price-container-" + $lepusTemplate._data.marketingV2VO.lowPriceVO.type) {
                __SetID($n421, $value);
              }
            }
          }
          {
            let $n422 = $update2 ? $lepusGetElementRefByLepusID("template", 422) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n422) {
              $templateCreated = false;
              $n422 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n422, 422, "template");
              __AppendElement($n421, $n422);
              $templateId = __GetElementUniqueID($n422);
              $childLepusTemplate = $templateConstructor($templateId, $n422);
            } else {
              $templateId = __GetElementUniqueID($n422);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("vo", $data.marketingV2VO.lowPriceVO, $update2);
            $childLepusTemplate.setData("tagCountdownState", $data.tagCountdownState, $update2);
            $childLepusTemplate.setData("tagMoreIconState", $data.tagMoreIconState, $update2);
            $childLepusTemplate.setData("tagAnimateProgressState", $data.tagAnimateProgressState, $update2);
            $childLepusTemplate.setData("tagSecKillCountDownState", $data.tagSecKillCountDownState, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["TagInfo"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["TagInfo"].entry($n422, $childLepusTemplate, $childLepusTemplate.data, false);
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
  update_1cfa368_423: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.marketingV2VO.layoutVO.showTagInfo) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n424 = $update2 ? $lepusGetElementRefByLepusID("view", 424) : null;
          let $temp2 = $update2;
          if (!$n424) {
            $update2 = false;
            $n424 = __CreateView($currentComponentId);
            let $nid424 = $lepusStoreElementRefByLepusID($n424, 424, "view");
            __SetAttribute($n424, 1004, $nid424[1]);
            __SetStyleObject($n424, [182, 3, 44, 4, 0]);
            __AppendElement($parent, $n424);
          }
          if (!$update2 || $renderTemplates["ProcessCompV2"].varUpdateState[0]) {
            {
              let $value = "tag-info-container-" + $data.marketingV2VO.tagInfoVO.type;
              if (!$update2 || $value !== "tag-info-container-" + $lepusTemplate._data.marketingV2VO.tagInfoVO.type) {
                __SetID($n424, $value);
              }
            }
          }
          {
            let $n425 = $update2 ? $lepusGetElementRefByLepusID("template", 425) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n425) {
              $templateCreated = false;
              $n425 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n425, 425, "template");
              __AppendElement($n424, $n425);
              $templateId = __GetElementUniqueID($n425);
              $childLepusTemplate = $templateConstructor($templateId, $n425);
            } else {
              $templateId = __GetElementUniqueID($n425);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("vo", $data.marketingV2VO.tagInfoVO, $update2);
            $childLepusTemplate.setData("tagCountdownState", $data.tagCountdownState, $update2);
            $childLepusTemplate.setData("tagMoreIconState", $data.tagMoreIconState, $update2);
            $childLepusTemplate.setData("tagAnimateProgressState", $data.tagAnimateProgressState, $update2);
            $childLepusTemplate.setData("tagSecKillCountDownState", $data.tagSecKillCountDownState, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["TagInfo"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["TagInfo"].entry($n425, $childLepusTemplate, $childLepusTemplate.data, false);
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
  update_cffa30_376: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "ProcessCompV2";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[6] || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[3] || $renderTemplates[$path].varUpdateState[4] || $renderTemplates[$path].varUpdateState[5]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.marketingV2VO.layoutVO.styleType == "one_line") {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n377 = $update2 ? $lepusGetElementRefByLepusID("view", 377) : null;
            let $temp2 = $update2;
            if (!$n377) {
              $update2 = false;
              $n377 = __CreateView($currentComponentId);
              let $nid377 = $lepusStoreElementRefByLepusID($n377, 377, "view");
              __SetAttribute($n377, 1004, $nid377[1]);
              __AppendElement($parent, $n377);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.marketingV2VO.layoutVO.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.marketingV2VO.layoutVO.containerStyle) {
                  __SetStyleObject($n377, [0, 3, 44, 79, 62, getCssPropertyIDObj($data.marketingV2VO.layoutVO.containerStyle)]);
                }
              }
            }
            {
              let $n378 = $update2 ? $lepusGetElementRefByLepusID("view", 378) : null;
              let $temp3 = $update2;
              if (!$n378) {
                $update2 = false;
                $n378 = __CreateView($currentComponentId);
                let $nid378 = $lepusStoreElementRefByLepusID($n378, 378, "view");
                __SetAttribute($n378, 1004, $nid378[1]);
                __AppendElement($n377, $n378);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value74 = $data.marketingV2VO.layoutVO.priceContainerStyle;
                  if (!$update2 || _$value74 !== $lepusTemplate._data.marketingV2VO.layoutVO.priceContainerStyle) {
                    __SetStyleObject($n378, [14, 3, 44, 104, 0, 66, 103, 45, getCssPropertyIDObj($data.marketingV2VO.layoutVO.priceContainerStyle)]);
                  }
                }
              }
              {
                let $template_update = $update2;
                let $n379 = $update2 ? $lepusGetElementRefByLepusID("if", 379) : null;
                if (!$n379) {
                  $update2 = false;
                  $n379 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n379, 379, "if");
                  __AppendElement($n378, $n379);
                }
                $renderTemplates[$path].update_1ee56c8_379($lepusTemplate, $n379, $data, $update2);
                $update2 = $template_update;
              }
              {
                let _$template_update21 = $update2;
                let $n395 = $update2 ? $lepusGetElementRefByLepusID("if", 395) : null;
                if (!$n395) {
                  $update2 = false;
                  $n395 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n395, 395, "if");
                  __AppendElement($n378, $n395);
                }
                $renderTemplates[$path].update_1cfa368_395($lepusTemplate, $n395, $data, $update2);
                $update2 = _$template_update21;
              }
              $update2 = $temp3;
            }
            {
              let _$template_update22 = $update2;
              let $n398 = $update2 ? $lepusGetElementRefByLepusID("if", 398) : null;
              if (!$n398) {
                $update2 = false;
                $n398 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n398, 398, "if");
                __AppendElement($n377, $n398);
              }
              $renderTemplates[$path].update_10a53b0_398($lepusTemplate, $n398, $data, $update2);
              $update2 = _$template_update22;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp66 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n401 = $update2 ? $lepusGetElementRefByLepusID("view", 401) : null;
            let _$temp67 = $update2;
            if (!$n401) {
              $update2 = false;
              $n401 = __CreateView($currentComponentId);
              let $nid401 = $lepusStoreElementRefByLepusID($n401, 401, "view");
              __SetAttribute($n401, 1004, $nid401[1]);
              __AppendElement($parent, $n401);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value75 = $data.marketingV2VO.layoutVO.containerStyle;
                if (!$update2 || _$value75 !== $lepusTemplate._data.marketingV2VO.layoutVO.containerStyle) {
                  __SetStyleObject($n401, [0, 3, 44, 79, 62, getCssPropertyIDObj($data.marketingV2VO.layoutVO.containerStyle)]);
                }
              }
            }
            {
              let $n402 = $update2 ? $lepusGetElementRefByLepusID("view", 402) : null;
              let _$temp68 = $update2;
              if (!$n402) {
                $update2 = false;
                $n402 = __CreateView($currentComponentId);
                let $nid402 = $lepusStoreElementRefByLepusID($n402, 402, "view");
                __SetAttribute($n402, 1004, $nid402[1]);
                __SetStyleObject($n402, [14, 3, 100]);
                __AppendElement($n401, $n402);
              }
              {
                let $n403 = $update2 ? $lepusGetElementRefByLepusID("view", 403) : null;
                let $temp4 = $update2;
                if (!$n403) {
                  $update2 = false;
                  $n403 = __CreateView($currentComponentId);
                  let $nid403 = $lepusStoreElementRefByLepusID($n403, 403, "view");
                  __SetAttribute($n403, 1004, $nid403[1]);
                  __AppendElement($n402, $n403);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value76 = $data.marketingV2VO.layoutVO.priceContainerStyle;
                    if (!$update2 || _$value76 !== $lepusTemplate._data.marketingV2VO.layoutVO.priceContainerStyle) {
                      __SetStyleObject($n403, [3, 44, 104, 0, 66, 103, 45, getCssPropertyIDObj($data.marketingV2VO.layoutVO.priceContainerStyle)]);
                    }
                  }
                }
                {
                  let _$template_update23 = $update2;
                  let $n404 = $update2 ? $lepusGetElementRefByLepusID("if", 404) : null;
                  if (!$n404) {
                    $update2 = false;
                    $n404 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n404, 404, "if");
                    __AppendElement($n403, $n404);
                  }
                  $renderTemplates[$path].update_1ee56c8_404($lepusTemplate, $n404, $data, $update2);
                  $update2 = _$template_update23;
                }
                {
                  let _$template_update24 = $update2;
                  let $n420 = $update2 ? $lepusGetElementRefByLepusID("if", 420) : null;
                  if (!$n420) {
                    $update2 = false;
                    $n420 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n420, 420, "if");
                    __AppendElement($n403, $n420);
                  }
                  $renderTemplates[$path].update_1cfa368_420($lepusTemplate, $n420, $data, $update2);
                  $update2 = _$template_update24;
                }
                $update2 = $temp4;
              }
              {
                let _$template_update25 = $update2;
                let $n423 = $update2 ? $lepusGetElementRefByLepusID("if", 423) : null;
                if (!$n423) {
                  $update2 = false;
                  $n423 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n423, 423, "if");
                  __AppendElement($n402, $n423);
                }
                $renderTemplates[$path].update_1cfa368_423($lepusTemplate, $n423, $data, $update2);
                $update2 = _$template_update25;
              }
              $update2 = _$temp68;
            }
            {
              let _$template_update26 = $update2;
              let $n426 = $update2 ? $lepusGetElementRefByLepusID("if", 426) : null;
              if (!$n426) {
                $update2 = false;
                $n426 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n426, 426, "if");
                __AppendElement($n401, $n426);
              }
              $renderTemplates[$path].update_10a53b0_426($lepusTemplate, $n426, $data, $update2);
              $update2 = _$template_update26;
            }
            $update2 = _$temp67;
          }
          $update2 = _$temp66;
        }
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "ProcessCompV2";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n376 = $lepusGetElementRefByLepusID("if", 376);
    $renderTemplates[$path].update_cffa30_376($lepusTemplate, $n376, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n375 = __CreateView($currentComponentId);
    __SetAttribute($n375, 1004, 375);
    __SetStyleObject($n375, [0, 178, 101]);
    __AppendElement($template, $n375);
    let $n376 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n376, 376, "if");
    __AppendElement($n375, $n376);
    $renderTemplates["ProcessCompV2"].update_cffa30_376($lepusTemplate, $n376, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagInfo"] = {
  variables: ["vo", "tagCountdownState", "tagMoreIconState", "tagAnimateProgressState", "tagSecKillCountDownState"],
  varUpdateState: [],
  update_377e4c8_471: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_simple_text") {
        __UpdateIfNodeIndex($parent, 18);
        $conditionNodeIndex[uniqueId] = 18;
        let $temp = $update2;
        if ($ifNodeIndex !== 18) {
          $update2 = false;
        }
        {
          let $n472 = $update2 ? $lepusGetElementRefByLepusID("template", 472) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n472) {
            $templateCreated = false;
            $n472 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n472, 472, "template");
            __AppendElement($parent, $n472);
            $templateId = __GetElementUniqueID($n472);
            $childLepusTemplate = $templateConstructor($templateId, $n472);
          } else {
            $templateId = __GetElementUniqueID($n472);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagSimpleTextVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagSimpleText"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagSimpleText"].entry($n472, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 19);
        $conditionNodeIndex[uniqueId] = 19;
        let _$temp69 = $update2;
        if ($ifNodeIndex !== 19) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n473 = $update2 ? $lepusGetElementRefByLepusID("if", 473) : null;
          if (!$n473) {
            $update2 = false;
            $n473 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n473, 473, "if");
            __AppendElement($parent, $n473);
          }
          $renderTemplates["TagInfo"].update_377e4c8_473($lepusTemplate, $n473, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp69;
      }
    }
  },
  update_377e4c8_473: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_dynamic") {
        __UpdateIfNodeIndex($parent, 20);
        $conditionNodeIndex[uniqueId] = 20;
        let $temp = $update2;
        if ($ifNodeIndex !== 20) {
          $update2 = false;
        }
        {
          let $n474 = $update2 ? $lepusGetElementRefByLepusID("template", 474) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n474) {
            $templateCreated = false;
            $n474 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n474, 474, "template");
            __AppendElement($parent, $n474);
            $templateId = __GetElementUniqueID($n474);
            $childLepusTemplate = $templateConstructor($templateId, $n474);
          } else {
            $templateId = __GetElementUniqueID($n474);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagDynamicVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagDynamic"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagDynamic"].entry($n474, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_39822b0_469: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_sec_kill_count_down") {
        __UpdateIfNodeIndex($parent, 16);
        $conditionNodeIndex[uniqueId] = 16;
        let $temp = $update2;
        if ($ifNodeIndex !== 16) {
          $update2 = false;
        }
        {
          let $n470 = $update2 ? $lepusGetElementRefByLepusID("template", 470) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n470) {
            $templateCreated = false;
            $n470 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n470, 470, "template");
            __AppendElement($parent, $n470);
            $templateId = __GetElementUniqueID($n470);
            $childLepusTemplate = $templateConstructor($templateId, $n470);
          } else {
            $templateId = __GetElementUniqueID($n470);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagSecKillCountDownVO, $update2);
          $childLepusTemplate.setData("tagSecKillCountDownState", $data.tagSecKillCountDownState, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagSecKillCountDown"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagSecKillCountDown"].entry($n470, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 17);
        $conditionNodeIndex[uniqueId] = 17;
        let _$temp70 = $update2;
        if ($ifNodeIndex !== 17) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n471 = $update2 ? $lepusGetElementRefByLepusID("if", 471) : null;
          if (!$n471) {
            $update2 = false;
            $n471 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n471, 471, "if");
            __AppendElement($parent, $n471);
          }
          $renderTemplates["TagInfo"].update_377e4c8_471($lepusTemplate, $n471, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp70;
      }
    }
  },
  update_59e7a0_465: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_split") {
        __UpdateIfNodeIndex($parent, 12);
        $conditionNodeIndex[uniqueId] = 12;
        let $temp = $update2;
        if ($ifNodeIndex !== 12) {
          $update2 = false;
        }
        {
          let $n466 = $update2 ? $lepusGetElementRefByLepusID("template", 466) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n466) {
            $templateCreated = false;
            $n466 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n466, 466, "template");
            __AppendElement($parent, $n466);
            $templateId = __GetElementUniqueID($n466);
            $childLepusTemplate = $templateConstructor($templateId, $n466);
          } else {
            $templateId = __GetElementUniqueID($n466);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagSplitVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagSplit"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagSplit"].entry($n466, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 13);
        $conditionNodeIndex[uniqueId] = 13;
        let _$temp71 = $update2;
        if ($ifNodeIndex !== 13) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n467 = $update2 ? $lepusGetElementRefByLepusID("if", 467) : null;
          if (!$n467) {
            $update2 = false;
            $n467 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n467, 467, "if");
            __AppendElement($parent, $n467);
          }
          $renderTemplates["TagInfo"].update_59e7a0_467($lepusTemplate, $n467, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp71;
      }
    }
  },
  update_59e7a0_467: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_animate_progress") {
        __UpdateIfNodeIndex($parent, 14);
        $conditionNodeIndex[uniqueId] = 14;
        let $temp = $update2;
        if ($ifNodeIndex !== 14) {
          $update2 = false;
        }
        {
          let $n468 = $update2 ? $lepusGetElementRefByLepusID("template", 468) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n468) {
            $templateCreated = false;
            $n468 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n468, 468, "template");
            __AppendElement($parent, $n468);
            $templateId = __GetElementUniqueID($n468);
            $childLepusTemplate = $templateConstructor($templateId, $n468);
          } else {
            $templateId = __GetElementUniqueID($n468);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagAnimateProgressVO, $update2);
          $childLepusTemplate.setData("tagAnimateProgressState", $data.tagAnimateProgressState, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagAnimateProgress"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagAnimateProgress"].entry($n468, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 15);
        $conditionNodeIndex[uniqueId] = 15;
        let _$temp72 = $update2;
        if ($ifNodeIndex !== 15) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n469 = $update2 ? $lepusGetElementRefByLepusID("if", 469) : null;
          if (!$n469) {
            $update2 = false;
            $n469 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n469, 469, "if");
            __AppendElement($parent, $n469);
          }
          $renderTemplates["TagInfo"].update_39822b0_469($lepusTemplate, $n469, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp72;
      }
    }
  },
  update_7b5a20_461: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_progress") {
        __UpdateIfNodeIndex($parent, 8);
        $conditionNodeIndex[uniqueId] = 8;
        let $temp = $update2;
        if ($ifNodeIndex !== 8) {
          $update2 = false;
        }
        {
          let $n462 = $update2 ? $lepusGetElementRefByLepusID("template", 462) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n462) {
            $templateCreated = false;
            $n462 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n462, 462, "template");
            __AppendElement($parent, $n462);
            $templateId = __GetElementUniqueID($n462);
            $childLepusTemplate = $templateConstructor($templateId, $n462);
          } else {
            $templateId = __GetElementUniqueID($n462);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagProgressVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagProgress"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagProgress"].entry($n462, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 9);
        $conditionNodeIndex[uniqueId] = 9;
        let _$temp73 = $update2;
        if ($ifNodeIndex !== 9) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n463 = $update2 ? $lepusGetElementRefByLepusID("if", 463) : null;
          if (!$n463) {
            $update2 = false;
            $n463 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n463, 463, "if");
            __AppendElement($parent, $n463);
          }
          $renderTemplates["TagInfo"].update_7b5a20_463($lepusTemplate, $n463, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp73;
      }
    }
  },
  update_7b5a20_463: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_more_icon") {
        __UpdateIfNodeIndex($parent, 10);
        $conditionNodeIndex[uniqueId] = 10;
        let $temp = $update2;
        if ($ifNodeIndex !== 10) {
          $update2 = false;
        }
        {
          let $n464 = $update2 ? $lepusGetElementRefByLepusID("template", 464) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n464) {
            $templateCreated = false;
            $n464 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n464, 464, "template");
            __AppendElement($parent, $n464);
            $templateId = __GetElementUniqueID($n464);
            $childLepusTemplate = $templateConstructor($templateId, $n464);
          } else {
            $templateId = __GetElementUniqueID($n464);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagMoreIconVO, $update2);
          $childLepusTemplate.setData("tagMoreIconState", $data.tagMoreIconState, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagMoreIcon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagMoreIcon"].entry($n464, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 11);
        $conditionNodeIndex[uniqueId] = 11;
        let _$temp74 = $update2;
        if ($ifNodeIndex !== 11) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n465 = $update2 ? $lepusGetElementRefByLepusID("if", 465) : null;
          if (!$n465) {
            $update2 = false;
            $n465 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n465, 465, "if");
            __AppendElement($parent, $n465);
          }
          $renderTemplates["TagInfo"].update_59e7a0_465($lepusTemplate, $n465, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp74;
      }
    }
  },
  update_2d65568_453: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "TagInfo";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[3] || $renderTemplates[$path].varUpdateState[4]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.vo.type === "tag_simple_border") {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n454 = $update2 ? $lepusGetElementRefByLepusID("template", 454) : null;
            let $templateCreated = true;
            let $childLepusTemplate = null;
            let $templateId = null;
            if (!$n454) {
              $templateCreated = false;
              $n454 = __CreateWrapperElement($currentComponentId);
              $lepusStoreElementRefByLepusID($n454, 454, "template");
              __AppendElement($parent, $n454);
              $templateId = __GetElementUniqueID($n454);
              $childLepusTemplate = $templateConstructor($templateId, $n454);
            } else {
              $templateId = __GetElementUniqueID($n454);
              $childLepusTemplate = $templateInfo[$templateId];
            }
            $updatePropsSet = [];
            $childLepusTemplate.setData("vo", $data.vo.tagSimpleBorderVO, $update2);
            if ($templateCreated) {
              let $update_keys = $updatePropsSet;
              if ($update_keys.length > 0) {
                $renderTemplates["TagSimpleBorder"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
              }
            } else {
              $renderTemplates["TagSimpleBorder"].entry($n454, $childLepusTemplate, $childLepusTemplate.data, false);
            }
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp75 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $template_update = $update2;
            let $n455 = $update2 ? $lepusGetElementRefByLepusID("if", 455) : null;
            if (!$n455) {
              $update2 = false;
              $n455 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n455, 455, "if");
              __AppendElement($parent, $n455);
            }
            $renderTemplates[$path].update_2d65568_455($lepusTemplate, $n455, $data, $update2);
            $update2 = $template_update;
          }
          $update2 = _$temp75;
        }
      }
    }
  },
  update_2d65568_455: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_simple") {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n456 = $update2 ? $lepusGetElementRefByLepusID("template", 456) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n456) {
            $templateCreated = false;
            $n456 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n456, 456, "template");
            __AppendElement($parent, $n456);
            $templateId = __GetElementUniqueID($n456);
            $childLepusTemplate = $templateConstructor($templateId, $n456);
          } else {
            $templateId = __GetElementUniqueID($n456);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagSimpleVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagSimple"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagSimple"].entry($n456, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 3);
        $conditionNodeIndex[uniqueId] = 3;
        let _$temp76 = $update2;
        if ($ifNodeIndex !== 3) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n457 = $update2 ? $lepusGetElementRefByLepusID("if", 457) : null;
          if (!$n457) {
            $update2 = false;
            $n457 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n457, 457, "if");
            __AppendElement($parent, $n457);
          }
          $renderTemplates["TagInfo"].update_2d65568_457($lepusTemplate, $n457, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp76;
      }
    }
  },
  update_2d65568_457: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_normal") {
        __UpdateIfNodeIndex($parent, 4);
        $conditionNodeIndex[uniqueId] = 4;
        let $temp = $update2;
        if ($ifNodeIndex !== 4) {
          $update2 = false;
        }
        {
          let $n458 = $update2 ? $lepusGetElementRefByLepusID("template", 458) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n458) {
            $templateCreated = false;
            $n458 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n458, 458, "template");
            __AppendElement($parent, $n458);
            $templateId = __GetElementUniqueID($n458);
            $childLepusTemplate = $templateConstructor($templateId, $n458);
          } else {
            $templateId = __GetElementUniqueID($n458);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagNormalVO, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagNormal"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagNormal"].entry($n458, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 5);
        $conditionNodeIndex[uniqueId] = 5;
        let _$temp77 = $update2;
        if ($ifNodeIndex !== 5) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n459 = $update2 ? $lepusGetElementRefByLepusID("if", 459) : null;
          if (!$n459) {
            $update2 = false;
            $n459 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n459, 459, "if");
            __AppendElement($parent, $n459);
          }
          $renderTemplates["TagInfo"].update_2d65568_459($lepusTemplate, $n459, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp77;
      }
    }
  },
  update_2d65568_459: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.type === "tag_countdown") {
        __UpdateIfNodeIndex($parent, 6);
        $conditionNodeIndex[uniqueId] = 6;
        let $temp = $update2;
        if ($ifNodeIndex !== 6) {
          $update2 = false;
        }
        {
          let $n460 = $update2 ? $lepusGetElementRefByLepusID("template", 460) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n460) {
            $templateCreated = false;
            $n460 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n460, 460, "template");
            __AppendElement($parent, $n460);
            $templateId = __GetElementUniqueID($n460);
            $childLepusTemplate = $templateConstructor($templateId, $n460);
          } else {
            $templateId = __GetElementUniqueID($n460);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("vo", $data.vo.tagCountdownVO, $update2);
          $childLepusTemplate.setData("tagCountdownState", $data.tagCountdownState, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["TagCountdown"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["TagCountdown"].entry($n460, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 7);
        $conditionNodeIndex[uniqueId] = 7;
        let _$temp78 = $update2;
        if ($ifNodeIndex !== 7) {
          $update2 = false;
        }
        {
          let $template_update = $update2;
          let $n461 = $update2 ? $lepusGetElementRefByLepusID("if", 461) : null;
          if (!$n461) {
            $update2 = false;
            $n461 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n461, 461, "if");
            __AppendElement($parent, $n461);
          }
          $renderTemplates["TagInfo"].update_7b5a20_461($lepusTemplate, $n461, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp78;
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagInfo";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n453 = $lepusGetElementRefByLepusID("if", 453);
    $renderTemplates[$path].update_2d65568_453($lepusTemplate, $n453, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n453 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n453, 453, "if");
    __AppendElement($template, $n453);
    $renderTemplates["TagInfo"].update_2d65568_453($lepusTemplate, $n453, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagSimpleBorder"] = {
  variables: ["vo", "tagSimpleBorderState"],
  varUpdateState: [],
  update_377e4c8_501: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixIconUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n502 = $update2 ? $lepusGetElementRefByLepusID("image", 502) : null;
          let $temp2 = $update2;
          if (!$n502) {
            $update2 = false;
            $n502 = __CreateImage($currentComponentId);
            let $nid502 = $lepusStoreElementRefByLepusID($n502, 502, "image");
            __SetAttribute($n502, 1004, $nid502[1]);
            __SetAttribute($n502, "skip-redirection", true);
            __AppendElement($parent, $n502);
          }
          if (!$update2 || $renderTemplates["TagSimpleBorder"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixIconStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixIconStyle) {
                __SetStyleObject($n502, [38, 39, 140, getCssPropertyIDObj($data.vo.prefixIconStyle)]);
              }
            }
            {
              let _$value77 = $data.vo.prefixIconUrl;
              if (!$update2 || _$value77 !== $lepusTemplate._data.vo.prefixIconUrl) {
                __SetAttribute($n502, "src", _$value77);
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
  update_377e4c8_503: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixIconType) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n504 = $update2 ? $lepusGetElementRefByLepusID("template", 504) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n504) {
            $templateCreated = false;
            $n504 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n504, 504, "template");
            __AppendElement($parent, $n504);
            $templateId = __GetElementUniqueID($n504);
            $childLepusTemplate = $templateConstructor($templateId, $n504);
          } else {
            $templateId = __GetElementUniqueID($n504);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.prefixIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.prefixIconColor, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n504, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_377e4c8_505: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "TagSimpleBorder";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.text) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n506 = $update2 ? $lepusGetElementRefByLepusID("text", 506) : null;
          let $temp2 = $update2;
          if (!$n506) {
            $update2 = false;
            $n506 = __CreateText($currentComponentId);
            let $nid506 = $lepusStoreElementRefByLepusID($n506, 506, "text");
            __SetAttribute($n506, 1004, $nid506[1]);
            __SetAttribute($n506, "text-single-line-vertical-align", "center");
            __AppendElement($parent, $n506);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.textStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.textStyle) {
                __SetStyleObject($n506, [21, 139, 89, 41, 117, getCssPropertyIDObj($data.vo.textStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value78 = $data.vo.text;
              if (!$update2 || _$value78 !== $lepusTemplate._data.vo.text) {
                __SetAttribute($n506, "text", _$value78);
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
  update_12e1408_508: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.text && $data.vo.suffixText && !((_a = $data.tagSimpleBorderState) == null ? undefined : _a.isOverflow)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n509 = $update2 ? $lepusGetElementRefByLepusID("image", 509) : null;
          let $temp2 = $update2;
          if (!$n509) {
            $update2 = false;
            $n509 = __CreateImage($currentComponentId);
            let $nid509 = $lepusStoreElementRefByLepusID($n509, 509, "image");
            __SetAttribute($n509, 1004, $nid509[1]);
            __SetStyleObject($n509, [164, 165, 105, 31]);
            __SetAttribute($n509, "src", "https://test.com/obj/test/life/marketing/lynx/life_marketing_resource/marketing_expression_tool/super-discount/coupon-separator.png");
            __SetAttribute($n509, "skip-redirection", true);
            __AppendElement($parent, $n509);
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
  update_12e1408_510: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagSimpleBorder";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.suffixText && (!$data.vo.text || !((_a = $data.tagSimpleBorderState) == null ? undefined : _a.isOverflow))) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n511 = $update2 ? $lepusGetElementRefByLepusID("text", 511) : null;
          let $temp2 = $update2;
          if (!$n511) {
            $update2 = false;
            $n511 = __CreateText($currentComponentId);
            let $nid511 = $lepusStoreElementRefByLepusID($n511, 511, "text");
            __SetAttribute($n511, 1004, $nid511[1]);
            __SetAttribute($n511, "text-single-line-vertical-align", "center");
            __AppendElement($parent, $n511);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.suffixTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.suffixTextStyle) {
                __SetStyleObject($n511, [21, 105, 139, 89, 41, 16, 117, getCssPropertyIDObj($data.vo.suffixTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value79 = $data.vo.suffixText;
              if (!$update2 || _$value79 !== $lepusTemplate._data.vo.suffixText) {
                __SetAttribute($n511, "text", _$value79);
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
  update_3947930_497: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "TagSimpleBorder";
    if (!$update2 || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!$data.vo.text || !!$data.vo.suffixText) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n498 = $update2 ? $lepusGetElementRefByLepusID("view", 498) : null;
            let $temp2 = $update2;
            if (!$n498) {
              $update2 = false;
              $n498 = __CreateView($currentComponentId);
              let $nid498 = $lepusStoreElementRefByLepusID($n498, 498, "view");
              __SetAttribute($n498, 1004, $nid498[1]);
              __SetID($n498, "tagSimpleBorderWrapper");
              __AddEvent($n498, "bindEvent", "uiappear", "onTagSimpleBorderHandleAppear");
              __AppendElement($parent, $n498);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.wrapperStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.wrapperStyle) {
                  __SetStyleObject($n498, [52, 101, 119, getCssPropertyIDObj($data.vo.wrapperStyle)]);
                }
              }
            }
            {
              let $n499 = $update2 ? $lepusGetElementRefByLepusID("view", 499) : null;
              let $temp3 = $update2;
              if (!$n499) {
                $update2 = false;
                $n499 = __CreateView($currentComponentId);
                let $nid499 = $lepusStoreElementRefByLepusID($n499, 499, "view");
                __SetAttribute($n499, 1004, $nid499[1]);
                __SetID($n499, "tagSimpleBorderInner");
                __AppendElement($n498, $n499);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value80 = $data.vo.innerStyle;
                  if (!$update2 || _$value80 !== $lepusTemplate._data.vo.innerStyle) {
                    __SetStyleObject($n499, [173, 1, 3, 4, 101, getCssPropertyIDObj($data.vo.innerStyle)]);
                  }
                }
              }
              {
                let $n500 = $update2 ? $lepusGetElementRefByLepusID("view", 500) : null;
                let $temp4 = $update2;
                if (!$n500) {
                  $update2 = false;
                  $n500 = __CreateView($currentComponentId);
                  let $nid500 = $lepusStoreElementRefByLepusID($n500, 500, "view");
                  __SetAttribute($n500, 1004, $nid500[1]);
                  __AppendElement($n499, $n500);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value81 = $data.vo.containerStyle;
                    if (!$update2 || _$value81 !== $lepusTemplate._data.vo.containerStyle) {
                      __SetStyleObject($n500, [3, 4, 32, 31, 183, 184, 173, 1, getCssPropertyIDObj($data.vo.containerStyle)]);
                    }
                  }
                }
                {
                  let $template_update = $update2;
                  let $n501 = $update2 ? $lepusGetElementRefByLepusID("if", 501) : null;
                  if (!$n501) {
                    $update2 = false;
                    $n501 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n501, 501, "if");
                    __AppendElement($n500, $n501);
                  }
                  $renderTemplates[$path].update_377e4c8_501($lepusTemplate, $n501, $data, $update2);
                  $update2 = $template_update;
                }
                {
                  let _$template_update27 = $update2;
                  let $n503 = $update2 ? $lepusGetElementRefByLepusID("if", 503) : null;
                  if (!$n503) {
                    $update2 = false;
                    $n503 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n503, 503, "if");
                    __AppendElement($n500, $n503);
                  }
                  $renderTemplates[$path].update_377e4c8_503($lepusTemplate, $n503, $data, $update2);
                  $update2 = _$template_update27;
                }
                {
                  let _$template_update28 = $update2;
                  let $n505 = $update2 ? $lepusGetElementRefByLepusID("if", 505) : null;
                  if (!$n505) {
                    $update2 = false;
                    $n505 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n505, 505, "if");
                    __AppendElement($n500, $n505);
                  }
                  $renderTemplates[$path].update_377e4c8_505($lepusTemplate, $n505, $data, $update2);
                  $update2 = _$template_update28;
                }
                {
                  let _$template_update29 = $update2;
                  let $n508 = $update2 ? $lepusGetElementRefByLepusID("if", 508) : null;
                  if (!$n508) {
                    $update2 = false;
                    $n508 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n508, 508, "if");
                    __AppendElement($n500, $n508);
                  }
                  $renderTemplates[$path].update_12e1408_508($lepusTemplate, $n508, $data, $update2);
                  $update2 = _$template_update29;
                }
                {
                  let _$template_update30 = $update2;
                  let $n510 = $update2 ? $lepusGetElementRefByLepusID("if", 510) : null;
                  if (!$n510) {
                    $update2 = false;
                    $n510 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n510, 510, "if");
                    __AppendElement($n500, $n510);
                  }
                  $renderTemplates[$path].update_12e1408_510($lepusTemplate, $n510, $data, $update2);
                  $update2 = _$template_update30;
                }
                $update2 = $temp4;
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagSimpleBorder";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n497 = $lepusGetElementRefByLepusID("if", 497);
    $renderTemplates[$path].update_3947930_497($lepusTemplate, $n497, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n497 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n497, 497, "if");
    __AppendElement($template, $n497);
    $renderTemplates["TagSimpleBorder"].update_3947930_497($lepusTemplate, $n497, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["Icon"] = {
  variables: ["type", "dynamic_style", "color"],
  varUpdateState: [],
  update_2251e38_514: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Icon";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.type == "help") {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n515 = $update2 ? $lepusGetElementRefByLepusID("svg", 515) : null;
            let $temp2 = $update2;
            if (!$n515) {
              $update2 = false;
              $n515 = __CreateElement("svg", $currentComponentId);
              let $nid515 = $lepusStoreElementRefByLepusID($n515, 515, "svg");
              __SetAttribute($n515, 1004, $nid515[1]);
              __AppendElement($parent, $n515);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
              {
                let $value = $data.dynamic_style;
                if (!$update2 || $value !== $lepusTemplate._data.dynamic_style) {
                  __SetStyleObject($n515, [145, 146, 105, getCssPropertyIDObj($data.dynamic_style)]);
                }
              }
              {
                let _$value82 = "          <svg width='10' height='10' viewBox='0 0 10 10' fill='none' xmlns='http://www.w3.org/2000/svg'>              <path fill-rule='evenodd' clip-rule='evenodd' d='M1.24984 5.00008C1.24984 2.92901 2.92877 1.25008 4.99984 1.25008C7.07091 1.25008 8.74984 2.92901 8.74984 5.00008C8.74984 7.07115 7.07091 8.75008 4.99984 8.75008C2.92877 8.75008 1.24984 7.07115 1.24984 5.00008ZM4.99984 0.416748C2.46853 0.416748 0.416504 2.46878 0.416504 5.00008C0.416504 7.53139 2.46853 9.58342 4.99984 9.58342C7.53114 9.58342 9.58317 7.53139 9.58317 5.00008C9.58317 2.46878 7.53114 0.416748 4.99984 0.416748ZM3.90648 3.06653C4.19314 2.78653 4.57981 2.64653 5.07314 2.64653C5.50648 2.64653 5.85981 2.75986 6.12648 2.99986C6.39314 3.23319 6.52648 3.55319 6.52648 3.95986C6.52648 4.29319 6.43981 4.56653 6.27981 4.77986C6.21981 4.84653 6.02648 5.02653 5.70648 5.30653C5.58648 5.40653 5.49981 5.51986 5.43981 5.63986C5.37314 5.77319 5.33981 5.91319 5.33981 6.07319V6.16653H4.57314V6.07319C4.57314 5.81986 4.61314 5.59986 4.70648 5.41986C4.79314 5.23986 5.05314 4.95986 5.48648 4.57319L5.56648 4.47986C5.68648 4.33319 5.74648 4.17319 5.74648 4.00653C5.74648 3.78653 5.67981 3.61319 5.55981 3.48653C5.43314 3.35986 5.25314 3.29986 5.02648 3.29986C4.73314 3.29986 4.52648 3.38653 4.39981 3.57319C4.28648 3.72653 4.23314 3.94652 4.23314 4.22653H3.47314C3.47314 3.73319 3.61314 3.34653 3.90648 3.06653ZM4.58648 6.63319C4.68648 6.53986 4.80648 6.49319 4.95314 6.49319C5.09981 6.49319 5.22648 6.53986 5.32648 6.63319C5.41981 6.72653 5.47314 6.84653 5.47314 6.99319C5.47314 7.13986 5.41981 7.26653 5.31981 7.35986C5.21981 7.45319 5.09981 7.49986 4.95314 7.49986C4.80648 7.49986 4.68648 7.44653 4.58648 7.35319C4.48648 7.25986 4.43981 7.13986 4.43981 6.99319C4.43981 6.84653 4.48648 6.72653 4.58648 6.63319Z' fill=" + ($data.color + "/>          </svg>");
                if (!$update2 || _$value82 !== undefined) {
                  __SetAttribute($n515, "content", _$value82);
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
  update_2251e38_516: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Icon";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.type == "arrow") {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n517 = $update2 ? $lepusGetElementRefByLepusID("svg", 517) : null;
            let $temp2 = $update2;
            if (!$n517) {
              $update2 = false;
              $n517 = __CreateElement("svg", $currentComponentId);
              let $nid517 = $lepusStoreElementRefByLepusID($n517, 517, "svg");
              __SetAttribute($n517, 1004, $nid517[1]);
              __AppendElement($parent, $n517);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
              {
                let $value = $data.dynamic_style;
                if (!$update2 || $value !== $lepusTemplate._data.dynamic_style) {
                  __SetStyleObject($n517, [154, 146, 105, getCssPropertyIDObj($data.dynamic_style)]);
                }
              }
              {
                let _$value83 = "          <svg xmlns='http://www.w3.org/2000/svg' width='6' height='10' viewBox='0 0 6 10' fill='none'>              <path d='M2 2L5 5L2 8' stroke=" + ($data.color + " stroke-width='1.3' stroke-linecap='round' stroke-linejoin='round'/>          </svg>          ");
                if (!$update2 || _$value83 !== undefined) {
                  __SetAttribute($n517, "content", _$value83);
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
  update_2251e38_518: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Icon";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.type == "circle_arrow") {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n519 = $update2 ? $lepusGetElementRefByLepusID("svg", 519) : null;
            let $temp2 = $update2;
            if (!$n519) {
              $update2 = false;
              $n519 = __CreateElement("svg", $currentComponentId);
              let $nid519 = $lepusStoreElementRefByLepusID($n519, 519, "svg");
              __SetAttribute($n519, 1004, $nid519[1]);
              __AppendElement($parent, $n519);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
              {
                let $value = $data.dynamic_style;
                if (!$update2 || $value !== $lepusTemplate._data.dynamic_style) {
                  __SetStyleObject($n519, [145, 146, 105, getCssPropertyIDObj($data.dynamic_style)]);
                }
              }
              {
                let _$value84 = "          <svg width='10' height='10' viewBox='0 0 10 10' fill='none' xmlns='http://www.w3.org/2000/svg'>              <path fill-rule='evenodd' clip-rule='evenodd' d='M5 0C2.23858 0 0 2.23858 0 5C0 7.76142 2.23858 10 5 10C7.76142 10 10 7.76142 10 5C10 2.23858 7.76142 0 5 0ZM6.99204 1.91779H8.05702C6.85269 1.91779 5.87639 2.89409 5.87639 4.09842V4.96199H7.07747C7.30083 4.96199 7.42047 5.2248 7.27379 5.39325L5.04367 7.95449C4.8946 8.1257 4.62825 8.12456 4.48065 7.95208L2.28952 5.39156C2.14499 5.22266 2.265 4.96199 2.48731 4.96199H3.64508V4.89286C3.64508 3.24977 4.97707 1.91779 6.62016 1.91779H6.99204Z' fill=" + ($data.color + "/>          </svg>          ");
                if (!$update2 || _$value84 !== undefined) {
                  __SetAttribute($n519, "content", _$value84);
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
  update_2251e38_520: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "Icon";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.type == "broken_line") {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n521 = $update2 ? $lepusGetElementRefByLepusID("svg", 521) : null;
            let $temp2 = $update2;
            if (!$n521) {
              $update2 = false;
              $n521 = __CreateElement("svg", $currentComponentId);
              let $nid521 = $lepusStoreElementRefByLepusID($n521, 521, "svg");
              __SetAttribute($n521, 1004, $nid521[1]);
              __AppendElement($parent, $n521);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
              {
                let $value = $data.dynamic_style;
                if (!$update2 || $value !== $lepusTemplate._data.dynamic_style) {
                  __SetStyleObject($n521, [38, 185, 105, getCssPropertyIDObj($data.dynamic_style)]);
                }
              }
              {
                let _$value85 = "          <svg width='9' height='6' viewBox='0 0 9 6' fill='none' xmlns='http://www.w3.org/2000/svg'>              <path d='M1.36635 0.438629C1.22202 0.236301 0.941007 0.189279 0.738678 0.333603C0.53635 0.477927 0.489328 0.758944 0.633652 0.961273L1.36635 0.438629ZM4.32877 0.843977L4.03911 0.499604L4.32877 0.843977ZM2.47827 2.40051L2.1886 2.05614L2.47827 2.40051ZM0.633652 0.961273L1.82036 2.62492L2.55306 2.10228L1.36635 0.438629L0.633652 0.961273ZM2.76794 2.74489L4.61844 1.18835L4.03911 0.499604L2.1886 2.05614L2.76794 2.74489ZM4.24942 1.13558L7.18792 5.54933L7.93708 5.05057L4.99857 0.636816L4.24942 1.13558ZM4.61844 1.18835C4.50431 1.28435 4.33206 1.25971 4.24942 1.13558L4.99857 0.636816C4.78369 0.314057 4.33584 0.250011 4.03911 0.499604L4.61844 1.18835ZM1.82036 2.62492C2.04068 2.93379 2.4776 2.9891 2.76794 2.74489L2.1886 2.05614C2.30027 1.96221 2.46832 1.98349 2.55306 2.10228L1.82036 2.62492Z' fill=" + ($data.color + ("/>              <path d='M8 2.07983L7.5625 5.29983' stroke=" + ($data.color + (" stroke-width='0.9' stroke-linecap='round'/>              <path d='M4.5 4.37988L7.5625 5.29988' stroke=" + ($data.color + " stroke-width='0.9' stroke-linecap='round'/>          </svg>          ")))));
                if (!$update2 || _$value85 !== undefined) {
                  __SetAttribute($n521, "content", _$value85);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "Icon";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n514 = $lepusGetElementRefByLepusID("if", 514);
    $renderTemplates[$path].update_2251e38_514($lepusTemplate, $n514, $data, $update2);
    let $n516 = $lepusGetElementRefByLepusID("if", 516);
    $renderTemplates[$path].update_2251e38_516($lepusTemplate, $n516, $data, $update2);
    let $n518 = $lepusGetElementRefByLepusID("if", 518);
    $renderTemplates[$path].update_2251e38_518($lepusTemplate, $n518, $data, $update2);
    let $n520 = $lepusGetElementRefByLepusID("if", 520);
    $renderTemplates[$path].update_2251e38_520($lepusTemplate, $n520, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $path = "Icon";
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n514 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n514, 514, "if");
    __AppendElement($template, $n514);
    $renderTemplates[$path].update_2251e38_514($lepusTemplate, $n514, $data, $update2);
    let $n516 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n516, 516, "if");
    __AppendElement($template, $n516);
    $renderTemplates[$path].update_2251e38_516($lepusTemplate, $n516, $data, $update2);
    let $n518 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n518, 518, "if");
    __AppendElement($template, $n518);
    $renderTemplates[$path].update_2251e38_518($lepusTemplate, $n518, $data, $update2);
    let $n520 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n520, 520, "if");
    __AppendElement($template, $n520);
    $renderTemplates[$path].update_2251e38_520($lepusTemplate, $n520, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagSimple"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_1f68c58_522: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "TagSimple";
    if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!$data.vo.text) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n523 = $update2 ? $lepusGetElementRefByLepusID("view", 523) : null;
            let $temp2 = $update2;
            if (!$n523) {
              $update2 = false;
              $n523 = __CreateView($currentComponentId);
              let $nid523 = $lepusStoreElementRefByLepusID($n523, 523, "view");
              __SetAttribute($n523, 1004, $nid523[1]);
              __AppendElement($parent, $n523);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n523, [3, 44, 4, 31, 52, 186, 173, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $template_update = $update2;
              let $n524 = $update2 ? $lepusGetElementRefByLepusID("if", 524) : null;
              if (!$n524) {
                $update2 = false;
                $n524 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n524, 524, "if");
                __AppendElement($n523, $n524);
              }
              $renderTemplates[$path].update_1f68c58_524($lepusTemplate, $n524, $data, $update2);
              $update2 = $template_update;
            }
            {
              let _$template_update31 = $update2;
              let $n526 = $update2 ? $lepusGetElementRefByLepusID("if", 526) : null;
              if (!$n526) {
                $update2 = false;
                $n526 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n526, 526, "if");
                __AppendElement($n523, $n526);
              }
              $renderTemplates[$path].update_377e4c8_526($lepusTemplate, $n526, $data, $update2);
              $update2 = _$template_update31;
            }
            {
              let $n528 = $update2 ? $lepusGetElementRefByLepusID("text", 528) : null;
              let $temp3 = $update2;
              if (!$n528) {
                $update2 = false;
                $n528 = __CreateText($currentComponentId);
                let $nid528 = $lepusStoreElementRefByLepusID($n528, 528, "text");
                __SetAttribute($n528, 1004, $nid528[1]);
                __SetAttribute($n528, "text-maxline", "1");
                __AppendElement($n523, $n528);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value86 = $data.vo.textStyle;
                  if (!$update2 || _$value86 !== $lepusTemplate._data.vo.textStyle) {
                    __SetStyleObject($n528, [11, 21, 89, 117, getCssPropertyIDObj($data.vo.textStyle)]);
                  }
                }
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let _$value87 = $data.vo.text;
                  if (!$update2 || _$value87 !== $lepusTemplate._data.vo.text) {
                    __SetAttribute($n528, "text", _$value87);
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
  update_1f68c58_524: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.prefixIconUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n525 = $update2 ? $lepusGetElementRefByLepusID("image", 525) : null;
          let $temp2 = $update2;
          if (!$n525) {
            $update2 = false;
            $n525 = __CreateImage($currentComponentId);
            let $nid525 = $lepusStoreElementRefByLepusID($n525, 525, "image");
            __SetAttribute($n525, 1004, $nid525[1]);
            __SetAttribute($n525, "skip-redirection", true);
            __AppendElement($parent, $n525);
          }
          if (!$update2 || $renderTemplates["TagSimple"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixIconStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixIconStyle) {
                __SetStyleObject($n525, [145, 146, 140, getCssPropertyIDObj($data.vo.prefixIconStyle)]);
              }
            }
            {
              let _$value88 = $data.vo.prefixIconUrl;
              if (!$update2 || _$value88 !== $lepusTemplate._data.vo.prefixIconUrl) {
                __SetAttribute($n525, "src", _$value88);
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
  update_377e4c8_526: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixIconType) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n527 = $update2 ? $lepusGetElementRefByLepusID("template", 527) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n527) {
            $templateCreated = false;
            $n527 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n527, 527, "template");
            __AppendElement($parent, $n527);
            $templateId = __GetElementUniqueID($n527);
            $childLepusTemplate = $templateConstructor($templateId, $n527);
          } else {
            $templateId = __GetElementUniqueID($n527);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.prefixIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.prefixIconColor, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n527, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagSimple";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n522 = $lepusGetElementRefByLepusID("if", 522);
    $renderTemplates[$path].update_1f68c58_522($lepusTemplate, $n522, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n522 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n522, 522, "if");
    __AppendElement($template, $n522);
    $renderTemplates["TagSimple"].update_1f68c58_522($lepusTemplate, $n522, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagNormal"] = {
  variables: ["vo", "tagNormalState"],
  varUpdateState: [],
  update_1f68c58_539: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.suffixIconUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n540 = $update2 ? $lepusGetElementRefByLepusID("image", 540) : null;
          let $temp2 = $update2;
          if (!$n540) {
            $update2 = false;
            $n540 = __CreateImage($currentComponentId);
            let $nid540 = $lepusStoreElementRefByLepusID($n540, 540, "image");
            __SetAttribute($n540, 1004, $nid540[1]);
            __SetAttribute($n540, "auto-size", true);
            __SetAttribute($n540, "skip-redirection", true);
            __AppendElement($parent, $n540);
          }
          if (!$update2 || $renderTemplates["TagNormal"].varUpdateState[0]) {
            {
              let $value = $data.vo.suffixIconStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.suffixIconStyle) {
                __SetStyleObject($n540, [163, 165, getCssPropertyIDObj($data.vo.suffixIconStyle)]);
              }
            }
            {
              let _$value89 = $data.vo.suffixIconUrl;
              if (!$update2 || _$value89 !== $lepusTemplate._data.vo.suffixIconUrl) {
                __SetAttribute($n540, "src", _$value89);
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
  update_27ceeb0_531: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagNormal";
    if (!$update2 || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!($data.vo.prefixText || $data.vo.prefixIconUrl || $data.vo.prefixBackgroundImage) || !!$data.vo.suffixText && !((_a = $data.tagNormalState) == null ? undefined : _a.isOverflow)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n532 = $update2 ? $lepusGetElementRefByLepusID("view", 532) : null;
            let $temp2 = $update2;
            if (!$n532) {
              $update2 = false;
              $n532 = __CreateView($currentComponentId);
              let $nid532 = $lepusStoreElementRefByLepusID($n532, 532, "view");
              __SetAttribute($n532, 1004, $nid532[1]);
              __SetID($n532, "tagNormalWrapper");
              __AddEvent($n532, "bindEvent", "uiappear", "onTagNormalHandleAppear");
              __AppendElement($parent, $n532);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n532, [52, 101, 119, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $n533 = $update2 ? $lepusGetElementRefByLepusID("view", 533) : null;
              let $temp3 = $update2;
              if (!$n533) {
                $update2 = false;
                $n533 = __CreateView($currentComponentId);
                let $nid533 = $lepusStoreElementRefByLepusID($n533, 533, "view");
                __SetAttribute($n533, 1004, $nid533[1]);
                __SetID($n533, "tagNormalInner");
                __AppendElement($n532, $n533);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value90 = $data.vo.innerStyle;
                  if (!$update2 || _$value90 !== $lepusTemplate._data.vo.innerStyle) {
                    __SetStyleObject($n533, [173, 1, 3, 187, 4, 91, 32, 101, 188, getCssPropertyIDObj($data.vo.innerStyle)]);
                  }
                }
              }
              {
                let $template_update = $update2;
                let $n534 = $update2 ? $lepusGetElementRefByLepusID("if", 534) : null;
                if (!$n534) {
                  $update2 = false;
                  $n534 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n534, 534, "if");
                  __AppendElement($n533, $n534);
                }
                $renderTemplates[$path].update_27ceeb0_534($lepusTemplate, $n534, $data, $update2);
                $update2 = $template_update;
              }
              {
                let _$template_update32 = $update2;
                let $n541 = $update2 ? $lepusGetElementRefByLepusID("if", 541) : null;
                if (!$n541) {
                  $update2 = false;
                  $n541 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n541, 541, "if");
                  __AppendElement($n533, $n541);
                }
                $renderTemplates[$path].update_1c6f8a8_541($lepusTemplate, $n541, $data, $update2);
                $update2 = _$template_update32;
              }
              {
                let _$template_update33 = $update2;
                let $n543 = $update2 ? $lepusGetElementRefByLepusID("if", 543) : null;
                if (!$n543) {
                  $update2 = false;
                  $n543 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n543, 543, "if");
                  __AppendElement($n533, $n543);
                }
                $renderTemplates[$path].update_1c6f8a8_543($lepusTemplate, $n543, $data, $update2);
                $update2 = _$template_update33;
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
  update_27ceeb0_534: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagNormal";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.suffixText && !((_a = $data.tagNormalState) == null ? undefined : _a.isOverflow)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n535 = $update2 ? $lepusGetElementRefByLepusID("view", 535) : null;
          let $temp2 = $update2;
          if (!$n535) {
            $update2 = false;
            $n535 = __CreateView($currentComponentId);
            let $nid535 = $lepusStoreElementRefByLepusID($n535, 535, "view");
            __SetAttribute($n535, 1004, $nid535[1]);
            __AppendElement($parent, $n535);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.suffixContainerStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.suffixContainerStyle) {
                __SetStyleObject($n535, [1, 189, 45, 160, 190, 191, 192, 193, getCssPropertyIDObj($data.vo.suffixContainerStyle)]);
              }
            }
          }
          {
            let $n536 = $update2 ? $lepusGetElementRefByLepusID("view", 536) : null;
            let $temp3 = $update2;
            if (!$n536) {
              $update2 = false;
              $n536 = __CreateView($currentComponentId);
              let $nid536 = $lepusStoreElementRefByLepusID($n536, 536, "view");
              __SetAttribute($n536, 1004, $nid536[1]);
              __AppendElement($n535, $n536);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value91 = $data.vo.suffixInnerStyle;
                if (!$update2 || _$value91 !== $lepusTemplate._data.vo.suffixInnerStyle) {
                  __SetStyleObject($n536, [188, 194, 0, 1, 3, 44, 4, 45, 189, getCssPropertyIDObj($data.vo.suffixInnerStyle)]);
                }
              }
            }
            {
              let $n537 = $update2 ? $lepusGetElementRefByLepusID("text", 537) : null;
              let $temp4 = $update2;
              if (!$n537) {
                $update2 = false;
                $n537 = __CreateText($currentComponentId);
                let $nid537 = $lepusStoreElementRefByLepusID($n537, 537, "text");
                __SetAttribute($n537, 1004, $nid537[1]);
                __SetAttribute($n537, "text-maxline", "1");
                __AppendElement($n536, $n537);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value92 = $data.vo.suffixTextStyle;
                  if (!$update2 || _$value92 !== $lepusTemplate._data.vo.suffixTextStyle) {
                    __SetStyleObject($n537, [139, 21, 117, 89, getCssPropertyIDObj($data.vo.suffixTextStyle)]);
                  }
                }
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let _$value93 = $data.vo.suffixText;
                  if (!$update2 || _$value93 !== $lepusTemplate._data.vo.suffixText) {
                    __SetAttribute($n537, "text", _$value93);
                  }
                }
              }
              $update2 = $temp4;
            }
            {
              let $template_update = $update2;
              let $n539 = $update2 ? $lepusGetElementRefByLepusID("if", 539) : null;
              if (!$n539) {
                $update2 = false;
                $n539 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n539, 539, "if");
                __AppendElement($n536, $n539);
              }
              $renderTemplates[$path].update_1f68c58_539($lepusTemplate, $n539, $data, $update2);
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
  },
  update_1c6f8a8_541: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (($data.vo.prefixText || $data.vo.prefixIconUrl || $data.vo.prefixBackgroundImage) && $data.vo.suffixText && !((_a = $data.tagNormalState) == null ? undefined : _a.isOverflow) && $data.vo.connectImage) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n542 = $update2 ? $lepusGetElementRefByLepusID("image", 542) : null;
          let $temp2 = $update2;
          if (!$n542) {
            $update2 = false;
            $n542 = __CreateImage($currentComponentId);
            let $nid542 = $lepusStoreElementRefByLepusID($n542, 542, "image");
            __SetAttribute($n542, 1004, $nid542[1]);
            __SetAttribute($n542, "skip-redirection", true);
            __AppendElement($parent, $n542);
          }
          if (!$update2 || $renderTemplates["TagNormal"].varUpdateState[0]) {
            {
              let $value = $data.vo.connectImageStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.connectImageStyle) {
                __SetStyleObject($n542, [154, 52, getCssPropertyIDObj($data.vo.connectImageStyle)]);
              }
            }
            {
              let _$value94 = $data.vo.connectImage;
              if (!$update2 || _$value94 !== $lepusTemplate._data.vo.connectImage) {
                __SetAttribute($n542, "src", _$value94);
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
  update_1c6f8a8_543: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagNormal";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.suffixText && !((_a = $data.tagNormalState) == null ? undefined : _a.isOverflow)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n544 = $update2 ? $lepusGetElementRefByLepusID("view", 544) : null;
          let $temp2 = $update2;
          if (!$n544) {
            $update2 = false;
            $n544 = __CreateView($currentComponentId);
            let $nid544 = $lepusStoreElementRefByLepusID($n544, 544, "view");
            __SetAttribute($n544, 1004, $nid544[1]);
            __AppendElement($parent, $n544);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixContainerStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixContainerStyle) {
                __SetStyleObject($n544, [1, 195, 3, 44, 4, 196, 74, 31, 197, getCssPropertyIDObj($data.vo.prefixContainerStyle)]);
              }
            }
          }
          {
            let $template_update = $update2;
            let $n545 = $update2 ? $lepusGetElementRefByLepusID("if", 545) : null;
            if (!$n545) {
              $update2 = false;
              $n545 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n545, 545, "if");
              __AppendElement($n544, $n545);
            }
            $renderTemplates[$path].update_377e4c8_545($lepusTemplate, $n545, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update34 = $update2;
            let $n547 = $update2 ? $lepusGetElementRefByLepusID("if", 547) : null;
            if (!$n547) {
              $update2 = false;
              $n547 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n547, 547, "if");
              __AppendElement($n544, $n547);
            }
            $renderTemplates[$path].update_377e4c8_547($lepusTemplate, $n547, $data, $update2);
            $update2 = _$template_update34;
          }
          {
            let $n549 = $update2 ? $lepusGetElementRefByLepusID("text", 549) : null;
            let $temp3 = $update2;
            if (!$n549) {
              $update2 = false;
              $n549 = __CreateText($currentComponentId);
              let $nid549 = $lepusStoreElementRefByLepusID($n549, 549, "text");
              __SetAttribute($n549, 1004, $nid549[1]);
              __SetAttribute($n549, "text-maxline", "1");
              __AppendElement($n544, $n549);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value95 = $data.vo.prefixTextStyle;
                if (!$update2 || _$value95 !== $lepusTemplate._data.vo.prefixTextStyle) {
                  __SetStyleObject($n549, [139, 21, 117, 89, 139, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value96 = $data.vo.prefixText;
                if (!$update2 || _$value96 !== $lepusTemplate._data.vo.prefixText) {
                  __SetAttribute($n549, "text", _$value96);
                }
              }
            }
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp79 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n551 = $update2 ? $lepusGetElementRefByLepusID("view", 551) : null;
          let _$temp80 = $update2;
          if (!$n551) {
            $update2 = false;
            $n551 = __CreateView($currentComponentId);
            let $nid551 = $lepusStoreElementRefByLepusID($n551, 551, "view");
            __SetAttribute($n551, 1004, $nid551[1]);
            __AppendElement($parent, $n551);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let _$value97 = $data.vo.prefixContainerStyle;
              if (!$update2 || _$value97 !== $lepusTemplate._data.vo.prefixContainerStyle) {
                __SetStyleObject($n551, [1, 32, 3, 44, 4, 61, 74, 31, 197, getCssPropertyIDObj($data.vo.prefixContainerStyle)]);
              }
            }
          }
          {
            let _$template_update35 = $update2;
            let $n552 = $update2 ? $lepusGetElementRefByLepusID("if", 552) : null;
            if (!$n552) {
              $update2 = false;
              $n552 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n552, 552, "if");
              __AppendElement($n551, $n552);
            }
            $renderTemplates[$path].update_377e4c8_552($lepusTemplate, $n552, $data, $update2);
            $update2 = _$template_update35;
          }
          {
            let _$template_update36 = $update2;
            let $n554 = $update2 ? $lepusGetElementRefByLepusID("if", 554) : null;
            if (!$n554) {
              $update2 = false;
              $n554 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n554, 554, "if");
              __AppendElement($n551, $n554);
            }
            $renderTemplates[$path].update_377e4c8_554($lepusTemplate, $n554, $data, $update2);
            $update2 = _$template_update36;
          }
          {
            let $n556 = $update2 ? $lepusGetElementRefByLepusID("text", 556) : null;
            let _$temp81 = $update2;
            if (!$n556) {
              $update2 = false;
              $n556 = __CreateText($currentComponentId);
              let $nid556 = $lepusStoreElementRefByLepusID($n556, 556, "text");
              __SetAttribute($n556, 1004, $nid556[1]);
              __SetAttribute($n556, "text-maxline", "1");
              __AppendElement($n551, $n556);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value98 = $data.vo.prefixTextStyle;
                if (!$update2 || _$value98 !== $lepusTemplate._data.vo.prefixTextStyle) {
                  __SetStyleObject($n556, [139, 21, 117, 89, 139, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value99 = $data.vo.prefixText;
                if (!$update2 || _$value99 !== $lepusTemplate._data.vo.prefixText) {
                  __SetAttribute($n556, "text", _$value99);
                }
              }
            }
            $update2 = _$temp81;
          }
          $update2 = _$temp80;
        }
        $update2 = _$temp79;
      }
    }
  },
  update_377e4c8_545: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixBackgroundImage) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n546 = $update2 ? $lepusGetElementRefByLepusID("image", 546) : null;
          let $temp2 = $update2;
          if (!$n546) {
            $update2 = false;
            $n546 = __CreateImage($currentComponentId);
            let $nid546 = $lepusStoreElementRefByLepusID($n546, 546, "image");
            __SetAttribute($n546, 1004, $nid546[1]);
            __SetAttribute($n546, "cap-insets-scale", "3");
            __SetAttribute($n546, "skip-redirection", true);
            __AppendElement($parent, $n546);
          }
          if (!$update2 || $renderTemplates["TagNormal"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixBackgroundImageStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixBackgroundImageStyle) {
                __SetStyleObject($n546, [55, 1, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.prefixBackgroundImageStyle)]);
              }
            }
            {
              let _$value100 = $data.vo.prefixBackgroundImage;
              if (!$update2 || _$value100 !== $lepusTemplate._data.vo.prefixBackgroundImage) {
                __SetAttribute($n546, "src", _$value100);
              }
            }
            {
              let _$value101 = $data.vo.prefixBackgroundImageCapInsets;
              if (!$update2 || _$value101 !== $lepusTemplate._data.vo.prefixBackgroundImageCapInsets) {
                __SetAttribute($n546, "cap-insets", _$value101);
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
  update_377e4c8_547: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixIconUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n548 = $update2 ? $lepusGetElementRefByLepusID("image", 548) : null;
          let $temp2 = $update2;
          if (!$n548) {
            $update2 = false;
            $n548 = __CreateImage($currentComponentId);
            let $nid548 = $lepusStoreElementRefByLepusID($n548, 548, "image");
            __SetAttribute($n548, 1004, $nid548[1]);
            __SetAttribute($n548, "auto-size", "true");
            __SetAttribute($n548, "skip-redirection", true);
            __AppendElement($parent, $n548);
          }
          if (!$update2 || $renderTemplates["TagNormal"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixIconStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixIconStyle) {
                __SetStyleObject($n548, [1, getCssPropertyIDObj($data.vo.prefixIconStyle)]);
              }
            }
            {
              let _$value102 = $data.vo.prefixIconUrl;
              if (!$update2 || _$value102 !== $lepusTemplate._data.vo.prefixIconUrl) {
                __SetAttribute($n548, "src", _$value102);
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
  update_377e4c8_552: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixBackgroundImage) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n553 = $update2 ? $lepusGetElementRefByLepusID("image", 553) : null;
          let $temp2 = $update2;
          if (!$n553) {
            $update2 = false;
            $n553 = __CreateImage($currentComponentId);
            let $nid553 = $lepusStoreElementRefByLepusID($n553, 553, "image");
            __SetAttribute($n553, 1004, $nid553[1]);
            __SetAttribute($n553, "cap-insets-scale", "3");
            __SetAttribute($n553, "skip-redirection", true);
            __AppendElement($parent, $n553);
          }
          if (!$update2 || $renderTemplates["TagNormal"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixBackgroundImageStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixBackgroundImageStyle) {
                __SetStyleObject($n553, [55, 1, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.prefixBackgroundImageStyle)]);
              }
            }
            {
              let _$value103 = $data.vo.prefixBackgroundImage;
              if (!$update2 || _$value103 !== $lepusTemplate._data.vo.prefixBackgroundImage) {
                __SetAttribute($n553, "src", _$value103);
              }
            }
            {
              let _$value104 = $data.vo.prefixBackgroundImageCapInsets;
              if (!$update2 || _$value104 !== $lepusTemplate._data.vo.prefixBackgroundImageCapInsets) {
                __SetAttribute($n553, "cap-insets", _$value104);
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
  update_377e4c8_554: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixIconUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n555 = $update2 ? $lepusGetElementRefByLepusID("image", 555) : null;
          let $temp2 = $update2;
          if (!$n555) {
            $update2 = false;
            $n555 = __CreateImage($currentComponentId);
            let $nid555 = $lepusStoreElementRefByLepusID($n555, 555, "image");
            __SetAttribute($n555, 1004, $nid555[1]);
            __SetAttribute($n555, "auto-size", "true");
            __SetAttribute($n555, "skip-redirection", true);
            __AppendElement($parent, $n555);
          }
          if (!$update2 || $renderTemplates["TagNormal"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixIconStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixIconStyle) {
                __SetStyleObject($n555, [1, getCssPropertyIDObj($data.vo.prefixIconStyle)]);
              }
            }
            {
              let _$value105 = $data.vo.prefixIconUrl;
              if (!$update2 || _$value105 !== $lepusTemplate._data.vo.prefixIconUrl) {
                __SetAttribute($n555, "src", _$value105);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagNormal";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n531 = $lepusGetElementRefByLepusID("if", 531);
    $renderTemplates[$path].update_27ceeb0_531($lepusTemplate, $n531, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n531 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n531, 531, "if");
    __AppendElement($template, $n531);
    $renderTemplates["TagNormal"].update_27ceeb0_531($lepusTemplate, $n531, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagCountdown"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_1f68c58_558: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagCountdown";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.vo.canShow) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n559 = $update2 ? $lepusGetElementRefByLepusID("view", 559) : null;
            let $temp2 = $update2;
            if (!$n559) {
              $update2 = false;
              $n559 = __CreateView($currentComponentId);
              let $nid559 = $lepusStoreElementRefByLepusID($n559, 559, "view");
              __SetAttribute($n559, 1004, $nid559[1]);
              __AppendElement($parent, $n559);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n559, [31, 170, 183, 3, 87, 4, 52, 74, 173, 198, 123, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $n560 = $update2 ? $lepusGetElementRefByLepusID("view", 560) : null;
              let $temp3 = $update2;
              if (!$n560) {
                $update2 = false;
                $n560 = __CreateView($currentComponentId);
                let $nid560 = $lepusStoreElementRefByLepusID($n560, 560, "view");
                __SetAttribute($n560, 1004, $nid560[1]);
                __AppendElement($n559, $n560);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value106 = $data.vo.prefixContainerStyle;
                  if (!$update2 || _$value106 !== $lepusTemplate._data.vo.prefixContainerStyle) {
                    __SetStyleObject($n560, [3, 1, 4, 31, 101, 29, getCssPropertyIDObj($data.vo.prefixContainerStyle)]);
                  }
                }
              }
              {
                let $n561 = $update2 ? $lepusGetElementRefByLepusID("image", 561) : null;
                let $temp4 = $update2;
                if (!$n561) {
                  $update2 = false;
                  $n561 = __CreateImage($currentComponentId);
                  let $nid561 = $lepusStoreElementRefByLepusID($n561, 561, "image");
                  __SetAttribute($n561, 1004, $nid561[1]);
                  __SetAttribute($n561, "skip-redirection", true);
                  __AppendElement($n560, $n561);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value107 = $data.vo.prefixIconStyle;
                    if (!$update2 || _$value107 !== $lepusTemplate._data.vo.prefixIconStyle) {
                      __SetStyleObject($n561, [174, 52, 7, 31, getCssPropertyIDObj($data.vo.prefixIconStyle)]);
                    }
                  }
                  {
                    let _$value108 = $data.vo.prefixIconUrl;
                    if (!$update2 || _$value108 !== $lepusTemplate._data.vo.prefixIconUrl) {
                      __SetAttribute($n561, "src", _$value108);
                    }
                  }
                }
                $update2 = $temp4;
              }
              {
                let $n562 = $update2 ? $lepusGetElementRefByLepusID("text", 562) : null;
                let _$temp82 = $update2;
                if (!$n562) {
                  $update2 = false;
                  $n562 = __CreateText($currentComponentId);
                  let $nid562 = $lepusStoreElementRefByLepusID($n562, 562, "text");
                  __SetAttribute($n562, 1004, $nid562[1]);
                  __SetAttribute($n562, "text-single-line-vertical-align", "center");
                  __AppendElement($n560, $n562);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value109 = $data.vo.prefixTextStyle;
                    if (!$update2 || _$value109 !== $lepusTemplate._data.vo.prefixTextStyle) {
                      __SetStyleObject($n562, [89, 139, 21, 31, 117, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
                    }
                  }
                }
                {
                  if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                    let _$value110 = $data.vo.prefixText;
                    if (!$update2 || _$value110 !== $lepusTemplate._data.vo.prefixText) {
                      __SetAttribute($n562, "text", _$value110);
                    }
                  }
                }
                $update2 = _$temp82;
              }
              $update2 = $temp3;
            }
            {
              let $n564 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 564) : null;
              let _$temp83 = $update2;
              if (!$n564) {
                $update2 = false;
                $n564 = __CreateElement("countdown-view", $currentComponentId);
                let $nid564 = $lepusStoreElementRefByLepusID($n564, 564, "countdown-view");
                __SetAttribute($n564, 1004, $nid564[1]);
                __AddEvent($n564, "bindEvent", "countdownend", "onTagCountDownEnd");
                __AppendElement($n559, $n564);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value111 = $data.vo.countdownTextStyle;
                  if (!$update2 || _$value111 !== $lepusTemplate._data.vo.countdownTextStyle) {
                    __SetStyleObject($n564, [199, 21, 139, getCssPropertyIDObj($data.vo.countdownTextStyle)]);
                  }
                }
                {
                  let _$value112 = "" + $data.vo.endTime;
                  if (!$update2 || _$value112 !== "" + $lepusTemplate._data.vo.endTime) {
                    __SetAttribute($n564, "end-time", _$value112);
                  }
                }
                {
                  let _$value113 = ((_a = lynx.__globalProps) == null ? undefined : _a.os) === "android" ? "ms" : "millseconds";
                  if (!$update2 || _$value113 !== undefined) {
                    __SetAttribute($n564, "unit", _$value113);
                  }
                }
              }
              {
                let $template_update = $update2;
                let $n565 = $update2 ? $lepusGetElementRefByLepusID("if", 565) : null;
                if (!$n565) {
                  $update2 = false;
                  $n565 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n565, 565, "if");
                  __AppendElement($n564, $n565);
                }
                $renderTemplates[$path].update_1f68c58_565($lepusTemplate, $n565, $data, $update2);
                $update2 = $template_update;
              }
              {
                let $n568 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 568) : null;
                let _$temp84 = $update2;
                if (!$n568) {
                  $update2 = false;
                  $n568 = __CreateElement("countdown-item", $currentComponentId);
                  let $nid568 = $lepusStoreElementRefByLepusID($n568, 568, "countdown-item");
                  __SetAttribute($n568, 1004, $nid568[1]);
                  __SetAttribute($n568, "text-maxline", "1");
                  __SetAttribute($n568, "countdown-display", "HH:mm:ss");
                  __AppendElement($n564, $n568);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value114 = $data.vo.countdownTextStyle;
                    if (!$update2 || _$value114 !== $lepusTemplate._data.vo.countdownTextStyle) {
                      __SetStyleObject($n568, [199, 21, 139, getCssPropertyIDObj($data.vo.countdownTextStyle)]);
                    }
                  }
                }
                $update2 = _$temp84;
              }
              $update2 = _$temp83;
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
  update_1f68c58_565: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "TagCountdown";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.countdownPrefix) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n566 = $update2 ? $lepusGetElementRefByLepusID("text", 566) : null;
          let $temp2 = $update2;
          if (!$n566) {
            $update2 = false;
            $n566 = __CreateText($currentComponentId);
            let $nid566 = $lepusStoreElementRefByLepusID($n566, 566, "text");
            __SetAttribute($n566, 1004, $nid566[1]);
            __AppendElement($parent, $n566);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.countdownTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.countdownTextStyle) {
                __SetStyleObject($n566, [199, 21, 139, getCssPropertyIDObj($data.vo.countdownTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value115 = $data.vo.countdownPrefix;
              if (!$update2 || _$value115 !== $lepusTemplate._data.vo.countdownPrefix) {
                __SetAttribute($n566, "text", _$value115);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagCountdown";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n558 = $lepusGetElementRefByLepusID("if", 558);
    $renderTemplates[$path].update_1f68c58_558($lepusTemplate, $n558, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n558 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n558, 558, "if");
    __AppendElement($template, $n558);
    $renderTemplates["TagCountdown"].update_1f68c58_558($lepusTemplate, $n558, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagProgress"] = {
  variables: ["vo", "tagProgressState"],
  varUpdateState: [],
  update_377e4c8_576: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagProgress";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.progressType == "countdown") {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n577 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 577) : null;
          let $temp2 = $update2;
          if (!$n577) {
            $update2 = false;
            $n577 = __CreateElement("countdown-view", $currentComponentId);
            let $nid577 = $lepusStoreElementRefByLepusID($n577, 577, "countdown-view");
            __SetAttribute($n577, 1004, $nid577[1]);
            __SetID($n577, "countdown-seckill");
            __AddEvent($n577, "bindEvent", "countdownend", "onTagProgressCountDownEnd");
            __AppendElement($parent, $n577);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.progressCountdownTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.progressCountdownTextStyle) {
                __SetStyleObject($n577, [121, 21, 116, 117, getCssPropertyIDObj($data.vo.progressCountdownTextStyle)]);
              }
            }
            {
              let _$value116 = "" + $data.vo.progressCountdownEndTime;
              if (!$update2 || _$value116 !== "" + $lepusTemplate._data.vo.progressCountdownEndTime) {
                __SetAttribute($n577, "end-time", _$value116);
              }
            }
            {
              let _$value117 = ((_a = lynx.__globalProps) == null ? undefined : _a.os) === "android" ? "ms" : "millseconds";
              if (!$update2 || _$value117 !== undefined) {
                __SetAttribute($n577, "unit", _$value117);
              }
            }
          }
          {
            let $n578 = $update2 ? $lepusGetElementRefByLepusID("text", 578) : null;
            let $temp3 = $update2;
            if (!$n578) {
              $update2 = false;
              $n578 = __CreateText($currentComponentId);
              let $nid578 = $lepusStoreElementRefByLepusID($n578, 578, "text");
              __SetAttribute($n578, 1004, $nid578[1]);
              __AppendElement($n577, $n578);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value118 = $data.vo.progressCountdownTextStyle;
                if (!$update2 || _$value118 !== $lepusTemplate._data.vo.progressCountdownTextStyle) {
                  __SetStyleObject($n578, [121, 21, 116, 117, getCssPropertyIDObj($data.vo.progressCountdownTextStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value119 = $data.vo.progressCountdownPrefix;
                if (!$update2 || _$value119 !== $lepusTemplate._data.vo.progressCountdownPrefix) {
                  __SetAttribute($n578, "text", _$value119);
                }
              }
            }
            $update2 = $temp3;
          }
          {
            let $n580 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 580) : null;
            let _$temp85 = $update2;
            if (!$n580) {
              $update2 = false;
              $n580 = __CreateElement("countdown-item", $currentComponentId);
              let $nid580 = $lepusStoreElementRefByLepusID($n580, 580, "countdown-item");
              __SetAttribute($n580, 1004, $nid580[1]);
              __SetAttribute($n580, "countdown-display", "HH:mm:ss");
              __AppendElement($n577, $n580);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value120 = $data.vo.progressCountdownTextStyle;
                if (!$update2 || _$value120 !== $lepusTemplate._data.vo.progressCountdownTextStyle) {
                  __SetStyleObject($n580, [121, 21, 116, 117, getCssPropertyIDObj($data.vo.progressCountdownTextStyle)]);
                }
              }
            }
            $update2 = _$temp85;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp86 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n581 = $update2 ? $lepusGetElementRefByLepusID("text", 581) : null;
          let _$temp87 = $update2;
          if (!$n581) {
            $update2 = false;
            $n581 = __CreateText($currentComponentId);
            let $nid581 = $lepusStoreElementRefByLepusID($n581, 581, "text");
            __SetAttribute($n581, 1004, $nid581[1]);
            __SetAttribute($n581, "text-maxline", "1");
            __AppendElement($parent, $n581);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let _$value121 = $data.vo.progressTextStyle;
              if (!$update2 || _$value121 !== $lepusTemplate._data.vo.progressTextStyle) {
                __SetStyleObject($n581, [139, 21, 117, 89, getCssPropertyIDObj($data.vo.progressTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value122 = $data.vo.progressText;
              if (!$update2 || _$value122 !== $lepusTemplate._data.vo.progressText) {
                __SetAttribute($n581, "text", _$value122);
              }
            }
          }
          $update2 = _$temp87;
        }
        $update2 = _$temp86;
      }
    }
  },
  update_377e4c8_589: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixIconUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n590 = $update2 ? $lepusGetElementRefByLepusID("image", 590) : null;
          let $temp2 = $update2;
          if (!$n590) {
            $update2 = false;
            $n590 = __CreateImage($currentComponentId);
            let $nid590 = $lepusStoreElementRefByLepusID($n590, 590, "image");
            __SetAttribute($n590, 1004, $nid590[1]);
            __SetAttribute($n590, "auto-size", "true");
            __SetAttribute($n590, "skip-redirection", true);
            __AppendElement($parent, $n590);
          }
          if (!$update2 || $renderTemplates["TagProgress"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixIconStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixIconStyle) {
                __SetStyleObject($n590, [1, getCssPropertyIDObj($data.vo.prefixIconStyle)]);
              }
            }
            {
              let _$value123 = $data.vo.prefixIconUrl;
              if (!$update2 || _$value123 !== $lepusTemplate._data.vo.prefixIconUrl) {
                __SetAttribute($n590, "src", _$value123);
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
  update_377e4c8_593: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.showMoreIcon) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n594 = $update2 ? $lepusGetElementRefByLepusID("template", 594) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n594) {
            $templateCreated = false;
            $n594 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n594, 594, "template");
            __AppendElement($parent, $n594);
            $templateId = __GetElementUniqueID($n594);
            $childLepusTemplate = $templateConstructor($templateId, $n594);
          } else {
            $templateId = __GetElementUniqueID($n594);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.prefixMoreIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.prefixMoreIconColor, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n594, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_377e4c8_601: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixIconUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n602 = $update2 ? $lepusGetElementRefByLepusID("image", 602) : null;
          let $temp2 = $update2;
          if (!$n602) {
            $update2 = false;
            $n602 = __CreateImage($currentComponentId);
            let $nid602 = $lepusStoreElementRefByLepusID($n602, 602, "image");
            __SetAttribute($n602, 1004, $nid602[1]);
            __SetAttribute($n602, "auto-size", "true");
            __SetAttribute($n602, "skip-redirection", true);
            __AppendElement($parent, $n602);
          }
          if (!$update2 || $renderTemplates["TagProgress"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixIconStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixIconStyle) {
                __SetStyleObject($n602, [1, getCssPropertyIDObj($data.vo.prefixIconStyle)]);
              }
            }
            {
              let _$value124 = $data.vo.prefixIconUrl;
              if (!$update2 || _$value124 !== $lepusTemplate._data.vo.prefixIconUrl) {
                __SetAttribute($n602, "src", _$value124);
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
  update_377e4c8_605: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.showMoreIcon) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n606 = $update2 ? $lepusGetElementRefByLepusID("template", 606) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n606) {
            $templateCreated = false;
            $n606 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n606, 606, "template");
            __AppendElement($parent, $n606);
            $templateId = __GetElementUniqueID($n606);
            $childLepusTemplate = $templateConstructor($templateId, $n606);
          } else {
            $templateId = __GetElementUniqueID($n606);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.prefixMoreIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.prefixMoreIconColor, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n606, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_518ee8_571: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagProgress";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!((_a = $data.tagProgressState) == null ? undefined : _a.isOverflow) && ($data.vo.progressType == "countdown" ? $data.vo.progressCountdownEndTime : $data.vo.progressText)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n572 = $update2 ? $lepusGetElementRefByLepusID("view", 572) : null;
          let $temp2 = $update2;
          if (!$n572) {
            $update2 = false;
            $n572 = __CreateView($currentComponentId);
            let $nid572 = $lepusStoreElementRefByLepusID($n572, 572, "view");
            __SetAttribute($n572, 1004, $nid572[1]);
            __SetID($n572, "tagProgressInner");
            __AppendElement($parent, $n572);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.innerStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.innerStyle) {
                __SetStyleObject($n572, [201, 173, 1, 3, 187, 91, 4, 101, getCssPropertyIDObj($data.vo.innerStyle)]);
              }
            }
          }
          {
            let $n573 = $update2 ? $lepusGetElementRefByLepusID("view", 573) : null;
            let $temp3 = $update2;
            if (!$n573) {
              $update2 = false;
              $n573 = __CreateView($currentComponentId);
              let $nid573 = $lepusStoreElementRefByLepusID($n573, 573, "view");
              __SetAttribute($n573, 1004, $nid573[1]);
              __AppendElement($n572, $n573);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value125 = $data.vo.progressContainerStyle;
                if (!$update2 || _$value125 !== $lepusTemplate._data.vo.progressContainerStyle) {
                  __SetStyleObject($n573, [1, 202, 3, 44, 4, 45, 203, 59, 204, 14, getCssPropertyIDObj($data.vo.progressContainerStyle)]);
                }
              }
            }
            {
              let $n574 = $update2 ? $lepusGetElementRefByLepusID("image", 574) : null;
              let $temp4 = $update2;
              if (!$n574) {
                $update2 = false;
                $n574 = __CreateImage($currentComponentId);
                let $nid574 = $lepusStoreElementRefByLepusID($n574, 574, "image");
                __SetAttribute($n574, 1004, $nid574[1]);
                __SetAttribute($n574, "cap-insets-scale", "3");
                __SetAttribute($n574, "skip-redirection", true);
                __AppendElement($n573, $n574);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value126 = $data.vo.progressBackgroundImageStyle;
                  if (!$update2 || _$value126 !== $lepusTemplate._data.vo.progressBackgroundImageStyle) {
                    __SetStyleObject($n574, [55, 1, 0, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.progressBackgroundImageStyle)]);
                  }
                }
                {
                  let _$value127 = $data.vo.progressBackgroundImageCapInsets;
                  if (!$update2 || _$value127 !== $lepusTemplate._data.vo.progressBackgroundImageCapInsets) {
                    __SetAttribute($n574, "cap-insets", _$value127);
                  }
                }
                {
                  let _$value128 = $data.vo.progressBackgroundImage;
                  if (!$update2 || _$value128 !== $lepusTemplate._data.vo.progressBackgroundImage) {
                    __SetAttribute($n574, "src", _$value128);
                  }
                }
              }
              $update2 = $temp4;
            }
            {
              let $n575 = $update2 ? $lepusGetElementRefByLepusID("image", 575) : null;
              let _$temp88 = $update2;
              if (!$n575) {
                $update2 = false;
                $n575 = __CreateImage($currentComponentId);
                let $nid575 = $lepusStoreElementRefByLepusID($n575, 575, "image");
                __SetAttribute($n575, 1004, $nid575[1]);
                __SetAttribute($n575, "cap-insets-scale", "3");
                __SetAttribute($n575, "skip-redirection", true);
                __AppendElement($n573, $n575);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n575, [55, 1, 75, 76, 77, {
                    27: $data.vo.progressPercent + "%"
                  }, getCssPropertyIDObj($data.vo.progressRateImageStyle)]);
                }
                {
                  let _$value129 = $data.vo.progressRateImageCapInsets;
                  if (!$update2 || _$value129 !== $lepusTemplate._data.vo.progressRateImageCapInsets) {
                    __SetAttribute($n575, "cap-insets", _$value129);
                  }
                }
                {
                  let _$value130 = $data.vo.progressRateImage;
                  if (!$update2 || _$value130 !== $lepusTemplate._data.vo.progressRateImage) {
                    __SetAttribute($n575, "src", _$value130);
                  }
                }
              }
              $update2 = _$temp88;
            }
            {
              let $template_update = $update2;
              let $n576 = $update2 ? $lepusGetElementRefByLepusID("if", 576) : null;
              if (!$n576) {
                $update2 = false;
                $n576 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n576, 576, "if");
                __AppendElement($n573, $n576);
              }
              $renderTemplates[$path].update_377e4c8_576($lepusTemplate, $n576, $data, $update2);
              $update2 = $template_update;
            }
            $update2 = $temp3;
          }
          {
            let _$template_update37 = $update2;
            let $n583 = $update2 ? $lepusGetElementRefByLepusID("if", 583) : null;
            if (!$n583) {
              $update2 = false;
              $n583 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n583, 583, "if");
              __AppendElement($n572, $n583);
            }
            $renderTemplates[$path].update_518ee8_583($lepusTemplate, $n583, $data, $update2);
            $update2 = _$template_update37;
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
  update_518ee8_583: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagProgress";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (($data.vo.prefixText || $data.vo.prefixIconUrl || $data.vo.prefixBackgroundImage) && !((_a = $data.tagProgressState) == null ? undefined : _a.isOverflow) && ($data.vo.progressType == "countdown" ? $data.vo.progressCountdownEndTime : $data.vo.progressText)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n584 = $update2 ? $lepusGetElementRefByLepusID("view", 584) : null;
          let $temp2 = $update2;
          if (!$n584) {
            $update2 = false;
            $n584 = __CreateView($currentComponentId);
            let $nid584 = $lepusStoreElementRefByLepusID($n584, 584, "view");
            __SetAttribute($n584, 1004, $nid584[1]);
            __AddEvent($n584, "catchEvent", "tap", $data.vo.showMoreIcon ? "onTagClick" : "");
            __AppendElement($parent, $n584);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixContainerStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixContainerStyle) {
                __SetStyleObject($n584, [1, 3, 44, 4, 205, 74, 31, 173, getCssPropertyIDObj($data.vo.prefixContainerStyle)]);
              }
            }
          }
          {
            let $template_update = $update2;
            let $n585 = $update2 ? $lepusGetElementRefByLepusID("if", 585) : null;
            if (!$n585) {
              $update2 = false;
              $n585 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n585, 585, "if");
              __AppendElement($n584, $n585);
            }
            $renderTemplates[$path].update_518ee8_585($lepusTemplate, $n585, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update38 = $update2;
            let $n587 = $update2 ? $lepusGetElementRefByLepusID("if", 587) : null;
            if (!$n587) {
              $update2 = false;
              $n587 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n587, 587, "if");
              __AppendElement($n584, $n587);
            }
            $renderTemplates[$path].update_518ee8_587($lepusTemplate, $n587, $data, $update2);
            $update2 = _$template_update38;
          }
          {
            let _$template_update39 = $update2;
            let $n589 = $update2 ? $lepusGetElementRefByLepusID("if", 589) : null;
            if (!$n589) {
              $update2 = false;
              $n589 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n589, 589, "if");
              __AppendElement($n584, $n589);
            }
            $renderTemplates[$path].update_377e4c8_589($lepusTemplate, $n589, $data, $update2);
            $update2 = _$template_update39;
          }
          {
            let $n591 = $update2 ? $lepusGetElementRefByLepusID("text", 591) : null;
            let $temp3 = $update2;
            if (!$n591) {
              $update2 = false;
              $n591 = __CreateText($currentComponentId);
              let $nid591 = $lepusStoreElementRefByLepusID($n591, 591, "text");
              __SetAttribute($n591, 1004, $nid591[1]);
              __SetAttribute($n591, "text-maxline", "1");
              __AppendElement($n584, $n591);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value131 = $data.vo.prefixTextStyle;
                if (!$update2 || _$value131 !== $lepusTemplate._data.vo.prefixTextStyle) {
                  __SetStyleObject($n591, [139, 21, 117, 63, 139, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value132 = $data.vo.prefixText;
                if (!$update2 || _$value132 !== $lepusTemplate._data.vo.prefixText) {
                  __SetAttribute($n591, "text", _$value132);
                }
              }
            }
            $update2 = $temp3;
          }
          {
            let _$template_update40 = $update2;
            let $n593 = $update2 ? $lepusGetElementRefByLepusID("if", 593) : null;
            if (!$n593) {
              $update2 = false;
              $n593 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n593, 593, "if");
              __AppendElement($n584, $n593);
            }
            $renderTemplates[$path].update_377e4c8_593($lepusTemplate, $n593, $data, $update2);
            $update2 = _$template_update40;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp89 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let _$template_update41 = $update2;
          let $n595 = $update2 ? $lepusGetElementRefByLepusID("if", 595) : null;
          if (!$n595) {
            $update2 = false;
            $n595 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n595, 595, "if");
            __AppendElement($parent, $n595);
          }
          $renderTemplates[$path].update_518ee8_595($lepusTemplate, $n595, $data, $update2);
          $update2 = _$template_update41;
        }
        $update2 = _$temp89;
      }
    }
  },
  update_518ee8_585: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!((_a = $data.tagProgressState) == null ? undefined : _a.isOverflow) && ($data.vo.progressType == "countdown" ? $data.vo.progressCountdownEndTime : $data.vo.progressText) && $data.vo.prefixBackgroundImage) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n586 = $update2 ? $lepusGetElementRefByLepusID("image", 586) : null;
          let $temp2 = $update2;
          if (!$n586) {
            $update2 = false;
            $n586 = __CreateImage($currentComponentId);
            let $nid586 = $lepusStoreElementRefByLepusID($n586, 586, "image");
            __SetAttribute($n586, 1004, $nid586[1]);
            __SetAttribute($n586, "cap-insets-scale", "3");
            __SetAttribute($n586, "skip-redirection", true);
            __AppendElement($parent, $n586);
          }
          if (!$update2 || $renderTemplates["TagProgress"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixBackgroundImageStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixBackgroundImageStyle) {
                __SetStyleObject($n586, [55, 1, 0, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.prefixBackgroundImageStyle)]);
              }
            }
            {
              let _$value133 = $data.vo.prefixBackgroundImage;
              if (!$update2 || _$value133 !== $lepusTemplate._data.vo.prefixBackgroundImage) {
                __SetAttribute($n586, "src", _$value133);
              }
            }
            {
              let _$value134 = $data.vo.prefixBackgroundImageCapInsets;
              if (!$update2 || _$value134 !== $lepusTemplate._data.vo.prefixBackgroundImageCapInsets) {
                __SetAttribute($n586, "cap-insets", _$value134);
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
  update_518ee8_587: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!(!((_a = $data.tagProgressState) == null ? undefined : _a.isOverflow) && ($data.vo.progressType == "countdown" ? $data.vo.progressCountdownEndTime : $data.vo.progressText)) && $data.vo.prefixBackgroundWithoutProgressImage) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n588 = $update2 ? $lepusGetElementRefByLepusID("image", 588) : null;
          let $temp2 = $update2;
          if (!$n588) {
            $update2 = false;
            $n588 = __CreateImage($currentComponentId);
            let $nid588 = $lepusStoreElementRefByLepusID($n588, 588, "image");
            __SetAttribute($n588, 1004, $nid588[1]);
            __SetAttribute($n588, "cap-insets-scale", "3");
            __SetAttribute($n588, "skip-redirection", true);
            __AppendElement($parent, $n588);
          }
          if (!$update2 || $renderTemplates["TagProgress"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixBackgroundWithoutProgressImageStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixBackgroundWithoutProgressImageStyle) {
                __SetStyleObject($n588, [55, 1, 0, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.prefixBackgroundWithoutProgressImageStyle)]);
              }
            }
            {
              let _$value135 = $data.vo.prefixBackgroundWithoutProgressImage;
              if (!$update2 || _$value135 !== $lepusTemplate._data.vo.prefixBackgroundWithoutProgressImage) {
                __SetAttribute($n588, "src", _$value135);
              }
            }
            {
              let _$value136 = $data.vo.prefixBackgroundWithoutProgressImageCapInsets;
              if (!$update2 || _$value136 !== $lepusTemplate._data.vo.prefixBackgroundWithoutProgressImageCapInsets) {
                __SetAttribute($n588, "cap-insets", _$value136);
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
  update_518ee8_595: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "TagProgress";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.prefixText || $data.vo.prefixIconUrl || $data.vo.prefixBackgroundImage) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n596 = $update2 ? $lepusGetElementRefByLepusID("view", 596) : null;
          let $temp2 = $update2;
          if (!$n596) {
            $update2 = false;
            $n596 = __CreateView($currentComponentId);
            let $nid596 = $lepusStoreElementRefByLepusID($n596, 596, "view");
            __SetAttribute($n596, 1004, $nid596[1]);
            __AddEvent($n596, "catchEvent", "tap", $data.vo.showMoreIcon ? "onTagClick" : "");
            __AppendElement($parent, $n596);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixContainerStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixContainerStyle) {
                __SetStyleObject($n596, [1, 3, 44, 4, 206, 74, 31, 173, 32, getCssPropertyIDObj($data.vo.prefixContainerStyle)]);
              }
            }
          }
          {
            let $template_update = $update2;
            let $n597 = $update2 ? $lepusGetElementRefByLepusID("if", 597) : null;
            if (!$n597) {
              $update2 = false;
              $n597 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n597, 597, "if");
              __AppendElement($n596, $n597);
            }
            $renderTemplates[$path].update_518ee8_597($lepusTemplate, $n597, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update42 = $update2;
            let $n599 = $update2 ? $lepusGetElementRefByLepusID("if", 599) : null;
            if (!$n599) {
              $update2 = false;
              $n599 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n599, 599, "if");
              __AppendElement($n596, $n599);
            }
            $renderTemplates[$path].update_518ee8_599($lepusTemplate, $n599, $data, $update2);
            $update2 = _$template_update42;
          }
          {
            let _$template_update43 = $update2;
            let $n601 = $update2 ? $lepusGetElementRefByLepusID("if", 601) : null;
            if (!$n601) {
              $update2 = false;
              $n601 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n601, 601, "if");
              __AppendElement($n596, $n601);
            }
            $renderTemplates[$path].update_377e4c8_601($lepusTemplate, $n601, $data, $update2);
            $update2 = _$template_update43;
          }
          {
            let $n603 = $update2 ? $lepusGetElementRefByLepusID("text", 603) : null;
            let $temp3 = $update2;
            if (!$n603) {
              $update2 = false;
              $n603 = __CreateText($currentComponentId);
              let $nid603 = $lepusStoreElementRefByLepusID($n603, 603, "text");
              __SetAttribute($n603, 1004, $nid603[1]);
              __SetAttribute($n603, "text-maxline", "1");
              __AppendElement($n596, $n603);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value137 = $data.vo.prefixTextStyle;
                if (!$update2 || _$value137 !== $lepusTemplate._data.vo.prefixTextStyle) {
                  __SetStyleObject($n603, [139, 21, 117, 63, 139, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
                }
              }
            }
            {
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                let _$value138 = $data.vo.prefixText;
                if (!$update2 || _$value138 !== $lepusTemplate._data.vo.prefixText) {
                  __SetAttribute($n603, "text", _$value138);
                }
              }
            }
            $update2 = $temp3;
          }
          {
            let _$template_update44 = $update2;
            let $n605 = $update2 ? $lepusGetElementRefByLepusID("if", 605) : null;
            if (!$n605) {
              $update2 = false;
              $n605 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n605, 605, "if");
              __AppendElement($n596, $n605);
            }
            $renderTemplates[$path].update_377e4c8_605($lepusTemplate, $n605, $data, $update2);
            $update2 = _$template_update44;
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
  update_518ee8_597: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!((_a = $data.tagProgressState) == null ? undefined : _a.isOverflow) && ($data.vo.progressType == "countdown" ? $data.vo.progressCountdownEndTime : $data.vo.progressText) && $data.vo.prefixBackgroundImage) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n598 = $update2 ? $lepusGetElementRefByLepusID("image", 598) : null;
          let $temp2 = $update2;
          if (!$n598) {
            $update2 = false;
            $n598 = __CreateImage($currentComponentId);
            let $nid598 = $lepusStoreElementRefByLepusID($n598, 598, "image");
            __SetAttribute($n598, 1004, $nid598[1]);
            __SetAttribute($n598, "cap-insets-scale", "3");
            __SetAttribute($n598, "skip-redirection", true);
            __AppendElement($parent, $n598);
          }
          if (!$update2 || $renderTemplates["TagProgress"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixBackgroundImageStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixBackgroundImageStyle) {
                __SetStyleObject($n598, [55, 1, 0, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.prefixBackgroundImageStyle)]);
              }
            }
            {
              let _$value139 = $data.vo.prefixBackgroundImage;
              if (!$update2 || _$value139 !== $lepusTemplate._data.vo.prefixBackgroundImage) {
                __SetAttribute($n598, "src", _$value139);
              }
            }
            {
              let _$value140 = $data.vo.prefixBackgroundImageCapInsets;
              if (!$update2 || _$value140 !== $lepusTemplate._data.vo.prefixBackgroundImageCapInsets) {
                __SetAttribute($n598, "cap-insets", _$value140);
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
  update_518ee8_599: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!(!((_a = $data.tagProgressState) == null ? undefined : _a.isOverflow) && ($data.vo.progressType == "countdown" ? $data.vo.progressCountdownEndTime : $data.vo.progressText)) && $data.vo.prefixBackgroundWithoutProgressImage) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n600 = $update2 ? $lepusGetElementRefByLepusID("image", 600) : null;
          let $temp2 = $update2;
          if (!$n600) {
            $update2 = false;
            $n600 = __CreateImage($currentComponentId);
            let $nid600 = $lepusStoreElementRefByLepusID($n600, 600, "image");
            __SetAttribute($n600, 1004, $nid600[1]);
            __SetAttribute($n600, "cap-insets-scale", "3");
            __SetAttribute($n600, "skip-redirection", true);
            __AppendElement($parent, $n600);
          }
          if (!$update2 || $renderTemplates["TagProgress"].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixBackgroundWithoutProgressImageStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixBackgroundWithoutProgressImageStyle) {
                __SetStyleObject($n600, [55, 1, 0, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.prefixBackgroundWithoutProgressImageStyle)]);
              }
            }
            {
              let _$value141 = $data.vo.prefixBackgroundWithoutProgressImage;
              if (!$update2 || _$value141 !== $lepusTemplate._data.vo.prefixBackgroundWithoutProgressImage) {
                __SetAttribute($n600, "src", _$value141);
              }
            }
            {
              let _$value142 = $data.vo.prefixBackgroundWithoutProgressImageCapInsets;
              if (!$update2 || _$value142 !== $lepusTemplate._data.vo.prefixBackgroundWithoutProgressImageCapInsets) {
                __SetAttribute($n600, "cap-insets", _$value142);
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
  update_1477fd8_569: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "TagProgress";
    if (!$update2 || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!($data.vo.prefixText || $data.vo.prefixIconUrl) || $data.vo.prefixBackgroundImage) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n570 = $update2 ? $lepusGetElementRefByLepusID("view", 570) : null;
            let $temp2 = $update2;
            if (!$n570) {
              $update2 = false;
              $n570 = __CreateView($currentComponentId);
              let $nid570 = $lepusStoreElementRefByLepusID($n570, 570, "view");
              __SetAttribute($n570, 1004, $nid570[1]);
              __SetID($n570, "tagProgressWrapper");
              __AddEvent($n570, "bindEvent", "uiappear", "onTagProgressHandleAppear");
              __AppendElement($parent, $n570);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n570, [0, 200, 52, 101, 119, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $template_update = $update2;
              let $n571 = $update2 ? $lepusGetElementRefByLepusID("if", 571) : null;
              if (!$n571) {
                $update2 = false;
                $n571 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n571, 571, "if");
                __AppendElement($n570, $n571);
              }
              $renderTemplates[$path].update_518ee8_571($lepusTemplate, $n571, $data, $update2);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagProgress";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n569 = $lepusGetElementRefByLepusID("if", 569);
    $renderTemplates[$path].update_1477fd8_569($lepusTemplate, $n569, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n569 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n569, 569, "if");
    __AppendElement($template, $n569);
    $renderTemplates["TagProgress"].update_1477fd8_569($lepusTemplate, $n569, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagMoreIcon"] = {
  variables: ["vo", "tagMoreIconState", "suffixText"],
  varUpdateState: [],
  update_1f68c58_613: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.suffixPrependUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n614 = $update2 ? $lepusGetElementRefByLepusID("image", 614) : null;
          let $temp2 = $update2;
          if (!$n614) {
            $update2 = false;
            $n614 = __CreateImage($currentComponentId);
            let $nid614 = $lepusStoreElementRefByLepusID($n614, 614, "image");
            __SetAttribute($n614, 1004, $nid614[1]);
            __SetAttribute($n614, "mode", "scaleToFill");
            __AppendElement($parent, $n614);
          }
          if (!$update2 || $renderTemplates["TagMoreIcon"].varUpdateState[0]) {
            {
              let $value = $data.vo.suffixPrependStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.suffixPrependStyle) {
                __SetStyleObject($n614, [getCssPropertyIDObj($data.vo.suffixPrependStyle)]);
              }
            }
            {
              let _$value143 = $data.vo.suffixPrependUrl;
              if (!$update2 || _$value143 !== $lepusTemplate._data.vo.suffixPrependUrl) {
                __SetAttribute($n614, "src", _$value143);
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
  update_1f68c58_622: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.prefixPrependUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n623 = $update2 ? $lepusGetElementRefByLepusID("view", 623) : null;
          let $temp2 = $update2;
          if (!$n623) {
            $update2 = false;
            $n623 = __CreateView($currentComponentId);
            let $nid623 = $lepusStoreElementRefByLepusID($n623, 623, "view");
            __SetAttribute($n623, 1004, $nid623[1]);
            __AppendElement($parent, $n623);
          }
          if (!$update2 || $renderTemplates["TagMoreIcon"].varUpdateState[0]) {
            {
              __SetStyleObject($n623, [1, 208, 209, 210, {
                98: "url(" + $data.vo.prefixPrependUrl + ")"
              }, getCssPropertyIDObj($data.vo.prefixPrependStyle)]);
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
  update_1f68c58_624: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.prefixIconUrl) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n625 = $update2 ? $lepusGetElementRefByLepusID("view", 625) : null;
          let $temp2 = $update2;
          if (!$n625) {
            $update2 = false;
            $n625 = __CreateView($currentComponentId);
            let $nid625 = $lepusStoreElementRefByLepusID($n625, 625, "view");
            __SetAttribute($n625, 1004, $nid625[1]);
            __AppendElement($parent, $n625);
          }
          if (!$update2 || $renderTemplates["TagMoreIcon"].varUpdateState[0]) {
            {
              __SetStyleObject($n625, [1, 208, 209, 210, 140, {
                98: "url(" + $data.vo.prefixIconUrl + ")"
              }, getCssPropertyIDObj($data.vo.prefixIconStyle)]);
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
  update_377e4c8_615: function ($lepusTemplate, $parent, $data, $update2) {
    if (!$update2 || $renderTemplates["TagMoreIcon"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo6 = $lepusPushFiberForNode($parent, 615, uniqueId),
          $forLepus = _$lepusPushFiberForNo6[0],
          $lastForLepus = _$lepusPushFiberForNo6[1];
      let $object = $data.vo.suffixTextArr;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n616 = $update2 ? $lepusGetElementRefByLepusID("text", 616) : null;
          let $temp2 = $update2;
          if (!$n616) {
            $update2 = false;
            $n616 = __CreateText($currentComponentId);
            let $nid616 = $lepusStoreElementRefByLepusID($n616, 616, "text");
            __SetAttribute($n616, 1004, $nid616[1]);
            __SetAttribute($n616, "text-maxline", "1");
            __AppendElement($parent, $n616);
          }
          __SetAttribute($n616, "key", item.text);
          __SetAttribute($n616, "text", item.text);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_377e4c8_618: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.showMoreIcon && !$data.vo.showMoreIconInPrefix) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n619 = $update2 ? $lepusGetElementRefByLepusID("template", 619) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n619) {
            $templateCreated = false;
            $n619 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n619, 619, "template");
            __AppendElement($parent, $n619);
            $templateId = __GetElementUniqueID($n619);
            $childLepusTemplate = $templateConstructor($templateId, $n619);
          } else {
            $templateId = __GetElementUniqueID($n619);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.suffixMoreIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.suffixMoreIconColor, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n619, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_377e4c8_626: function ($lepusTemplate, $parent, $data, $update2) {
    if (!$update2 || $renderTemplates["TagMoreIcon"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo7 = $lepusPushFiberForNode($parent, 626, uniqueId),
          $forLepus = _$lepusPushFiberForNo7[0],
          $lastForLepus = _$lepusPushFiberForNo7[1];
      let $object = $data.vo.prefixTextArr;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n627 = $update2 ? $lepusGetElementRefByLepusID("text", 627) : null;
          let $temp2 = $update2;
          if (!$n627) {
            $update2 = false;
            $n627 = __CreateText($currentComponentId);
            let $nid627 = $lepusStoreElementRefByLepusID($n627, 627, "text");
            __SetAttribute($n627, 1004, $nid627[1]);
            __SetAttribute($n627, "text-maxline", "1");
            __AppendElement($parent, $n627);
          }
          __SetAttribute($n627, "key", item.text);
          __SetAttribute($n627, "text", item.text);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_1be51d0_611: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagMoreIcon";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.suffixText && !((_a = $data.tagMoreIconState) == null ? undefined : _a.isOverflow)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n612 = $update2 ? $lepusGetElementRefByLepusID("view", 612) : null;
          let $temp2 = $update2;
          if (!$n612) {
            $update2 = false;
            $n612 = __CreateView($currentComponentId);
            let $nid612 = $lepusStoreElementRefByLepusID($n612, 612, "view");
            __SetAttribute($n612, 1004, $nid612[1]);
            __AppendElement($parent, $n612);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[3]) {
            {
              __SetStyleObject($n612, [1, getCssPropertyIDObj($data.vo.suffixContainerStyle)]);
            }
          }
          {
            let $template_update = $update2;
            let $n613 = $update2 ? $lepusGetElementRefByLepusID("if", 613) : null;
            if (!$n613) {
              $update2 = false;
              $n613 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n613, 613, "if");
              __AppendElement($n612, $n613);
            }
            $renderTemplates[$path].update_1f68c58_613($lepusTemplate, $n613, $data, $update2);
            $update2 = $template_update;
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            let $n615 = $update2 ? $lepusGetElementRefByLepusID("for", 615) : null;
            if (!$n615) {
              $n615 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n615, 615, "for");
              __AppendElement($n612, $n615);
            }
            $renderTemplates[$path].update_377e4c8_615($lepusTemplate, $n615, $data, $update2);
          }
          {
            let _$template_update45 = $update2;
            let $n618 = $update2 ? $lepusGetElementRefByLepusID("if", 618) : null;
            if (!$n618) {
              $update2 = false;
              $n618 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n618, 618, "if");
              __AppendElement($n612, $n618);
            }
            $renderTemplates[$path].update_377e4c8_618($lepusTemplate, $n618, $data, $update2);
            $update2 = _$template_update45;
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
  update_1312d0_608: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagMoreIcon";
    if (!$update2 || $renderTemplates[$path].varUpdateState[3] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!($data.vo.prefixText || $data.vo.prefixIconUrl) || !!$data.vo.suffixText && !((_a = $data.tagMoreIconState) == null ? undefined : _a.isOverflow)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n609 = $update2 ? $lepusGetElementRefByLepusID("view", 609) : null;
            let $temp2 = $update2;
            if (!$n609) {
              $update2 = false;
              $n609 = __CreateView($currentComponentId);
              let $nid609 = $lepusStoreElementRefByLepusID($n609, 609, "view");
              __SetAttribute($n609, 1004, $nid609[1]);
              __SetID($n609, "tagMoreIconWrapper");
              __AddEvent($n609, "bindEvent", "uiappear", "onTagMoreIconHandleAppear");
              __AppendElement($parent, $n609);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n609, [66, 101, 119, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $n610 = $update2 ? $lepusGetElementRefByLepusID("view", 610) : null;
              let $temp3 = $update2;
              if (!$n610) {
                $update2 = false;
                $n610 = __CreateView($currentComponentId);
                let $nid610 = $lepusStoreElementRefByLepusID($n610, 610, "view");
                __SetAttribute($n610, 1004, $nid610[1]);
                __SetID($n610, "tagMoreIconInner");
                __AppendElement($n609, $n610);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value144 = $data.vo.innerStyle;
                  if (!$update2 || _$value144 !== $lepusTemplate._data.vo.innerStyle) {
                    __SetStyleObject($n610, [173, 1, 3, 187, 4, 62, 101, getCssPropertyIDObj($data.vo.innerStyle)]);
                  }
                }
              }
              {
                let $template_update = $update2;
                let $n611 = $update2 ? $lepusGetElementRefByLepusID("if", 611) : null;
                if (!$n611) {
                  $update2 = false;
                  $n611 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n611, 611, "if");
                  __AppendElement($n610, $n611);
                }
                $renderTemplates[$path].update_1be51d0_611($lepusTemplate, $n611, $data, $update2);
                $update2 = $template_update;
              }
              {
                let _$template_update46 = $update2;
                let $n620 = $update2 ? $lepusGetElementRefByLepusID("if", 620) : null;
                if (!$n620) {
                  $update2 = false;
                  $n620 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n620, 620, "if");
                  __AppendElement($n610, $n620);
                }
                $renderTemplates[$path].update_1312d0_620($lepusTemplate, $n620, $data, $update2);
                $update2 = _$template_update46;
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
  update_1312d0_620: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagMoreIcon";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!($data.vo.prefixText || $data.vo.prefixIconUrl)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n621 = $update2 ? $lepusGetElementRefByLepusID("view", 621) : null;
          let $temp2 = $update2;
          if (!$n621) {
            $update2 = false;
            $n621 = __CreateView($currentComponentId);
            let $nid621 = $lepusStoreElementRefByLepusID($n621, 621, "view");
            __SetAttribute($n621, 1004, $nid621[1]);
            __AppendElement($parent, $n621);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[3] || $renderTemplates[$path].varUpdateState[1]) {
            {
              __SetStyleObject($n621, [1, 3, 44, 4, 61, 207, 101, {
                12: !(!!$data.vo.suffixText && !((_a = $data.tagMoreIconState) == null ? undefined : _a.isOverflow)) ? "4px" : "4px 0 6px 4px"
              }, getCssPropertyIDObj($data.vo.prefixContainerStyle)]);
            }
          }
          {
            let $template_update = $update2;
            let $n622 = $update2 ? $lepusGetElementRefByLepusID("if", 622) : null;
            if (!$n622) {
              $update2 = false;
              $n622 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n622, 622, "if");
              __AppendElement($n621, $n622);
            }
            $renderTemplates[$path].update_1f68c58_622($lepusTemplate, $n622, $data, $update2);
            $update2 = $template_update;
          }
          {
            let _$template_update47 = $update2;
            let $n624 = $update2 ? $lepusGetElementRefByLepusID("if", 624) : null;
            if (!$n624) {
              $update2 = false;
              $n624 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n624, 624, "if");
              __AppendElement($n621, $n624);
            }
            $renderTemplates[$path].update_1f68c58_624($lepusTemplate, $n624, $data, $update2);
            $update2 = _$template_update47;
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            let $n626 = $update2 ? $lepusGetElementRefByLepusID("for", 626) : null;
            if (!$n626) {
              $n626 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n626, 626, "for");
              __AppendElement($n621, $n626);
            }
            $renderTemplates[$path].update_377e4c8_626($lepusTemplate, $n626, $data, $update2);
          }
          {
            let _$template_update48 = $update2;
            let $n629 = $update2 ? $lepusGetElementRefByLepusID("if", 629) : null;
            if (!$n629) {
              $update2 = false;
              $n629 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n629, 629, "if");
              __AppendElement($n621, $n629);
            }
            $renderTemplates[$path].update_1312d0_629($lepusTemplate, $n629, $data, $update2);
            $update2 = _$template_update48;
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
  update_1312d0_629: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.showMoreIcon && (!!(!!$data.suffixText && !((_a = $data.tagMoreIconState) == null ? undefined : _a.isOverflow)) || $data.vo.showMoreIconInPrefix)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n630 = $update2 ? $lepusGetElementRefByLepusID("template", 630) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n630) {
            $templateCreated = false;
            $n630 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n630, 630, "template");
            __AppendElement($parent, $n630);
            $templateId = __GetElementUniqueID($n630);
            $childLepusTemplate = $templateConstructor($templateId, $n630);
          } else {
            $templateId = __GetElementUniqueID($n630);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.prefixMoreIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.prefixMoreIconColor, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n630, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagMoreIcon";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n608 = $lepusGetElementRefByLepusID("if", 608);
    $renderTemplates[$path].update_1312d0_608($lepusTemplate, $n608, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n608 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n608, 608, "if");
    __AppendElement($template, $n608);
    $renderTemplates["TagMoreIcon"].update_1312d0_608($lepusTemplate, $n608, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagSplit"] = {
  variables: ["vo", "tagSplitState"],
  varUpdateState: [],
  update_377e4c8_638: function ($lepusTemplate, $parent, $data, $update2) {
    if (!$update2 || $renderTemplates["TagSplit"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo8 = $lepusPushFiberForNode($parent, 638, uniqueId),
          $forLepus = _$lepusPushFiberForNo8[0],
          $lastForLepus = _$lepusPushFiberForNo8[1];
      let $object = $data.vo.prefixTextArr;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n639 = $update2 ? $lepusGetElementRefByLepusID("text", 639) : null;
          let $temp2 = $update2;
          if (!$n639) {
            $update2 = false;
            $n639 = __CreateText($currentComponentId);
            let $nid639 = $lepusStoreElementRefByLepusID($n639, 639, "text");
            __SetAttribute($n639, 1004, $nid639[1]);
            __SetAttribute($n639, "text-maxline", "1");
            __AppendElement($parent, $n639);
          }
          __SetStyleObject($n639, [63, 41, 31, {
            47: item.isAmount ? "12px" : "10px"
          }, {
            48: item.isAmount ? "600" : "500"
          }, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
          __SetAttribute($n639, "key", item.text);
          __SetAttribute($n639, "text", item.text);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_377e4c8_641: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.showMoreIcon) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n642 = $update2 ? $lepusGetElementRefByLepusID("template", 642) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n642) {
            $templateCreated = false;
            $n642 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n642, 642, "template");
            __AppendElement($parent, $n642);
            $templateId = __GetElementUniqueID($n642);
            $childLepusTemplate = $templateConstructor($templateId, $n642);
          } else {
            $templateId = __GetElementUniqueID($n642);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.prefixMoreIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.prefixMoreIconColor, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n642, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_377e4c8_645: function ($lepusTemplate, $parent, $data, $update2) {
    if (!$update2 || $renderTemplates["TagSplit"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo9 = $lepusPushFiberForNode($parent, 645, uniqueId),
          $forLepus = _$lepusPushFiberForNo9[0],
          $lastForLepus = _$lepusPushFiberForNo9[1];
      let $object = $data.vo.suffixTextArr;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n646 = $update2 ? $lepusGetElementRefByLepusID("text", 646) : null;
          let $temp2 = $update2;
          if (!$n646) {
            $update2 = false;
            $n646 = __CreateText($currentComponentId);
            let $nid646 = $lepusStoreElementRefByLepusID($n646, 646, "text");
            __SetAttribute($n646, 1004, $nid646[1]);
            __SetAttribute($n646, "text-maxline", "1");
            __AppendElement($parent, $n646);
          }
          __SetStyleObject($n646, [214, 41, 31, {
            47: item.isAmount ? "12px" : "10px"
          }, {
            48: item.isAmount ? "600" : "500"
          }, getCssPropertyIDObj($data.vo.suffixTextStyle)]);
          __SetAttribute($n646, "key", item.text);
          __SetAttribute($n646, "text", item.text);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_1f68c58_636: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "TagSplit";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.prefixText) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n637 = $update2 ? $lepusGetElementRefByLepusID("view", 637) : null;
          let $temp2 = $update2;
          if (!$n637) {
            $update2 = false;
            $n637 = __CreateView($currentComponentId);
            let $nid637 = $lepusStoreElementRefByLepusID($n637, 637, "view");
            __SetAttribute($n637, 1004, $nid637[1]);
            __AppendElement($parent, $n637);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixContainerStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixContainerStyle) {
                __SetStyleObject($n637, [1, 62, 3, 44, 4, 61, 207, getCssPropertyIDObj($data.vo.prefixContainerStyle)]);
              }
            }
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            let $n638 = $update2 ? $lepusGetElementRefByLepusID("for", 638) : null;
            if (!$n638) {
              $n638 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n638, 638, "for");
              __AppendElement($n637, $n638);
            }
            $renderTemplates[$path].update_377e4c8_638($lepusTemplate, $n638, $data, $update2);
          }
          {
            let $template_update = $update2;
            let $n641 = $update2 ? $lepusGetElementRefByLepusID("if", 641) : null;
            if (!$n641) {
              $update2 = false;
              $n641 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n641, 641, "if");
              __AppendElement($n637, $n641);
            }
            $renderTemplates[$path].update_377e4c8_641($lepusTemplate, $n641, $data, $update2);
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
  },
  update_1f68c58_648: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.showMoreIcon && !!!$data.vo.prefixText) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n649 = $update2 ? $lepusGetElementRefByLepusID("template", 649) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n649) {
            $templateCreated = false;
            $n649 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n649, 649, "template");
            __AppendElement($parent, $n649);
            $templateId = __GetElementUniqueID($n649);
            $childLepusTemplate = $templateConstructor($templateId, $n649);
          } else {
            $templateId = __GetElementUniqueID($n649);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.suffixMoreIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.suffixMoreIconColor, $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n649, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_11c2e00_633: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagSplit";
    if (!$update2 || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!($data.vo.prefixText || $data.vo.suffixText && !((_a = $data.tagSplitState) == null ? undefined : _a.isOverflow))) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n634 = $update2 ? $lepusGetElementRefByLepusID("view", 634) : null;
            let $temp2 = $update2;
            if (!$n634) {
              $update2 = false;
              $n634 = __CreateView($currentComponentId);
              let $nid634 = $lepusStoreElementRefByLepusID($n634, 634, "view");
              __SetAttribute($n634, 1004, $nid634[1]);
              __SetID($n634, "tagSplitWrapper");
              __AddEvent($n634, "bindEvent", "uiappear", "onTagSplitHandleAppear");
              __AppendElement($parent, $n634);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n634, [66, 101, 119, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $n635 = $update2 ? $lepusGetElementRefByLepusID("view", 635) : null;
              let $temp3 = $update2;
              if (!$n635) {
                $update2 = false;
                $n635 = __CreateView($currentComponentId);
                let $nid635 = $lepusStoreElementRefByLepusID($n635, 635, "view");
                __SetAttribute($n635, 1004, $nid635[1]);
                __SetID($n635, "tagSplitInner");
                __AppendElement($n634, $n635);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value145 = $data.vo.innerStyle;
                  if (!$update2 || _$value145 !== $lepusTemplate._data.vo.innerStyle) {
                    __SetStyleObject($n635, [173, 1, 3, 44, 4, 101, getCssPropertyIDObj($data.vo.innerStyle)]);
                  }
                }
              }
              {
                let $template_update = $update2;
                let $n636 = $update2 ? $lepusGetElementRefByLepusID("if", 636) : null;
                if (!$n636) {
                  $update2 = false;
                  $n636 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n636, 636, "if");
                  __AppendElement($n635, $n636);
                }
                $renderTemplates[$path].update_1f68c58_636($lepusTemplate, $n636, $data, $update2);
                $update2 = $template_update;
              }
              {
                let _$template_update49 = $update2;
                let $n643 = $update2 ? $lepusGetElementRefByLepusID("if", 643) : null;
                if (!$n643) {
                  $update2 = false;
                  $n643 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n643, 643, "if");
                  __AppendElement($n635, $n643);
                }
                $renderTemplates[$path].update_11c2e00_643($lepusTemplate, $n643, $data, $update2);
                $update2 = _$template_update49;
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
  update_11c2e00_643: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagSplit";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.suffixText && !((_a = $data.tagSplitState) == null ? undefined : _a.isOverflow)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n644 = $update2 ? $lepusGetElementRefByLepusID("view", 644) : null;
          let $temp2 = $update2;
          if (!$n644) {
            $update2 = false;
            $n644 = __CreateView($currentComponentId);
            let $nid644 = $lepusStoreElementRefByLepusID($n644, 644, "view");
            __SetAttribute($n644, 1004, $nid644[1]);
            __AppendElement($parent, $n644);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.suffixContainerStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.suffixContainerStyle) {
                __SetStyleObject($n644, [211, 1, 62, 3, 44, 4, 61, 212, 213, getCssPropertyIDObj($data.vo.suffixContainerStyle)]);
              }
            }
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            let $n645 = $update2 ? $lepusGetElementRefByLepusID("for", 645) : null;
            if (!$n645) {
              $n645 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n645, 645, "for");
              __AppendElement($n644, $n645);
            }
            $renderTemplates[$path].update_377e4c8_645($lepusTemplate, $n645, $data, $update2);
          }
          {
            let $template_update = $update2;
            let $n648 = $update2 ? $lepusGetElementRefByLepusID("if", 648) : null;
            if (!$n648) {
              $update2 = false;
              $n648 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n648, 648, "if");
              __AppendElement($n644, $n648);
            }
            $renderTemplates[$path].update_1f68c58_648($lepusTemplate, $n648, $data, $update2);
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
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagSplit";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n633 = $lepusGetElementRefByLepusID("if", 633);
    $renderTemplates[$path].update_11c2e00_633($lepusTemplate, $n633, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n633 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n633, 633, "if");
    __AppendElement($template, $n633);
    $renderTemplates["TagSplit"].update_11c2e00_633($lepusTemplate, $n633, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagAnimateProgress"] = {
  variables: ["vo", "tagAnimateProgressState"],
  varUpdateState: [],
  update_377e4c8_657: function ($lepusTemplate, $parent, $data, $update2) {
    if (!$update2 || $renderTemplates["TagAnimateProgress"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo10 = $lepusPushFiberForNode($parent, 657, uniqueId),
          $forLepus = _$lepusPushFiberForNo10[0],
          $lastForLepus = _$lepusPushFiberForNo10[1];
      let $object = $data.vo.prefixTextArr;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n658 = $update2 ? $lepusGetElementRefByLepusID("text", 658) : null;
          let $temp2 = $update2;
          if (!$n658) {
            $update2 = false;
            $n658 = __CreateText($currentComponentId);
            let $nid658 = $lepusStoreElementRefByLepusID($n658, 658, "text");
            __SetAttribute($n658, 1004, $nid658[1]);
            __SetAttribute($n658, "text-maxline", "1");
            __AppendElement($parent, $n658);
          }
          __SetStyleObject($n658, [216, 41, 31, {
            47: (item.isAmount ? 12 : 10) + "px"
          }, {
            48: (item.isAmount ? 600 : 500) + ""
          }, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
          __SetAttribute($n658, "key", item.text);
          __SetAttribute($n658, "text", item.text);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_377e4c8_660: function ($lepusTemplate, $parent, $data, $update2) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.showMoreIcon) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n661 = $update2 ? $lepusGetElementRefByLepusID("template", 661) : null;
          let $templateCreated = true;
          let $childLepusTemplate = null;
          let $templateId = null;
          if (!$n661) {
            $templateCreated = false;
            $n661 = __CreateWrapperElement($currentComponentId);
            $lepusStoreElementRefByLepusID($n661, 661, "template");
            __AppendElement($parent, $n661);
            $templateId = __GetElementUniqueID($n661);
            $childLepusTemplate = $templateConstructor($templateId, $n661);
          } else {
            $templateId = __GetElementUniqueID($n661);
            $childLepusTemplate = $templateInfo[$templateId];
          }
          $updatePropsSet = [];
          $childLepusTemplate.setData("type", $data.vo.prefixMoreIconType, $update2);
          $childLepusTemplate.setData("color", $data.vo.prefixMoreIconColor, $update2);
          $childLepusTemplate.setData("style", "margin-left: 2px;", $update2);
          if ($templateCreated) {
            let $update_keys = $updatePropsSet;
            if ($update_keys.length > 0) {
              $renderTemplates["Icon"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
            }
          } else {
            $renderTemplates["Icon"].entry($n661, $childLepusTemplate, $childLepusTemplate.data, false);
          }
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_377e4c8_667: function ($lepusTemplate, $parent, $data, $update2) {
    if (!$update2 || $renderTemplates["TagAnimateProgress"].varUpdateState[0]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo11 = $lepusPushFiberForNode($parent, 667, uniqueId),
          $forLepus = _$lepusPushFiberForNo11[0],
          $lastForLepus = _$lepusPushFiberForNo11[1];
      let $object = $data.vo.progressTextArr;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        {
          let $n668 = $update2 ? $lepusGetElementRefByLepusID("text", 668) : null;
          let $temp2 = $update2;
          if (!$n668) {
            $update2 = false;
            $n668 = __CreateText($currentComponentId);
            let $nid668 = $lepusStoreElementRefByLepusID($n668, 668, "text");
            __SetAttribute($n668, 1004, $nid668[1]);
            __SetAttribute($n668, "text-maxline", "1");
            __AppendElement($parent, $n668);
          }
          __SetStyleObject($n668, [121, 41, 31, {
            47: (item.isAmount ? 12 : 10) + "px"
          }, {
            48: (item.isAmount ? 600 : 500) + ""
          }, getCssPropertyIDObj($data.vo.progressTextStyle)]);
          __SetAttribute($n668, "key", item.text);
          __SetAttribute($n668, "text", item.text);
          $update2 = $temp2;
        }
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_2e66e80_652: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "TagAnimateProgress";
    if (!$update2 || $renderTemplates[$path].varUpdateState[2] || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!$data.vo.prefixText) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n653 = $update2 ? $lepusGetElementRefByLepusID("view", 653) : null;
            let $temp2 = $update2;
            if (!$n653) {
              $update2 = false;
              $n653 = __CreateView($currentComponentId);
              let $nid653 = $lepusStoreElementRefByLepusID($n653, 653, "view");
              __SetAttribute($n653, 1004, $nid653[1]);
              __SetID($n653, "wrapper");
              __AppendElement($parent, $n653);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n653, [66, 101, 119, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $n654 = $update2 ? $lepusGetElementRefByLepusID("view", 654) : null;
              let $temp3 = $update2;
              if (!$n654) {
                $update2 = false;
                $n654 = __CreateView($currentComponentId);
                let $nid654 = $lepusStoreElementRefByLepusID($n654, 654, "view");
                __SetAttribute($n654, 1004, $nid654[1]);
                __SetID($n654, "inner");
                __AppendElement($n653, $n654);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value146 = $data.vo.innerStyle;
                  if (!$update2 || _$value146 !== $lepusTemplate._data.vo.innerStyle) {
                    __SetStyleObject($n654, [173, 1, 3, 44, 4, 101, getCssPropertyIDObj($data.vo.innerStyle)]);
                  }
                }
              }
              {
                let $n655 = $update2 ? $lepusGetElementRefByLepusID("view", 655) : null;
                let $temp4 = $update2;
                if (!$n655) {
                  $update2 = false;
                  $n655 = __CreateView($currentComponentId);
                  let $nid655 = $lepusStoreElementRefByLepusID($n655, 655, "view");
                  __SetAttribute($n655, 1004, $nid655[1]);
                  __AppendElement($n654, $n655);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value147 = $data.vo.prefixContainerStyle;
                    if (!$update2 || _$value147 !== $lepusTemplate._data.vo.prefixContainerStyle) {
                      __SetStyleObject($n655, [66, 3, 44, 4, 215, 74, getCssPropertyIDObj($data.vo.prefixContainerStyle)]);
                    }
                  }
                }
                {
                  let $n656 = $update2 ? $lepusGetElementRefByLepusID("image", 656) : null;
                  let $temp5 = $update2;
                  if (!$n656) {
                    $update2 = false;
                    $n656 = __CreateImage($currentComponentId);
                    let $nid656 = $lepusStoreElementRefByLepusID($n656, 656, "image");
                    __SetAttribute($n656, 1004, $nid656[1]);
                    __SetAttribute($n656, "cap-insets-scale", "3");
                    __SetAttribute($n656, "disable-default-resize", "true");
                    __SetAttribute($n656, "skip-redirection", true);
                    __AppendElement($n655, $n656);
                  }
                  if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                    {
                      let _$value148 = $data.vo.prefixBackgroundImageStyle;
                      if (!$update2 || _$value148 !== $lepusTemplate._data.vo.prefixBackgroundImageStyle) {
                        __SetStyleObject($n656, [55, 1, 0, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.prefixBackgroundImageStyle)]);
                      }
                    }
                    {
                      let _$value149 = $data.vo.prefixBackgroundImage;
                      if (!$update2 || _$value149 !== $lepusTemplate._data.vo.prefixBackgroundImage) {
                        __SetAttribute($n656, "src", _$value149);
                      }
                    }
                    {
                      let _$value150 = $data.vo.prefixBackgroundImageCapInsets;
                      if (!$update2 || _$value150 !== $lepusTemplate._data.vo.prefixBackgroundImageCapInsets) {
                        __SetAttribute($n656, "cap-insets", _$value150);
                      }
                    }
                  }
                  $update2 = $temp5;
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let $n657 = $update2 ? $lepusGetElementRefByLepusID("for", 657) : null;
                  if (!$n657) {
                    $n657 = __CreateFor($currentComponentId);
                    $lepusStoreElementRefByLepusID($n657, 657, "for");
                    __AppendElement($n655, $n657);
                  }
                  $renderTemplates[$path].update_377e4c8_657($lepusTemplate, $n657, $data, $update2);
                }
                {
                  let $template_update = $update2;
                  let $n660 = $update2 ? $lepusGetElementRefByLepusID("if", 660) : null;
                  if (!$n660) {
                    $update2 = false;
                    $n660 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n660, 660, "if");
                    __AppendElement($n655, $n660);
                  }
                  $renderTemplates[$path].update_377e4c8_660($lepusTemplate, $n660, $data, $update2);
                  $update2 = $template_update;
                }
                $update2 = $temp4;
              }
              {
                let $n662 = $update2 ? $lepusGetElementRefByLepusID("view", 662) : null;
                let _$temp90 = $update2;
                if (!$n662) {
                  $update2 = false;
                  $n662 = __CreateView($currentComponentId);
                  let $nid662 = $lepusStoreElementRefByLepusID($n662, 662, "view");
                  __SetAttribute($n662, 1004, $nid662[1]);
                  __AppendElement($n654, $n662);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value151 = $data.vo.progressWrapperStyle;
                    if (!$update2 || _$value151 !== $lepusTemplate._data.vo.progressWrapperStyle) {
                      __SetStyleObject($n662, [217, 66, 3, 4, 218, 101, getCssPropertyIDObj($data.vo.progressWrapperStyle)]);
                    }
                  }
                }
                {
                  let $n663 = $update2 ? $lepusGetElementRefByLepusID("view", 663) : null;
                  let _$temp91 = $update2;
                  if (!$n663) {
                    $update2 = false;
                    $n663 = __CreateView($currentComponentId);
                    let $nid663 = $lepusStoreElementRefByLepusID($n663, 663, "view");
                    __SetAttribute($n663, 1004, $nid663[1]);
                    __AppendElement($n662, $n663);
                  }
                  if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                    {
                      let _$value152 = $data.vo.progressInnerStyle;
                      if (!$update2 || _$value152 !== $lepusTemplate._data.vo.progressInnerStyle) {
                        __SetStyleObject($n663, [14, 66, 219, 220, 101, getCssPropertyIDObj($data.vo.progressInnerStyle)]);
                      }
                    }
                  }
                  {
                    let $n664 = $update2 ? $lepusGetElementRefByLepusID("view", 664) : null;
                    let $temp6 = $update2;
                    if (!$n664) {
                      $update2 = false;
                      $n664 = __CreateView($currentComponentId);
                      let $nid664 = $lepusStoreElementRefByLepusID($n664, 664, "view");
                      __SetAttribute($n664, 1004, $nid664[1]);
                      __SetAttribute($n664, "enable-new-animator", "true");
                      __SetAttribute($n664, "async-display", "false");
                      __AppendElement($n663, $n664);
                    }
                    if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
                      {
                        let _$value153 = "width:" + ((((_a = $data.tagAnimateProgressState) == null ? undefined : _a.renderReady) ? $data.vo.progressPercent + "%" : "20px") + ";") + "height:18px;min-width:20px;transition:width 0.25s;display:flex;align-items:center;position:relative;overflow:visible;padding-right:8px;";
                        if (!$update2 || _$value153 !== "width:" + ((((_b = $lepusTemplate._data.tagAnimateProgressState) == null ? undefined : _b.renderReady) ? $lepusTemplate._data.vo.progressPercent + "%" : "20px") + ";") + "height:18px;min-width:20px;transition:width 0.25s;display:flex;align-items:center;position:relative;overflow:visible;padding-right:8px;") {
                          __SetStyleObject($n664, [66, 221, 222, 3, 4, 74, 101, 93, {
                            27: ((_c = $data.tagAnimateProgressState) == null ? undefined : _c.renderReady) ? $data.vo.progressPercent + "%" : "20px"
                          }]);
                        }
                      }
                    }
                    {
                      let $n665 = $update2 ? $lepusGetElementRefByLepusID("image", 665) : null;
                      let $temp7 = $update2;
                      if (!$n665) {
                        $update2 = false;
                        $n665 = __CreateImage($currentComponentId);
                        let $nid665 = $lepusStoreElementRefByLepusID($n665, 665, "image");
                        __SetAttribute($n665, 1004, $nid665[1]);
                        __SetAttribute($n665, "cap-insets-scale", "3");
                        __SetAttribute($n665, "disable-default-resize", "true");
                        __SetAttribute($n665, "skip-redirection", true);
                        __AppendElement($n664, $n665);
                      }
                      if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                        {
                          let _$value154 = $data.vo.progressBackgroundImageStyle;
                          if (!$update2 || _$value154 !== $lepusTemplate._data.vo.progressBackgroundImageStyle) {
                            __SetStyleObject($n665, [55, 1, 0, 75, 76, 153, 77, getCssPropertyIDObj($data.vo.progressBackgroundImageStyle)]);
                          }
                        }
                        {
                          let _$value155 = $data.vo.progressBackgroundImage;
                          if (!$update2 || _$value155 !== $lepusTemplate._data.vo.progressBackgroundImage) {
                            __SetAttribute($n665, "src", _$value155);
                          }
                        }
                        {
                          let _$value156 = $data.vo.progressBackgroundImageCapInsets;
                          if (!$update2 || _$value156 !== $lepusTemplate._data.vo.progressBackgroundImageCapInsets) {
                            __SetAttribute($n665, "cap-insets", _$value156);
                          }
                        }
                      }
                      $update2 = $temp7;
                    }
                    {
                      let $n666 = $update2 ? $lepusGetElementRefByLepusID("view", 666) : null;
                      let _$temp92 = $update2;
                      if (!$n666) {
                        $update2 = false;
                        $n666 = __CreateView($currentComponentId);
                        let $nid666 = $lepusStoreElementRefByLepusID($n666, 666, "view");
                        __SetAttribute($n666, 1004, $nid666[1]);
                        __SetAttribute($n666, "enable-new-animator", "true");
                        __AppendElement($n664, $n666);
                      }
                      if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
                        {
                          let _$value157 = ((_d = $data.tagAnimateProgressState) == null ? undefined : _d.renderReady) ? __spreadProps(__spreadValues({}, $data.vo.progressMainStyle), {
                            opacity: 1
                          }) : $data.vo.progressMainStyle;
                          if (!$update2 || _$value157 !== (((_e = $lepusTemplate._data.tagAnimateProgressState) == null ? undefined : _e.renderReady) ? __spreadProps(__spreadValues({}, $lepusTemplate._data.vo.progressMainStyle), {
                            opacity: 1
                          }) : $lepusTemplate._data.vo.progressMainStyle)) {
                            __SetStyleObject($n666, [223, 224, 225, 3, 44, 4, getCssPropertyIDObj(((_f = $data.tagAnimateProgressState) == null ? undefined : _f.renderReady) ? __spreadProps(__spreadValues({}, $data.vo.progressMainStyle), {
                              opacity: 1
                            }) : $data.vo.progressMainStyle)]);
                          }
                        }
                      }
                      if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                        let $n667 = $update2 ? $lepusGetElementRefByLepusID("for", 667) : null;
                        if (!$n667) {
                          $n667 = __CreateFor($currentComponentId);
                          $lepusStoreElementRefByLepusID($n667, 667, "for");
                          __AppendElement($n666, $n667);
                        }
                        $renderTemplates[$path].update_377e4c8_667($lepusTemplate, $n667, $data, $update2);
                      }
                      $update2 = _$temp92;
                    }
                    {
                      let $n670 = $update2 ? $lepusGetElementRefByLepusID("image", 670) : null;
                      let _$temp93 = $update2;
                      if (!$n670) {
                        $update2 = false;
                        $n670 = __CreateImage($currentComponentId);
                        let $nid670 = $lepusStoreElementRefByLepusID($n670, 670, "image");
                        __SetAttribute($n670, 1004, $nid670[1]);
                        __SetAttribute($n670, "skip-redirection", true);
                        __AppendElement($n664, $n670);
                      }
                      if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                        {
                          let _$value158 = $data.vo.progressAnimateImageStyle;
                          if (!$update2 || _$value158 !== $lepusTemplate._data.vo.progressAnimateImageStyle) {
                            __SetStyleObject($n670, [226, 97, 227, 55, 153, 228, getCssPropertyIDObj($data.vo.progressAnimateImageStyle)]);
                          }
                        }
                        {
                          let _$value159 = $data.vo.progressAnimateImage;
                          if (!$update2 || _$value159 !== $lepusTemplate._data.vo.progressAnimateImage) {
                            __SetAttribute($n670, "src", _$value159);
                          }
                        }
                      }
                      $update2 = _$temp93;
                    }
                    $update2 = $temp6;
                  }
                  $update2 = _$temp91;
                }
                $update2 = _$temp90;
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagAnimateProgress";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n652 = $lepusGetElementRefByLepusID("if", 652);
    $renderTemplates[$path].update_2e66e80_652($lepusTemplate, $n652, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n652 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n652, 652, "if");
    __AppendElement($template, $n652);
    $renderTemplates["TagAnimateProgress"].update_2e66e80_652($lepusTemplate, $n652, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagSecKillCountDown"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_377e4c8_677: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagSecKillCountDown";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.vo.showCountDown) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n678 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 678) : null;
            let $temp2 = $update2;
            if (!$n678) {
              $update2 = false;
              $n678 = __CreateElement("countdown-view", $currentComponentId);
              let $nid678 = $lepusStoreElementRefByLepusID($n678, 678, "countdown-view");
              __SetAttribute($n678, 1004, $nid678[1]);
              __AddEvent($n678, "bindEvent", "countdownend", "onTagSecKillCountDownEnd");
              __AppendElement($parent, $n678);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.countDownStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.countDownStyle) {
                  __SetStyleObject($n678, [89, 21, 139, 118, getCssPropertyIDObj($data.vo.countDownStyle)]);
                }
              }
              {
                let _$value160 = "" + $data.vo.endTime;
                if (!$update2 || _$value160 !== "" + $lepusTemplate._data.vo.endTime) {
                  __SetAttribute($n678, "end-time", _$value160);
                }
              }
              {
                let _$value161 = ((_a = lynx.__globalProps) == null ? undefined : _a.os) === "android" ? "ms" : "millseconds";
                if (!$update2 || _$value161 !== undefined) {
                  __SetAttribute($n678, "unit", _$value161);
                }
              }
            }
            {
              let $n679 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 679) : null;
              let $temp3 = $update2;
              if (!$n679) {
                $update2 = false;
                $n679 = __CreateElement("countdown-item", $currentComponentId);
                let $nid679 = $lepusStoreElementRefByLepusID($n679, 679, "countdown-item");
                __SetAttribute($n679, 1004, $nid679[1]);
                __SetAttribute($n679, "text-maxline", "1");
                __SetAttribute($n679, "countdown-display", "HH:mm:ss");
                __AppendElement($n678, $n679);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value162 = $data.vo.countDownStyle;
                  if (!$update2 || _$value162 !== $lepusTemplate._data.vo.countDownStyle) {
                    __SetStyleObject($n679, [89, 21, 139, 118, getCssPropertyIDObj($data.vo.countDownStyle)]);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagSecKillCountDown";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n672 = $lepusGetElementRefByLepusID("view", 672);
      {
        let $value = $data.vo.containerStyle;
        if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
          __SetStyleObject($n672, [3, 4, 91, getCssPropertyIDObj($data.vo.containerStyle)]);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n673 = $lepusGetElementRefByLepusID("view", 673);
      {
        __SetStyleObject($n673, [52, 3, 31, 4, {
          59: " url(" + $data.vo.backgroundUrl + ")100%/100% no-repeat"
        }, getCssPropertyIDObj($data.vo.innerContainerStyle)]);
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n674 = $lepusGetElementRefByLepusID("text", 674);
      {
        __SetStyleObject($n674, [229, 230, 118, 139, {
          39: $data.vo.secKillTagWidth * $data.vo.imageArea.gap1 + "px"
        }, getCssPropertyIDObj($data.vo.textStyle)]);
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let _$value163 = $data.vo.prefixText;
      if (_$value163 !== $lepusTemplate._data.vo.prefixText) {
        let _$n4 = $lepusGetElementRefByLepusID("text", 674);
        __SetAttribute(_$n4, "text", _$value163);
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n676 = $lepusGetElementRefByLepusID("view", 676);
      {
        __SetStyleObject($n676, [118, 231, 21, {
          27: $data.vo.secKillTagWidth * $data.vo.imageArea.countdown + "px"
        }, getCssPropertyIDObj($data.vo.countDownContainerStyle)]);
      }
    }
    let $n677 = $lepusGetElementRefByLepusID("if", 677);
    $renderTemplates[$path].update_377e4c8_677($lepusTemplate, $n677, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n672 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n672, 672, "view");
    __SetAttribute($n672, 1004, 672);
    __SetStyleObject($n672, [3, 4, 91, getCssPropertyIDObj($data.vo.containerStyle)]);
    __AppendElement($template, $n672);
    let $n673 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n673, 673, "view");
    __SetAttribute($n673, 1004, 673);
    __SetStyleObject($n673, [52, 3, 31, 4, {
      59: " url(" + $data.vo.backgroundUrl + ")100%/100% no-repeat"
    }, getCssPropertyIDObj($data.vo.innerContainerStyle)]);
    __AppendElement($n672, $n673);
    let $n674 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n674, 674, "text");
    __SetAttribute($n674, 1004, 674);
    __SetStyleObject($n674, [229, 230, 118, 139, {
      39: $data.vo.secKillTagWidth * $data.vo.imageArea.gap1 + "px"
    }, getCssPropertyIDObj($data.vo.textStyle)]);
    __SetAttribute($n674, "accessibility-element", "false");
    __SetAttribute($n674, "text-maxline", "1");
    __AppendElement($n673, $n674);
    __SetAttribute($n674, "text", $data.vo.prefixText);
    let $n676 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n676, 676, "view");
    __SetAttribute($n676, 1004, 676);
    __SetStyleObject($n676, [118, 231, 21, {
      27: $data.vo.secKillTagWidth * $data.vo.imageArea.countdown + "px"
    }, getCssPropertyIDObj($data.vo.countDownContainerStyle)]);
    __AppendElement($n673, $n676);
    let $n677 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n677, 677, "if");
    __AppendElement($n676, $n677);
    $renderTemplates["TagSecKillCountDown"].update_377e4c8_677($lepusTemplate, $n677, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagSimpleText"] = {
  variables: ["vo"],
  varUpdateState: [],
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagSimpleText";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n680 = $lepusGetElementRefByLepusID("view", 680);
      {
        let $value = $data.vo.containerStyle;
        if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
          __SetStyleObject($n680, [232, 4, 8, 52, getCssPropertyIDObj($data.vo.containerStyle)]);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n681 = $lepusGetElementRefByLepusID("image", 681);
      {
        __SetStyleObject($n681, [55, 76, 75, 0, 1, {
          26: ($data.vo.height || 0) + "px,"
        }, getCssPropertyIDObj($data.vo.imageStyle)]);
      }
      {
        let _$value164 = $data.vo.background_url;
        if (!$update2 || _$value164 !== $lepusTemplate._data.vo.background_url) {
          __SetAttribute($n681, "src", _$value164);
        }
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let $n682 = $lepusGetElementRefByLepusID("text", 682);
      {
        __SetStyleObject($n682, [230, 118, 139, {
          45: ($data.vo.height || 0) + "px"
        }, getCssPropertyIDObj($data.vo.textStyle)]);
      }
    }
    if ($renderTemplates[$path].varUpdateState[0]) {
      let _$value165 = $data.vo.tagText;
      if (_$value165 !== $lepusTemplate._data.vo.tagText) {
        let _$n5 = $lepusGetElementRefByLepusID("text", 682);
        __SetAttribute(_$n5, "text", _$value165);
      }
    }
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n680 = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($n680, 680, "view");
    __SetAttribute($n680, 1004, 680);
    __SetStyleObject($n680, [232, 4, 8, 52, getCssPropertyIDObj($data.vo.containerStyle)]);
    __AppendElement($template, $n680);
    let $n681 = __CreateImage($currentComponentId);
    $lepusStoreElementRefByLepusID($n681, 681, "image");
    __SetAttribute($n681, 1004, 681);
    __SetStyleObject($n681, [55, 76, 75, 0, 1, {
      26: ($data.vo.height || 0) + "px,"
    }, getCssPropertyIDObj($data.vo.imageStyle)]);
    __SetAttribute($n681, "cap-insets-scale", "3");
    __SetAttribute($n681, "cap-insets", "0 7px 0 14px");
    __SetAttribute($n681, "disable-default-resize", "true");
    __SetAttribute($n681, "src", $data.vo.background_url);
    __AppendElement($n680, $n681);
    let $n682 = __CreateText($currentComponentId);
    $lepusStoreElementRefByLepusID($n682, 682, "text");
    __SetAttribute($n682, 1004, 682);
    __SetStyleObject($n682, [230, 118, 139, {
      45: ($data.vo.height || 0) + "px"
    }, getCssPropertyIDObj($data.vo.textStyle)]);
    __AppendElement($n680, $n682);
    __SetAttribute($n682, "text", $data.vo.tagText);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["TagDynamic"] = {
  variables: ["tagDynamicState", "vo"],
  varUpdateState: [],
  update_2f8a690_684: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "TagDynamic";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!((_a = $data.tagDynamicState) == null ? undefined : _a.imageWidth)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n685 = $update2 ? $lepusGetElementRefByLepusID("image", 685) : null;
            let $temp2 = $update2;
            if (!$n685) {
              $update2 = false;
              $n685 = __CreateImage($currentComponentId);
              let $nid685 = $lepusStoreElementRefByLepusID($n685, 685, "image");
              __SetAttribute($n685, 1004, $nid685[1]);
              __SetStyleObject($n685, [233, 234, 223, 32]);
              __AddEvent($n685, "bindEvent", "load", "onTagDynamicHandleImageLoad");
              __AppendElement($parent, $n685);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[1]) {
              {
                let $value = $data.vo.imageUrl;
                if (!$update2 || $value !== $lepusTemplate._data.vo.imageUrl) {
                  __SetAttribute($n685, "src", $value);
                }
              }
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp94 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n686 = $update2 ? $lepusGetElementRefByLepusID("image", 686) : null;
            let _$temp95 = $update2;
            if (!$n686) {
              $update2 = false;
              $n686 = __CreateImage($currentComponentId);
              let $nid686 = $lepusStoreElementRefByLepusID($n686, 686, "image");
              __SetAttribute($n686, 1004, $nid686[1]);
              __SetAttribute($n686, "skip-redirection", "cdnImageSkipDirection");
              __AppendElement($parent, $n686);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[1]) {
              {
                __SetStyleObject($n686, [235, 32, {
                  27: $data.vo.imageWidth + "px"
                }, getCssPropertyIDObj($data.vo.imageStyle)]);
              }
              {
                let _$value166 = $data.vo.imageUrl;
                if (!$update2 || _$value166 !== $lepusTemplate._data.vo.imageUrl) {
                  __SetAttribute($n686, "src", _$value166);
                }
              }
            }
            $update2 = _$temp95;
          }
          $update2 = _$temp94;
        }
      }
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "TagDynamic";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n684 = $lepusGetElementRefByLepusID("if", 684);
    $renderTemplates[$path].update_2f8a690_684($lepusTemplate, $n684, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n684 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n684, 684, "if");
    __AppendElement($template, $n684);
    $renderTemplates["TagDynamic"].update_2f8a690_684($lepusTemplate, $n684, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["ShelfButton"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_1f68c58_687: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p;
    let $path = "ShelfButton";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if ($data.vo.showDualBtn) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n688 = $update2 ? $lepusGetElementRefByLepusID("view", 688) : null;
            let $temp2 = $update2;
            if (!$n688) {
              $update2 = false;
              $n688 = __CreateView($currentComponentId);
              let $nid688 = $lepusStoreElementRefByLepusID($n688, 688, "view");
              __SetAttribute($n688, 1004, $nid688[1]);
              __SetStyleObject($n688, [3, 4]);
              __AppendElement($parent, $n688);
            }
            {
              let $n689 = $update2 ? $lepusGetElementRefByLepusID("view", 689) : null;
              let $temp3 = $update2;
              if (!$n689) {
                $update2 = false;
                $n689 = __CreateView($currentComponentId);
                let $nid689 = $lepusStoreElementRefByLepusID($n689, 689, "view");
                __SetAttribute($n689, 1004, $nid689[1]);
                __AddEvent($n689, "catchEvent", "tap", "onMarketingV2LeftBtnClick");
                __AppendElement($n688, $n689);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n689, [3, getCssPropertyIDObj((_a = $data.vo.dualBtnInfo) == null ? undefined : _a.leftButtonStyle)]);
                }
              }
              {
                let $n690 = $update2 ? $lepusGetElementRefByLepusID("text", 690) : null;
                let $temp4 = $update2;
                if (!$n690) {
                  $update2 = false;
                  $n690 = __CreateText($currentComponentId);
                  let $nid690 = $lepusStoreElementRefByLepusID($n690, 690, "text");
                  __SetAttribute($n690, 1004, $nid690[1]);
                  __SetStyleObject($n690, [118, 117, 134, 21, 121]);
                  __AppendElement($n689, $n690);
                }
                {
                  if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                    let $value = (_b = $data.vo.dualBtnInfo) == null ? undefined : _b.leftBtnText;
                    if (!$update2 || $value !== ((_c = $lepusTemplate._data.vo.dualBtnInfo) == null ? undefined : _c.leftBtnText)) {
                      __SetAttribute($n690, "text", $value);
                    }
                  }
                }
                $update2 = $temp4;
              }
              $update2 = $temp3;
            }
            {
              let $n692 = $update2 ? $lepusGetElementRefByLepusID("view", 692) : null;
              let _$temp96 = $update2;
              if (!$n692) {
                $update2 = false;
                $n692 = __CreateView($currentComponentId);
                let $nid692 = $lepusStoreElementRefByLepusID($n692, 692, "view");
                __SetAttribute($n692, 1004, $nid692[1]);
                __AddEvent($n692, "catchEvent", "tap", "onMarketingV2RightBtnClick");
                __AppendElement($n688, $n692);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value167 = (_d = $data.vo.dualBtnInfo) == null ? undefined : _d.rightButtonStyle;
                  if (!$update2 || _$value167 !== ((_e = $lepusTemplate._data.vo.dualBtnInfo) == null ? undefined : _e.rightButtonStyle)) {
                    __SetStyleObject($n692, [3, 4, 97, 236, 237, 238, getCssPropertyIDObj((_f = $data.vo.dualBtnInfo) == null ? undefined : _f.rightButtonStyle)]);
                  }
                }
              }
              {
                let $n693 = $update2 ? $lepusGetElementRefByLepusID("text", 693) : null;
                let _$temp97 = $update2;
                if (!$n693) {
                  $update2 = false;
                  $n693 = __CreateText($currentComponentId);
                  let $nid693 = $lepusStoreElementRefByLepusID($n693, 693, "text");
                  __SetAttribute($n693, 1004, $nid693[1]);
                  __SetStyleObject($n693, [118, 117, 134, 21, 138]);
                  __AppendElement($n692, $n693);
                }
                {
                  if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                    let _$value168 = (_g = $data.vo.dualBtnInfo) == null ? undefined : _g.rightBtnText;
                    if (!$update2 || _$value168 !== ((_h = $lepusTemplate._data.vo.dualBtnInfo) == null ? undefined : _h.rightBtnText)) {
                      __SetAttribute($n693, "text", _$value168);
                    }
                  }
                }
                $update2 = _$temp97;
              }
              $update2 = _$temp96;
            }
            $update2 = $temp2;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp98 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let $n695 = $update2 ? $lepusGetElementRefByLepusID("view", 695) : null;
            let _$temp99 = $update2;
            if (!$n695) {
              $update2 = false;
              $n695 = __CreateView($currentComponentId);
              let $nid695 = $lepusStoreElementRefByLepusID($n695, 695, "view");
              __SetAttribute($n695, 1004, $nid695[1]);
              __AppendElement($parent, $n695);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value169 = "opacity:" + ((((_i = $data.vo.btnInfo) == null ? undefined : _i.disabled) ? 0.5 : 1) + ";");
                if (!$update2 || _$value169 !== "opacity:" + ((((_j = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _j.disabled) ? 0.5 : 1) + ";")) {
                  __SetStyleObject($n695, [{
                    23: (((_k = $data.vo.btnInfo) == null ? undefined : _k.disabled) ? 0.5 : 1) + ""
                  }]);
                }
              }
            }
            {
              let $n696 = $update2 ? $lepusGetElementRefByLepusID("view", 696) : null;
              let _$temp100 = $update2;
              if (!$n696) {
                $update2 = false;
                $n696 = __CreateView($currentComponentId);
                let $nid696 = $lepusStoreElementRefByLepusID($n696, 696, "view");
                __SetAttribute($n696, 1004, $nid696[1]);
                __AddEvent($n696, "catchEvent", "tap", "onMarketingV2BtnClick");
                __AppendElement($n695, $n696);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  __SetStyleObject($n696, [239, 3, 4, 8, 98, 31, 101, 97, {
                    27: ((_l = $data.vo.btnInfo) == null ? undefined : _l.size) === "sm" ? "52px" : "64px"
                  }, getCssPropertyIDObj((_m = $data.vo.btnInfo) == null ? undefined : _m.containerStyle)]);
                }
              }
              {
                let $template_update = $update2;
                let $n697 = $update2 ? $lepusGetElementRefByLepusID("if", 697) : null;
                if (!$n697) {
                  $update2 = false;
                  $n697 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n697, 697, "if");
                  __AppendElement($n696, $n697);
                }
                $renderTemplates[$path].update_1f68c58_697($lepusTemplate, $n697, $data, $update2);
                $update2 = $template_update;
              }
              {
                let $n699 = $update2 ? $lepusGetElementRefByLepusID("view", 699) : null;
                let _$temp101 = $update2;
                if (!$n699) {
                  $update2 = false;
                  $n699 = __CreateView($currentComponentId);
                  let $nid699 = $lepusStoreElementRefByLepusID($n699, 699, "view");
                  __SetAttribute($n699, 1004, $nid699[1]);
                  __AppendElement($n696, $n699);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value170 = (_n = $data.vo.btnInfo) == null ? undefined : _n.textContainerStyle;
                    if (!$update2 || _$value170 !== ((_o = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _o.textContainerStyle)) {
                      __SetStyleObject($n699, [3, 4, 8, 100, getCssPropertyIDObj((_p = $data.vo.btnInfo) == null ? undefined : _p.textContainerStyle)]);
                    }
                  }
                }
                {
                  let _$template_update50 = $update2;
                  let $n700 = $update2 ? $lepusGetElementRefByLepusID("if", 700) : null;
                  if (!$n700) {
                    $update2 = false;
                    $n700 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n700, 700, "if");
                    __AppendElement($n699, $n700);
                  }
                  $renderTemplates[$path].update_1f68c58_700($lepusTemplate, $n700, $data, $update2);
                  $update2 = _$template_update50;
                }
                $update2 = _$temp101;
              }
              $update2 = _$temp100;
            }
            $update2 = _$temp99;
          }
          $update2 = _$temp98;
        }
      }
    }
  },
  update_1f68c58_697: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!((_a = $data.vo.btnInfo) == null ? undefined : _a.backgroundImage)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n698 = $update2 ? $lepusGetElementRefByLepusID("image", 698) : null;
          let $temp2 = $update2;
          if (!$n698) {
            $update2 = false;
            $n698 = __CreateImage($currentComponentId);
            let $nid698 = $lepusStoreElementRefByLepusID($n698, 698, "image");
            __SetAttribute($n698, 1004, $nid698[1]);
            __SetAttribute($n698, "skip-redirection", true);
            __AppendElement($parent, $n698);
          }
          if (!$update2 || $renderTemplates["ShelfButton"].varUpdateState[0]) {
            {
              let $value = (_b = $data.vo.btnInfo) == null ? undefined : _b.backgroundImageStyle;
              if (!$update2 || $value !== ((_c = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _c.backgroundImageStyle)) {
                __SetStyleObject($n698, [76, 153, 55, 75, 77, 1, 0, getCssPropertyIDObj((_d = $data.vo.btnInfo) == null ? undefined : _d.backgroundImageStyle)]);
              }
            }
            {
              let _$value171 = (_e = $data.vo.btnInfo) == null ? undefined : _e.backgroundImage;
              if (!$update2 || _$value171 !== ((_f = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _f.backgroundImage)) {
                __SetAttribute($n698, "src", _$value171);
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
  update_1f68c58_700: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j;
    let $path = "ShelfButton";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!((_a = $data.vo.btnInfo) == null ? undefined : _a.textImage)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n701 = $update2 ? $lepusGetElementRefByLepusID("image", 701) : null;
          let $temp2 = $update2;
          if (!$n701) {
            $update2 = false;
            $n701 = __CreateImage($currentComponentId);
            let $nid701 = $lepusStoreElementRefByLepusID($n701, 701, "image");
            __SetAttribute($n701, 1004, $nid701[1]);
            __SetAttribute($n701, "skip-redirection", true);
            __AppendElement($parent, $n701);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = (_b = $data.vo.btnInfo) == null ? undefined : _b.textImageStyle;
              if (!$update2 || $value !== ((_c = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _c.textImageStyle)) {
                __SetStyleObject($n701, [169, 66, getCssPropertyIDObj((_d = $data.vo.btnInfo) == null ? undefined : _d.textImageStyle)]);
              }
            }
            {
              let _$value172 = (_e = $data.vo.btnInfo) == null ? undefined : _e.textImage;
              if (!$update2 || _$value172 !== ((_f = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _f.textImage)) {
                __SetAttribute($n701, "src", _$value172);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp102 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n702 = $update2 ? $lepusGetElementRefByLepusID("text", 702) : null;
          let _$temp103 = $update2;
          if (!$n702) {
            $update2 = false;
            $n702 = __CreateText($currentComponentId);
            let $nid702 = $lepusStoreElementRefByLepusID($n702, 702, "text");
            __SetAttribute($n702, 1004, $nid702[1]);
            __AppendElement($parent, $n702);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
            {
              __SetStyleObject($n702, [63, 21, 117, {
                47: !!((_g = $data.vo.btnInfo) == null ? undefined : _g.subText) ? "12px" : "13px"
              }, getCssPropertyIDObj((_h = $data.vo.btnInfo) == null ? undefined : _h.textStyle)]);
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value173 = (_i = $data.vo.btnInfo) == null ? undefined : _i.text;
              if (!$update2 || _$value173 !== ((_j = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _j.text)) {
                __SetAttribute($n702, "text", _$value173);
              }
            }
          }
          $update2 = _$temp103;
        }
        {
          let $template_update = $update2;
          let $n704 = $update2 ? $lepusGetElementRefByLepusID("if", 704) : null;
          if (!$n704) {
            $update2 = false;
            $n704 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n704, 704, "if");
            __AppendElement($parent, $n704);
          }
          $renderTemplates[$path].update_1f68c58_704($lepusTemplate, $n704, $data, $update2);
          $update2 = $template_update;
        }
        $update2 = _$temp102;
      }
    }
  },
  update_1f68c58_704: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f, _g;
    let $path = "ShelfButton";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ((_a = $data.vo.btnInfo) == null ? undefined : _a.showCountdown) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n705 = $update2 ? $lepusGetElementRefByLepusID("countdown-view", 705) : null;
          let $temp2 = $update2;
          if (!$n705) {
            $update2 = false;
            $n705 = __CreateElement("countdown-view", $currentComponentId);
            let $nid705 = $lepusStoreElementRefByLepusID($n705, 705, "countdown-view");
            __SetAttribute($n705, 1004, $nid705[1]);
            __SetStyleObject($n705, [4, 8, 101, 119, 120]);
            __AddEvent($n705, "bindEvent", "countdownend", "onShelfButtonCountDownEnd");
            __AppendElement($parent, $n705);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = "" + ((_b = $data.vo.btnInfo) == null ? undefined : _b.endTime);
              if (!$update2 || $value !== "" + ((_c = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _c.endTime)) {
                __SetAttribute($n705, "end-time", $value);
              }
            }
            {
              let _$value174 = ((_d = lynx.__globalProps) == null ? undefined : _d.os) === "android" ? "ms" : "millseconds";
              if (!$update2 || _$value174 !== undefined) {
                __SetAttribute($n705, "unit", _$value174);
              }
            }
          }
          {
            let $template_update = $update2;
            let $n706 = $update2 ? $lepusGetElementRefByLepusID("if", 706) : null;
            if (!$n706) {
              $update2 = false;
              $n706 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n706, 706, "if");
              __AppendElement($n705, $n706);
            }
            $renderTemplates[$path].update_1f68c58_706($lepusTemplate, $n706, $data, $update2);
            $update2 = $template_update;
          }
          {
            let $n709 = $update2 ? $lepusGetElementRefByLepusID("countdown-item", 709) : null;
            let $temp3 = $update2;
            if (!$n709) {
              $update2 = false;
              $n709 = __CreateElement("countdown-item", $currentComponentId);
              let $nid709 = $lepusStoreElementRefByLepusID($n709, 709, "countdown-item");
              __SetAttribute($n709, 1004, $nid709[1]);
              __SetAttribute($n709, "text-maxline", "1");
              __SetAttribute($n709, "countdown-display", "HH:mm:ss");
              __AppendElement($n705, $n709);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let _$value175 = (_e = $data.vo.btnInfo) == null ? undefined : _e.countdownTextStyle;
                if (!$update2 || _$value175 !== ((_f = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _f.countdownTextStyle)) {
                  __SetStyleObject($n709, [63, 116, 117, getCssPropertyIDObj((_g = $data.vo.btnInfo) == null ? undefined : _g.countdownTextStyle)]);
                }
              }
            }
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp104 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let _$template_update51 = $update2;
          let $n710 = $update2 ? $lepusGetElementRefByLepusID("if", 710) : null;
          if (!$n710) {
            $update2 = false;
            $n710 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n710, 710, "if");
            __AppendElement($parent, $n710);
          }
          $renderTemplates[$path].update_1f68c58_710($lepusTemplate, $n710, $data, $update2);
          $update2 = _$template_update51;
        }
        $update2 = _$temp104;
      }
    }
  },
  update_1f68c58_706: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "ShelfButton";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!((_a = $data.vo.btnInfo) == null ? undefined : _a.countdownPrefix)) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n707 = $update2 ? $lepusGetElementRefByLepusID("text", 707) : null;
          let $temp2 = $update2;
          if (!$n707) {
            $update2 = false;
            $n707 = __CreateText($currentComponentId);
            let $nid707 = $lepusStoreElementRefByLepusID($n707, 707, "text");
            __SetAttribute($n707, 1004, $nid707[1]);
            __AppendElement($parent, $n707);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = (_b = $data.vo.btnInfo) == null ? undefined : _b.countdownTextStyle;
              if (!$update2 || $value !== ((_c = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _c.countdownTextStyle)) {
                __SetStyleObject($n707, [63, 116, 117, getCssPropertyIDObj((_d = $data.vo.btnInfo) == null ? undefined : _d.countdownTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value176 = ((_e = $data.vo.btnInfo) == null ? undefined : _e.countdownPrefix) + " ";
              if (!$update2 || _$value176 !== ((_f = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _f.countdownPrefix) + " ") {
                __SetAttribute($n707, "text", _$value176);
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
  update_1f68c58_710: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d, _e, _f;
    let $path = "ShelfButton";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!((_a = $data.vo.btnInfo) == null ? undefined : _a.subText)) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        {
          let $n711 = $update2 ? $lepusGetElementRefByLepusID("text", 711) : null;
          let $temp2 = $update2;
          if (!$n711) {
            $update2 = false;
            $n711 = __CreateText($currentComponentId);
            let $nid711 = $lepusStoreElementRefByLepusID($n711, 711, "text");
            __SetAttribute($n711, 1004, $nid711[1]);
            __AppendElement($parent, $n711);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = (_b = $data.vo.btnInfo) == null ? undefined : _b.subTextStyle;
              if (!$update2 || $value !== ((_c = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _c.subTextStyle)) {
                __SetStyleObject($n711, [63, 116, 117, getCssPropertyIDObj((_d = $data.vo.btnInfo) == null ? undefined : _d.subTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value177 = (_e = $data.vo.btnInfo) == null ? undefined : _e.subText;
              if (!$update2 || _$value177 !== ((_f = $lepusTemplate._data.vo.btnInfo) == null ? undefined : _f.subText)) {
                __SetAttribute($n711, "text", _$value177);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "ShelfButton";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n687 = $lepusGetElementRefByLepusID("if", 687);
    $renderTemplates[$path].update_1f68c58_687($lepusTemplate, $n687, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n687 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n687, 687, "if");
    __AppendElement($template, $n687);
    $renderTemplates["ShelfButton"].update_1f68c58_687($lepusTemplate, $n687, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["PriceFinal"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_1f68c58_727: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceFinal";
    if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!!(!($data.vo.amount === 0) && !$data.vo.amount)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n728 = $update2 ? $lepusGetElementRefByLepusID("view", 728) : null;
            let $temp2 = $update2;
            if (!$n728) {
              $update2 = false;
              $n728 = __CreateView($currentComponentId);
              let $nid728 = $lepusStoreElementRefByLepusID($n728, 728, "view");
              __SetAttribute($n728, 1004, $nid728[1]);
              __AppendElement($parent, $n728);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n728, [3, 104, 31, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $template_update = $update2;
              let $n729 = $update2 ? $lepusGetElementRefByLepusID("if", 729) : null;
              if (!$n729) {
                $update2 = false;
                $n729 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n729, 729, "if");
                __AppendElement($n728, $n729);
              }
              $renderTemplates[$path].update_1f68c58_729($lepusTemplate, $n729, $data, $update2);
              $update2 = $template_update;
            }
            {
              let _$template_update52 = $update2;
              let $n732 = $update2 ? $lepusGetElementRefByLepusID("if", 732) : null;
              if (!$n732) {
                $update2 = false;
                $n732 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n732, 732, "if");
                __AppendElement($n728, $n732);
              }
              $renderTemplates[$path].update_1f68c58_732($lepusTemplate, $n732, $data, $update2);
              $update2 = _$template_update52;
            }
            {
              let $n735 = $update2 ? $lepusGetElementRefByLepusID("text", 735) : null;
              let $temp3 = $update2;
              if (!$n735) {
                $update2 = false;
                $n735 = __CreateText($currentComponentId);
                let $nid735 = $lepusStoreElementRefByLepusID($n735, 735, "text");
                __SetAttribute($n735, 1004, $nid735[1]);
                __SetAttribute($n735, "text-maxline", "1");
                __AppendElement($n728, $n735);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value178 = $data.vo.amountTextStyle;
                  if (!$update2 || _$value178 !== $lepusTemplate._data.vo.amountTextStyle) {
                    __SetStyleObject($n735, [89, 53, 12, 240, 31, 241, getCssPropertyIDObj($data.vo.amountTextStyle)]);
                  }
                }
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let _$value179 = $data.vo.intAmount;
                  if (!$update2 || _$value179 !== $lepusTemplate._data.vo.intAmount) {
                    __SetAttribute($n735, "text", _$value179);
                  }
                }
              }
              $update2 = $temp3;
            }
            {
              let _$template_update53 = $update2;
              let $n737 = $update2 ? $lepusGetElementRefByLepusID("if", 737) : null;
              if (!$n737) {
                $update2 = false;
                $n737 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n737, 737, "if");
                __AppendElement($n728, $n737);
              }
              $renderTemplates[$path].update_1f68c58_737($lepusTemplate, $n737, $data, $update2);
              $update2 = _$template_update53;
            }
            {
              let _$template_update54 = $update2;
              let $n740 = $update2 ? $lepusGetElementRefByLepusID("if", 740) : null;
              if (!$n740) {
                $update2 = false;
                $n740 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n740, 740, "if");
                __AppendElement($n728, $n740);
              }
              $renderTemplates[$path].update_1f68c58_740($lepusTemplate, $n740, $data, $update2);
              $update2 = _$template_update54;
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
  update_1f68c58_729: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceFinal";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.prefix) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n730 = $update2 ? $lepusGetElementRefByLepusID("text", 730) : null;
          let $temp2 = $update2;
          if (!$n730) {
            $update2 = false;
            $n730 = __CreateText($currentComponentId);
            let $nid730 = $lepusStoreElementRefByLepusID($n730, 730, "text");
            __SetAttribute($n730, 1004, $nid730[1]);
            __SetAttribute($n730, "text-maxline", "1");
            __AppendElement($parent, $n730);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixTextStyle) {
                __SetStyleObject($n730, [139, 21, 140, 240, 31, 89, 117, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value180 = $data.vo.prefix;
              if (!$update2 || _$value180 !== $lepusTemplate._data.vo.prefix) {
                __SetAttribute($n730, "text", _$value180);
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
  update_1f68c58_732: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceFinal";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.currency) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n733 = $update2 ? $lepusGetElementRefByLepusID("text", 733) : null;
          let $temp2 = $update2;
          if (!$n733) {
            $update2 = false;
            $n733 = __CreateText($currentComponentId);
            let $nid733 = $lepusStoreElementRefByLepusID($n733, 733, "text");
            __SetAttribute($n733, 1004, $nid733[1]);
            __SetAttribute($n733, "text-maxline", "1");
            __AppendElement($parent, $n733);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.currencyTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.currencyTextStyle) {
                __SetStyleObject($n733, [31, 40, 89, 17, 12, 240, 117, getCssPropertyIDObj($data.vo.currencyTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value181 = $data.vo.currency;
              if (!$update2 || _$value181 !== $lepusTemplate._data.vo.currency) {
                __SetAttribute($n733, "text", _$value181);
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
  update_1f68c58_737: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceFinal";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.decimalAmount) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n738 = $update2 ? $lepusGetElementRefByLepusID("text", 738) : null;
          let $temp2 = $update2;
          if (!$n738) {
            $update2 = false;
            $n738 = __CreateText($currentComponentId);
            let $nid738 = $lepusStoreElementRefByLepusID($n738, 738, "text");
            __SetAttribute($n738, 1004, $nid738[1]);
            __SetAttribute($n738, "text-maxline", "1");
            __AppendElement($parent, $n738);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n738, [89, 12, 240, 31, 241, {
                47: $data.vo.isDecimalSmall ? "10px" : "14px"
              }, getCssPropertyIDObj(__spreadValues(__spreadValues({}, $data.vo.amountTextStyle), $data.vo.decimalTextStyle))]);
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let $value = "." + $data.vo.decimalAmount;
              if (!$update2 || $value !== "." + $lepusTemplate._data.vo.decimalAmount) {
                __SetAttribute($n738, "text", $value);
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
  update_1f68c58_740: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceFinal";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.postfix) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n741 = $update2 ? $lepusGetElementRefByLepusID("text", 741) : null;
          let $temp2 = $update2;
          if (!$n741) {
            $update2 = false;
            $n741 = __CreateText($currentComponentId);
            let $nid741 = $lepusStoreElementRefByLepusID($n741, 741, "text");
            __SetAttribute($n741, 1004, $nid741[1]);
            __SetAttribute($n741, "text-maxline", "1");
            __AppendElement($parent, $n741);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.postfixTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.postfixTextStyle) {
                __SetStyleObject($n741, [11, 21, 240, 31, 89, 117, getCssPropertyIDObj($data.vo.postfixTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value182 = $data.vo.postfix;
              if (!$update2 || _$value182 !== $lepusTemplate._data.vo.postfix) {
                __SetAttribute($n741, "text", _$value182);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "PriceFinal";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n727 = $lepusGetElementRefByLepusID("if", 727);
    $renderTemplates[$path].update_1f68c58_727($lepusTemplate, $n727, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n727 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n727, 727, "if");
    __AppendElement($template, $n727);
    $renderTemplates["PriceFinal"].update_1f68c58_727($lepusTemplate, $n727, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["PriceBorder"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_1f68c58_743: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceBorder";
    if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!!(!($data.vo.amount === 0) && !$data.vo.amount)) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n744 = $update2 ? $lepusGetElementRefByLepusID("view", 744) : null;
            let $temp2 = $update2;
            if (!$n744) {
              $update2 = false;
              $n744 = __CreateView($currentComponentId);
              let $nid744 = $lepusStoreElementRefByLepusID($n744, 744, "view");
              __SetAttribute($n744, 1004, $nid744[1]);
              __AppendElement($parent, $n744);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n744, [3, 104, 242, 243, 244, 245, 173, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $template_update = $update2;
              let $n745 = $update2 ? $lepusGetElementRefByLepusID("if", 745) : null;
              if (!$n745) {
                $update2 = false;
                $n745 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n745, 745, "if");
                __AppendElement($n744, $n745);
              }
              $renderTemplates[$path].update_1f68c58_745($lepusTemplate, $n745, $data, $update2);
              $update2 = $template_update;
            }
            {
              let _$template_update55 = $update2;
              let $n748 = $update2 ? $lepusGetElementRefByLepusID("if", 748) : null;
              if (!$n748) {
                $update2 = false;
                $n748 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n748, 748, "if");
                __AppendElement($n744, $n748);
              }
              $renderTemplates[$path].update_1f68c58_748($lepusTemplate, $n748, $data, $update2);
              $update2 = _$template_update55;
            }
            {
              let $n751 = $update2 ? $lepusGetElementRefByLepusID("text", 751) : null;
              let $temp3 = $update2;
              if (!$n751) {
                $update2 = false;
                $n751 = __CreateText($currentComponentId);
                let $nid751 = $lepusStoreElementRefByLepusID($n751, 751, "text");
                __SetAttribute($n751, 1004, $nid751[1]);
                __SetAttribute($n751, "text-maxline", "1");
                __AppendElement($n744, $n751);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value183 = $data.vo.amountTextStyle;
                  if (!$update2 || _$value183 !== $lepusTemplate._data.vo.amountTextStyle) {
                    __SetStyleObject($n751, [248, 12, 249, 84, 31, 63, 247, getCssPropertyIDObj($data.vo.amountTextStyle)]);
                  }
                }
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let _$value184 = $data.vo.intAmount;
                  if (!$update2 || _$value184 !== $lepusTemplate._data.vo.intAmount) {
                    __SetAttribute($n751, "text", _$value184);
                  }
                }
              }
              $update2 = $temp3;
            }
            {
              let _$template_update56 = $update2;
              let $n753 = $update2 ? $lepusGetElementRefByLepusID("if", 753) : null;
              if (!$n753) {
                $update2 = false;
                $n753 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n753, 753, "if");
                __AppendElement($n744, $n753);
              }
              $renderTemplates[$path].update_1f68c58_753($lepusTemplate, $n753, $data, $update2);
              $update2 = _$template_update56;
            }
            {
              let _$template_update57 = $update2;
              let $n756 = $update2 ? $lepusGetElementRefByLepusID("if", 756) : null;
              if (!$n756) {
                $update2 = false;
                $n756 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n756, 756, "if");
                __AppendElement($n744, $n756);
              }
              $renderTemplates[$path].update_1f68c58_756($lepusTemplate, $n756, $data, $update2);
              $update2 = _$template_update57;
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
  update_1f68c58_745: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceBorder";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.prefix) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n746 = $update2 ? $lepusGetElementRefByLepusID("text", 746) : null;
          let $temp2 = $update2;
          if (!$n746) {
            $update2 = false;
            $n746 = __CreateText($currentComponentId);
            let $nid746 = $lepusStoreElementRefByLepusID($n746, 746, "text");
            __SetAttribute($n746, 1004, $nid746[1]);
            __SetAttribute($n746, "text-maxline", "1");
            __AppendElement($parent, $n746);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixTextStyle) {
                __SetStyleObject($n746, [26, 21, 140, 240, 31, 63, 246, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value185 = $data.vo.prefix;
              if (!$update2 || _$value185 !== $lepusTemplate._data.vo.prefix) {
                __SetAttribute($n746, "text", _$value185);
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
  update_1f68c58_748: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceBorder";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.currency) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n749 = $update2 ? $lepusGetElementRefByLepusID("text", 749) : null;
          let $temp2 = $update2;
          if (!$n749) {
            $update2 = false;
            $n749 = __CreateText($currentComponentId);
            let $nid749 = $lepusStoreElementRefByLepusID($n749, 749, "text");
            __SetAttribute($n749, 1004, $nid749[1]);
            __SetAttribute($n749, "text-maxline", "1");
            __AppendElement($parent, $n749);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.currencyTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.currencyTextStyle) {
                __SetStyleObject($n749, [53, 21, 40, 240, 31, 63, 247, getCssPropertyIDObj($data.vo.currencyTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value186 = $data.vo.currency;
              if (!$update2 || _$value186 !== $lepusTemplate._data.vo.currency) {
                __SetAttribute($n749, "text", _$value186);
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
  update_1f68c58_753: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceBorder";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.decimalAmount) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n754 = $update2 ? $lepusGetElementRefByLepusID("text", 754) : null;
          let $temp2 = $update2;
          if (!$n754) {
            $update2 = false;
            $n754 = __CreateText($currentComponentId);
            let $nid754 = $lepusStoreElementRefByLepusID($n754, 754, "text");
            __SetAttribute($n754, 1004, $nid754[1]);
            __SetAttribute($n754, "text-maxline", "1");
            __AppendElement($parent, $n754);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              __SetStyleObject($n754, [12, 249, 31, 63, 247, {
                47: $data.vo.isDecimalSmall ? "14px" : "22px"
              }, {
                45: $data.vo.isDecimalSmall ? "22px" : "26px"
              }, getCssPropertyIDObj(__spreadValues(__spreadValues({}, $data.vo.amountTextStyle), $data.vo.decimalTextStyle))]);
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let $value = "." + $data.vo.decimalAmount;
              if (!$update2 || $value !== "." + $lepusTemplate._data.vo.decimalAmount) {
                __SetAttribute($n754, "text", $value);
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
  update_1f68c58_756: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceBorder";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.postfix) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n757 = $update2 ? $lepusGetElementRefByLepusID("text", 757) : null;
          let $temp2 = $update2;
          if (!$n757) {
            $update2 = false;
            $n757 = __CreateText($currentComponentId);
            let $nid757 = $lepusStoreElementRefByLepusID($n757, 757, "text");
            __SetAttribute($n757, 1004, $nid757[1]);
            __SetAttribute($n757, "text-maxline", "1");
            __AppendElement($parent, $n757);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.postfixTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.postfixTextStyle) {
                __SetStyleObject($n757, [26, 18, 240, 31, 43, 63, 247, getCssPropertyIDObj($data.vo.postfixTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value187 = $data.vo.postfix;
              if (!$update2 || _$value187 !== $lepusTemplate._data.vo.postfix) {
                __SetAttribute($n757, "text", _$value187);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "PriceBorder";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n743 = $lepusGetElementRefByLepusID("if", 743);
    $renderTemplates[$path].update_1f68c58_743($lepusTemplate, $n743, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n743 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n743, 743, "if");
    __AppendElement($template, $n743);
    $renderTemplates["PriceBorder"].update_1f68c58_743($lepusTemplate, $n743, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["PriceDeduction"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_1f68c58_759: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceDeduction";
    if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!$data.vo.postfix) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n760 = $update2 ? $lepusGetElementRefByLepusID("view", 760) : null;
            let $temp2 = $update2;
            if (!$n760) {
              $update2 = false;
              $n760 = __CreateView($currentComponentId);
              let $nid760 = $lepusStoreElementRefByLepusID($n760, 760, "view");
              __SetAttribute($n760, 1004, $nid760[1]);
              __AppendElement($parent, $n760);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n760, [3, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $n761 = $update2 ? $lepusGetElementRefByLepusID("text", 761) : null;
              let $temp3 = $update2;
              if (!$n761) {
                $update2 = false;
                $n761 = __CreateText($currentComponentId);
                let $nid761 = $lepusStoreElementRefByLepusID($n761, 761, "text");
                __SetAttribute($n761, 1004, $nid761[1]);
                __SetAttribute($n761, "text-maxline", "1");
                __AppendElement($n760, $n761);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value188 = $data.vo.deductionTextStyle;
                  if (!$update2 || _$value188 !== $lepusTemplate._data.vo.deductionTextStyle) {
                    __SetStyleObject($n761, [41, 45, 16, 240, 89, 117, getCssPropertyIDObj($data.vo.deductionTextStyle)]);
                  }
                }
              }
              {
                let $template_update = $update2;
                let $n762 = $update2 ? $lepusGetElementRefByLepusID("if", 762) : null;
                if (!$n762) {
                  $update2 = false;
                  $n762 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n762, 762, "if");
                  __AppendElement($n761, $n762);
                }
                $renderTemplates[$path].update_1f68c58_762($lepusTemplate, $n762, $data, $update2);
                $update2 = $template_update;
              }
              {
                let $n765 = $update2 ? $lepusGetElementRefByLepusID("text", 765) : null;
                let $temp4 = $update2;
                if (!$n765) {
                  $update2 = false;
                  $n765 = __CreateText($currentComponentId);
                  let $nid765 = $lepusStoreElementRefByLepusID($n765, 765, "text");
                  __SetAttribute($n765, 1004, $nid765[1]);
                  __AppendElement($n761, $n765);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value189 = $data.vo.placeholderStyle;
                    if (!$update2 || _$value189 !== $lepusTemplate._data.vo.placeholderStyle) {
                      __SetStyleObject($n765, [26, 250, getCssPropertyIDObj($data.vo.placeholderStyle)]);
                    }
                  }
                }
                __SetAttribute($n765, "text", " ");
                $update2 = $temp4;
              }
              {
                let $n767 = $update2 ? $lepusGetElementRefByLepusID("text", 767) : null;
                let _$temp105 = $update2;
                if (!$n767) {
                  $update2 = false;
                  $n767 = __CreateText($currentComponentId);
                  let $nid767 = $lepusStoreElementRefByLepusID($n767, 767, "text");
                  __SetAttribute($n767, 1004, $nid767[1]);
                  __AppendElement($n761, $n767);
                }
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  {
                    let _$value190 = $data.vo.postfixTextStyle;
                    if (!$update2 || _$value190 !== $lepusTemplate._data.vo.postfixTextStyle) {
                      __SetStyleObject($n767, [26, 21, 117, getCssPropertyIDObj($data.vo.postfixTextStyle)]);
                    }
                  }
                }
                {
                  if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                    let _$value191 = $data.vo.postfix;
                    if (!$update2 || _$value191 !== $lepusTemplate._data.vo.postfix) {
                      __SetAttribute($n767, "text", _$value191);
                    }
                  }
                }
                $update2 = _$temp105;
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
  update_1f68c58_762: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceDeduction";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (!!$data.vo.prefix) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n763 = $update2 ? $lepusGetElementRefByLepusID("text", 763) : null;
          let $temp2 = $update2;
          if (!$n763) {
            $update2 = false;
            $n763 = __CreateText($currentComponentId);
            let $nid763 = $lepusStoreElementRefByLepusID($n763, 763, "text");
            __SetAttribute($n763, 1004, $nid763[1]);
            __AppendElement($parent, $n763);
          }
          if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
            {
              let $value = $data.vo.prefixTextStyle;
              if (!$update2 || $value !== $lepusTemplate._data.vo.prefixTextStyle) {
                __SetStyleObject($n763, [17, 21, 117, getCssPropertyIDObj($data.vo.prefixTextStyle)]);
              }
            }
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value192 = $data.vo.prefix;
              if (!$update2 || _$value192 !== $lepusTemplate._data.vo.prefix) {
                __SetAttribute($n763, "text", _$value192);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "PriceDeduction";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n759 = $lepusGetElementRefByLepusID("if", 759);
    $renderTemplates[$path].update_1f68c58_759($lepusTemplate, $n759, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n759 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n759, 759, "if");
    __AppendElement($template, $n759);
    $renderTemplates["PriceDeduction"].update_1f68c58_759($lepusTemplate, $n759, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["PriceText"] = {
  variables: ["vo"],
  varUpdateState: [],
  update_377e4c8_773: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.vo.isLineThrough) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n774 = $update2 ? $lepusGetElementRefByLepusID("view", 774) : null;
          let $temp2 = $update2;
          if (!$n774) {
            $update2 = false;
            $n774 = __CreateView($currentComponentId);
            let $nid774 = $lepusStoreElementRefByLepusID($n774, 774, "view");
            __SetAttribute($n774, 1004, $nid774[1]);
            __AppendElement($parent, $n774);
          }
          if (!$update2 || $renderTemplates["PriceText"].varUpdateState[0]) {
            {
              let $value = "background:" + ((((_a = $data.vo.textStyle) == null ? undefined : _a.color) || "#fe2c55") + ";") + "position:absolute;left:0;right:0;top:50%;height:0.5px;";
              if (!$update2 || $value !== "background:" + ((((_b = $lepusTemplate._data.vo.textStyle) == null ? undefined : _b.color) || "#fe2c55") + ";") + "position:absolute;left:0;right:0;top:50%;height:0.5px;") {
                __SetStyleObject($n774, [55, 76, 153, 251, 252, {
                  59: (((_c = $data.vo.textStyle) == null ? undefined : _c.color) || "#fe2c55") + ""
                }]);
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
  update_1f68c58_769: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "PriceText";
    if (!$update2 || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[0]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (!!$data.vo.text) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $n770 = $update2 ? $lepusGetElementRefByLepusID("view", 770) : null;
            let $temp2 = $update2;
            if (!$n770) {
              $update2 = false;
              $n770 = __CreateView($currentComponentId);
              let $nid770 = $lepusStoreElementRefByLepusID($n770, 770, "view");
              __SetAttribute($n770, 1004, $nid770[1]);
              __AppendElement($parent, $n770);
            }
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              {
                let $value = $data.vo.containerStyle;
                if (!$update2 || $value !== $lepusTemplate._data.vo.containerStyle) {
                  __SetStyleObject($n770, [3, 31, 173, getCssPropertyIDObj($data.vo.containerStyle)]);
                }
              }
            }
            {
              let $n771 = $update2 ? $lepusGetElementRefByLepusID("text", 771) : null;
              let $temp3 = $update2;
              if (!$n771) {
                $update2 = false;
                $n771 = __CreateText($currentComponentId);
                let $nid771 = $lepusStoreElementRefByLepusID($n771, 771, "text");
                __SetAttribute($n771, 1004, $nid771[1]);
                __SetAttribute($n771, "text-maxline", "1");
                __AppendElement($n770, $n771);
              }
              if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                {
                  let _$value193 = $data.vo.textStyle;
                  if (!$update2 || _$value193 !== $lepusTemplate._data.vo.textStyle) {
                    __SetStyleObject($n771, [11, 31, 89, 117, getCssPropertyIDObj($data.vo.textStyle)]);
                  }
                }
              }
              {
                if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
                  let _$value194 = $data.vo.text;
                  if (!$update2 || _$value194 !== $lepusTemplate._data.vo.text) {
                    __SetAttribute($n771, "text", _$value194);
                  }
                }
              }
              $update2 = $temp3;
            }
            {
              let $template_update = $update2;
              let $n773 = $update2 ? $lepusGetElementRefByLepusID("if", 773) : null;
              if (!$n773) {
                $update2 = false;
                $n773 = __CreateIf($currentComponentId);
                $lepusStoreElementRefByLepusID($n773, 773, "if");
                __AppendElement($n770, $n773);
              }
              $renderTemplates[$path].update_377e4c8_773($lepusTemplate, $n773, $data, $update2);
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
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "PriceText";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n769 = $lepusGetElementRefByLepusID("if", 769);
    $renderTemplates[$path].update_1f68c58_769($lepusTemplate, $n769, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n769 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n769, 769, "if");
    __AppendElement($template, $n769);
    $renderTemplates["PriceText"].update_1f68c58_769($lepusTemplate, $n769, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
$renderTemplates["UserRecommend"] = {
  variables: ["vo", "mustShow", "userName"],
  varUpdateState: [],
  update_1949e08_806: function ($lepusTemplate, $parent, $data, $update2, index, item) {
    let $path = "UserRecommend";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (index == 0) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        let $n807 = $update2 ? $lepusGetElementRefByLepusID("if", 807) : null;
        if (!$n807) {
          $update2 = false;
          $n807 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n807, 807, "if");
          __AppendElement($parent, $n807);
        }
        $renderTemplates[$path].update_1949e08_807($lepusTemplate, $n807, $data, $update2, index, item);
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp106 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        let $n810 = $update2 ? $lepusGetElementRefByLepusID("if", 810) : null;
        if (!$n810) {
          $update2 = false;
          $n810 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n810, 810, "if");
          __AppendElement($parent, $n810);
        }
        $renderTemplates[$path].update_1949e08_810($lepusTemplate, $n810, $data, $update2, index, item);
        $update2 = _$temp106;
      }
    }
  },
  update_1949e08_807: function ($lepusTemplate, $parent, $data, $update2, index, item) {
    let _a, _b;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.mustShow) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n808 = $update2 ? $lepusGetElementRefByLepusID("image", 808) : null;
          let $temp2 = $update2;
          if (!$n808) {
            $update2 = false;
            $n808 = __CreateImage($currentComponentId);
            let $nid808 = $lepusStoreElementRefByLepusID($n808, 808, "image");
            __SetAttribute($n808, 1004, $nid808[1]);
            __SetStyleObject($n808, [174, 52, 253, 254, 9]);
            __SetAttribute($n808, "skip-redirection", true);
            __AppendElement($parent, $n808);
          }
          __SetAttribute($n808, "src", (_a = item == null ? undefined : item.url_list) == null ? undefined : _a[0]);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp107 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n809 = $update2 ? $lepusGetElementRefByLepusID("image", 809) : null;
          let _$temp108 = $update2;
          if (!$n809) {
            $update2 = false;
            $n809 = __CreateImage($currentComponentId);
            let $nid809 = $lepusStoreElementRefByLepusID($n809, 809, "image");
            __SetAttribute($n809, 1004, $nid809[1]);
            __SetStyleObject($n809, [86, 85, 253, 254, 9]);
            __SetAttribute($n809, "skip-redirection", true);
            __AppendElement($parent, $n809);
          }
          __SetAttribute($n809, "src", (_b = item == null ? undefined : item.url_list) == null ? undefined : _b[0]);
          $update2 = _$temp108;
        }
        $update2 = _$temp107;
      }
    }
  },
  update_1949e08_810: function ($lepusTemplate, $parent, $data, $update2, index, item) {
    let _a, _b;
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.mustShow) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n811 = $update2 ? $lepusGetElementRefByLepusID("image", 811) : null;
          let $temp2 = $update2;
          if (!$n811) {
            $update2 = false;
            $n811 = __CreateImage($currentComponentId);
            let $nid811 = $lepusStoreElementRefByLepusID($n811, 811, "image");
            __SetAttribute($n811, 1004, $nid811[1]);
            __SetStyleObject($n811, [174, 52, 253, 254, 9, 255]);
            __SetAttribute($n811, "skip-redirection", true);
            __AppendElement($parent, $n811);
          }
          __SetAttribute($n811, "src", (_a = item == null ? undefined : item.url_list) == null ? undefined : _a[0]);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp109 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n812 = $update2 ? $lepusGetElementRefByLepusID("image", 812) : null;
          let _$temp110 = $update2;
          if (!$n812) {
            $update2 = false;
            $n812 = __CreateImage($currentComponentId);
            let $nid812 = $lepusStoreElementRefByLepusID($n812, 812, "image");
            __SetAttribute($n812, 1004, $nid812[1]);
            __SetStyleObject($n812, [86, 85, 253, 254, 9, 255]);
            __SetAttribute($n812, "skip-redirection", true);
            __AppendElement($parent, $n812);
          }
          __SetAttribute($n812, "src", (_b = item == null ? undefined : item.url_list) == null ? undefined : _b[0]);
          $update2 = _$temp110;
        }
        $update2 = _$temp109;
      }
    }
  },
  update_1949e08_829: function ($lepusTemplate, $parent, $data, $update2, index, item) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.mustShow) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n830 = $update2 ? $lepusGetElementRefByLepusID("text", 830) : null;
          let $temp2 = $update2;
          if (!$n830) {
            $update2 = false;
            $n830 = __CreateText($currentComponentId);
            let $nid830 = $lepusStoreElementRefByLepusID($n830, 830, "text");
            __SetAttribute($n830, 1004, $nid830[1]);
            __SetStyleObject($n830, [17, 256]);
            __AppendElement($parent, $n830);
          }
          __SetAttribute($n830, "text", item);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp111 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n832 = $update2 ? $lepusGetElementRefByLepusID("text", 832) : null;
          let _$temp112 = $update2;
          if (!$n832) {
            $update2 = false;
            $n832 = __CreateText($currentComponentId);
            let $nid832 = $lepusStoreElementRefByLepusID($n832, 832, "text");
            __SetAttribute($n832, 1004, $nid832[1]);
            __SetStyleObject($n832, [11, 256]);
            __AppendElement($parent, $n832);
          }
          __SetAttribute($n832, "text", item);
          $update2 = _$temp112;
        }
        $update2 = _$temp111;
      }
    }
  },
  update_dda630_805: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "UserRecommend";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo12 = $lepusPushFiberForNode($parent, 805, uniqueId),
          $forLepus = _$lepusPushFiberForNo12[0],
          $lastForLepus = _$lepusPushFiberForNo12[1];
      let $object = $data.vo.prefix_list;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        let $n806 = $update2 ? $lepusGetElementRefByLepusID("if", 806) : null;
        if (!$n806) {
          $n806 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n806, 806, "if");
          __AppendElement($parent, $n806);
        }
        $renderTemplates[$path].update_1949e08_806($lepusTemplate, $n806, $data, $update2, index, item);
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update_dda630_815: function ($lepusTemplate, $parent, $data, $update2) {
    let _a, _b, _c, _d;
    let $path = "UserRecommend";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.mustShow) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n816 = $update2 ? $lepusGetElementRefByLepusID("text", 816) : null;
          let $temp2 = $update2;
          if (!$n816) {
            $update2 = false;
            $n816 = __CreateText($currentComponentId);
            let $nid816 = $lepusStoreElementRefByLepusID($n816, 816, "text");
            __SetAttribute($n816, 1004, $nid816[1]);
            __SetStyleObject($n816, [17, 256]);
            __AppendElement($parent, $n816);
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let $value = (_a = $data.vo.content_list) == null ? undefined : _a[0];
              if (!$update2 || $value !== ((_b = $lepusTemplate._data.vo.content_list) == null ? undefined : _b[0])) {
                __SetAttribute($n816, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp113 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n818 = $update2 ? $lepusGetElementRefByLepusID("text", 818) : null;
          let _$temp114 = $update2;
          if (!$n818) {
            $update2 = false;
            $n818 = __CreateText($currentComponentId);
            let $nid818 = $lepusStoreElementRefByLepusID($n818, 818, "text");
            __SetAttribute($n818, 1004, $nid818[1]);
            __SetStyleObject($n818, [11, 256]);
            __AppendElement($parent, $n818);
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[0]) {
              let _$value195 = (_c = $data.vo.content_list) == null ? undefined : _c[0];
              if (!$update2 || _$value195 !== ((_d = $lepusTemplate._data.vo.content_list) == null ? undefined : _d[0])) {
                __SetAttribute($n818, "text", _$value195);
              }
            }
          }
          $update2 = _$temp114;
        }
        $update2 = _$temp113;
      }
    }
  },
  update_2491540_822: function ($lepusTemplate, $parent, $data, $update2, index, item) {
    let $path = "UserRecommend";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (index == 1) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n823 = $update2 ? $lepusGetElementRefByLepusID("view", 823) : null;
          let $temp2 = $update2;
          if (!$n823) {
            $update2 = false;
            $n823 = __CreateView($currentComponentId);
            let $nid823 = $lepusStoreElementRefByLepusID($n823, 823, "view");
            __SetAttribute($n823, 1004, $nid823[1]);
            __SetStyleObject($n823, [105, 140]);
            __AppendElement($parent, $n823);
          }
          let $n824 = $update2 ? $lepusGetElementRefByLepusID("if", 824) : null;
          if (!$n824) {
            $update2 = false;
            $n824 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n824, 824, "if");
            __AppendElement($n823, $n824);
          }
          $renderTemplates[$path].update_2491540_824($lepusTemplate, $n824, $data, $update2, index, item);
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp115 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        let $n829 = $update2 ? $lepusGetElementRefByLepusID("if", 829) : null;
        if (!$n829) {
          $update2 = false;
          $n829 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n829, 829, "if");
          __AppendElement($parent, $n829);
        }
        $renderTemplates[$path].update_1949e08_829($lepusTemplate, $n829, $data, $update2, index, item);
        $update2 = _$temp115;
      }
    }
  },
  update_2491540_824: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "UserRecommend";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.mustShow) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n825 = $update2 ? $lepusGetElementRefByLepusID("text", 825) : null;
          let $temp2 = $update2;
          if (!$n825) {
            $update2 = false;
            $n825 = __CreateText($currentComponentId);
            let $nid825 = $lepusStoreElementRefByLepusID($n825, 825, "text");
            __SetAttribute($n825, 1004, $nid825[1]);
            __SetStyleObject($n825, [17, 256, 65]);
            __AppendElement($parent, $n825);
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[2]) {
              let $value = $data.userName;
              if (!$update2 || $value !== $lepusTemplate._data.userName) {
                __SetAttribute($n825, "text", $value);
              }
            }
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp116 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n827 = $update2 ? $lepusGetElementRefByLepusID("text", 827) : null;
          let _$temp117 = $update2;
          if (!$n827) {
            $update2 = false;
            $n827 = __CreateText($currentComponentId);
            let $nid827 = $lepusStoreElementRefByLepusID($n827, 827, "text");
            __SetAttribute($n827, 1004, $nid827[1]);
            __SetStyleObject($n827, [11, 256, 257]);
            __AppendElement($parent, $n827);
          }
          {
            if (!$update2 || $renderTemplates[$path].varUpdateState[2]) {
              let _$value196 = $data.userName;
              if (!$update2 || _$value196 !== $lepusTemplate._data.userName) {
                __SetAttribute($n827, "text", _$value196);
              }
            }
          }
          $update2 = _$temp117;
        }
        $update2 = _$temp116;
      }
    }
  },
  update_20e5e0_814: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "UserRecommend";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
      {
        let uniqueId = __GetElementUniqueID($parent);
        if (!$update2) {
          $conditionNodeIndex[uniqueId] = -1;
        }
        let $ifNodeIndex = $conditionNodeIndex[uniqueId];
        if (((_a = $data.vo.content_list) == null ? undefined : _a.length) == 1) {
          __UpdateIfNodeIndex($parent, 0);
          $conditionNodeIndex[uniqueId] = 0;
          let $temp = $update2;
          if ($ifNodeIndex !== 0) {
            $update2 = false;
          }
          {
            let $template_update = $update2;
            let $n815 = $update2 ? $lepusGetElementRefByLepusID("if", 815) : null;
            if (!$n815) {
              $update2 = false;
              $n815 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n815, 815, "if");
              __AppendElement($parent, $n815);
            }
            $renderTemplates[$path].update_dda630_815($lepusTemplate, $n815, $data, $update2);
            $update2 = $template_update;
          }
          $update2 = $temp;
        } else {
          __UpdateIfNodeIndex($parent, 1);
          $conditionNodeIndex[uniqueId] = 1;
          let _$temp118 = $update2;
          if ($ifNodeIndex !== 1) {
            $update2 = false;
          }
          {
            let _$template_update58 = $update2;
            let $n820 = $update2 ? $lepusGetElementRefByLepusID("if", 820) : null;
            if (!$n820) {
              $update2 = false;
              $n820 = __CreateIf($currentComponentId);
              $lepusStoreElementRefByLepusID($n820, 820, "if");
              __AppendElement($parent, $n820);
            }
            $renderTemplates[$path].update_20e5e0_820($lepusTemplate, $n820, $data, $update2);
            $update2 = _$template_update58;
          }
          $update2 = _$temp118;
        }
      }
    }
  },
  update_20e5e0_820: function ($lepusTemplate, $parent, $data, $update2) {
    let _a;
    let $path = "UserRecommend";
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if (((_a = $data.vo.content_list) == null ? undefined : _a.length) > 1) {
        __UpdateIfNodeIndex($parent, 2);
        $conditionNodeIndex[uniqueId] = 2;
        let $temp = $update2;
        if ($ifNodeIndex !== 2) {
          $update2 = false;
        }
        if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
          let $n821 = $update2 ? $lepusGetElementRefByLepusID("for", 821) : null;
          if (!$n821) {
            $n821 = __CreateFor($currentComponentId);
            $lepusStoreElementRefByLepusID($n821, 821, "for");
            __AppendElement($parent, $n821);
          }
          $renderTemplates[$path].update_20e5e0_821($lepusTemplate, $n821, $data, $update2);
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, -1);
        $conditionNodeIndex[uniqueId] = -1;
      }
    }
  },
  update_20e5e0_821: function ($lepusTemplate, $parent, $data, $update2) {
    let $path = "UserRecommend";
    if (!$update2 || $renderTemplates[$path].varUpdateState[0] || $renderTemplates[$path].varUpdateState[1] || $renderTemplates[$path].varUpdateState[2]) {
      let uniqueId = __GetElementUniqueID($parent);
      let _$lepusPushFiberForNo13 = $lepusPushFiberForNode($parent, 821, uniqueId),
          $forLepus = _$lepusPushFiberForNo13[0],
          $lastForLepus = _$lepusPushFiberForNo13[1];
      let $object = $data.vo.content_list;
      let $length = _GetLength($object);
      __UpdateForChildCount($parent, $length);
      let $temp = $update2;
      for (let index = 0; index < $length; ++index) {
        $update2 = index < $forLepus._lastLength ? $update2 : false;
        $lepusUpdateFiberForNodeIndex(index);
        let item = $object[index];
        let $n822 = $update2 ? $lepusGetElementRefByLepusID("if", 822) : null;
        if (!$n822) {
          $n822 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n822, 822, "if");
          __AppendElement($parent, $n822);
        }
        $renderTemplates[$path].update_2491540_822($lepusTemplate, $n822, $data, $update2, index, item);
      }
      $forLepus._lastLength = $length;
      $update2 = $temp;
      $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    }
  },
  update: function ($lepusTemplate, $data, $array, $update2) {
    let $path = "UserRecommend";
    let $lastTemplate = $templateUpdate($lepusTemplate, $path, $array);
    let $n805 = $lepusGetElementRefByLepusID("for", 805);
    $renderTemplates[$path].update_dda630_805($lepusTemplate, $n805, $data, $update2);
    let $n814 = $lepusGetElementRefByLepusID("if", 814);
    $renderTemplates[$path].update_20e5e0_814($lepusTemplate, $n814, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  },
  entry: function ($template, $lepusTemplate, $data, $update2) {
    let $path = "UserRecommend";
    let $lastTemplate = $lepusPushFiberTemplateNode($lepusTemplate);
    let $n803 = __CreateView($currentComponentId);
    __SetAttribute($n803, 1004, 803);
    __SetStyleObject($n803, [3, 4, 8]);
    __AppendElement($template, $n803);
    let $n804 = __CreateView($currentComponentId);
    __SetAttribute($n804, 1004, 804);
    __SetStyleObject($n804, [3, 4, 8]);
    __AppendElement($n803, $n804);
    let $n805 = __CreateFor($currentComponentId);
    $lepusStoreElementRefByLepusID($n805, 805, "for");
    __AppendElement($n804, $n805);
    $renderTemplates[$path].update_dda630_805($lepusTemplate, $n805, $data, $update2);
    let $n813 = __CreateView($currentComponentId);
    __SetAttribute($n813, 1004, 813);
    __SetStyleObject($n813, [3, 4, 8, 105]);
    __AppendElement($n803, $n813);
    let $n814 = __CreateIf($currentComponentId);
    $lepusStoreElementRefByLepusID($n814, 814, "if");
    __AppendElement($n813, $n814);
    $renderTemplates[$path].update_20e5e0_814($lepusTemplate, $n814, $data, $update2);
    $lepusPushFiberTemplateNode($lastTemplate);
  }
};
function $$update_3d4f50_841($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.spurCardDescitionVo.saleTagVOListSize > 0) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n842 = $update2 ? $lepusGetElementRefByLepusID("view", 842) : null;
        let $temp2 = $update2;
        if (!$n842) {
          $update2 = false;
          $n842 = __CreateView($currentComponentId);
          let $nid842 = $lepusStoreElementRefByLepusID($n842, 842, "view");
          __SetAttribute($n842, 1004, $nid842[1]);
          __SetStyleObject($n842, [263, 264]);
          __AppendElement($parent, $n842);
        }
        {
          let $n843 = $update2 ? $lepusGetElementRefByLepusID("view", 843) : null;
          let $temp3 = $update2;
          if (!$n843) {
            $update2 = false;
            $n843 = __CreateView($currentComponentId);
            let $nid843 = $lepusStoreElementRefByLepusID($n843, 843, "view");
            __SetAttribute($n843, 1004, $nid843[1]);
            __SetStyleObject($n843, [3, 265, 44, 4, 103, 45]);
            __AppendElement($n842, $n843);
          }
          if (!$update2 || $varUpdateState[3]) {
            let $n844 = $update2 ? $lepusGetElementRefByLepusID("for", 844) : null;
            if (!$n844) {
              $n844 = __CreateFor($currentComponentId);
              $lepusStoreElementRefByLepusID($n844, 844, "for");
              __AppendElement($n843, $n844);
            }
            $$update_3d4f50_844($n844, $data, $update2);
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
$$update_3d4f50_844 = function ($parent, $data, $update2) {
  if (!$update2 || $varUpdateState[3]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo14 = $lepusPushFiberForNode($parent, 844, uniqueId),
        $forLepus = _$lepusPushFiberForNo14[0],
        $lastForLepus = _$lepusPushFiberForNo14[1];
    let $object = $data.spurCardDescitionVo.saleTagVOList;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      {
        let $n845 = $update2 ? $lepusGetElementRefByLepusID("view", 845) : null;
        let $temp2 = $update2;
        if (!$n845) {
          $update2 = false;
          $n845 = __CreateView($currentComponentId);
          let $nid845 = $lepusStoreElementRefByLepusID($n845, 845, "view");
          __SetAttribute($n845, 1004, $nid845[1]);
          __SetStyleObject($n845, [3, 266, 4]);
          __AppendElement($parent, $n845);
        }
        {
          let $n846 = $update2 ? $lepusGetElementRefByLepusID("if", 846) : null;
          if (!$n846) {
            $n846 = __CreateIf($currentComponentId);
            $lepusStoreElementRefByLepusID($n846, 846, "if");
            __AppendElement($n845, $n846);
          }
          let uniqueId2 = __GetElementUniqueID($n846);
          if (!$update2) {
            $conditionNodeIndex[uniqueId2] = -1;
          }
          let $ifNodeIndex = $conditionNodeIndex[uniqueId2];
          if (index != 0) {
            __UpdateIfNodeIndex($n846, 0);
            $conditionNodeIndex[uniqueId2] = 0;
            let $temp3 = $update2;
            if ($ifNodeIndex !== 0) {
              $update2 = false;
            }
            {
              let $n847 = $update2 ? $lepusGetElementRefByLepusID("view", 847) : null;
              let $temp4 = $update2;
              if (!$n847) {
                $update2 = false;
                $n847 = __CreateView($currentComponentId);
                let $nid847 = $lepusStoreElementRefByLepusID($n847, 847, "view");
                __SetAttribute($n847, 1004, $nid847[1]);
                __SetStyleObject($n847, [105, 140]);
                __AppendElement($n846, $n847);
              }
              {
                let $n848 = $update2 ? $lepusGetElementRefByLepusID("text", 848) : null;
                let $temp5 = $update2;
                if (!$n848) {
                  $update2 = false;
                  $n848 = __CreateText($currentComponentId);
                  let $nid848 = $lepusStoreElementRefByLepusID($n848, 848, "text");
                  __SetAttribute($n848, 1004, $nid848[1]);
                  __SetStyleObject($n848, [267, 26]);
                  __AppendElement($n847, $n848);
                }
                __SetAttribute($n848, "text", "·");
                $update2 = $temp5;
              }
              $update2 = $temp4;
            }
            $update2 = $temp3;
          } else {
            __UpdateIfNodeIndex($n846, -1);
            $conditionNodeIndex[uniqueId2] = -1;
          }
        }
        {
          let $n850 = $update2 ? $lepusGetElementRefByLepusID("text", 850) : null;
          let _$temp119 = $update2;
          if (!$n850) {
            $update2 = false;
            $n850 = __CreateText($currentComponentId);
            let $nid850 = $lepusStoreElementRefByLepusID($n850, 850, "text");
            __SetAttribute($n850, 1004, $nid850[1]);
            __SetStyleObject($n850, [267, 26]);
            __AppendElement($n845, $n850);
          }
          __SetAttribute($n850, "text", item.content || "");
          $update2 = _$temp119;
        }
        $update2 = $temp2;
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
};
function $$update_3d4f50_853($parent, $data, $update2) {
  let _a, _b, _c, _d, _e;
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ((_a = $data.spurCardDescitionVo) == null ? undefined : _a.saleCount) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n854 = $update2 ? $lepusGetElementRefByLepusID("text", 854) : null;
        let $temp2 = $update2;
        if (!$n854) {
          $update2 = false;
          $n854 = __CreateText($currentComponentId);
          let $nid854 = $lepusStoreElementRefByLepusID($n854, 854, "text");
          __SetAttribute($n854, 1004, $nid854[1]);
          __SetStyleObject($n854, [17, 41, 31, 267]);
          __SetAttribute($n854, "accessibility-element", false);
          __AppendElement($parent, $n854);
        }
        {
          if (!$update2 || $varUpdateState[3]) {
            let $value = (((_b = $data.spurCardDescitionVo) == null ? undefined : _b.saleName) || "已售") + ((_c = $data.spurCardDescitionVo) == null ? undefined : _c.saleCount);
            if (!$update2 || $value !== (((_d = $cardInstance._data.spurCardDescitionVo) == null ? undefined : _d.saleName) || "已售") + ((_e = $cardInstance._data.spurCardDescitionVo) == null ? undefined : _e.saleCount)) {
              __SetAttribute($n854, "text", $value);
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
function $$update_3d4f50_881($parent, $data, $update2) {
  let _a, _b;
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ((_a = $data.spurCardDescitionVo) == null ? undefined : _a.decisionText) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n882 = $update2 ? $lepusGetElementRefByLepusID("template", 882) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n882) {
          $templateCreated = false;
          $n882 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n882, 882, "template");
          __AppendElement($parent, $n882);
          $templateId = __GetElementUniqueID($n882);
          $childLepusTemplate = $templateConstructor($templateId, $n882);
        } else {
          $templateId = __GetElementUniqueID($n882);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("decisionText", (_b = $data.spurCardDescitionVo) == null ? undefined : _b.decisionText, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["SpuDecisionText"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["SpuDecisionText"].entry($n882, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
}
function $$update_3d4f50_886($parent, $data, $update2) {
  let _a;
  if (!$update2 || $varUpdateState[3]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo15 = $lepusPushFiberForNode($parent, 886, uniqueId),
        $forLepus = _$lepusPushFiberForNo15[0],
        $lastForLepus = _$lepusPushFiberForNo15[1];
    let $object = (_a = $data.spurCardDescitionVo) == null ? undefined : _a.saleTagVOList;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      {
        let $n887 = $update2 ? $lepusGetElementRefByLepusID("template", 887) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n887) {
          $templateCreated = false;
          $n887 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n887, 887, "template");
          __AppendElement($parent, $n887);
          $templateId = __GetElementUniqueID($n887);
          $childLepusTemplate = $templateConstructor($templateId, $n887);
        } else {
          $templateId = __GetElementUniqueID($n887);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("saleTagVO", item, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["SpuTag"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["SpuTag"].entry($n887, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}
function $$update_3d4f50_888($parent, $data, $update2) {
  let _a, _b, _c, _d, _e;
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ((_a = $data.spurCardDescitionVo) == null ? undefined : _a.saleCount) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n889 = $update2 ? $lepusGetElementRefByLepusID("text", 889) : null;
        let $temp2 = $update2;
        if (!$n889) {
          $update2 = false;
          $n889 = __CreateText($currentComponentId);
          let $nid889 = $lepusStoreElementRefByLepusID($n889, 889, "text");
          __SetAttribute($n889, 1004, $nid889[1]);
          __SetStyleObject($n889, [17, 41, 31, 267]);
          __SetAttribute($n889, "accessibility-element", false);
          __AppendElement($parent, $n889);
        }
        {
          if (!$update2 || $varUpdateState[3]) {
            let $value = (((_b = $data.spurCardDescitionVo) == null ? undefined : _b.saleName) || "已售") + ((_c = $data.spurCardDescitionVo) == null ? undefined : _c.saleCount);
            if (!$update2 || $value !== (((_d = $cardInstance._data.spurCardDescitionVo) == null ? undefined : _d.saleName) || "已售") + ((_e = $cardInstance._data.spurCardDescitionVo) == null ? undefined : _e.saleCount)) {
              __SetAttribute($n889, "text", $value);
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
function $$update_3d4f50_893($parent, $data, $update2) {
  let _a;
  if (!$update2 || $varUpdateState[3]) {
    let uniqueId = __GetElementUniqueID($parent);
    let _$lepusPushFiberForNo16 = $lepusPushFiberForNode($parent, 893, uniqueId),
        $forLepus = _$lepusPushFiberForNo16[0],
        $lastForLepus = _$lepusPushFiberForNo16[1];
    let $object = (_a = $data.spurCardDescitionVo) == null ? undefined : _a.displayPointVOList;
    let $length = _GetLength($object);
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    for (let index = 0; index < $length; ++index) {
      $update2 = index < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(index);
      let item = $object[index];
      let $n894 = $update2 ? $lepusGetElementRefByLepusID("if", 894) : null;
      if (!$n894) {
        $n894 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n894, 894, "if");
        __AppendElement($parent, $n894);
      }
      $$update_a91910_894($n894, $data, $update2, index, item);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}
function $$update_2a86f68_861($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (!!$data.marketingInfoVO) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n862 = $update2 ? $lepusGetElementRefByLepusID("template", 862) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n862) {
          $templateCreated = false;
          $n862 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n862, 862, "template");
          __AppendElement($parent, $n862);
          $templateId = __GetElementUniqueID($n862);
          $childLepusTemplate = $templateConstructor($templateId, $n862);
        } else {
          $templateId = __GetElementUniqueID($n862);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
        $childLepusTemplate.setData("seckill", $data.seckill, $update2);
        $childLepusTemplate.setData("isCouponOverflow", $data.isCouponOverflow, $update2);
        $childLepusTemplate.setData("isSiblingOverflow", $data.isSiblingOverflow, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["ProcessComp"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["ProcessComp"].entry($n862, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
}
function $$update_2a86f68_906($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (!!$data.marketingInfoVO) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n907 = $update2 ? $lepusGetElementRefByLepusID("template", 907) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n907) {
          $templateCreated = false;
          $n907 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n907, 907, "template");
          __AppendElement($parent, $n907);
          $templateId = __GetElementUniqueID($n907);
          $childLepusTemplate = $templateConstructor($templateId, $n907);
        } else {
          $templateId = __GetElementUniqueID($n907);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("marketingInfoVO", $data.marketingInfoVO, $update2);
        $childLepusTemplate.setData("seckill", $data.seckill, $update2);
        $childLepusTemplate.setData("isCouponOverflow", $data.isCouponOverflow, $update2);
        $childLepusTemplate.setData("isSiblingOverflow", $data.isSiblingOverflow, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["ProcessComp"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["ProcessComp"].entry($n907, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
}
function $$update_34b9120_859($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (!!$data.marketingV2VO) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n860 = $update2 ? $lepusGetElementRefByLepusID("template", 860) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n860) {
          $templateCreated = false;
          $n860 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n860, 860, "template");
          __AppendElement($parent, $n860);
          $templateId = __GetElementUniqueID($n860);
          $childLepusTemplate = $templateConstructor($templateId, $n860);
        } else {
          $templateId = __GetElementUniqueID($n860);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("marketingV2VO", $data.marketingV2VO, $update2);
        $childLepusTemplate.setData("priceInfoState", $data.priceInfoState, $update2);
        $childLepusTemplate.setData("tagCountdownState", $data.tagCountdownState, $update2);
        $childLepusTemplate.setData("tagMoreIconState", $data.tagMoreIconState, $update2);
        $childLepusTemplate.setData("tagAnimateProgressState", $data.tagAnimateProgressState, $update2);
        $childLepusTemplate.setData("tagSecKillCountDownState", $data.tagSecKillCountDownState, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["ProcessCompV2"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["ProcessCompV2"].entry($n860, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp120 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let $template_update = $update2;
        let $n861 = $update2 ? $lepusGetElementRefByLepusID("if", 861) : null;
        if (!$n861) {
          $update2 = false;
          $n861 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n861, 861, "if");
          __AppendElement($parent, $n861);
        }
        $$update_2a86f68_861($n861, $data, $update2);
        $update2 = $template_update;
      }
      $update2 = _$temp120;
    }
  }
}
function $$update_34b9120_904($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (!!$data.marketingV2VO) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n905 = $update2 ? $lepusGetElementRefByLepusID("template", 905) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n905) {
          $templateCreated = false;
          $n905 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n905, 905, "template");
          __AppendElement($parent, $n905);
          $templateId = __GetElementUniqueID($n905);
          $childLepusTemplate = $templateConstructor($templateId, $n905);
        } else {
          $templateId = __GetElementUniqueID($n905);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("marketingV2VO", $data.marketingV2VO, $update2);
        $childLepusTemplate.setData("priceInfoState", $data.priceInfoState, $update2);
        $childLepusTemplate.setData("tagCountdownState", $data.tagCountdownState, $update2);
        $childLepusTemplate.setData("tagMoreIconState", $data.tagMoreIconState, $update2);
        $childLepusTemplate.setData("tagAnimateProgressState", $data.tagAnimateProgressState, $update2);
        $childLepusTemplate.setData("tagSecKillCountDownState", $data.tagSecKillCountDownState, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["ProcessCompV2"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["ProcessCompV2"].entry($n905, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp121 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let $template_update = $update2;
        let $n906 = $update2 ? $lepusGetElementRefByLepusID("if", 906) : null;
        if (!$n906) {
          $update2 = false;
          $n906 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n906, 906, "if");
          __AppendElement($parent, $n906);
        }
        $$update_2a86f68_906($n906, $data, $update2);
        $update2 = $template_update;
      }
      $update2 = _$temp121;
    }
  }
}
function $$update_7a5080_857($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.isDeliveryModule && !$data.useMarketing) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n858 = $update2 ? $lepusGetElementRefByLepusID("template", 858) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n858) {
          $templateCreated = false;
          $n858 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n858, 858, "template");
          __AppendElement($parent, $n858);
          $templateId = __GetElementUniqueID($n858);
          $childLepusTemplate = $templateConstructor($templateId, $n858);
        } else {
          $templateId = __GetElementUniqueID($n858);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("deliveryPurchaseInfoVO", $data.deliveryPurchaseInfoVO, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["DeliveryPurchaseInfo"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["DeliveryPurchaseInfo"].entry($n858, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp122 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let $template_update = $update2;
        let $n859 = $update2 ? $lepusGetElementRefByLepusID("if", 859) : null;
        if (!$n859) {
          $update2 = false;
          $n859 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n859, 859, "if");
          __AppendElement($parent, $n859);
        }
        $$update_34b9120_859($n859, $data, $update2);
        $update2 = $template_update;
      }
      $update2 = _$temp122;
    }
  }
}
function $$update_7a5080_902($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.isDeliveryModule && !$data.useMarketing) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n903 = $update2 ? $lepusGetElementRefByLepusID("template", 903) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n903) {
          $templateCreated = false;
          $n903 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n903, 903, "template");
          __AppendElement($parent, $n903);
          $templateId = __GetElementUniqueID($n903);
          $childLepusTemplate = $templateConstructor($templateId, $n903);
        } else {
          $templateId = __GetElementUniqueID($n903);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("deliveryPurchaseInfoVO", $data.deliveryPurchaseInfoVO, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["DeliveryPurchaseInfo"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["DeliveryPurchaseInfo"].entry($n903, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp123 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let $template_update = $update2;
        let $n904 = $update2 ? $lepusGetElementRefByLepusID("if", 904) : null;
        if (!$n904) {
          $update2 = false;
          $n904 = __CreateIf($currentComponentId);
          $lepusStoreElementRefByLepusID($n904, 904, "if");
          __AppendElement($parent, $n904);
        }
        $$update_34b9120_904($n904, $data, $update2);
        $update2 = $template_update;
      }
      $update2 = _$temp123;
    }
  }
}
function $$update_1f7b920_866($parent, $data, $update2) {
  let _a, _b, _c, _d, _e, _f, _g, _h;
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.spuCardCoverUrl) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n867 = $update2 ? $lepusGetElementRefByLepusID("image", 867) : null;
        let $temp2 = $update2;
        if (!$n867) {
          $update2 = false;
          $n867 = __CreateImage($currentComponentId);
          let $nid867 = $lepusStoreElementRefByLepusID($n867, 867, "image");
          __SetAttribute($n867, 1004, $nid867[1]);
          __SetStyleObject($n867, [0, 1, 272, 36, 35, 273, 274]);
          __SetAttribute($n867, "mode", "aspectFill");
          __SetAttribute($n867, "accessibility-element", false);
          __SetAttribute($n867, "skip-redirection", true);
          __AppendElement($parent, $n867);
        }
        if (!$update2 || $varUpdateState[18] || $varUpdateState[17] || $varUpdateState[19]) {
          {
            let $value = $data.spuCardCoverUrl;
            if (!$update2 || $value !== $cardInstance._data.spuCardCoverUrl) {
              __SetAttribute($n867, "src", $value);
            }
          }
          {
            let _$value197 = lynx.__globalProps.imageSimpleCache == 1;
            if (!$update2 || _$value197 !== undefined) {
              __SetAttribute($n867, "android-simple-cache-key", _$value197);
            }
          }
          {
            let _$value198 = $data.hasElementListInSpu ? "103px" : "84px";
            if (!$update2 || _$value198 !== ($cardInstance._data.hasElementListInSpu ? "103px" : "84px")) {
              __SetAttribute($n867, "prefetch-width", _$value198);
            }
          }
          {
            let _$value199 = $data.hasElementListInSpu ? "103px" : "84px";
            if (!$update2 || _$value199 !== ($cardInstance._data.hasElementListInSpu ? "103px" : "84px")) {
              __SetAttribute($n867, "prefetch-height", _$value199);
            }
          }
          {
            let _$value200 = ((_b = (_a = $data.ditoContext) == null ? undefined : _a.image_optimize_ab) == null ? undefined : _b.image_fetch_priority) ? "high" : "";
            if (!$update2 || _$value200 !== (((_d = (_c = $cardInstance._data.ditoContext) == null ? undefined : _c.image_optimize_ab) == null ? undefined : _d.image_fetch_priority) ? "high" : "")) {
              __SetAttribute($n867, "fetch-priority", _$value200);
            }
          }
          {
            let _$value201 = ((_f = (_e = $data.ditoContext) == null ? undefined : _e.image_optimize_ab) == null ? undefined : _f.image_cache_opti) ? "life_homepage_image_cache" : "";
            if (!$update2 || _$value201 !== (((_h = (_g = $cardInstance._data.ditoContext) == null ? undefined : _g.image_optimize_ab) == null ? undefined : _h.image_cache_opti) ? "life_homepage_image_cache" : "")) {
              __SetAttribute($n867, "cache-choice", _$value201);
            }
          }
        }
        $update2 = $temp2;
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp124 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      {
        let $n868 = $update2 ? $lepusGetElementRefByLepusID("view", 868) : null;
        let _$temp125 = $update2;
        if (!$n868) {
          $update2 = false;
          $n868 = __CreateView($currentComponentId);
          let $nid868 = $lepusStoreElementRefByLepusID($n868, 868, "view");
          __SetAttribute($n868, 1004, $nid868[1]);
          __SetStyleObject($n868, [0, 1, 272, 36, 35, 273]);
          __AppendElement($parent, $n868);
        }
        $update2 = _$temp125;
      }
      $update2 = _$temp124;
    }
  }
}
function $$update_3d2b6c8_869($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.showMarketingSpuCoverAtmosphere) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n870 = $update2 ? $lepusGetElementRefByLepusID("image", 870) : null;
        let $temp2 = $update2;
        if (!$n870) {
          $update2 = false;
          $n870 = __CreateImage($currentComponentId);
          let $nid870 = $lepusStoreElementRefByLepusID($n870, 870, "image");
          __SetAttribute($n870, 1004, $nid870[1]);
          __SetStyleObject($n870, [55, 75, 76, 0, 1, 272, 36, 35, 273]);
          __SetAttribute($n870, "mode", "aspectFill");
          __AppendElement($parent, $n870);
        }
        if (!$update2 || $varUpdateState[21]) {
          {
            let $value = $data.spuCardCoverAtmosphereUrl;
            if (!$update2 || $value !== $cardInstance._data.spuCardCoverAtmosphereUrl) {
              __SetAttribute($n870, "src", $value);
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
function $$update_34b0098_875($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.subSpuInfoList != null && $data.subSpuInfoList != undefined) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n876 = $update2 ? $lepusGetElementRefByLepusID("template", 876) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n876) {
          $templateCreated = false;
          $n876 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n876, 876, "template");
          __AppendElement($parent, $n876);
          $templateId = __GetElementUniqueID($n876);
          $childLepusTemplate = $templateConstructor($templateId, $n876);
        } else {
          $templateId = __GetElementUniqueID($n876);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("subSpuInfoList", $data.subSpuInfoList, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["CombSubSpuInfo"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["CombSubSpuInfo"].entry($n876, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
}
function $$update_8b290_877($parent, $data, $update2) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ($data.subCombinationInfoList != null && $data.subCombinationInfoList != undefined) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n878 = $update2 ? $lepusGetElementRefByLepusID("template", 878) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n878) {
          $templateCreated = false;
          $n878 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n878, 878, "template");
          __AppendElement($parent, $n878);
          $templateId = __GetElementUniqueID($n878);
          $childLepusTemplate = $templateConstructor($templateId, $n878);
        } else {
          $templateId = __GetElementUniqueID($n878);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("subCombinationInfoList", $data.subCombinationInfoList, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["SpuCombinationInfoList"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["SpuCombinationInfoList"].entry($n878, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, -1);
      $conditionNodeIndex[uniqueId] = -1;
    }
  }
}
function $$update_a91910_891($parent, $data, $update2) {
  let _a, _b, _c, _d, _e;
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if ((_a = $data.spurCardDescitionVo) == null ? undefined : _a.displayPointShow) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n892 = $update2 ? $lepusGetElementRefByLepusID("view", 892) : null;
        let $temp2 = $update2;
        if (!$n892) {
          $update2 = false;
          $n892 = __CreateView($currentComponentId);
          let $nid892 = $lepusStoreElementRefByLepusID($n892, 892, "view");
          __SetAttribute($n892, 1004, $nid892[1]);
          __AppendElement($parent, $n892);
        }
        if (!$update2 || $varUpdateState[3]) {
          {
            let $value = "margin-bottom:" + ((((_b = $data.spurCardDescitionVo) == null ? undefined : _b.mustShow) ? "0px" : "-6px") + ";") + ("max-width:" + ((((_c = $data.spurCardDescitionVo) == null ? undefined : _c.mustShow) ? "100%" : "calc(100% - 56px)") + ";")) + "display:flex;flex-wrap:wrap;align-items:flex-end;height:14px;margin-top:4px;overflow-y:hidden;";
            if (!$update2 || $value !== undefined) {
              __SetStyleObject($n892, [3, 103, 79, 52, 2, 278, {
                41: ((_d = $data.spurCardDescitionVo) == null ? undefined : _d.mustShow) ? "0px" : "-6px"
              }, {
                28: ((_e = $data.spurCardDescitionVo) == null ? undefined : _e.mustShow) ? "100%" : "calc(100% - 56px)"
              }]);
            }
          }
        }
        if (!$update2 || $varUpdateState[3]) {
          let $n893 = $update2 ? $lepusGetElementRefByLepusID("for", 893) : null;
          if (!$n893) {
            $n893 = __CreateFor($currentComponentId);
            $lepusStoreElementRefByLepusID($n893, 893, "for");
            __AppendElement($n892, $n893);
          }
          $$update_3d4f50_893($n893, $data, $update2);
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
$$update_a91910_894 = function ($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.product_point_type == 3001) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      {
        let $n895 = $update2 ? $lepusGetElementRefByLepusID("template", 895) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n895) {
          $templateCreated = false;
          $n895 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n895, 895, "template");
          __AppendElement($parent, $n895);
          $templateId = __GetElementUniqueID($n895);
          $childLepusTemplate = $templateConstructor($templateId, $n895);
        } else {
          $templateId = __GetElementUniqueID($n895);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("vo", item, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["RecommendDishTag"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["RecommendDishTag"].entry($n895, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp126 = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n896 = $update2 ? $lepusGetElementRefByLepusID("if", 896) : null;
      if (!$n896) {
        $update2 = false;
        $n896 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n896, 896, "if");
        __AppendElement($parent, $n896);
      }
      $$update_a91910_896($n896, $data, $update2, index, item);
      $update2 = _$temp126;
    }
  }
};
$$update_a91910_896 = function ($parent, $data, $update2, index, item) {
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.product_point_type == 6001) {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let $temp = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      {
        let $n897 = $update2 ? $lepusGetElementRefByLepusID("template", 897) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n897) {
          $templateCreated = false;
          $n897 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n897, 897, "template");
          __AppendElement($parent, $n897);
          $templateId = __GetElementUniqueID($n897);
          $childLepusTemplate = $templateConstructor($templateId, $n897);
        } else {
          $templateId = __GetElementUniqueID($n897);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("vo", item, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["ImpressionTag"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["ImpressionTag"].entry($n897, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 3);
      $conditionNodeIndex[uniqueId] = 3;
      let _$temp127 = $update2;
      if ($ifNodeIndex !== 3) {
        $update2 = false;
      }
      let $n898 = $update2 ? $lepusGetElementRefByLepusID("if", 898) : null;
      if (!$n898) {
        $update2 = false;
        $n898 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n898, 898, "if");
        __AppendElement($parent, $n898);
      }
      $$update_a91910_898($n898, $data, $update2, index, item);
      $update2 = _$temp127;
    }
  }
};
$$update_a91910_898 = function ($parent, $data, $update2, index, item) {
  let _a, _b;
  {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (item.product_point_type == 8004) {
      __UpdateIfNodeIndex($parent, 4);
      $conditionNodeIndex[uniqueId] = 4;
      let $temp = $update2;
      if ($ifNodeIndex !== 4) {
        $update2 = false;
      }
      {
        let $n899 = $update2 ? $lepusGetElementRefByLepusID("template", 899) : null;
        let $templateCreated = true;
        let $childLepusTemplate = null;
        let $templateId = null;
        if (!$n899) {
          $templateCreated = false;
          $n899 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n899, 899, "template");
          __AppendElement($parent, $n899);
          $templateId = __GetElementUniqueID($n899);
          $childLepusTemplate = $templateConstructor($templateId, $n899);
        } else {
          $templateId = __GetElementUniqueID($n899);
          $childLepusTemplate = $templateInfo[$templateId];
        }
        $updatePropsSet = [];
        $childLepusTemplate.setData("vo", item, $update2);
        $childLepusTemplate.setData("mustShow", (_a = $data.spurCardDescitionVo) == null ? undefined : _a.mustShow, $update2);
        $childLepusTemplate.setData("userName", (_b = $data.spurCardDescitionVo) == null ? undefined : _b.userRecommendUserName, $update2);
        if ($templateCreated) {
          let $update_keys = $updatePropsSet;
          if ($update_keys.length > 0) {
            $renderTemplates["UserRecommend"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
          }
        } else {
          $renderTemplates["UserRecommend"].entry($n899, $childLepusTemplate, $childLepusTemplate.data, false);
        }
      }
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 5);
      $conditionNodeIndex[uniqueId] = 5;
      let _$temp128 = $update2;
      if ($ifNodeIndex !== 5) {
        $update2 = false;
      }
      {
        let $n900 = $update2 ? $lepusGetElementRefByLepusID("template", 900) : null;
        let _$templateCreated8 = true;
        let _$childLepusTemplate8 = null;
        let _$templateId8 = null;
        if (!$n900) {
          _$templateCreated8 = false;
          $n900 = __CreateWrapperElement($currentComponentId);
          $lepusStoreElementRefByLepusID($n900, 900, "template");
          __AppendElement($parent, $n900);
          _$templateId8 = __GetElementUniqueID($n900);
          _$childLepusTemplate8 = $templateConstructor(_$templateId8, $n900);
        } else {
          _$templateId8 = __GetElementUniqueID($n900);
          _$childLepusTemplate8 = $templateInfo[_$templateId8];
        }
        $updatePropsSet = [];
        _$childLepusTemplate8.setData("vo", item, $update2);
        if (_$templateCreated8) {
          let _$update_keys8 = $updatePropsSet;
          if (_$update_keys8.length > 0) {
            $renderTemplates["ProductPointTag"].update(_$childLepusTemplate8, _$childLepusTemplate8.data, _$update_keys8, true);
          }
        } else {
          $renderTemplates["ProductPointTag"].entry($n900, _$childLepusTemplate8, _$childLepusTemplate8.data, false);
        }
      }
      $update2 = _$temp128;
    }
  }
};
function $$update_35d3c90_834($parent, $data, $update2) {
  let _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l;
  if (!$update2 || $varUpdateState[0] || $varUpdateState[2] || $varUpdateState[3] || $varUpdateState[4] || $varUpdateState[5] || $varUpdateState[6] || $varUpdateState[25] || $varUpdateState[7] || $varUpdateState[8] || $varUpdateState[9] || $varUpdateState[10] || $varUpdateState[11] || $varUpdateState[12] || $varUpdateState[13] || $varUpdateState[14] || $varUpdateState[15] || $varUpdateState[16] || $varUpdateState[17] || $varUpdateState[18] || $varUpdateState[19] || $varUpdateState[20] || $varUpdateState[21] || $varUpdateState[22] || $varUpdateState[23] || $varUpdateState[24]) {
    {
      let uniqueId = __GetElementUniqueID($parent);
      if (!$update2) {
        $conditionNodeIndex[uniqueId] = -1;
      }
      let $ifNodeIndex = $conditionNodeIndex[uniqueId];
      if ($data.isVoucherShelf) {
        __UpdateIfNodeIndex($parent, 0);
        $conditionNodeIndex[uniqueId] = 0;
        let $temp = $update2;
        if ($ifNodeIndex !== 0) {
          $update2 = false;
        }
        {
          let $n835 = $update2 ? $lepusGetElementRefByLepusID("view", 835) : null;
          let $temp2 = $update2;
          if (!$n835) {
            $update2 = false;
            $n835 = __CreateView($currentComponentId);
            let $nid835 = $lepusStoreElementRefByLepusID($n835, 835, "view");
            __SetAttribute($n835, 1004, $nid835[1]);
            __SetStyleObject($n835, [258, 92, 101, 31]);
            __AppendElement($parent, $n835);
          }
          {
            let $n836 = $update2 ? $lepusGetElementRefByLepusID("view", 836) : null;
            let $temp3 = $update2;
            if (!$n836) {
              $update2 = false;
              $n836 = __CreateView($currentComponentId);
              let $nid836 = $lepusStoreElementRefByLepusID($n836, 836, "view");
              __SetAttribute($n836, 1004, $nid836[1]);
              __SetStyleObject($n836, [259, 260, 261, 210, 262]);
              __AddEvent($n836, "catchEvent", "tap", "handleCardClick");
              __AppendElement($n835, $n836);
            }
            {
              let $n837 = $update2 ? $lepusGetElementRefByLepusID("image", 837) : null;
              let $temp4 = $update2;
              if (!$n837) {
                $update2 = false;
                $n837 = __CreateImage($currentComponentId);
                let $nid837 = $lepusStoreElementRefByLepusID($n837, 837, "image");
                __SetAttribute($n837, 1004, $nid837[1]);
                __SetStyleObject($n837, [55, 75, 76, 0, 1]);
                __SetAttribute($n837, "src", "https://test.com/obj/eden-cn/lm_pvw_lmps/ljhwZthlaukjlkulzlp/poi_shelf/voucher-bg.png");
                __SetAttribute($n837, "mode", "heightFix");
                __AppendElement($n836, $n837);
              }
              $update2 = $temp4;
            }
            {
              let $n838 = $update2 ? $lepusGetElementRefByLepusID("view", 838) : null;
              let _$temp129 = $update2;
              if (!$n838) {
                $update2 = false;
                $n838 = __CreateView($currentComponentId);
                let $nid838 = $lepusStoreElementRefByLepusID($n838, 838, "view");
                __SetAttribute($n838, 1004, $nid838[1]);
                __SetStyleObject($n838, [3, 44]);
                __AppendElement($n836, $n838);
              }
              {
                let $n839 = $update2 ? $lepusGetElementRefByLepusID("view", 839) : null;
                let $temp5 = $update2;
                if (!$n839) {
                  $update2 = false;
                  $n839 = __CreateView($currentComponentId);
                  let $nid839 = $lepusStoreElementRefByLepusID($n839, 839, "view");
                  __SetAttribute($n839, 1004, $nid839[1]);
                  __SetStyleObject($n839, [14]);
                  __AppendElement($n838, $n839);
                }
                {
                  let $n840 = $update2 ? $lepusGetElementRefByLepusID("template", 840) : null;
                  let $templateCreated = true;
                  let $childLepusTemplate = null;
                  let $templateId = null;
                  if (!$n840) {
                    $templateCreated = false;
                    $n840 = __CreateWrapperElement($currentComponentId);
                    $lepusStoreElementRefByLepusID($n840, 840, "template");
                    __AppendElement($n839, $n840);
                    $templateId = __GetElementUniqueID($n840);
                    $childLepusTemplate = $templateConstructor($templateId, $n840);
                  } else {
                    $templateId = __GetElementUniqueID($n840);
                    $childLepusTemplate = $templateInfo[$templateId];
                  }
                  $updatePropsSet = [];
                  $childLepusTemplate.setData("spuCardTitleVO", $data.spuCardTitleVO, $update2);
                  if ($templateCreated) {
                    let $update_keys = $updatePropsSet;
                    if ($update_keys.length > 0) {
                      $renderTemplates["SpuTitle"].update($childLepusTemplate, $childLepusTemplate.data, $update_keys, true);
                    }
                  } else {
                    $renderTemplates["SpuTitle"].entry($n840, $childLepusTemplate, $childLepusTemplate.data, false);
                  }
                }
                {
                  let $template_update = $update2;
                  let $n841 = $update2 ? $lepusGetElementRefByLepusID("if", 841) : null;
                  if (!$n841) {
                    $update2 = false;
                    $n841 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n841, 841, "if");
                    __AppendElement($n839, $n841);
                  }
                  $$update_3d4f50_841($n841, $data, $update2);
                  $update2 = $template_update;
                }
                $update2 = $temp5;
              }
              {
                let $n852 = $update2 ? $lepusGetElementRefByLepusID("view", 852) : null;
                let _$temp130 = $update2;
                if (!$n852) {
                  $update2 = false;
                  $n852 = __CreateView($currentComponentId);
                  let $nid852 = $lepusStoreElementRefByLepusID($n852, 852, "view");
                  __SetAttribute($n852, 1004, $nid852[1]);
                  __SetStyleObject($n852, [268, 269]);
                  __AppendElement($n838, $n852);
                }
                {
                  let _$template_update59 = $update2;
                  let $n853 = $update2 ? $lepusGetElementRefByLepusID("if", 853) : null;
                  if (!$n853) {
                    $update2 = false;
                    $n853 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n853, 853, "if");
                    __AppendElement($n852, $n853);
                  }
                  $$update_3d4f50_853($n853, $data, $update2);
                  $update2 = _$template_update59;
                }
                $update2 = _$temp130;
              }
              $update2 = _$temp129;
            }
            {
              let $n856 = $update2 ? $lepusGetElementRefByLepusID("view", 856) : null;
              let _$temp131 = $update2;
              if (!$n856) {
                $update2 = false;
                $n856 = __CreateView($currentComponentId);
                let $nid856 = $lepusStoreElementRefByLepusID($n856, 856, "view");
                __SetAttribute($n856, 1004, $nid856[1]);
                __SetStyleObject($n856, [100, 14, 91]);
                __AppendElement($n836, $n856);
              }
              {
                let _$template_update60 = $update2;
                let $n857 = $update2 ? $lepusGetElementRefByLepusID("if", 857) : null;
                if (!$n857) {
                  $update2 = false;
                  $n857 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n857, 857, "if");
                  __AppendElement($n856, $n857);
                }
                $$update_7a5080_857($n857, $data, $update2);
                $update2 = _$template_update60;
              }
              $update2 = _$temp131;
            }
            $update2 = $temp3;
          }
          $update2 = $temp2;
        }
        $update2 = $temp;
      } else {
        __UpdateIfNodeIndex($parent, 1);
        $conditionNodeIndex[uniqueId] = 1;
        let _$temp132 = $update2;
        if ($ifNodeIndex !== 1) {
          $update2 = false;
        }
        {
          let $n863 = $update2 ? $lepusGetElementRefByLepusID("view", 863) : null;
          let _$temp133 = $update2;
          if (!$n863) {
            $update2 = false;
            $n863 = __CreateView($currentComponentId);
            let $nid863 = $lepusStoreElementRefByLepusID($n863, 863, "view");
            __SetAttribute($n863, 1004, $nid863[1]);
            __SetStyleObject($n863, [270, 271, 101, 31]);
            __AppendElement($parent, $n863);
          }
          {
            let $n864 = $update2 ? $lepusGetElementRefByLepusID("view", 864) : null;
            let _$temp134 = $update2;
            if (!$n864) {
              $update2 = false;
              $n864 = __CreateView($currentComponentId);
              let $nid864 = $lepusStoreElementRefByLepusID($n864, 864, "view");
              __SetAttribute($n864, 1004, $nid864[1]);
              __SetStyleObject($n864, [3, 44, 0]);
              __AddEvent($n864, "bindEvent", "tap", "handleCardClick");
              __AppendElement($n863, $n864);
            }
            {
              let $n865 = $update2 ? $lepusGetElementRefByLepusID("view", 865) : null;
              let _$temp135 = $update2;
              if (!$n865) {
                $update2 = false;
                $n865 = __CreateView($currentComponentId);
                let $nid865 = $lepusStoreElementRefByLepusID($n865, 865, "view");
                __SetAttribute($n865, 1004, $nid865[1]);
                __AddEvent($n865, "catchEvent", "tap", "handleSpuCoverClick");
                __AppendElement($n864, $n865);
              }
              if (!$update2 || $varUpdateState[17]) {
                {
                  let $value = "width:" + (($data.hasElementListInSpu ? "103px" : "84px") + ";") + ("height:" + (($data.hasElementListInSpu ? "103px" : "84px") + ";")) + "position:relative;flex-shrink:0;";
                  if (!$update2 || $value !== "width:" + (($cardInstance._data.hasElementListInSpu ? "103px" : "84px") + ";") + ("height:" + (($cardInstance._data.hasElementListInSpu ? "103px" : "84px") + ";")) + "position:relative;flex-shrink:0;") {
                    __SetStyleObject($n865, [74, 31, {
                      27: $data.hasElementListInSpu ? "103px" : "84px"
                    }, {
                      26: $data.hasElementListInSpu ? "103px" : "84px"
                    }]);
                  }
                }
              }
              {
                let _$template_update61 = $update2;
                let $n866 = $update2 ? $lepusGetElementRefByLepusID("if", 866) : null;
                if (!$n866) {
                  $update2 = false;
                  $n866 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n866, 866, "if");
                  __AppendElement($n865, $n866);
                }
                $$update_1f7b920_866($n866, $data, $update2);
                $update2 = _$template_update61;
              }
              {
                let _$template_update62 = $update2;
                let $n869 = $update2 ? $lepusGetElementRefByLepusID("if", 869) : null;
                if (!$n869) {
                  $update2 = false;
                  $n869 = __CreateIf($currentComponentId);
                  $lepusStoreElementRefByLepusID($n869, 869, "if");
                  __AppendElement($n865, $n869);
                }
                $$update_3d2b6c8_869($n869, $data, $update2);
                $update2 = _$template_update62;
              }
              $update2 = _$temp135;
            }
            {
              let $n871 = $update2 ? $lepusGetElementRefByLepusID("template", 871) : null;
              let _$templateCreated9 = true;
              let _$childLepusTemplate9 = null;
              let _$templateId9 = null;
              if (!$n871) {
                _$templateCreated9 = false;
                $n871 = __CreateWrapperElement($currentComponentId);
                $lepusStoreElementRefByLepusID($n871, 871, "template");
                __AppendElement($n864, $n871);
                _$templateId9 = __GetElementUniqueID($n871);
                _$childLepusTemplate9 = $templateConstructor(_$templateId9, $n871);
              } else {
                _$templateId9 = __GetElementUniqueID($n871);
                _$childLepusTemplate9 = $templateInfo[_$templateId9];
              }
              $updatePropsSet = [];
              _$childLepusTemplate9.setData("spuCardImageTagVO", $data.spuCardImageTagVO, $update2);
              if (_$templateCreated9) {
                let _$update_keys9 = $updatePropsSet;
                if (_$update_keys9.length > 0) {
                  $renderTemplates["ImageTag"].update(_$childLepusTemplate9, _$childLepusTemplate9.data, _$update_keys9, true);
                }
              } else {
                $renderTemplates["ImageTag"].entry($n871, _$childLepusTemplate9, _$childLepusTemplate9.data, false);
              }
            }
            {
              let $n872 = $update2 ? $lepusGetElementRefByLepusID("view", 872) : null;
              let _$temp136 = $update2;
              if (!$n872) {
                $update2 = false;
                $n872 = __CreateView($currentComponentId);
                let $nid872 = $lepusStoreElementRefByLepusID($n872, 872, "view");
                __SetAttribute($n872, 1004, $nid872[1]);
                __SetStyleObject($n872, [100, 87, 275, 0, 102]);
                __AppendElement($n864, $n872);
              }
              {
                let $n873 = $update2 ? $lepusGetElementRefByLepusID("view", 873) : null;
                let _$temp137 = $update2;
                if (!$n873) {
                  $update2 = false;
                  $n873 = __CreateView($currentComponentId);
                  let $nid873 = $lepusStoreElementRefByLepusID($n873, 873, "view");
                  __SetAttribute($n873, 1004, $nid873[1]);
                  __SetStyleObject($n873, [3, 31, 74, 100, 0]);
                  __AppendElement($n872, $n873);
                }
                {
                  let $n874 = $update2 ? $lepusGetElementRefByLepusID("template", 874) : null;
                  let _$templateCreated10 = true;
                  let _$childLepusTemplate10 = null;
                  let _$templateId10 = null;
                  if (!$n874) {
                    _$templateCreated10 = false;
                    $n874 = __CreateWrapperElement($currentComponentId);
                    $lepusStoreElementRefByLepusID($n874, 874, "template");
                    __AppendElement($n873, $n874);
                    _$templateId10 = __GetElementUniqueID($n874);
                    _$childLepusTemplate10 = $templateConstructor(_$templateId10, $n874);
                  } else {
                    _$templateId10 = __GetElementUniqueID($n874);
                    _$childLepusTemplate10 = $templateInfo[_$templateId10];
                  }
                  $updatePropsSet = [];
                  _$childLepusTemplate10.setData("spuCardTitleVO", $data.spuCardTitleVO, $update2);
                  if (_$templateCreated10) {
                    let _$update_keys10 = $updatePropsSet;
                    if (_$update_keys10.length > 0) {
                      $renderTemplates["SpuTitle"].update(_$childLepusTemplate10, _$childLepusTemplate10.data, _$update_keys10, true);
                    }
                  } else {
                    $renderTemplates["SpuTitle"].entry($n874, _$childLepusTemplate10, _$childLepusTemplate10.data, false);
                  }
                }
                {
                  let _$template_update63 = $update2;
                  let $n875 = $update2 ? $lepusGetElementRefByLepusID("if", 875) : null;
                  if (!$n875) {
                    $update2 = false;
                    $n875 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n875, 875, "if");
                    __AppendElement($n873, $n875);
                  }
                  $$update_34b0098_875($n875, $data, $update2);
                  $update2 = _$template_update63;
                }
                {
                  let _$template_update64 = $update2;
                  let $n877 = $update2 ? $lepusGetElementRefByLepusID("if", 877) : null;
                  if (!$n877) {
                    $update2 = false;
                    $n877 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n877, 877, "if");
                    __AppendElement($n873, $n877);
                  }
                  $$update_8b290_877($n877, $data, $update2);
                  $update2 = _$template_update64;
                }
                {
                  let $n879 = $update2 ? $lepusGetElementRefByLepusID("view", 879) : null;
                  let $temp6 = $update2;
                  if (!$n879) {
                    $update2 = false;
                    $n879 = __CreateView($currentComponentId);
                    let $nid879 = $lepusStoreElementRefByLepusID($n879, 879, "view");
                    __SetAttribute($n879, 1004, $nid879[1]);
                    __AppendElement($n873, $n879);
                  }
                  if (!$update2 || $varUpdateState[3]) {
                    {
                      let _$value202 = "margin-bottom:" + ((((_a = $data.spurCardDescitionVo) == null ? undefined : _a.hideWrapperBottom) ? ((_b = $data.spurCardDescitionVo) == null ? undefined : _b.mustShow) ? "4px" : "0px" : "6px") + ";") + "justify-content:space-between;flex-direction:column;padding-top:3px;";
                      if (!$update2 || _$value202 !== "margin-bottom:" + ((((_c = $cardInstance._data.spurCardDescitionVo) == null ? undefined : _c.hideWrapperBottom) ? ((_d = $cardInstance._data.spurCardDescitionVo) == null ? undefined : _d.mustShow) ? "4px" : "0px" : "6px") + ";") + "justify-content:space-between;flex-direction:column;padding-top:3px;") {
                        __SetStyleObject($n879, [87, 100, 276, {
                          41: ((_e = $data.spurCardDescitionVo) == null ? undefined : _e.hideWrapperBottom) ? ((_f = $data.spurCardDescitionVo) == null ? undefined : _f.mustShow) ? "4px" : "0px" : "6px"
                        }]);
                      }
                    }
                  }
                  {
                    let $n880 = $update2 ? $lepusGetElementRefByLepusID("view", 880) : null;
                    let $temp7 = $update2;
                    if (!$n880) {
                      $update2 = false;
                      $n880 = __CreateView($currentComponentId);
                      let $nid880 = $lepusStoreElementRefByLepusID($n880, 880, "view");
                      __SetAttribute($n880, 1004, $nid880[1]);
                      __AppendElement($n879, $n880);
                    }
                    if (!$update2 || $varUpdateState[3]) {
                      {
                        let _$value203 = "margin-top:" + ((((_g = $data.spurCardDescitionVo) == null ? undefined : _g.titleShowAll) && ((_h = $data.spurCardDescitionVo) == null ? undefined : _h.isTitleScaleLimit) ? "1px" : "2px") + ";") + "flex-shrink:0;flex-direction:column;";
                        if (!$update2 || _$value203 !== "margin-top:" + ((((_i = $cardInstance._data.spurCardDescitionVo) == null ? undefined : _i.titleShowAll) && ((_j = $cardInstance._data.spurCardDescitionVo) == null ? undefined : _j.isTitleScaleLimit) ? "1px" : "2px") + ";") + "flex-shrink:0;flex-direction:column;") {
                          __SetStyleObject($n880, [31, 100, {
                            40: ((_k = $data.spurCardDescitionVo) == null ? undefined : _k.titleShowAll) && ((_l = $data.spurCardDescitionVo) == null ? undefined : _l.isTitleScaleLimit) ? "1px" : "2px"
                          }]);
                        }
                      }
                    }
                    {
                      let _$template_update65 = $update2;
                      let $n881 = $update2 ? $lepusGetElementRefByLepusID("if", 881) : null;
                      if (!$n881) {
                        $update2 = false;
                        $n881 = __CreateIf($currentComponentId);
                        $lepusStoreElementRefByLepusID($n881, 881, "if");
                        __AppendElement($n880, $n881);
                      }
                      $$update_3d4f50_881($n881, $data, $update2);
                      $update2 = _$template_update65;
                    }
                    {
                      let $n883 = $update2 ? $lepusGetElementRefByLepusID("view", 883) : null;
                      let $temp8 = $update2;
                      if (!$n883) {
                        $update2 = false;
                        $n883 = __CreateView($currentComponentId);
                        let $nid883 = $lepusStoreElementRefByLepusID($n883, 883, "view");
                        __SetAttribute($n883, 1004, $nid883[1]);
                        __SetStyleObject($n883, [3, 87, 4, 277]);
                        __AppendElement($n880, $n883);
                      }
                      {
                        let $n884 = $update2 ? $lepusGetElementRefByLepusID("view", 884) : null;
                        let $temp9 = $update2;
                        if (!$n884) {
                          $update2 = false;
                          $n884 = __CreateView($currentComponentId);
                          let $nid884 = $lepusStoreElementRefByLepusID($n884, 884, "view");
                          __SetAttribute($n884, 1004, $nid884[1]);
                          __SetStyleObject($n884, [3, 100]);
                          __SetAttribute($n884, "implicit-animation", false);
                          __AppendElement($n883, $n884);
                        }
                        {
                          let $n885 = $update2 ? $lepusGetElementRefByLepusID("view", 885) : null;
                          let $temp10 = $update2;
                          if (!$n885) {
                            $update2 = false;
                            $n885 = __CreateView($currentComponentId);
                            let $nid885 = $lepusStoreElementRefByLepusID($n885, 885, "view");
                            __SetAttribute($n885, 1004, $nid885[1]);
                            __SetStyleObject($n885, [3, 44, 103, 45, 6]);
                            __SetAttribute($n885, "implicit-animation", false);
                            __AppendElement($n884, $n885);
                          }
                          if (!$update2 || $varUpdateState[3]) {
                            let $n886 = $update2 ? $lepusGetElementRefByLepusID("for", 886) : null;
                            if (!$n886) {
                              $n886 = __CreateFor($currentComponentId);
                              $lepusStoreElementRefByLepusID($n886, 886, "for");
                              __AppendElement($n885, $n886);
                            }
                            $$update_3d4f50_886($n886, $data, $update2);
                          }
                          $update2 = $temp10;
                        }
                        $update2 = $temp9;
                      }
                      {
                        let _$template_update66 = $update2;
                        let $n888 = $update2 ? $lepusGetElementRefByLepusID("if", 888) : null;
                        if (!$n888) {
                          $update2 = false;
                          $n888 = __CreateIf($currentComponentId);
                          $lepusStoreElementRefByLepusID($n888, 888, "if");
                          __AppendElement($n883, $n888);
                        }
                        $$update_3d4f50_888($n888, $data, $update2);
                        $update2 = _$template_update66;
                      }
                      $update2 = $temp8;
                    }
                    {
                      let _$template_update67 = $update2;
                      let $n891 = $update2 ? $lepusGetElementRefByLepusID("if", 891) : null;
                      if (!$n891) {
                        $update2 = false;
                        $n891 = __CreateIf($currentComponentId);
                        $lepusStoreElementRefByLepusID($n891, 891, "if");
                        __AppendElement($n880, $n891);
                      }
                      $$update_a91910_891($n891, $data, $update2);
                      $update2 = _$template_update67;
                    }
                    $update2 = $temp7;
                  }
                  $update2 = $temp6;
                }
                $update2 = _$temp137;
              }
              {
                let $n901 = $update2 ? $lepusGetElementRefByLepusID("view", 901) : null;
                let _$temp138 = $update2;
                if (!$n901) {
                  $update2 = false;
                  $n901 = __CreateView($currentComponentId);
                  let $nid901 = $lepusStoreElementRefByLepusID($n901, 901, "view");
                  __SetAttribute($n901, 1004, $nid901[1]);
                  __SetStyleObject($n901, [44, 102]);
                  __AppendElement($n872, $n901);
                }
                {
                  let _$template_update68 = $update2;
                  let $n902 = $update2 ? $lepusGetElementRefByLepusID("if", 902) : null;
                  if (!$n902) {
                    $update2 = false;
                    $n902 = __CreateIf($currentComponentId);
                    $lepusStoreElementRefByLepusID($n902, 902, "if");
                    __AppendElement($n901, $n902);
                  }
                  $$update_7a5080_902($n902, $data, $update2);
                  $update2 = _$template_update68;
                }
                $update2 = _$temp138;
              }
              $update2 = _$temp136;
            }
            $update2 = _$temp134;
          }
          $update2 = _$temp133;
        }
        $update2 = _$temp132;
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
  if ($varUpdateState[0] || $varUpdateState[1]) {
    {
      let $value = $data.isVoucherShelf ? "shelf_voucher_card_exposure_event-" + $data.spu_id : "shelf_spu_card_exposure_event-" + $data.spu_id;
      if (!$update || $value !== ($cardInstance._data.isVoucherShelf ? "shelf_voucher_card_exposure_event-" + $cardInstance._data.spu_id : "shelf_spu_card_exposure_event-" + $cardInstance._data.spu_id)) {
        __SetAttribute($page, "exposure-id", $value);
      }
    }
    {
      let _$value204 = $data.isVoucherShelf ? "shelf_voucher_card_exposure_event" : "shelf_spu_card_exposure_event";
      if (!$update || _$value204 !== ($cardInstance._data.isVoucherShelf ? "shelf_voucher_card_exposure_event" : "shelf_spu_card_exposure_event")) {
        __SetAttribute($page, "exposure-scene", _$value204);
      }
    }
  }
  let $n834 = $lepusGetElementRefByLepusID("if", 834);
  $$update_35d3c90_834($n834, $data, $update);
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
  __SetAttribute($page, "exposure-id", $data.isVoucherShelf ? "shelf_voucher_card_exposure_event-" + $data.spu_id : "shelf_spu_card_exposure_event-" + $data.spu_id);
  __SetAttribute($page, "exposure-scene", $data.isVoucherShelf ? "shelf_voucher_card_exposure_event" : "shelf_spu_card_exposure_event");
  let $n834 = __CreateIf($currentComponentId);
  $lepusStoreElementRefByLepusID($n834, 834, "if");
  __AppendElement($page, $n834);
  $$update_35d3c90_834($n834, $data, $update);
  $airFirstScreen = false;
  $cardVariables = ["isVoucherShelf", "spu_id", "spuCardTitleVO", "spurCardDescitionVo", "isDeliveryModule", "useMarketing", "deliveryPurchaseInfoVO", "marketingV2VO", "priceInfoState", "tagCountdownState", "tagMoreIconState", "tagAnimateProgressState", "tagSecKillCountDownState", "marketingInfoVO", "seckill", "isCouponOverflow", "isSiblingOverflow", "hasElementListInSpu", "spuCardCoverUrl", "ditoContext", "showMarketingSpuCoverAtmosphere", "spuCardCoverAtmosphereUrl", "spuCardImageTagVO", "subSpuInfoList", "subCombinationInfoList"];
  return true;
};
//# sourceMappingURL=http://10.91.108.134:8787/shelf_product_card/intermediate/lepus.js.map
