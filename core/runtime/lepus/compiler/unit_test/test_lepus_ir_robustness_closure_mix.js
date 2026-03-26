
let top_var1 = 100;
let top_var2 = 200;

function closure_mix(outer_param) {
    let captured1 = outer_param + top_var1;
    let captured2 = top_var2;

    function inner1(inner_param) {
        let local1 = captured1 + captured2 + inner_param;
        top_var1 = local1; // Write to top-level
        
        function inner2() {
            return captured1 + captured2 + top_var1 + top_var2;
        }
        
        return inner2;
    }

    if (outer_param > 50) {
        captured1 *= 2;
    }

    return inner1;
}

function test_top_level_updates() {
    let f1 = closure_mix(10);
    let f2 = f1(5);
    return f2();
}

var res = test_top_level_updates();
Assert(res == 825);
