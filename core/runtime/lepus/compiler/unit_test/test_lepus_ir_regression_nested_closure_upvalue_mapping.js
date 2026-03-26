// Regression: stress nested closures + repeated writes/reads to a captured
// toplevel variable. This exercises UpdateToplevelClosureVarPass mapping and
// later VM-target passes.

let g = 0;

function makeOuter(seed) {
  g = seed;

  function mid(step) {
    g = g + step;
    function inner(mult) {
      // Multiple reads, mixed ops.
      let t = g;
      t = t + 1;
      return (t * mult) - (g - 1);
    }
    return inner;
  }

  return mid;
}

let mid1 = makeOuter(10);
let inner1 = mid1(5);
Assert(inner1(2) === ((g + 1) * 2) - (g - 1));

// Force more closure instances and register pressure.
let mid2 = makeOuter(100);
let inner2 = mid2(7);
Assert(inner2(3) === ((g + 1) * 3) - (g - 1));

// Re-assign and re-read.
g = 1;
Assert(inner2(1) === ((g + 1) * 1) - (g - 1));

print("regression_nested_closure_upvalue_mapping passed");
