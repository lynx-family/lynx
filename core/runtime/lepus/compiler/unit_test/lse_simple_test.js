// E2E (LSE stability - stable): 大量混合读写（field + 闭包 + 全局）
// 目标：构造大量“读-写-再读”与函数调用边界，验证 LSE 不会破坏值一致性。

// 全局变量（toplevel var）
var GA = 1;
var GB = 2;
var GT = { v: 3 };

function touch(obj, i) {
  // side-effect：同时写 field + 全局
  GA = GA + 1;
  obj.a = obj.a + GA;
  obj.child.b = obj.child.b + (i + 1);
  GT.v = GT.v + obj.child.b;
  return obj.a + obj.child.b + GT.v;
}

function makeUpdater(seed) {
  // 闭包捕获
  let reg = seed;
  let buf = { x: seed + 10, y: seed + 20 };

  function localStep(k) {
    reg = reg + k;
    buf.x = buf.x + reg;
    buf.y = buf.y + buf.x;
    return reg + buf.x + buf.y;
  }

  return function run(obj, rounds) {
    let sum = 0;
    for (let i = 0; i < rounds; i++) {
      // 反复 field 读（为 LSE 提供机会）
      let a1 = obj.a;
      let a2 = obj.a;
      let b1 = obj.child.b;
      let b2 = obj.child.b;
      sum += a1 + a2 + b1 + b2;

      // 写入 field
      obj.a = obj.a + i;
      obj.child.b = obj.child.b + (obj.a % 3);

      // 全局读写
      GB = GB + (obj.child.b % 2);
      GT.v = GT.v + (GA + GB);
      sum += GA + GB + GT.v;

      // 闭包读写
      sum += localStep(i + 1);

      // side-effect call boundary
      sum += touch(obj, i);
    }

    // 把关键状态回传给 main 做稳定断言（避免仅靠“精确最终值”导致 baseline/opt 波动时误报）。
    let out = sum + reg + buf.x + buf.y + obj.a + obj.child.b + GA + GB + GT.v;
    return {
      rounds: rounds,
      sum: sum,
      out: out,
      reg: reg,
      bx: buf.x,
      by: buf.y,
      a: obj.a,
      b: obj.child.b,
      ga: GA,
      gb: GB,
      gv: GT.v,
    };
  };
}

function main() {
  let obj = { a: 5, child: { b: 7 } };
  let u1 = makeUpdater(3);
  let r1 = u1(obj, 4);
}

main();
