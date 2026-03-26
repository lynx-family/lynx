
function test_precision_identifier_vs_number() {
    var obj = {prop: 100, 1: 200};
    var a = obj.prop;
    obj[1] = 300;
    var b = obj.prop; // Should be 100, LSE should eliminate this load
    Assert(a == 100);
    Assert(b == 100);
    Assert(obj[1] == 300);
}

function test_precision_identifier_vs_variable_number() {
    var obj = {prop: "hello"};
    var i = 0;
    var a = obj.prop;
    obj[i] = "world"; // i is a number, should not invalidate obj.prop
    var b = obj.prop;
    Assert(a == "hello");
    Assert(b == "hello");
    Assert(obj[0] == "world");
}

function test_precision_numeric_string_aliases_number() {
    var obj = {1: "a"};
    var a = obj["1"];
    obj[1] = "b";
    var b = obj["1"]; // "1" and 1 alias, so this load should NOT be eliminated if it was redundant (but here it changes)
    Assert(a == "a");
    Assert(b == "b");
}

function test_precision_different_identifiers() {
    var obj = {x: 1, y: 2};
    var x1 = obj.x;
    obj.y = 3;
    var x2 = obj.x; // Should be eliminated
    Assert(x1 == 1);
    Assert(x2 == 1);
    Assert(obj.y == 3);
}

function test_precision_complex_mix() {
    var obj = {a: 1, b: 2, 0: 3};
    var r1 = obj.a;
    var r2 = obj.b;
    var r3 = obj[0];
    
    obj[0] = 30; // invalidates obj[0], but not obj.a or obj.b
    obj.a = 10;  // invalidates obj.a, but not obj.b or obj[0]
    
    var r4 = obj.b; // Should be eliminated
    Assert(r1 == 1);
    Assert(r2 == 2);
    Assert(r3 == 3);
    Assert(obj[0] == 30);
    Assert(obj.a == 10);
    Assert(r4 == 2);
}

test_precision_identifier_vs_number();
test_precision_identifier_vs_variable_number();
test_precision_numeric_string_aliases_number();
test_precision_different_identifiers();
test_precision_complex_mix();

print("lse_precision.js passed");
