var GA = 1;
var GB = 2;
var GT = { v: 3 };

function touch(obj, i) {
  GA = GA + 1;
  obj.a = obj.a + GA;
  obj.child.b = obj.child.b + (i + 1);
  GT.v = GT.v + obj.child.b;
  return obj.a + obj.child.b + GT.v;
}

function makeUpdater(seed) {
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
      let a1 = obj.a;
      let a2 = obj.a;
      let b1 = obj.child.b;
      let b2 = obj.child.b;
      sum += a1 + a2 + b1 + b2;

      obj.a = obj.a + i;
      obj.child.b = obj.child.b + (obj.a % 3);

      GB = GB + (obj.child.b % 2);
      GT.v = GT.v + (GA + GB);
      sum += GA + GB + GT.v;

      sum += localStep(i + 1);

      sum += touch(obj, i);
    }

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
  let u2 = makeUpdater(4);

  let ga0 = GA;
  let gb0 = GB;
  let gv0 = GT.v;

  let r1 = u1(obj, 4);
  let r2 = u2(obj, 3);

  Assert(r1.rounds === 4);
  Assert(r2.rounds === 3);
  Assert(r1.ga === ga0 + 4);
  Assert(r2.ga === ga0 + 7);
  Assert(GA === ga0 + 7);

  Assert(r1.out === r1.sum + r1.reg + r1.bx + r1.by + r1.a + r1.b + r1.ga + r1.gb + r1.gv);
  Assert(r2.out === r2.sum + r2.reg + r2.bx + r2.by + r2.a + r2.b + r2.ga + r2.gb + r2.gv);

  Assert(r1.a > 5);
  Assert(r1.b > 7);
  Assert(r2.a >= r1.a);
  Assert(r2.b >= r1.b);
  Assert(r2.gb >= gb0);
  Assert(r2.gv > gv0);
  Assert(obj.a === r2.a);
  Assert(obj.child.b === r2.b);
}

main();
