var GCOUNT = 0;
var GBOX = { v: 1 };

function bumpGlobal(step) {
  GCOUNT = GCOUNT + step;
  GBOX.v = GBOX.v + (GCOUNT & 3);
  return GCOUNT + GBOX.v;
}

function makeWorker(seed) {
  let reg = seed;
  let local = { t: seed + 7 };

  function step(x) {
    reg = reg + x;
    local.t = local.t + (reg & 7);
    return reg + local.t;
  }

  return function run(obj, other, rounds) {
    let acc = 0;

    let kNum = 1;
    let kStr = "1";
    let kFoo = "foo";

    for (let i = 0; i < rounds; i++) {
      let choose = (i & 1) === 0;
      let mid = 0;

      if (choose) {
        let a1 = obj[kNum];
        let a2 = obj[kNum];
        let b1 = obj[kStr];
        let b2 = obj[kStr];
        mid = a1 + a2 + b1 + b2;

        obj[1] = obj[1] + 1;
        obj["1"] = obj["1"] + 2;

        obj["foo"] = obj["foo"] + (i % 5);
      } else {
        let dk = i % 5; // number
        let v1 = obj[dk];
        let v2 = obj[dk];
        mid = v1 + v2;

        obj[dk] = (obj[dk] || 0) + 1;
        other[dk] = (other[dk] || 0) + 1;
        mid = mid + step(i + 1);
        mid = mid + bumpGlobal(1);
      }

      let r1 = obj[1];
      let r2 = obj["1"];
      Assert(r1 === r2);

      let j1 = obj["1"];
      let j2 = obj["1"];
      acc = acc + mid + r1 + r2 + j1 + j2;

      let tag = i & 3;
      if (tag === 0) {
        acc = acc + (obj["foo"] | 0);
      } else if (tag === 1) {
        acc = acc + (obj[1] | 0);
      } else if (tag === 2) {
        acc = acc + (other["foo"] | 0);
      } else {
        acc = acc + (other[1] | 0);
      }

      let fresh = { "1": i, foo: i + 100 };
      fresh["1"] = fresh["1"] + 1;
      fresh[1] = fresh[1] + 2;
      Assert(fresh[1] === fresh["1"]);
      Assert(obj["1"] === r1);
    }

    Assert(obj[1] === obj["1"]);
    Assert(other[1] === other["1"]);

    return acc + obj["1"] + obj["foo"] + other["foo"] + reg + local.t + GCOUNT + GBOX.v;
  };
}

function main() {
  let obj = { 0: 0, 1: 0, 2: 0, 3: 0, 4: 0, foo: 1, bar: 2 };
  let other = { 0: 0, 1: 0, 2: 0, 3: 0, 4: 0, foo: 0 };

  let w1 = makeWorker(3);
  let w2 = makeWorker(5);

  let out1 = w1(obj, other, 16);
  let out2 = w2(obj, other, 12);

  Assert(obj[1] === obj["1"]);
  Assert(other[1] === other["1"]);
  Assert(obj["foo"] >= 1);
  Assert(GCOUNT === 14);
  Assert(GBOX.v === 22);
  Assert(out1 !== 0);
  Assert(out2 !== 0);
}

main();
