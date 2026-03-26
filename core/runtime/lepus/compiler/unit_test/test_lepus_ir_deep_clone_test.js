// Verify $deepClone builtin fast-path behavior.
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
    console.log("in here");
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
    // array: shallow clone
    let a = [1, 2];
    let b = $deepClone(a);
    console.log(b.length);
    Assert(b.length == 2);
    Assert(b[0] == 1);
    Assert(b[1] == 2);

    // modifications on clone should not affect original container
    b.push(3);
    Assert(b.length == 3);
    Assert(a.length == 2);

    // object(table): shallow clone
    let o1 = { k1: 10, k2: "v" };
    let o2 = $deepClone(o1);
    Assert(o2.k1 == 10);
    Assert(o2.k2 == "v");
    o2.k1 = 20;
    Assert(o1.k1 == 10);

    // nested refs are not deep-copied (shallow semantics)
    let inner = { x: 1 };
    let arr2 = [inner];
    let arr3 = $deepClone(arr2);
    arr3[0].x = 2;
    Assert(inner.x == 2);

    // primitive / null / undefined: return as-is
    Assert($deepClone(123) == 123);
    Assert($deepClone("abc") == "abc");
    Assert($deepClone(null) == null);
    Assert($deepClone(undefined) == undefined);
}
main();
