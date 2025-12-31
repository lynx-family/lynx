
function test_nested_lse() {
    let obj = {a: {b: 1, c: 2}, d: 3};
    let sum = 0;
    for (let i = 0; i < 100; i++) {
        sum += obj.a.b;
        sum += obj.a.c;
        if (i % 10 === 0) {
            obj.d = i;
        }
        sum += obj.a.b; 
    }
    return sum;
}

function test_array_object_alias() {
    let arr = [1, 2, 3];
    let o = {0: 10, 1: 20};
    let sum = 0;
    for (let i = 0; i < 10; i++) {
        sum += arr[0];
        o[0] = i; 
        sum += arr[0]; 
    }
    return sum;
}

function test_optional_chaining_side_effect() {
    let o = {a: {b: 1}};
    let sum = 0;
    function get_val() {
        sum += 1;
        return o;
    }
    for (let i = 0; i < 10; i++) {
        let val1 = o.a.b;
        let v = get_val();
        let val2 = v ? v.a.b : undefined;
        let val3 = o.a.b; // Should NOT be eliminated if get_val() could have changed 'o'
        sum += val1 + val2 + val3;
    }
    return sum;
}

function test_loop_with_branch_lse() {
    let o = {x: 1};
    let sum = 0;
    for (let i = 0; i < 100; i++) {
        sum += o.x;
        if (i > 50) {
            o.x = i;
        } else {
            // No store here
        }
        sum += o.x; 
    }
    return sum;
}

let r1 = test_nested_lse();
console.log("r1: " + r1);
let r2 = test_array_object_alias();
console.log("r2: " + r2);
let r3 = test_optional_chaining_side_effect();
console.log("r3: " + r3);
let r4 = test_loop_with_branch_lse();
console.log("r4: " + r4);

export const data = {
    test_nested_lse: r1,
    test_array_object_alias: r2,
    test_optional_chaining_side_effect: r3,
    test_loop_with_branch_lse: r4
};
