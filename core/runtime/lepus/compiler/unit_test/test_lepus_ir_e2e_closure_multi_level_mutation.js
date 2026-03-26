// E2E: multi-level closures with captured vars + mutation + loops.
// Keep arithmetic-only to avoid engine-specific bitwise corner cases.

function makeCounter(seed) {
  let a = seed;
  function step(x) {
    a = a + x;
    return a;
  }
  return function run(times) {
    let s = 0;
    for (let i = 0; i < times; i++) {
      // nested closure each iteration
      let inner = function (k) {
        return step(k + 1);
      };
      s += inner(i);
    }
    return s * 100 + a;
  };
}

let f = makeCounter(3);
let r1 = f(4);
let r2 = f(2);

Assert(r1 === 3213);
Assert(r2 === 3016);
