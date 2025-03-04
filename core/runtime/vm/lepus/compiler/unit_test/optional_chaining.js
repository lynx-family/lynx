let adventurer = {
  name: 'Alice',
  cat: {
    name: 'Dinah'
  }
};

let dogName = adventurer.dog?.name;
Assert(dogName == undefined);
// console.log(dogName);
// expected output: undefined

Assert(adventurer.someNonExistentMethod?.() == undefined);
// console.log(adventurer.someNonExistentMethod?.());

// let object = {};
// object?.property = 1;

let potentiallyNullObj = null;
let x = 0;
let prop = potentiallyNullObj?.[x++];
// console.log(x); // x will not be incremented and will still output 0.
Assert(x == 0);


function a() {
  let b = {
    pro1 : 1,
    pro2 : {
      foo1 : "foo1",
      foo2 : null,
      foo3 : 12.3,
    },
  }

  return b;
}

Assert(a?.() == {pro1:1,pro2:{foo1:'foo1',foo2:null,foo3:12.3}});
Assert(a?.()?.pro1 == 1);
Assert(a?.()?.pro3?.foo1 == undefined);
Assert(a?.()?.pro2?.foo1 == 'foo1');
Assert(a?.()?.pro2?.foo4 == undefined);


let arr = [1,2,3];
Assert(arr?.[1] == 2);
Assert([1, 2, 3]?.[1] == 2)

let obj = {
  pro1: 1,
  pro2: {
    foo1:'foo1',
    foo2:null,
    foo3:12.3
  }
};

Assert(obj?.["pro1"] == 1);
Assert(obj?.["pro2"]?.["foo1"] == 'foo1');