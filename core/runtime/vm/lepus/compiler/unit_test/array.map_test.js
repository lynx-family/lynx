let numbers = [1, 4, 9];
let roots = numbers.map(Math.sqrt);
// The value of roots is [1, 2, 3], and the value of numbers remains [1, 4, 9]

Assert(roots == [1, 2, 3]);

let doubles = roots.map(function(num) {
  return num * 2;
});

Assert(doubles == [2, 4, 6]);

doubles = roots.map(function f(num) {
  num = 2 * num;
});

Assert(doubles == [null, null, null]);

function f(x) {
  return x * x;
}
Assert(f(5) == 25);

let s = ['hello', 'world', 'hua', 'zhong'];
let t = s.map(function(s) {
  if (s.length > 4) {
    return s.length;
  }
});

Assert(t == [5, 5, null, 5]);

t = s.map(function(s, index) {
  if (s.length > 4) {
    return s.length;
  } else if (index == 2) {
    return 'YES!!!!!';
  }
});

Assert(t == [5, 5, 'YES!!!!!', 5]);

let arr = [];
arr[1] = "hello";
Assert(arr == [null,"hello"]);
