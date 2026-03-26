
function test_lse_complex_loops() {
    var obj = {a: 1, b: 2};
    for (var i = 0; i < 5; i++) {
        var a1 = obj.a;
        for (var j = 0; j < 5; j++) {
            var b1 = obj.b;
            if (i == j) {
                obj.a = i + j;
            }
            var b2 = obj.b;
            Assert(b1 == 2);
            Assert(b2 == 2);
        }
    }
}

function test_lse_switch_case_if() {
    var obj = {prop: "val"};
    var x = 1;
    var res = "";
    var p1 = obj.prop;
    if (x == 1) {
        res = obj.prop;
    } else {
        res = "other";
    }
    Assert(p1 == "val");
    Assert(res == "val");
}

function test_lse_multiple_objects_alias() {
    var obj1 = {x: 10};
    var obj2 = {x: 20};
    
    var v1 = obj1.x;
    var v2 = obj2.x;
    
    obj1.x = 100;
    
    var v3 = obj2.x;
    Assert(v1 == 10);
    Assert(v2 == 20);
    Assert(v3 == 20);
    Assert(obj1.x == 100);
}

var global_x = "initial";
function test_lse_global_and_table() {
    global_x = "global_val";
    var obj = {p: "table_val"};
    
    var g1 = global_x;
    var t1 = obj.p;
    
    global_x = "changed";
    
    var t2 = obj.p;
    Assert(g1 == "global_val");
    Assert(t1 == "table_val");
    Assert(t2 == "table_val");
    Assert(global_x == "changed");
}

function outer_test() {
    var closure_v = 42;
    function inner_test() {
        var obj = {a: 1};
        var c1 = closure_v;
        var a1 = obj.a;
        
        closure_v = 100;
        
        var a2 = obj.a;
        Assert(c1 == 42);
        Assert(a1 == 1);
        Assert(a2 == 1);
        Assert(closure_v == 100);
    }
    inner_test();
}

test_lse_complex_loops();
test_lse_switch_case_if();
test_lse_multiple_objects_alias();
test_lse_global_and_table();
outer_test();

print("lse_complex_cases.js passed");
