// Compile-only testcase:
// - toplevel has try/catch
// - child function has NO try/catch
// Expectation: compilation succeeds with opt enabled.

var base = 1;
try {
  base = base + 3;
} catch (e) {
  base = 7;
}

function childNoTry(n) {
  return n + base;
}

var y = childNoTry(10);

