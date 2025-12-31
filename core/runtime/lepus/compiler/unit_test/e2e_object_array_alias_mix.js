// E2E: object/array mixed access, numeric/string key aliasing, computed keys.

function build() {
  let o = { a: 1, b: 2 };
  // numeric key write then string key read (potential alias case)
  o[10] = 100;
  o["10"] = o[10] + 23;
  // computed prop name in loop
  for (let i = 0; i < 3; i++) {
    let k = "p" + i;
    o[k] = i * i + o.a;
  }
  return o;
}

function touch(o) {
  // side-effect call boundary
  o.a = o.a + 7;
  o.b = o.b * 3;
  return o.a + o.b;
}

let obj = build();
let x1 = obj[10] + obj["10"]; // 100 + 123
let x2 = touch(obj);          // a=8, b=6 => 14
let x3 = obj.p0 + obj.p1 + obj.p2; // (0+1) + (1+1) + (4+1) = 8
let x4 = obj["10"] - obj[10];

// Lepus treats numeric key and string key as alias ("10" and 10 refer to same slot).
// So obj[10] becomes 123 after writing obj["10"].
Assert(x1 === 246);
Assert(x2 === 14);
Assert(x3 === 8);
Assert(x4 === 0);
