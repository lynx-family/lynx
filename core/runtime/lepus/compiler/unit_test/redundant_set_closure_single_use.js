// Target: RedundantSetToplevelEliminationPass (SetToplevelClosureVarInst)
//
// Make `x` a toplevel closure variable by capturing it.

var x = 0;

function read_x() {
  return x;
}

// Single-use producers for `x`.
x = 1 + 2;
x = (3 * 4) + 5;

read_x();

