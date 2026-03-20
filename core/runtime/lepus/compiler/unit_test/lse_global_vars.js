
var global_a = 1;
var global_b = 2;

function TestGlobalLoadElimination() {
  var x = global_a;
  var y = global_a; // Redundant load; should be eliminated.
  Assert(x == 1);
  Assert(y == 1);
}

function TestGlobalStoreToLoadForwarding() {
  global_a = 10;
  var x = global_a; // Should use 10 directly.
  Assert(x == 10);
}

function TestGlobalInvalidationByCall() {
  global_b = 20;
  function modifier() {
    global_b = 30;
  }
  var x = global_b;
  modifier(); // Call barrier
  var y = global_b;
  Assert(x == 20);
  Assert(y == 30);
}

function TestGlobalVsLocalShadowing() {
  var global_a = "local"; // Shadowing
  var x = global_a;
  var y = global_a; // Redundant local load; should be eliminated.
  Assert(x == "local");
  Assert(y == "local");
}

function TestMultipleGlobalsIndependence() {
  global_a = "A";
  global_b = "B";

  var r1 = global_a;
  global_b = "CHANGED_B"; // Should not invalidate cached global_a.
  var r2 = global_a;

  Assert(r1 == "A");
  Assert(r2 == "A");
  Assert(global_b == "CHANGED_B");
}

TestGlobalLoadElimination();
TestGlobalStoreToLoadForwarding();
TestGlobalInvalidationByCall();
TestGlobalVsLocalShadowing();
TestMultipleGlobalsIndependence();

print("lse_global_vars.js passed");
