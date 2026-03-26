
function test_cse_const() {
    let a = 42;
    let b = 42;
    let c = 42;
    return a + b + c;
}

function test_cse_builtin() {
    let a = Math.abs(-1);
    let b = Math.abs(-2);
    let c = Math.abs(-3);
    return a + b + c;
}

function test_unused_closure() {
    function unused() {
        return 1;
    }
    return 2;
}

console.log(test_cse_const());
console.log(test_cse_builtin());
console.log(test_unused_closure());
