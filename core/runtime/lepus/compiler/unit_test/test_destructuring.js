
// Use explicit access instead of destructuring to test LSE
function test_object_access() {
    let obj = {a: 1, b: 2, c: {d: 3, e: 4}};
    let a = obj.a;
    let b = obj.b;
    Assert(a == 1);
    Assert(b == 2);

    let d = obj.c.d;
    let e = obj.c.e;
    Assert(d == 3);
    Assert(e == 4);
}

function test_array_access() {
    let arr = [1, 2, [3, 4]];
    let x = arr[0];
    let y = arr[1];
    Assert(x == 1);
    Assert(y == 2);

    let inner = arr[2];
    let z = inner[0];
    let w = inner[1];
    Assert(z == 3);
    Assert(w == 4);
}

test_object_access();
test_array_access();
