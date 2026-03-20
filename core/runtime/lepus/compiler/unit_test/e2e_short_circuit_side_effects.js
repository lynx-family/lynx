// E2E: && / || short-circuit with side effects (order stability).

let g = 0;
function inc(x) {
  g = g + 1;
  return x;
}

let a = (inc(false) && inc(true));   // second inc should NOT run
let b = (inc(true) || inc(false));   // second inc should NOT run
let c = (inc(false) || inc(5));      // second inc should run
let d = (inc(true) && inc(7));       // second inc should run

Assert(a === false);
Assert(b === true);
Assert(c === 5);
Assert(d === 7);
// inc calls: false, true, false, 5, true, 7 => 6
Assert(g === 6);

