
function test_funcs() {
    function add(a, b) { return a + b; }
    function square(x) { return x * x; }
    Assert(add(2, 3) == 5);
    Assert(square(4) == 16);
}

function test_exponentiation() {
    Assert(2 ** 3 == 8);
    let x = 2;
    x **= 4;
    Assert(x == 16);
}

function test_chaining_coalescing() {
    let obj = { a: { b: 0 } };
    let val1 = obj.a?.b ?? 10;
    Assert(val1 == 0);
    let val2 = obj.c?.d ?? 20;
    Assert(val2 == 20);
}

console.log("ES6 Features tests...");
test_funcs();
test_exponentiation();
test_chaining_coalescing();
console.log("ES6 Features passed");
