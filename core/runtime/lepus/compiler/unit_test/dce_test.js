function test_dce_basic() {
    let a = 1;
    let b = 2;
    let c = a + b; // dead
    return 0;
}

function test_dce_table() {
    let t = {};
    t.a = 1; // dead if t is dead
    return 0;
}

function test_dce_array() {
    let a = [1, 2, 3];
    a[0] = 4; // dead if a is dead
    return 0;
}

Assert(test_dce_basic() === 0);
Assert(test_dce_table() === 0);
Assert(test_dce_array() === 0);
