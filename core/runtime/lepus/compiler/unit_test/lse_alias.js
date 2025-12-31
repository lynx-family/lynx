
var obj = {x: 1};
var alias = obj;
obj.x = 2;
Assert(alias.x == 2);

function test_alias() {
  var a = {v: 10};
  var b = a;
  a.v = 20;
  var c = b.v;
  return c; // Should be 20, not 10
}

Assert(test_alias() == 20);

function test_lse_aliasing_number_string() {
    var obj = {1: "a", "2": "b"};
    
    // Test 1: Number store invalidates string load
    var a = obj["1"];
    obj[1] = "new_a";
    var b = obj["1"];
    Assert(a == "a");
    Assert(b == "new_a");
    
    // Test 2: String store invalidates number load
    var c = obj[2];
    obj["2"] = "new_b";
    var d = obj[2];
    Assert(c == "b");
    Assert(d == "new_b");
}

function test_lse_side_effects_optional_chaining() {
    var obj = {
        a: 1,
        b: {
            c: 2
        },
        get_val: function() {
            return 10;
        }
    };

     function set_val() {
        obj.a = 100;
    };
    
    // Scenario 1: Optional call with side effect
    var r1 = obj.a;
    var dummy = obj.get_val?.(); 
    var r2 = obj.a; 
    Assert(r1 == 1);
    Assert(r2 == 1);
    set_val();
    var r3 = obj.a
    Assert(r3 == 100);

    
    // Scenario 2: Deep optional chaining
    var obj2 = {x: 1, y: {z: 2}};
    var x1 = obj2.x;
    var z = obj2?.y?.z;
    var x2 = obj2.x; // Should be eliminated to 1 (pure reads)
    Assert(x1 == 1);
    Assert(x2 == 1);
}

test_lse_aliasing_number_string();
test_lse_side_effects_optional_chaining();

print("lse_alias.js passed");
