
var global_val = 1;
function complex() {
  var obj = {a: {b: {c: 10}}};
  var v1 = obj.a.b.c;
  var v2 = obj.a.b.c;
  
  Assert(v1 == v2);
  
  function nested() {
    global_val = 2;
    return global_val;
  }
  
  var g1 = global_val;
  nested();
  var g2 = global_val; // Should not be g1 because nested() changed it
  Assert(!(g1 == g2 && g1 == 1));
  
  return v1 + g2;
}

Assert(complex() == 12);
print("lse_complex.js passed");
