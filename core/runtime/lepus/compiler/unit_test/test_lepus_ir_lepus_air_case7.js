// [TEST_TARGET: IR]
// Focus: short-circuit evaluation + side-effects order

let $currentComponentId = 1;

let __assert_fail_count = 0;
function SoftAssert(v, msg) {
  if (!v) {
    __assert_fail_count = __assert_fail_count + 1;
    print(msg);
  }
}

function __CreateText(a) {}
function __CreateView(a) {}
function __AppendElement(a, b) {}
function __SetAttribute(a, b, c) {}

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

function $renderShortCircuit(data) {
  let log = [];
  function push(x) {
    log.push(x);
    return x;
  }

  // && should not evaluate RHS when LHS is falsey
  let a = data.a;
  let b = data.b;
  let r1 = a && push("rhs_and");
  SoftAssert(r1 === a, "r1_mismatch");
  SoftAssert(log.length === 0, "and_evaluated_rhs");

  // || should not evaluate RHS when LHS is truthy
  let r2 = b || push("rhs_or");
  SoftAssert(r2 === b, "r2_mismatch");
  SoftAssert(log.length === 0, "or_evaluated_rhs");

  // Ternary condition evaluation order
  let cond = data.cond;
  let r3 = (cond ? push("then") : push("else"));
  SoftAssert(log.length === 1, "ternary_eval_count_wrong");
  SoftAssert(r3 === (cond ? "then" : "else"), "ternary_branch_wrong");

  // Make it look like a small template rendering
  let root = __CreateView($currentComponentId);
  let textNode = __CreateText($currentComponentId);
  __AppendElement(root, textNode);
  __SetAttribute(textNode, "text", "ok");

  return log.join(",");
}

let result = $renderShortCircuit({
  a: 0,
  b: 1,
  cond: true
});

SoftAssert(result === "then", "result_not_then");
print("FAIL=" + __assert_fail_count);
Assert(__assert_fail_count === 0);
