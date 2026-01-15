function test_closure() {
  let a = 1;
  function test1_closure() {
    let b = 2;
    a++;
    return a;
  }
  a++;
  return test1_closure();
}

let global2 = test_closure;
Assert(global2() == 3);

function closure_test2() {
  let a = 1;
  function closure_test21() {
    let b = 2;
    a++;
    return a;
  }
  a += 4;
  return closure_test21();
}

let global2_test = closure_test2;
Assert(global2_test() == 6);

let exports1 = {};
(function(exports) {
  function func(a, b) {
    return a + b;
  }
  exports.divide = function(a, b) {
    return func(a, b);
  };
})(exports1);
Assert(exports1.divide(4, 2) == 6);

function function1() {
  let a = 1;
  function function2() {
    let b = 10;
    function function3() {
      a++;
      b++;
      return a + b;
    }
    return function3;
  }
  a = 4;
  return function2();
}
let result1 = function1();
Assert(result1() == 16);
Assert(result1() == 18);

function function11() {
  let a = [1, 2, 3];
  function function21() {
    a.push(5);
    return a;
  }
  return function21;
}
let result2 = function11();
Assert(result2() == [1, 2, 3, 5]);
Assert(result2() == [1, 2, 3, 5, 5]);

function my() {
  function add(a, b) {
    return a + b;
  }
  function sum(a, b) {
    return add(a, b);
  }
  return sum(1, 2);
}

let result = my();
Assert(result == 3);

function function12() {
  let a = 1;
  let c = 7;
  function function22() {
    let b = 2;
    let a = 3;
    c++;
    a++;
    Assert(c == 8);
    function function32() {
      b++;
      a++;
      return a;
    }
    a = 3;
    c = 9;
    Assert(c == 9);
    return function32;
  }
  a++;
  Assert(a == 2);
  return function22();
}
let result12 = function12();
let resulta = result12();
let resultb = result12();
Assert(resulta == 4);
Assert(resultb == 5);

function makeAdder(x) {
  return function(y) {
    return x + y;
  };
}

let add5 = makeAdder(5);
let add10 = makeAdder(10);

Assert(add5(2) == 7);
Assert(add10(2) == 12);


function test_closure_block() {
  let a = 1;
  {
    let a = 3;
    function test_closure_block3() {
      a++;
      Assert(a == 4);
    }
    test_closure_block3();
  }
  {
    function test_closure_block2() {
      Assert(a == 1);
    }
    test_closure_block2();
  }
}
test_closure_block();


function test_closure_block2(function2) {
  let a = function2();
  Assert(a == 2);
}
function test_closure_block3() {
  {
    let a = 1;
    {
      let a = 3;
    }
    {
      test_closure_block2(function() {
        a++;
        return a;
      });
    }
  }
}
test_closure_block3();

// -------------------------------------------------------
// for engien version >= 2.6
let test_closure_block4 = [];
let test_closure_array = [0,1,2,3];
for(let idx = 0; idx < test_closure_array.length; idx++) {
    let itemName = test_closure_array[idx];
    test_closure_block4.push(function (num) {
	  let tmp = itemName;
	  Assert(tmp == num);
    })
}

test_closure_block4[0](0);
test_closure_block4[1](1);
test_closure_block4[2](2);
test_closure_block4[3](3);


let test_closure_block5 = [];
let test_closure_array1 = [0,1];
for(let idx = 0; idx < test_closure_array1.length; idx++) {
    let itemName = test_closure_array1[idx];
    test_closure_block5.push(function (num) {
	Assert(num == itemName);
	Assert(num == idx);
    })
}

test_closure_block5[0](0);
test_closure_block5[1](1);



let closure_test_data = [];
let closure_test_a = [0,1,2,3,4,5,6,7]
let closure_test_b = [1,2,3,4,5,6,7]
for(let idx = 0; idx < closure_test_a.length; idx++) {
    let itemName = closure_test_a[idx];
    closure_test_data.push(function (val1, val2) {
        Assert(val1 == itemName);
        idx++;
        Assert(val2 == idx);
    })

    {
        let res = closure_test_b[idx];
        idx++;
        Assert(res == idx);
    }
}
closure_test_data[0](0,2);
closure_test_data[1](2,4);
closure_test_data[2](4,6);


let test_closure_block6 = [];
function test_block6(num) {
    let test_closure_array = [0,1,2,3,4,5,7];
    for(let idx = 0; idx < test_closure_array.length; idx++) {
        let itemName = test_closure_array[idx];
        test_closure_block6.push(function (num) {
        let tmp = itemName;
        Assert(tmp == num);
        })
        if(idx  < num) continue;
    }
}

test_block6(2);
test_block6(4);
test_closure_block6[0](0);
test_closure_block6[1](1);
test_closure_block6[2](2);
test_closure_block6[3](3);
test_closure_block6[0](0);
test_closure_block6[1](1);
test_closure_block6[2](2);
test_closure_block6[3](3);


let test_closure_block7 = [];
let test_closure_array7 = [0,1,2,3];
let idx7 = 0;
while(idx7 < test_closure_array7.length) {
    let itemName = test_closure_array7[idx7];
    test_closure_block7.push(function (num) {
	  let tmp = itemName;
	  Assert(tmp == num);
    });
    idx7++;
}
test_closure_block7[0](0);
test_closure_block7[1](1);
test_closure_block7[2](2);
test_closure_block7[3](3);


let test_closure_block8 = [];
let test_closure_array8 = [0,1,2,3];
let idx8 = 0;
do {
    let itemName = test_closure_array8[idx8];
    test_closure_block8.push(function (num) {
	  let tmp = itemName;
	  Assert(tmp == num);
    });
    idx8++;
}while(idx8 < test_closure_array8.length);
test_closure_block8[0](0);
test_closure_block8[1](1);
test_closure_block8[2](2);
test_closure_block8[3](3);


let test_closure_block9 = [];
function test_block9(num1) {
let test_closure_array9 = [0,1,2,3];
let idx9 = 0;
do {
    let itemName = test_closure_array9[idx9];
    test_closure_block9.push(function (num) {
	  let tmp = itemName;
      Assert(tmp == num);
    });
    idx9++;
    if(idx9 == num1) {
        break;
    }
}while(idx9 < test_closure_array9.length);
}

test_block9(1);
test_block9(3);
test_closure_block9[0](0);
test_closure_block9[1](0);
test_closure_block9[2](1);
test_closure_block9[3](2);



let test_closure_block10 = [];
function test_block10(num1) {
let test_closure_array10 = [0,1,2,3];
let idx10 = 0;
do {
    let itemName = test_closure_array10[idx10];
    test_closure_block10.push(function (num) {
	  let tmp = itemName;
      Assert(tmp == num);
      
    });
    idx10++;
    if(idx10 == num1) {
        return;
    }
}while(idx10 < test_closure_array10.length);
}

test_block10(1);
test_block10(3);
test_closure_block10[0](0);
test_closure_block10[1](0);
test_closure_block10[2](1);
test_closure_block10[3](2);


let test_closure_block11 = [];
let test_closure_array11 = [0,1,2,3];
let idx11 = 0;
while(idx11 < test_closure_array11.length) {
    let itemName = test_closure_array11[idx11];
    test_closure_block11.push(function (num) {
	  let tmp = itemName;
      Assert(tmp == num);
    });

    let m = 0;
    for(m = 0; m < idx11; m++) {
    let itemName = test_closure_array11[idx11];
    test_closure_block11.push(function (num) {
	    let tmp = itemName;
        Assert(tmp == num);
    });
        if(idx11 == 2) {
            break;
        }
    }
    idx11++;
}

test_closure_block11[0](0);
test_closure_block11[1](1);
test_closure_block11[2](1);
test_closure_block11[3](2);
test_closure_block11[4](2);
test_closure_block11[5](3);
test_closure_block11[6](3);
test_closure_block11[7](3);

let test_closure_block12 = [];
function test_closure12(num1) {
let test_closure_array12 = [0,1,2,3];
let idx12 = 0;
while(idx12 < test_closure_array12.length) {
    let itemName = test_closure_array12[idx12];
    if(idx12 == num1) {
        idx12++;
        continue;
    }
    test_closure_block12.push(function (num) {
	  let tmp = itemName;
      Assert(tmp == num);
    });
    idx12++;
}
}

test_closure12(2);
test_closure12(1);
test_closure_block12[0](0);
test_closure_block12[1](1);
test_closure_block12[2](3);
test_closure_block12[3](0);
// -------------------------------------------------------