// Regression: mix redundant toplevel writes with closure reads.
// This is intended to stress VM-target passes that coalesce/eliminate redundant
// SetToplevel* patterns while still preserving closure/upvalue semantics.

var a = 1;
var b = 2;

function mk() {
  // Capture `a` and `b`.
  return function (x) {
    // Redundant/self writes (common patterns).
    a = a;
    b = b;

    // Redundant chained assignment.
    a = (a = a + 1);
    b = (b = b + 2);

    // Use captured vars.
    return (a + b) * x;
  };
}

var f = mk();
Assert(f(1) === (a + b) * 1);

// More writes between calls.
a = 10;
b = 20;
Assert(f(2) === (a + b) * 2);

// Another closure instance.
var g = mk();
a = 3;
b = 4;
Assert(g(3) === (a + b) * 3);

print("regression_toplevel_redundant_writes_mix passed");

