
function test_loop() {
  var obj = {count: 0};
  for (var i = 0; i < 10; i++) {
    obj.count = obj.count + 1;
  }
  return obj.count;
}

Assert(test_loop() == 10);

function test_loop_invariant() {
  var obj = {val: 42};
  var sum = 0;
  for (var i = 0; i < 5; i++) {
    sum += obj.val; // obj.val is invariant here
  }
  return sum;
}

Assert(test_loop_invariant() == 210);
print("lse_loop.js passed");
