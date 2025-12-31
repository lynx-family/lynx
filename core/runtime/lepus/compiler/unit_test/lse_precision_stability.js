
function test_loop_stability() {
    var obj = {data: 0, count: 0};
    for (var i = 0; i < 10; i++) {
        var c = obj.count; // This load should be optimized after the first iteration
        obj[i] = i * i;    // Numeric write
        obj.count = c + 1; // Explicit property write
    }
    Assert(obj.count == 10);
    Assert(obj[9] == 81);
}

function test_nested_object_stability() {
    var root = {
        child: {prop: "init"}
    };
    var c1 = root.child.prop;
    root[1] = "interference"; // Numeric write on root
    var c2 = root.child.prop; // root.child should still be cached
    Assert(c1 == "init");
    Assert(c2 == "init");
}

function test_string_keys_that_are_numbers() {
    var obj = { "10": "ten" };
    var v1 = obj["10"];
    obj[10] = "TEN"; // Numeric 10 should alias with string "10"
    var v2 = obj["10"]; // This should NOT be eliminated or should reflect the new value
    Assert(v1 == "ten");
    Assert(v2 == "TEN");
}

function test_large_object_stability() {
    var obj = {};
    for (var i = 0; i < 100; i++) {
        obj["prop" + i] = i;
    }
    
    var sum = 0;
    for (var i = 0; i < 100; i++) {
        sum += obj["prop" + i];
    }
    Assert(sum == 4950);
    
    // Mix with numeric writes
    for (var i = 0; i < 50; i++) {
        var p0 = obj.prop0;
        obj[i] = i;
        var p0_again = obj.prop0; // Should be eliminated
        Assert(p0 == 0);
        Assert(p0_again == 0);
    }
}

test_loop_stability();
test_nested_object_stability();
test_string_keys_that_are_numbers();
test_large_object_stability();

print("lse_precision_stability.js passed");
