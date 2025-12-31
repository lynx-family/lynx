function test_closure_join(x) {
    let a = 1;
    function inner() {
        let b;
        if (x) {
            b = a;
        } else {
            b = 0;
        }
        return b + a;
    }
    return inner();
}

Assert(test_closure_join(true) === 2);
Assert(test_closure_join(false) === 1);
