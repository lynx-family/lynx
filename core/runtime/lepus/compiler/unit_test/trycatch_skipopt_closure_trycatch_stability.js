// Compile-only testcase:
// - toplevel has NO try/catch
// - closure function (anonymous) contains try/catch
// Expectation: compilation succeeds with opt enabled (stability coverage).

function makeAdder() {
  var base = 1;
  return function (n) {
    try {
      return n + base;
    } catch (e) {
      return 0;
    }
  };
}

var f = makeAdder();
var y = f(5);

