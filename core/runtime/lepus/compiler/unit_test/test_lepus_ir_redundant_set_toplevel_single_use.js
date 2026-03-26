// Target: RedundantSetToplevelEliminationPass (SetToplevelVarInst)
//
// This file is compiled by `testing/tools/run_lepus_unittests.py`.

var a = 0;

// Single-use producers for `a`.
a = 1 + 2;
a = (3 * 4) + 5;

function id(x) { return x; }
a = id(6);

