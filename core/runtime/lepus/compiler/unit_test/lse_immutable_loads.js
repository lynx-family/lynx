
var global_x = 100;

function test_immutable_loads_across_calls() {
    var a = global_x;
    var b = Math.abs(-1);
    var c = 42;
    
    // Side effect: Call
    print("intervening call");
    
    var a2 = global_x;    // Should use cache
    var b2 = Math.abs(-1); // Should use cache
    var c2 = 42;           // Should use cache
    
    Assert(a == 100);
    Assert(a2 == 100);
    Assert(b == 1);
    Assert(b2 == 1);
    Assert(c == 42);
    Assert(c2 == 42);
}

function test_immutable_loads_across_stores() {
    var obj = {p: 1};
    var g1 = global_x;
    var m1 = Math.max(1, 2);
    
    // Side effect: SetTable
    obj.p = 2;
    obj[0] = 3;
    
    var g2 = global_x;    // Should use cache
    var m2 = Math.max(1, 2); // Should use cache
    
    Assert(g1 == 100);
    Assert(g2 == 100);
    Assert(m1 == 2);
    Assert(m2 == 2);
}

test_immutable_loads_across_calls();
test_immutable_loads_across_stores();

print("lse_immutable_loads.js passed");
