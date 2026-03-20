
function test_branch(cond) {
  var obj = {x: 1};
  var a = obj.x;
  if (cond) {
    obj.x = 2;
  }
  var b = obj.x; // Should not be optimized to 'a' because of the potential store in branch
  return b;
}

Assert(test_branch(true) == 2);
Assert(test_branch(false) == 1);

function test_dominance() {
  var obj = {x: 100};
  var a = obj.x;
  if (true) {
    var b = obj.x; // CAN be optimized to 'a' because no store happened yet
    Assert(b == 100);
  }
  return a;
}

test_dominance();
print("lse_branch.js passed");
