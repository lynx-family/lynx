// Compile-only testcase:
// - toplevel has NO try/catch
// - one child function has try/catch
// - another child function has NO try/catch
// Expectation: compilation succeeds with opt enabled.

var base = 3;

function childWithTry(n) {
  try {
    return n + base;
  } catch (e) {
    return -1;
  }
}

function childNoTry(n) {
  return n + base;
}

var y = childWithTry(10) + childNoTry(20);

