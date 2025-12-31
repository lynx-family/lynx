
function test_lse_loop_aliasing() {
    var obj = { a: 1, b: 2 };
    var sum = 0;
    for (var i = 0; i < 5; i++) {
        sum += obj.a;
        obj.b += i;
    }
    if (sum != 5) { print("Fail sum: ", sum); Assert(false); }
    if (obj.b != 12) { print("Fail obj.b: ", obj.b); Assert(false); }

    sum = 0;
    for (var j = 0; j < 5; j++) {
        sum += obj.a;
        obj["a"] = j + 10;
    }
    if (sum != 47) { print("Fail sum loop 2: ", sum); Assert(false); }
}

function test_lse_diamond_join() {
    var obj = { x: 100 };
    var cond = true;
    
    var val1 = obj.x;
    if (cond) {
        obj.x = 200;
    } else {
        obj.x = 300;
    }
    var val2 = obj.x;
    if (val2 != 200) { print("Fail val2: ", val2); Assert(false); }
}

function test_lse_object_aliasing() {
    var obj1 = { v: 1 };
    var obj2 = { v: 2 };
    
    var r1 = obj1.v;
    obj2.v = 10;
    var r2 = obj1.v; 
    if (r1 != 1) { print("Fail r1: ", r1); Assert(false); }
    if (r2 != 1) { print("Fail r2: ", r2); Assert(false); }
}

function test_lse_mixed_types() {
    var table = {};
    table[123] = "number";
    if (table["123"] != "number") { print("Fail mixed 1"); Assert(false); }
    
    table["123"] = "string";
    if (table[123] != "string") { print("Fail mixed 2"); Assert(false); }
}

function test_lse_side_effects() {
    var obj = { data: 10 };
    var v1 = obj.data;
    var dummy = Math.abs(-5);
    var v2 = obj.data; 
    if (v1 != 10 || v2 != 10) { print("Fail side effects 1"); Assert(false); }
    
    var arr = [1];
    arr.push(2);
    var v3 = obj.data;
    if (v3 != 10) { print("Fail side effects 2"); Assert(false); }
}

function test_lse_optional_chaining_complex() {
    var root = {
        level1: {
            level2: {
                val: 42
            }
        }
    };
    
    var a = root.level1.level2.val;
    var b = root?.level1?.level2?.val;
    if (a != 42 || b != 42) { print("Fail optional 1"); Assert(false); }
}

var toplevel_var = 10;
function test_lse_toplevel() {
    toplevel_var = 20;
    var a = toplevel_var;
    toplevel_var = 30;
    var b = toplevel_var;
    if (a != 20 || b != 30) { print("Fail toplevel: ", a, b); Assert(false); }
}

test_lse_loop_aliasing();
test_lse_diamond_join();
test_lse_object_aliasing();
test_lse_mixed_types();
test_lse_side_effects();
test_lse_optional_chaining_complex();
test_lse_toplevel();
print("lse_comprehensive_test.js passed");
