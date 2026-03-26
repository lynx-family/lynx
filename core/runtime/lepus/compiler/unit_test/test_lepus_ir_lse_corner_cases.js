function test_lse_aliasing() {
    let obj = {};
    obj.a = 1;
    obj.b = 2;
    let x = obj.a; 
    obj.b = 3;     
    let y = obj.a; 
    return x + y;
}

function test_lse_dynamic(key) {
    let obj = {};
    obj.a = 1;
    obj[key] = 2; 
    let y = obj.a; 
    return y;
}

Assert(test_lse_aliasing() === 2);
Assert(test_lse_dynamic("a") === 2);
Assert(test_lse_dynamic("b") === 1);
