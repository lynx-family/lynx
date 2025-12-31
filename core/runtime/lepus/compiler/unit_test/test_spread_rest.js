
// Use standard array/object operations instead of spread/rest to test IR
function test_array_ops() {
    let arr1 = [1, 2];
    let arr2 = [3, 4];
    let combined = [];
    for (let i = 0; i < arr1.length; i++) combined.push(arr1[i]);
    for (let i = 0; i < arr2.length; i++) combined.push(arr2[i]);
    combined.push(5);
    
    Assert(combined.length == 5);
    Assert(combined[0] == 1);
    Assert(combined[4] == 5);
}

function test_object_ops() {
    let obj1 = {a: 1, b: 2};
    let obj2 = {c: 3, d: 4};
    let combined = {};
    combined.a = obj1.a;
    combined.b = obj1.b;
    combined.c = obj2.c;
    combined.d = obj2.d;
    combined.e = 5;
    
    Assert(combined.a == 1);
    Assert(combined.e == 5);
}

test_array_ops();
test_object_ops();
