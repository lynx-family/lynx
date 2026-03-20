function test_eq_cond_branch(x, y) {
    if (x === y) {
        return 1;
    } else {
        return 0;
    }
}

function test_neq_cond_branch(x, y) {
    if (x !== y) {
        return 1;
    } else {
        return 0;
    }
}

Assert(test_eq_cond_branch(1, 1) === 1);
Assert(test_eq_cond_branch(1, 2) === 0);
Assert(test_neq_cond_branch(1, 1) === 0);
Assert(test_neq_cond_branch(1, 2) === 1);
