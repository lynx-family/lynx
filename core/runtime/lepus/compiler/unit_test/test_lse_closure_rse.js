
function test_redundant_context_store() {
    let x = 10;
    function inner1() {
        x = 20;
        x = 20; // Should be deleted
    }
    inner1();
    return x;
}

function test_context_store_safety_across_call() {
    let x = 10;
    function inner2() {
        x = 20;
        print(x); // Side effect
        x = 20; // Should NOT be deleted
    }
    inner2();
    return x;
}

function test_context_read_then_write_back() {
    let x = 10;
    function inner3() {
        let temp = x;
        x = temp; // Should be deleted
    }
    inner3();
    return x;
}

function test_outer_read_inner_write() {
    let x = 1;
    function inner4() {
        x = 2; // Should NOT be deleted
    }
    inner4();
    return x;
}

function test_no_call_load_elimination() {
    let x = 10;
    x = 20; // SetContextSlot
    return x; // GetContextSlot should be eliminated
}

console.log(test_redundant_context_store());
console.log(test_context_store_safety_across_call());
console.log(test_context_read_then_write_back());
console.log(test_outer_read_inner_write());
console.log(test_no_call_load_elimination());
