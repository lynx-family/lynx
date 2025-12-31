
function TestClosureLoadElimination() {
  var x = 10;
  function inner() {
    var a = x;
    var b = x; // Redundant load; should be eliminated.
    Assert(a == 10);
    Assert(b == 10);
  }
  inner();
}

function TestClosureStoreToLoadForwarding() {
  var x = 10;
  function inner() {
    x = 20;
    var a = x; // Should use 20 directly (load forwarding).
    Assert(a == 20);
  }
  inner();
  Assert(x == 20);
}

function TestClosureInvalidationByReassign() {
  var x = 10;
  function inner() {
    var a = x;
    x = 30; // Invalidate old cached value.
    var b = x; // Reload or use the new value.
    Assert(a == 10);
    Assert(b == 30);
  }
  inner();
}

function TestClosureInvalidationByCall() {
  var x = 10;
  function modifier() {
    x = 100;
  }
  function inner() {
    var a = x;
    modifier(); // Call is a barrier; must invalidate cached closure vars.
    var b = x;
    Assert(a == 10);
    Assert(b == 100);
  }
  inner();
}

function TestNestedClosureVars() {
  var x = 1;
  function middle() {
    var y = 2;
    function inner() {
      var a = x + y;
      var b = x + y; // Second loads of x and y should be eliminated.
      Assert(a == 3);
      Assert(b == 3);
    }
    inner();
  }
  middle();
}

TestClosureLoadElimination();
TestClosureStoreToLoadForwarding();
TestClosureInvalidationByReassign();
TestClosureInvalidationByCall();
TestNestedClosureVars();

print("lse_closure_vars.js passed");
