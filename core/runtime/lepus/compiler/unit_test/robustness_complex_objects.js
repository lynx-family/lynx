
function complex_objects(obj) {
    obj.a = 1;
    obj.b = 2;
    obj.inner = {
        x: 10,
        y: 20,
        nested: {
            z: 100
        }
    };

    // Redundant loads
    let a1 = obj.a;
    let a2 = obj.a;
    let b1 = obj.b;

    // Mutation and aliasing
    obj.a = a1 + a2 + b1;
    let a3 = obj.a;

    // Nested access
    let x1 = obj.inner.x;
    let y1 = obj.inner.y;
    let z1 = obj.inner.nested.z;

    obj.inner.x = x1 + y1;
    let x2 = obj.inner.x;

    // Array access
    let arr = [1, 2, 3, {val: 4}];
    arr[0] = 10;
    let v1 = arr[0];
    let v2 = arr[3].val;

    return a3 + x2 + z1 + v1 + v2;
}

var obj = {};
var res = complex_objects(obj);
Assert(res == 148);
