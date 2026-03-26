function test_unreachable() {
    if (false) {
        return 1; // unreachable
    }
    return 0;
}

function test_const_branch() {
    let x = true;
    if (x) {
        return 1;
    } else {
        return 0;
    }
}

function test_trampoline() {
    let x = 1;
    while (true) {
        if (x > 0) break;
        x++;
    }
    return x;
}

Assert(test_unreachable() === 0);
Assert(test_const_branch() === 1);
Assert(test_trampoline() === 1);
