
function test_basic_promotion() {
  let obj = {a: 1, b: 2};
  let sum1 = obj.a + obj.a; // Second obj.a should be promoted
  Assert(sum1 === 2);
}

function test_store_to_load() {
  let obj = {a: 0};
  obj.a = 42;
  let val = obj.a; // Should be promoted to 42
  Assert(val === 42);
}

function test_invalidation_by_call() {
  let obj = {a: 1};
  let sum = 0;
  function modify(o) {
    o.a = 100;
  }
  
  let v1 = obj.a;
  modify(obj); // This call must invalidate the cache for obj.a
  let v2 = obj.a;
  Assert(v1 === 1);
  Assert(v2 === 100);
}

function test_refined_invalidation() {
  let obj = {a: 1, b: 2};
  let v1 = obj.a;
  obj.b = 20; // This should NOT invalidate cache for obj.a
  let v2 = obj.a; // Should be promoted
  Assert(v1 === 1);
  Assert(v2 === 1);
  Assert(obj.b === 20);
}

function test_cross_block() {
  let obj = {a: 10};
  let v = 0;
  if (true) {
    v = obj.a;
  } else {
    v = obj.a;
  }
  Assert(v === 10);
  
  // After if-else join point, cache is cleared.
  let v2 = obj.a;
  Assert(v2 === 10);
}

function test_array_promotion() {
  let arr = [10, 20, 30];
  let v1 = arr[0];
  let v2 = arr[0]; // Should be promoted
  Assert(v1 === 10);
  Assert(v2 === 10);
  
  arr[1] = 200;
  Assert(arr[1] === 200);
}

function test_aliasing_safety() {
  let obj1 = {a: 1};
  let obj2 = obj1; // Aliasing
  
  let v1 = obj1.a;
  obj2.a = 42; // Modifying via alias
  let v2 = obj1.a; // Should see 42, NOT promoted to 1
  Assert(v1 === 1);
  Assert(v2 === 42);
}

test_basic_promotion();
test_store_to_load();
test_invalidation_by_call();
test_refined_invalidation();
test_cross_block();
test_array_promotion();
test_aliasing_safety();
