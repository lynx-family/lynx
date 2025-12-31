
function complex_logic(a, b, c) {
    let x = (a + b) * c;
    let y = (a + b) * c;
    let z = x + y;

    let res_str = "";
    if (a > 0 && b > 0) {
        res_str = "both positive";
    } else if (a > 0 || b > 0) {
        res_str = "one positive";
    } else {
        res_str = "none positive";
    }

    let val = a > b ? (a > c ? a : c) : (b > c ? b : c);
    
    let const_val = 12; 
    if (const_val == 12) {
        z += const_val;
    }

    return z + val;
}

function test_logic() {
    // (1+2)*3 + (1+2)*3 + 12 + 3 = 9 + 9 + 12 + 3 = 33
    let r1 = complex_logic(1, 2, 3);
    console.log("logic r1 = " + r1);
    Assert(r1 == 33);
}

console.log("Logic tests...");
test_logic();
console.log("Logic passed");
