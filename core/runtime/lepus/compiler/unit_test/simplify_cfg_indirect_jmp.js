function test_pattern1(x) {
    let a;
    if (x) {
        a = true;
    } else {
        a = false;
    }
    if (a) {
        return 1;
    } else {
        return 0;
    }
}

function test_pattern2(x) {
    let a = true;
    if (x) {
        a = false;
    }
    if (a) {
        return 1;
    } else {
        return 0;
    }
}

Assert(test_pattern1(true) === 1);
Assert(test_pattern1(false) === 0);
Assert(test_pattern2(true) === 0);
Assert(test_pattern2(false) === 1);
