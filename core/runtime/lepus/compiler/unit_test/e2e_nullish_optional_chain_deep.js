// E2E: optional chaining + nullish coalescing in deeper shapes.

function getDeep(o) {
  let v1 = o.a?.b?.c ?? 10;
  let v2 = o.a?.x?.y ?? 20;
  let v3 = o.missing?.q ?? (o.a?.b?.c ?? 30);
  return v1 + v2 + v3;
}

let obj1 = { a: { b: { c: 3 } } };
let obj2 = { a: { b: { c: 0 }, x: { y: 7 } } };
let obj3 = { a: { b: null } };

Assert(getDeep(obj1) === (3 + 20 + 3));
Assert(getDeep(obj2) === (0 + 7 + 0));
Assert(getDeep(obj3) === (10 + 20 + 30));

