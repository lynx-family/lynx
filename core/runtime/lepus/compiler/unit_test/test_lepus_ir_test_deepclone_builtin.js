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

function main() {
  let obj = {
    x: 1,
    child: [1, 2, 3]
  };

  let array = [1, 2, 3];

  let m = $deepClone(obj);
  let n = $deepClone(array);
  let o = $deepClone(123);

  Assert(o === 123);
  Assert(n.length === 3);
  Assert(m.x === 1);
  Assert(m.child.length === 3);

  obj.x = 2;
  Assert(m.x === 1);
}

function main2() {
  let func = function(n) {
    let m = $deepClone(n);
    console.log(m);
    return 42;
  }
  func(123);
}