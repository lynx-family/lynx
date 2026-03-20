function test_cse_basic(x, y) {
    let a = x + y;
    let b = x + y; // CSE
    return a + b;
}

function test_cse_dominated(cond, x, y) {
    let a = x + y;
    if (cond) {
        let b = x + y; // CSE from dominator
        return a + b;
    }
    return a;
}

Assert(test_cse_basic(1, 2) === 6);
Assert(test_cse_dominated(true, 1, 2) === 6);
Assert(test_cse_dominated(false, 1, 2) === 3);
