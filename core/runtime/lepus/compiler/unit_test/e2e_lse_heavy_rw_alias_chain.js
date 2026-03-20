
var GX = 10;
var GY = 20;
var GZ = { p: 3, arr: [5, 6, 7], nest: { t: 8 } };

function impure(root, i) {
  GX = GX + 1;
  GZ.p = GZ.p + 1;
  GZ.arr[0] = GZ.arr[0] + 1;

  root.alias.x = root.alias.x + (GX % 3);
  root.shared.y = root.shared.y + (GY % 2);

  root.left.v = root.left.v + GX;
  root.arr[i % 3] = root.arr[i % 3] + (GZ.p % 4);
  return root.left.v + root.arr[0] + root.shared.x + GZ.arr[0] + GZ.nest.t;
}

function makeRunner(seed) {
  let reg = seed;
  let box = { v: seed + 1 };
  let hist = [seed, seed + 2];

  function touchLocal(i) {
    reg = reg + (i + 1);
    box.v = box.v + reg;
    hist[0] = hist[0] + box.v;
    hist[1] = hist[1] + hist[0];
    return reg + box.v + hist[0] + hist[1];
  }

  return function run(root, rounds) {
    let sum = 0;
    for (let i = 0; i < rounds; i++) {
      let sref = (i % 2 === 0) ? root.shared : root.alias;

      let x1 = sref.x;
      let x2 = sref.x;
      let y1 = sref.y;
      let y2 = sref.y;
      sum += x1 + x2 + y1 + y2;

      let k = (i % 3 === 0) ? "k0" : "k1";
      if (root[k] == null) root[k] = 0;
      root[k] = root[k] + (sref.x % 2);

      sref.x = sref.x + i + reg;
      sref.y = sref.y + (sref.x % 5);

      GY = GY + (sref.y % 3);
      GZ.nest.t = GZ.nest.t + (GX + GY + i);
      sum += GX + GY + GZ.p + GZ.nest.t;

      sum += touchLocal(i);

      sum += impure(root, i);

      sum += root.shared.x + root.alias.x + root.shared.y + root.alias.y;
    }

    let k0 = (root.k0 == null) ? 0 : root.k0;
    let k1 = (root.k1 == null) ? 0 : root.k1;
    let out = sum + reg + box.v + hist[0] + hist[1] + root.shared.x + root.shared.y + GX + GY + GZ.p + GZ.nest.t;
    return {
      rounds: rounds,
      sum: sum,
      out: out,
      gx: GX,
      gy: GY,
      p: GZ.p,
      t: GZ.nest.t,
      sx: root.shared.x,
      sy: root.shared.y,
      ax: root.alias.x,
      ay: root.alias.y,
      lv: root.left.v,
      a0: root.arr[0],
      k0: k0,
      k1: k1,
      reg: reg,
      bv: box.v,
      h0: hist[0],
      h1: hist[1],
    };
  };
}

function main() {
  let ok = true;
  let root = {
    shared: { x: 1, y: 2 },
    alias: null,
    left: { v: 4 },
    right: { v: 5 },
    arr: [1, 2, 3],
  };
  root.alias = root.shared;

  let gx0 = GX;
  let gy0 = GY;
  let p0 = GZ.p;
  let a00 = GZ.arr[0];
  let t0 = GZ.nest.t;

  let r1 = makeRunner(3)(root, 6);
  let r2 = makeRunner(4)(root, 5);

  if (!(root.shared === root.alias)) ok = false;
  if (!(r2.sx === r2.ax)) ok = false;
  if (!(r2.sy === r2.ay)) ok = false;

  if (!(GX === gx0 + 11)) ok = false;
  if (!(GZ.p === p0 + 11)) ok = false;
  if (!(GZ.arr[0] === a00 + 11)) ok = false;
  if (!(r1.gx === gx0 + 6)) ok = false;
  if (!(r2.gx === gx0 + 11)) ok = false;
  if (!(r2.p === p0 + 11)) ok = false;

  if (!(GY >= gy0)) ok = false;
  if (!(GZ.nest.t > t0)) ok = false;
  if (!(root.left.v > 4)) ok = false;
  if (!(root.arr[0] !== 1 || root.arr[1] !== 2 || root.arr[2] !== 3)) ok = false;

  if (!(r1.out === r1.sum + r1.reg + r1.bv + r1.h0 + r1.h1 + r1.sx + r1.sy + r1.gx + r1.gy + r1.p + r1.t)) ok = false;
  if (!(r2.out === r2.sum + r2.reg + r2.bv + r2.h0 + r2.h1 + r2.sx + r2.sy + r2.gx + r2.gy + r2.p + r2.t)) ok = false;

  if (!(r2.k0 >= 0)) ok = false;
  if (!(r2.k1 >= 0)) ok = false;
  if (!(r1.rounds === 6)) ok = false;
  if (!(r2.rounds === 5)) ok = false;

  Assert(ok);
}

main();
