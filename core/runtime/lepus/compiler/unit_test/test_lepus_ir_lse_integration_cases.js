
var global_v = "global";

function TestLseIntegrationIsolation() {
  var closure_v = "closure";
  var obj = { prop: "table" };

  function Inner() {
    // Initial loads
    var g1 = global_v;
    var c1 = closure_v;
    var t1 = obj.prop;

    // 1) Modify global: should not invalidate closure and table.
    global_v = "global_new";
    var c2 = closure_v;
    var t2 = obj.prop;
    Assert(c2 == "closure"); // Redundant load; should use cached value.
    Assert(t2 == "table"); // Redundant load; should use cached value.

    // 2) Modify closure: should not invalidate global and table.
    closure_v = "closure_new";
    var g2 = global_v;
    var t3 = obj.prop;
    Assert(g2 == "global_new"); // Redundant load; should use cached value.
    Assert(t3 == "table"); // Redundant load; should use cached value.

    // 3) Modify table: should not invalidate global and closure.
    obj.prop = "table_new";
    var g3 = global_v;
    var c3 = closure_v;
    Assert(g3 == "global_new"); // Redundant load; should use cached value.
    Assert(c3 == "closure_new"); // Redundant load; should use cached value.

    Assert(global_v == "global_new");
    Assert(closure_v == "closure_new");
    Assert(obj.prop == "table_new");
  }
  Inner();
}

function TestLseSameIndexDifferentNamespaces() {
  // Verify there is no interference if a Global index equals a ToplevelClosure register index.
  // In Lepus, global indices usually start from 0, and closure registers can also start from 0.

  var local_closure = "C"; // Assume it is in toplevel scope.
  global_v = "G";

  function Inner() {
    var g = global_v;
    var c = local_closure;

    global_v = "G2";
    var c2 = local_closure; // Must remain "C" and not be affected by global update.
    Assert(c2 == "C");

    local_closure = "C2";
    var g2 = global_v; // Must remain "G2" and not be affected by closure update.
    Assert(g2 == "G2");
  }
  Inner();
}

TestLseIntegrationIsolation();
TestLseSameIndexDifferentNamespaces();

print("lse_integration_cases.js passed");
