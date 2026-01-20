let target = { a: 1, b: 2 };
let source = { b: 4, c: 5 };

let returnedTarget = Object.assign(target, source);
Assert(target == { a: 1, b: 4, c: 5 });
Assert(returnedTarget == { a: 1, b: 4, c: 5 });

let returnTarget2 = Object.assign(target);
Assert(returnTarget2 == { a: 1, b: 4, c: 5 });

let obj2 = { a: 1 };
let copy = Object.assign({}, obj2);
Assert(copy == { a: 1 });

function test() {
  let obj1 = { a: 0, b: { c: 0 } };
  let obj2 = Object.assign({}, obj1);
  Assert(obj2 == { a: 0, b: { c: 0 } });

  obj1.a = 1;
  Assert(obj1 == { a: 1, b: { c: 0 } });
  // { a: 1, b: { c: 0}}
  Assert(obj2 == { a: 0, b: { c: 0 } });

  obj2.a = 2;
  Assert(obj1 == { a: 1, b: { c: 0 } });
  // { a: 1, b: { c: 0}}
  Assert(obj2 == { a: 2, b: { c: 0 } });

  obj2.b.c = 3;
  Assert(obj1 == { a: 1, b: { c: 3 } });
  // { a: 1, b: { c: 3}}
  Assert(obj2 == { a: 2, b: { c: 3 } });

  obj1 = { a: 0, b: { c: 0 } };
  let obj3 = JSON.parse(JSON.stringify(obj1));
  obj1.a = 4;
  obj1.b.c = 4;
  Assert(obj3 == { a: 0, b: { c: 0 } });
}
test();

let o1 = { a: 1 };
let o2 = { b: 2 };
let o3 = { c: 3 };

let obj = Object.assign(o1, o2, o3);
Assert(obj == { a: 1, b: 2, c: 3 }); // { a: 1, b: 2, c: 3 }
Assert(o1 == { a: 1, b: 2, c: 3 }); // Note that the target object itself will also change.

o1 = { a: 1, b: 1, c: 1 };
o2 = { b: 2, c: 2 };
o3 = { c: 3 };

obj = Object.assign({}, o1, o2, o3);
Assert(obj == { a: 1, b: 2, c: 3 }); // { a: 1, b: 2, c: 3 }


let array1 = [{a: 1, b: 2}, 1];
let array2 = Object.assign([], array1);
Assert(array2 == [{a:1,b:2},1]);
let array3 = [{a: 1, b: 2}, 1, [1,2,3]];
let array4 = Object.assign([], array3);
Assert(array4 == [{a:1,b:2},1,[1,2,3]]);
let array5 = [{a: 1, b: 2}, 1, [1,2,3]];
let array6 = Object.assign([1,3,4], array3);
Assert(array6 == [{a:1,b:2},1,[1,2,3]]);
array6.push({a: 1});
Assert(array6 == [{a:1,b:2},1,[1,2,3],{a:1}]);
Assert(array5 == [{a:1,b:2},1,[1,2,3]]);
let array7 = [1];
let array8 = Object.assign([], array1, array7);
Assert(array8 == [1,1]);
