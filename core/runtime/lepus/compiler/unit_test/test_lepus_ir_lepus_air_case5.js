// [TEST_TARGET: IR]
// Goal: build a complex, template-like render/update workload to stress IR
// optimization passes. Coverage targets:
// 1) Multi-level for/if CFG
// 2) Toplevel vars + closure capture + write-back
// 3) number/string key alias reads/writes
// 4) Pseudo template node cache, conditional node switching, for-fiber state
// 5) Both render and update paths

// NOTE: Lepus compiler currently aborts on `throw` (even if unreachable).
// Keep assertions non-throwing to avoid masking IR optimization issues.
let __assert_fail_count = 0;
function Assert(v, msg) {
  if (!v) {
    __assert_fail_count = __assert_fail_count + 1;
  }
}

function print(a) {}
let console = {
  log: function () {}
};

let __nextElementId = 1;
let __elementMap = {};

function __CreateElement(tag, componentId) {
  let node = {
    _id: __nextElementId++,
    _tag: tag,
    _componentId: componentId,
    children: [],
    attrs: {},
    styles: [],
    text: ""
  };
  __elementMap[node._id] = node;
  return node;
}
function __CreateView(a) {
  return __CreateElement("view", a);
}
function __CreateText(a) {
  return __CreateElement("text", a);
}
function __CreateImage(a) {
  return __CreateElement("image", a);
}
function __CreateIf(a) {
  return __CreateElement("if", a);
}
function __CreateFor(a) {
  return __CreateElement("for", a);
}
function __CreatePage(a, b) {
  let page = __CreateElement("page", a);
  page.attrs.path = b;
  return page;
}
function __GetElementUniqueID(a) {
  return a ? a._id : 0;
}
function __GetElementByUniqueID(a) {
  return __elementMap[a] || null;
}
function __SetAttribute(a, b, c) {
  if (!a) {
    return;
  }
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
function __SetStyleObject(a, b) {
  if (a) {
    a.styles = b;
  }
}
function __UpdateIfNodeIndex(a, b) {}
function __UpdateForChildCount(a, b) {
  if (a) {
    a.expectedChildCount = b;
  }
}
function __FlushElementTree(a) {}
function __GetDiffData(a, b, c) {
  return {
    oldData: a,
    nextData: b,
    options: c,
    changed: true
  };
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

let $currentComponentId = 10;
let $lepusElementLepusIdMap = {};
let $cardInstance = null;
let $page = null;
let $cardOptions = {
  data: {}
};
let $conditionNodeIndex = {};
let $varUpdateState = [];
let $renderTemplates = {};
let $airFirstScreen = false;
let $update = false;
let __globalProps = {
  os: "ios",
  screenWidth: 390,
  screenHeight: 844
};

let GLOBAL_COUNTER = 0;
let GLOBAL_SLOT_SERIAL = 0;
let GLOBAL_THEME = "light";
let GLOBAL_ENV_SNAPSHOT = {
  scale: 1,
  width: 390
};
let GLOBAL_CACHE = {};

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
  let templateElementId = templateElement ? templateElement._templateId : -1;
  let forElementId = forElement ? forElement._uniqueId : -1;
  let maxId = templateElementId > forElementId ? templateElementId : forElementId;
  if (maxId === -1) {
    return [key, uniqueKey];
  }
  if (maxId === templateElementId) {
    key = templateElementId;
    uniqueKey = templateElementId;
  } else {
    uniqueKey = $getLepusUniqId(forElementId, forElement.activeIndex);
  }
  if (forElementId > 0) {
    key = $getLepusUniqId(forElement._lepusId, forElement.activeIndex);
  }
  return [key, uniqueKey];
}

let $lepusGetElementRefByLepusID = function (tag, lepusId) {
  let pair = $getKeyForCreatedElement(lepusId);
  let uniqId = pair[1];
  let elementId = $lepusElementLepusIdMap[$getLepusHash(uniqId, lepusId)];
  if (elementId) {
    return __GetElementByUniqueID(elementId);
  }
  return null;
};

let $lepusStoreElementRefByLepusID = function (elementRef, lepusId, tag) {
  let pair = $getKeyForCreatedElement(lepusId);
  let lepusUniqueId = pair[0];
  let uniqId = pair[1];
  let uniqueId = __GetElementUniqueID(elementRef);
  $lepusElementLepusIdMap[$getLepusHash(uniqId, lepusId)] = uniqueId;
  return [uniqueId, lepusUniqueId];
};

function $cardConstructor(componentId) {
  $cardOptions = $cardOptions != null ? $cardOptions : {};
  $cardOptions.data = $cardOptions.data != null ? $cardOptions.data : {};
  $cardOptions._componentId = componentId;
  $cardOptions._uniqueId = componentId;
  $cardOptions._data = {};
  $cardOptions.forCache = {};
  $cardOptions._currentForElement = undefined;
  $cardOptions._currentTemplateElement = undefined;
  $cardOptions._currentComponentElement = undefined;
  $cardInstance = $cardOptions;
  return $cardInstance;
}

function $lepusPushFiberForNode(elementRef, lepusId, uniqueId) {
  let forElement = elementRef;
  if (forElement) {
    if (!forElement._uniqueId) {
      forElement = $cardInstance.forCache[uniqueId];
      if (!forElement) {
        forElement = {
          _lepusId: lepusId,
          _uniqueId: uniqueId,
          activeIndex: 0,
          _lastLength: 0
        };
        $cardInstance.forCache[uniqueId] = forElement;
      }
    }
    let lastForElement = $cardInstance._currentForElement;
    $cardInstance._currentForElement = forElement;
    return [forElement, lastForElement];
  }
  $cardInstance._currentForElement = undefined;
  return [undefined, undefined];
}

function $lepusUpdateFiberForNodeIndex(index) {
  $cardInstance._currentForElement.activeIndex = index;
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

function normalizeData(raw) {
  let data = $deepClone(raw || {});
  let env = data.env || (data.env = {});
  let sections = data.sections || (data.sections = []);
  let fontScale = parseFloat(env.font_scale == null ? "1" : env.font_scale + "");
  if (!(fontScale > 0)) {
    fontScale = 1;
  }
  GLOBAL_ENV_SNAPSHOT.scale = fontScale;
  GLOBAL_ENV_SNAPSHOT.width = __globalProps.screenWidth;
  GLOBAL_THEME = env.theme || "light";
  for (let sectionIndex = 0; sectionIndex < sections.length; sectionIndex++) {
    let section = sections[sectionIndex];
    section.meta = section.meta || {};
    section.cards = section.cards || [];
    section.layout = section.layout || (sectionIndex % 2 === 0 ? "grid" : "list");
    section.visible = section.visible !== false;
    section.slotSeed = section.slotSeed == null ? sectionIndex + 3 : section.slotSeed;
    section.meta["1"] = section.meta["1"] == null ? sectionIndex : section.meta["1"];
    section.meta[1] = section.meta["1"];
    for (let cardIndex = 0; cardIndex < section.cards.length; cardIndex++) {
      let card = section.cards[cardIndex];
      card.id = card.id || section.key + "_" + cardIndex;
      card.title = card.title || "untitled";
      card.subtitle = card.subtitle || "";
      card.badges = card.badges || [];
      card.dynamic = card.dynamic || {};
      card.metrics = card.metrics || {};
      card.media = card.media || {};
      card.media.cover = card.media.cover || {};
      card.media.cover.url_list = card.media.cover.url_list || ["mock://cover/" + card.id];
      card.dynamic["1"] = card.dynamic["1"] == null ? (card.score || 0) : card.dynamic["1"];
      card.dynamic[1] = card.dynamic["1"];
      card.metrics.width = ((card.width == null ? 96 : card.width) * fontScale) + "px";
      card.metrics.height = ((card.height == null ? 96 : card.height) * fontScale) + "px";
      card.displayTitle = card.title + (card.suffix ? " · " + card.suffix : "");
    }
  }
  return data;
}

function getAliasValue(obj, key1, key2) {
  let v1 = obj[key1];
  let v2 = obj[key2];
  if (v1 !== v2) {
    obj[key2] = v1;
    v2 = obj[key2];
  }
  return (v1 || 0) + (v2 || 0);
}

function buildClassTokens(section, card, cardIndex) {
  let tokens = ["product-card", section.layout === "grid" ? "layout-grid" : "layout-list"];
  if (cardIndex === 0) {
    tokens.push("is-first");
  }
  if (GLOBAL_THEME === "dark") {
    tokens.push("theme-dark");
  }
  if ((card.score || 0) > 80) {
    tokens.push("score-hot");
  }
  if (card.badges.length > 2) {
    tokens.push("badge-rich");
  }
  return tokens.join(" ");
}

function buildInlineStyle(section, card, cardIndex, slotIndex) {
  let parts = [];
  let dynamic = card.dynamic || {};
  let keys = Object.keys(dynamic);
  let localSeed = section.slotSeed + cardIndex + slotIndex;
  function touch(weight) {
    GLOBAL_COUNTER = GLOBAL_COUNTER + 1;
    localSeed = localSeed + weight + (GLOBAL_COUNTER & 1);
    return localSeed;
  }
  for (let i = 0; i < keys.length; i++) {
    let key = keys[i];
    let value = dynamic[key];
    if (key === "1") {
      dynamic[1] = value;
    } else if (key == 1) {
      dynamic["1"] = value;
    }
    if (i < 3) {
      parts.push(key + ":" + value);
    } else {
      parts.push(key + ":" + touch(i));
    }
  }
  parts.push("width:" + card.metrics.width);
  parts.push("height:" + card.metrics.height);
  parts.push("margin-left:" + (cardIndex === 0 ? 0 : 4) + "px");
  parts.push("opacity:" + (section.visible ? 1 : 0));
  return parts.join(";");
}

function shouldRenderCard(section, card, cardIndex) {
  let flags = ((section.meta || {}).flags || {});
  let show = section.visible !== false;
  if (flags.forceHide && (cardIndex & 1) === 1) {
    show = false;
  }
  if ((card.score || 0) < -1) {
    show = false;
  }
  if (card.dynamic[1] === card.dynamic["1"]) {
    show = show && true;
  }
  return show;
}

function makeBadgeRenderer(sectionKey, seed) {
  let closureSeed = seed;
  let state = {
    sum: seed,
    lastIndex: -1
  };
  return function renderBadge($parent, $data, $update2, cardIndex, card, badgeIndex, badge) {
    function hit(weight) {
      GLOBAL_COUNTER = GLOBAL_COUNTER + 1;
      GLOBAL_SLOT_SERIAL = GLOBAL_SLOT_SERIAL + weight;
      closureSeed = closureSeed + weight + (cardIndex & 1);
      state.sum = state.sum + closureSeed;
      state.lastIndex = badgeIndex;
      return closureSeed + state.sum + GLOBAL_COUNTER;
    }

    let gate = hit((badge.weight || 0) + 1);
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (badge.type === "text") {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      let $n401 = $update2 ? $lepusGetElementRefByLepusID("text", 401) : null;
      if (!$n401) {
        $update2 = false;
        $n401 = __CreateText($currentComponentId);
        let $nid401 = $lepusStoreElementRefByLepusID($n401, 401, "text");
        __SetAttribute($n401, 1004, $nid401[1]);
        __AppendElement($parent, $n401);
      }
      __SetStyleObject($n401, [{
        gate: gate
      }, {
        weight: badge.weight || 0
      }]);
      __SetAttribute($n401, "text", (badge.text || "badge") + "#" + gate);
      $update2 = $temp;
    } else if (badge.type === "icon") {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n402 = $update2 ? $lepusGetElementRefByLepusID("image", 402) : null;
      if (!$n402) {
        $update2 = false;
        $n402 = __CreateImage($currentComponentId);
        let $nid402 = $lepusStoreElementRefByLepusID($n402, 402, "image");
        __SetAttribute($n402, 1004, $nid402[1]);
        __AppendElement($parent, $n402);
      }
      __SetStyleObject($n402, ["badge-icon", {
        serial: GLOBAL_SLOT_SERIAL
      }]);
      __SetAttribute($n402, "src", badge.src || "mock://icon/" + sectionKey + "/" + badgeIndex);
      $update2 = _$temp;
    } else {
      __UpdateIfNodeIndex($parent, 2);
      $conditionNodeIndex[uniqueId] = 2;
      let _$temp2 = $update2;
      if ($ifNodeIndex !== 2) {
        $update2 = false;
      }
      let $n403 = $update2 ? $lepusGetElementRefByLepusID("view", 403) : null;
      if (!$n403) {
        $update2 = false;
        $n403 = __CreateView($currentComponentId);
        let $nid403 = $lepusStoreElementRefByLepusID($n403, 403, "view");
        __SetAttribute($n403, 1004, $nid403[1]);
        __AppendElement($parent, $n403);
      }
      __SetAttribute($n403, "data-fallback", badge.type || "view");
      __SetStyleObject($n403, ["badge-fallback", {
        gate: gate + state.lastIndex
      }]);
      $update2 = _$temp2;
    }
    GLOBAL_CACHE[sectionKey + ":" + card.id + ":" + badgeIndex] = gate;
  };
}

function renderBadgeList($parent, $data, $update2, sectionIndex, section, cardIndex, card) {
  if (!$update2 || $varUpdateState[sectionIndex]) {
    let uniqueId = __GetElementUniqueID($parent);
    let pair = $lepusPushFiberForNode($parent, 320, uniqueId);
    let $forLepus = pair[0];
    let $lastForLepus = pair[1];
    let $object = card.badges;
    let $length = $object.length;
    __UpdateForChildCount($parent, $length);
    let $temp = $update2;
    let renderBadge = makeBadgeRenderer(section.key, section.slotSeed + cardIndex + 1);
    for (let badgeIndex = 0; badgeIndex < $length; ++badgeIndex) {
      $update2 = badgeIndex < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(badgeIndex);
      let badge = $object[badgeIndex];
      let $n321 = $update2 ? $lepusGetElementRefByLepusID("if", 321) : null;
      if (!$n321) {
        $n321 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n321, 321, "if");
        __AppendElement($parent, $n321);
      }
      renderBadge($n321, $data, $update2, cardIndex, card, badgeIndex, badge);
    }
    $forLepus._lastLength = $length;
    $update2 = $temp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
  }
}

$renderTemplates.Card = {
  entry: function ($parent, $data, $update2, sectionIndex, section, cardIndex, card) {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    if (shouldRenderCard(section, card, cardIndex)) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      let $n301 = $update2 ? $lepusGetElementRefByLepusID("view", 301) : null;
      if (!$n301) {
        $update2 = false;
        $n301 = __CreateView($currentComponentId);
        let $nid301 = $lepusStoreElementRefByLepusID($n301, 301, "view");
        __SetAttribute($n301, 1004, $nid301[1]);
        __AppendElement($parent, $n301);
      }
      __SetAttribute($n301, "class", buildClassTokens(section, card, cardIndex));
      __SetStyleObject($n301, [buildInlineStyle(section, card, cardIndex, 0), {
        exposed: !!(($data.expose_map || {})[card.id])
      }]);
      __SetAttribute($n301, "data-id", card.id);

      let $n302 = $update2 ? $lepusGetElementRefByLepusID("text", 302) : null;
      if (!$n302) {
        $n302 = __CreateText($currentComponentId);
        $lepusStoreElementRefByLepusID($n302, 302, "text");
        __AppendElement($n301, $n302);
      }
      __SetAttribute($n302, "text", card.displayTitle + "|" + getAliasValue(card.dynamic, 1, "1"));

      let $n303 = $update2 ? $lepusGetElementRefByLepusID("for", 303) : null;
      if (!$n303) {
        $n303 = __CreateFor($currentComponentId);
        $lepusStoreElementRefByLepusID($n303, 303, "for");
        __AppendElement($n301, $n303);
      }
      renderBadgeList($n303, $data, $update2, sectionIndex, section, cardIndex, card);

      let $n304 = $update2 ? $lepusGetElementRefByLepusID("if", 304) : null;
      if (!$n304) {
        $n304 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n304, 304, "if");
        __AppendElement($n301, $n304);
      }
      $renderTemplates.CardFooter.entry($n304, $data, $update2, sectionIndex, section, cardIndex, card);
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n305 = $update2 ? $lepusGetElementRefByLepusID("view", 305) : null;
      if (!$n305) {
        $update2 = false;
        $n305 = __CreateView($currentComponentId);
        let $nid305 = $lepusStoreElementRefByLepusID($n305, 305, "view");
        __SetAttribute($n305, 1004, $nid305[1]);
        __AppendElement($parent, $n305);
      }
      __SetAttribute($n305, "data-skip", card.id);
      __SetStyleObject($n305, ["skip-card", {
        reason: "gate"
      }]);
      $update2 = _$temp;
    }
  }
};

$renderTemplates.CardFooter = {
  entry: function ($parent, $data, $update2, sectionIndex, section, cardIndex, card) {
    let uniqueId = __GetElementUniqueID($parent);
    if (!$update2) {
      $conditionNodeIndex[uniqueId] = -1;
    }
    let $ifNodeIndex = $conditionNodeIndex[uniqueId];
    let hasPrice = card.price && (card.price.current != null || card.price.origin != null);
    if (hasPrice) {
      __UpdateIfNodeIndex($parent, 0);
      $conditionNodeIndex[uniqueId] = 0;
      let $temp = $update2;
      if ($ifNodeIndex !== 0) {
        $update2 = false;
      }
      let $n331 = $update2 ? $lepusGetElementRefByLepusID("text", 331) : null;
      if (!$n331) {
        $update2 = false;
        $n331 = __CreateText($currentComponentId);
        let $nid331 = $lepusStoreElementRefByLepusID($n331, 331, "text");
        __SetAttribute($n331, 1004, $nid331[1]);
        __AppendElement($parent, $n331);
      }
      __SetAttribute($n331, "text", "¥" + (card.price.current || 0) + " / " + (card.price.origin || 0));
      __SetStyleObject($n331, [{
        shrink: cardIndex & 1
      }, {
        theme: GLOBAL_THEME
      }]);
      $update2 = $temp;
    } else {
      __UpdateIfNodeIndex($parent, 1);
      $conditionNodeIndex[uniqueId] = 1;
      let _$temp = $update2;
      if ($ifNodeIndex !== 1) {
        $update2 = false;
      }
      let $n332 = $update2 ? $lepusGetElementRefByLepusID("image", 332) : null;
      if (!$n332) {
        $update2 = false;
        $n332 = __CreateImage($currentComponentId);
        let $nid332 = $lepusStoreElementRefByLepusID($n332, 332, "image");
        __SetAttribute($n332, 1004, $nid332[1]);
        __AppendElement($parent, $n332);
      }
      __SetAttribute($n332, "src", card.media.cover.url_list[0]);
      __SetStyleObject($n332, ["cover-only", {
        alias: getAliasValue(card.dynamic, 1, "1")
      }]);
      $update2 = _$temp;
    }
  }
};

$renderTemplates.Section = {
  entry: function ($parent, $data, $update2, sectionIndex, section) {
    let $n201 = $update2 ? $lepusGetElementRefByLepusID("view", 201) : null;
    let $temp = $update2;
    if (!$n201) {
      $update2 = false;
      $n201 = __CreateView($currentComponentId);
      let $nid201 = $lepusStoreElementRefByLepusID($n201, 201, "view");
      __SetAttribute($n201, 1004, $nid201[1]);
      __AppendElement($parent, $n201);
    }
    __SetAttribute($n201, "section-key", section.key);
    __SetStyleObject($n201, ["section-shell", {
      sectionIndex: sectionIndex
    }, {
      alias: getAliasValue(section.meta, 1, "1")
    }]);

    let $n202 = $update2 ? $lepusGetElementRefByLepusID("text", 202) : null;
    if (!$n202) {
      $n202 = __CreateText($currentComponentId);
      $lepusStoreElementRefByLepusID($n202, 202, "text");
      __AppendElement($n201, $n202);
    }
    __SetAttribute($n202, "text", section.title + "#" + section.layout);

    let $n203 = $update2 ? $lepusGetElementRefByLepusID("for", 203) : null;
    if (!$n203) {
      $n203 = __CreateFor($currentComponentId);
      $lepusStoreElementRefByLepusID($n203, 203, "for");
      __AppendElement($n201, $n203);
    }

    let uniqueId = __GetElementUniqueID($n203);
    let pair = $lepusPushFiberForNode($n203, 203, uniqueId);
    let $forLepus = pair[0];
    let $lastForLepus = pair[1];
    let $object = section.cards;
    let $length = $object.length;
    __UpdateForChildCount($n203, $length);
    let $loopTemp = $update2;
    for (let cardIndex = 0; cardIndex < $length; ++cardIndex) {
      $update2 = cardIndex < $forLepus._lastLength ? $update2 : false;
      $lepusUpdateFiberForNodeIndex(cardIndex);
      let card = $object[cardIndex];
      let $n204 = $update2 ? $lepusGetElementRefByLepusID("if", 204) : null;
      if (!$n204) {
        $n204 = __CreateIf($currentComponentId);
        $lepusStoreElementRefByLepusID($n204, 204, "if");
        __AppendElement($n203, $n204);
      }
      $renderTemplates.Card.entry($n204, $data, $update2, sectionIndex, section, cardIndex, card);
    }
    $forLepus._lastLength = $length;
    $update2 = $loopTemp;
    $lepusPushFiberForNode($lastForLepus, undefined, undefined);
    $update2 = $temp;
  }
};

function renderRoot($root, $data, $update2) {
  let sections = $data.sections;
  let uniqueId = __GetElementUniqueID($root);
  let pair = $lepusPushFiberForNode($root, 101, uniqueId);
  let $forLepus = pair[0];
  let $lastForLepus = pair[1];
  let $length = sections.length;
  __UpdateForChildCount($root, $length);
  let $temp = $update2;
  for (let sectionIndex = 0; sectionIndex < $length; ++sectionIndex) {
    $update2 = sectionIndex < $forLepus._lastLength ? $update2 : false;
    $lepusUpdateFiberForNodeIndex(sectionIndex);
    let section = sections[sectionIndex];
    let $n102 = $update2 ? $lepusGetElementRefByLepusID("view", 102) : null;
    if (!$n102) {
      $n102 = __CreateView($currentComponentId);
      $lepusStoreElementRefByLepusID($n102, 102, "view");
      __AppendElement($root, $n102);
    }
    $renderTemplates.Section.entry($n102, $data, $update2, sectionIndex, section);
  }
  $forLepus._lastLength = $length;
  $update2 = $temp;
  $lepusPushFiberForNode($lastForLepus, undefined, undefined);
}

let renderPage = function ($data) {
  let nextData = normalizeData($data);
  if (!$cardInstance) {
    $cardConstructor(10);
  }
  $cardInstance._data = $deepClone($cardInstance.data || {});
  $cardInstance.data = nextData;
  if (!$page) {
    $page = __CreatePage($currentComponentId, "your_test.js");
    let $root = __CreateView($currentComponentId);
    $lepusStoreElementRefByLepusID($root, 100, "view");
    __AppendElement($page, $root);
  }
  let root = $page.children[0];
  renderRoot(root, nextData, false);
  __FlushElementTree($page);
  return $page;
};

let updatePage = function ($newData, options) {
  let nextData = normalizeData($newData);
  let diff = __GetDiffData($cardInstance.data, nextData, options);
  $cardInstance._data = $deepClone($cardInstance.data);
  $cardInstance.data = diff.nextData;
  $update = true;
  let root = $page.children[0];
  renderRoot(root, diff.nextData, true);
  __FlushElementTree($page);
  return diff;
};

let initialData = {
  env: {
    font_scale: "1.25",
    theme: "dark"
  },
  expose_map: {
    hero_0: true,
    hero_1: false,
    feed_0: true
  },
  sections: [{
    key: "hero",
    title: "Hero Section",
    layout: "grid",
    visible: true,
    meta: {
      flags: {
        forceHide: false
      }
    },
    cards: [{
      id: "hero_0",
      title: "Alpha",
      suffix: "Top",
      score: 91,
      width: 120,
      height: 80,
      dynamic: {
        foo: 7
      },
      price: {
        current: 99,
        origin: 129
      },
      badges: [{
        type: "text",
        text: "hot",
        weight: 1
      }, {
        type: "icon",
        src: "mock://icon/hot",
        weight: 2
      }, {
        type: "view",
        weight: 3
      }]
    }, {
      id: "hero_1",
      title: "Beta",
      score: 55,
      width: 100,
      height: 72,
      dynamic: {
        bar: 9
      },
      badges: [{
        type: "text",
        text: "sale",
        weight: 2
      }],
      media: {
        cover: {
          url_list: ["mock://cover/hero_1"]
        }
      }
    }]
  }, {
    key: "feed",
    title: "Feed Section",
    layout: "list",
    visible: true,
    meta: {
      flags: {
        forceHide: true
      }
    },
    cards: [{
      id: "feed_0",
      title: "Gamma",
      subtitle: "alias-heavy",
      score: 88,
      width: 110,
      height: 90,
      dynamic: {
        foo: 3,
        bar: 4
      },
      price: {
        current: 59,
        origin: 79
      },
      badges: [{
        type: "icon",
        src: "mock://icon/feed_0",
        weight: 1
      }, {
        type: "text",
        text: "new",
        weight: 3
      }]
    }, {
      id: "feed_1",
      title: "Delta",
      score: 10,
      width: 98,
      height: 66,
      dynamic: {
        baz: 5
      },
      badges: [{
        type: "view",
        weight: 4
      }],
      media: {
        cover: {
          url_list: ["mock://cover/feed_1"]
        }
      }
    }]
  }]
};

let updatedData = $deepClone(initialData);
updatedData.env.font_scale = "1.5";
updatedData.sections[0].cards[0].price.current = 89;
updatedData.sections[0].cards[0].badges[0].text = "hot+";
updatedData.sections[1].meta.flags.forceHide = false;
updatedData.sections[1].cards[1].score = 77;
updatedData.sections[1].cards[1].price = {
  current: 42,
  origin: 66
};
updatedData.expose_map.feed_1 = true;

renderPage(initialData);
let diff = updatePage(updatedData, {
  patchMode: "full"
});

Assert(!!$page, "page should exist");
Assert($page.children.length > 0, "page should have root");
Assert(GLOBAL_COUNTER > 0, "global counter should be touched");
Assert(GLOBAL_SLOT_SERIAL > 0, "slot serial should increase");
Assert(diff.changed === true, "diff should mark changed");
Assert($cardInstance.data.sections.length === 2, "section length mismatch");
Assert($cardInstance.data.sections[1].cards[1].price.current === 42, "updated price mismatch");
Assert($cardInstance.data.sections[0].cards[0].dynamic[1] === $cardInstance.data.sections[0].cards[0].dynamic["1"], "alias should stay synced");

print("your_test compiled and executed");
