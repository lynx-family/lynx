var G0 = 1;
var G1 = 2;
var GTAB = { p: 5, q: 7, arr: [1, 2, 3] };

function sideEffect(tag, obj, k) {
  G0 = G0 + 1;
  if (tag === 0) {
    obj.x = obj.x + G0;
    obj.child.v = obj.child.v + obj.x;
    GTAB.arr[1] = GTAB.arr[1] + 10;
  } else {
    if (obj[k] == null) obj[k] = 0;
    obj[k] = obj[k] + G1;
    G1 = G1 + 3;
    GTAB.p = GTAB.p + 2;
  }
  return obj.x + obj.child.v + GTAB.p;
}

function makeWorker(seed) {
  let reg = seed;
  let buf = { a: seed + 1, b: seed + 2 };
  let counters = [0, 0, 0];

  function touchLocal(i) {
    counters[0] = counters[0] + 1;
    reg = reg + i + counters[0];
    buf.a = buf.a + reg;
    buf.b = buf.b + (buf.a % 3);
    return buf.a + buf.b + reg;
  }

  return function run(obj, rounds) {
    let sum = 0;
    for (let i = 0; i < rounds; i++) {
      let k = (i % 2 === 0) ? "dyn" : "alt";
      obj[k] = (obj[k] ?? 0) + i;

      let x1 = obj.x;
      let x2 = obj.x;
      sum += (x1 + x2);

      let v1 = obj.child.v;
      let v2 = obj.child.v;
      sum += (v1 + v2);

      sum += touchLocal(i);

      sum += sideEffect(i % 2, obj, k);

      let gArr1 = GTAB.arr[1];
      GTAB.arr[1] = gArr1 + (i % 3);
      sum += GTAB.arr[1] + GTAB.p;
    }

    return {
      sum: sum,
      c0: counters[0],
      reg: reg,
      a: buf.a,
      b: buf.b,
      x: obj.x,
      v: obj.child.v,
      g0: G0,
      g1: G1,
      p: GTAB.p,
      a1: GTAB.arr[1],
    };
  };
}

function main() {
  let ASSERT = Assert;

  let obj = { x: 3, dyn: 0, alt: 0, child: { v: 4 }, arr: [10, 20, 30] };

  let w1 = makeWorker(5);
  let w2 = makeWorker(7);

  let r1 = w1(obj, 5);
  let snapG0 = G0;
  let snapP = GTAB.p;
  let r2 = w2(obj, 4);

  let r3 = w1(obj, 3);

  ASSERT(r1.c0 === 5);
  ASSERT(r2.c0 === 4);
  ASSERT(r3.c0 === 8);

  ASSERT(r2.g0 > snapG0);
  ASSERT(r2.p > snapP);

  ASSERT(r2.x > 3);
  ASSERT(r2.v > 4);
  ASSERT(obj.dyn >= 0);
  ASSERT(obj.alt >= 0);
  ASSERT(r1.sum !== r2.sum);
  ASSERT(r3.sum !== r1.sum);
  ASSERT(r1.sum > 0 && r2.sum > 0 && r3.sum > 0);
  ASSERT(r2.a1 >= 2);

  ASSERT(true);
}

main();
