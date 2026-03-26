// Compile-only testcase: toplevel contains try/catch/finally.
// Expectation: compilation succeeds with opt enabled, and IR optimization is skipped for this script.

var x = 1;
try {
  x = x + 1;
} catch (e) {
  x = 0;
} finally {
  x = x + 10;
}

function foo() {
  return x + 2;
}

var y = foo();

