let x;

function foo() {
  return x;
}

x = 42;

let bar = function() {
  return x;
};

Assert(foo() == 42);
Assert(bar() == 42);

let a;

function getA1() {
  return a;
}

a = 10;

let getA2 = function() {
  return a;
};

a = 20;

let getA3 = function() {
  return a;
};

a = 30;

Assert(getA1() == 30);
Assert(getA2() == 30);
Assert(getA3() == 30);

let counter = 0;

function increment() {
  counter = counter + 1;
}

function getCounter() {
  return counter;
}

Assert(getCounter() == 0);
increment();
Assert(getCounter() == 1);
increment();
increment();
Assert(getCounter() == 3);

let m;
let n;

function sumMN() {
  return m + n;
}

m = 100;

let mulMN = function() {
  return m * n;
};

n = 5;

function diffMN() {
  return m - n;
}

Assert(sumMN() == 105);
Assert(mulMN() == 500);
Assert(diffMN() == 95);

let obj;

function getObj() {
  return obj;
}

obj = { name: "hello", value: 1 };

let getObjValue = function() {
  return obj.value;
};

obj.value = 99;

Assert(getObj().name == "hello");
Assert(getObjValue() == 99);

obj = { name: "world", value: 200 };
Assert(getObj().name == "world");
Assert(getObjValue() == 200);

let arr;

function getArr() {
  return arr;
}

arr = [1, 2, 3];

let getArrLen = function() {
  return arr.length;
};

Assert(getArr() == [1, 2, 3]);
Assert(getArrLen() == 3);

arr.push(4);
Assert(getArrLen() == 4);

arr = [10, 20];
Assert(getArr() == [10, 20]);
Assert(getArrLen() == 2);

let base = 0;

function makeAdder() {
  return function(v) {
    return base + v;
  };
}

base = 100;
let add = makeAdder();
Assert(add(5) == 105);

base = 200;
Assert(add(5) == 205);

let loopVar = 0;
let fns = [];

function getLoopVar() {
  return loopVar;
}

for (let i = 0; i < 3; i++) {
  loopVar = loopVar + 1;
  fns.push(function() {
    return loopVar;
  });
}

Assert(getLoopVar() == 3);
Assert(fns[0]() == 3);
Assert(fns[1]() == 3);
Assert(fns[2]() == 3);

let flag;

function getFlag() {
  return flag;
}

flag = true;

let checkFlag = function() {
  if (flag) {
    return "yes";
  } else {
    return "no";
  }
};

Assert(getFlag() == true);
Assert(checkFlag() == "yes");

flag = false;
Assert(getFlag() == false);
Assert(checkFlag() == "no");

let state = "init";

function transitionA() {
  if (state == "init") {
    state = "running";
  }
  return state;
}

function transitionB() {
  if (state == "running") {
    state = "done";
  }
  return state;
}

Assert(transitionA() == "running");
Assert(state == "running");
Assert(transitionB() == "done");
Assert(state == "done");

let undef_var;

function readUndef() {
  return undef_var;
}

Assert(readUndef() == undefined);

undef_var = null;
Assert(readUndef() == null);

undef_var = 0;
Assert(readUndef() == 0);

undef_var = "";
Assert(readUndef() == "");

let multiplier;

function applyToArray(arr, fn) {
  let result = [];
  for (let i = 0; i < arr.length; i++) {
    result.push(fn(arr[i]));
  }
  return result;
}

function makeMultiplier() {
  return function(v) {
    return v * multiplier;
  };
}

multiplier = 3;
let triple = makeMultiplier();
let tripled = applyToArray([1, 2, 3], triple);
Assert(tripled == [3, 6, 9]);

multiplier = 10;
let scaled = applyToArray([1, 2, 3], triple);
Assert(scaled == [10, 20, 30]);
