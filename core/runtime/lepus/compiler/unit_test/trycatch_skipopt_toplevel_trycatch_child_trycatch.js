// Compile-only testcase:
// - toplevel has try/catch
// - child function ALSO has try/catch
// Expectation: compilation succeeds with opt enabled.

var base = 2;
try {
  base = base * 3;
} catch (e) {
  base = 0;
}

function childWithTry(n) {
  try {
    return n + base;
  } catch (e) {
    return 0;
  }
}

var y = childWithTry(5);

