
function add(a, b) {
    return a + b;
}

function sub(a, b) {
    return a - b;
}

function complex_call(a, b, c, d, e, f) {
    let r1 = add(a, b);
    let r2 = sub(c, d);
    let r3 = add(e, f);
    return add(r1, add(r2, r3));
}

// Recursive call
function fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

// Calls in loop
function loop_calls(arr) {
    let sum = 0;
    for (let i = 0; i < arr.length; i++) {
        sum = add(sum, arr[i]);
    }
    return sum;
}

// Higher order functions
function apply(f, x, y) {
    return f(x, y);
}

function test_hoc() {
    return apply(add, 10, 20) + apply(sub, 100, 50);
}

// Many arguments to force register usage
function many_args(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) {
    return a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10;
}

function test_many_args() {
    return many_args(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}

// Interleaved logic
function interleaved(a, b) {
    let f = add;
    let x = a * 2;
    let y = b * 3;
    // Some instructions between getting 'f' and calling it
    let result = f(x, y);
    return result;
}

// Trigger compilation and dump
Assert(complex_call(1, 2, 3, 4, 5, 6) == 13);
Assert(fib(5) == 5);
Assert(loop_calls([1, 2, 3]) == 6);
Assert(test_hoc() == 80);
Assert(test_many_args() == 55);
Assert(interleaved(10, 20) == 80);
