
// Test Case 1: Object Aliasing (Safe but conservative)
function test_aliasing(obj1, obj2) {
  let v1 = obj1.a;
  obj2.a = 2;
  let v2 = obj1.a; 
  return v1 + v2;
}

// Test Case 2: Join Points
function test_join_points(cond, obj) {
  let v1 = obj.a;
  if (cond) {
    obj.a = 2;
  }
  let v2 = obj.a; 
  return v1 + v2;
}

// Test Case 3: Nested property invalidation
function test_nested(o) {
  let a = o.a;
  let v1 = a.x;
  o.a.x = 2;
  let v2 = a.x; 
  return v1 + v2;
}

// Test Case 4: Context Slots
function test_context_slots() {
  let x = 1;
  function f() {
    let v1 = x;
    let v2 = x; // Should be eliminated
    x = 2;
    let v3 = x; // Should be forwarded to 2
    return v1 + v2 + v3;
  }
  return f();
}

function unique_lse_test_func(o) {
  let v1 = o.a;
  let v2 = o.a;
  return v1 + v2;
}

console.log("aliasing:", test_aliasing({a: 1}, {a: 2}));
console.log("join_points:", test_join_points(true, {a: 1}));
console.log("nested:", test_nested({a: {x: 1}}));
console.log("context_slots:", test_context_slots());
console.log("unique:", unique_lse_test_func({a: 1}));
