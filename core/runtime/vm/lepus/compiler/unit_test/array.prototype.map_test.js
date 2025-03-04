let numbers = [1, 4, 9];
let roots = numbers.map(Math.sqrt);
// The value of roots is [1, 2, 3], and the value of numbers remains [1, 4, 9]

Assert(roots == [1, 2, 3]);
Assert([1, 4, 9].map(Math.sqrt) == [1, 2, 3]);

let doubles = roots.map(function(num) {
  return num * 2;
});

Assert(doubles == [2, 4, 6]);
Assert([1, 2, 3].map(function(num) {
  return num * 2;
}) == [2, 4, 6]);

doubles = roots.map(function f(num) {
  num = 2 * num;
});

Assert(doubles == [null, null, null]);
Assert([1, 2, 3].map(function f(num) {
  num = 2 * num;
}) == [null, null, null]);

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
Assert(['hello', 'world', 'hua', 'zhong'].map(function(s) {
  if (s.length > 4) {
    return s.length;
  }
}) == [5, 5, null, 5]);

s = ['hello', 'world', 'hua', 'zhong'];
t = s.map(function(s) {
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
Assert(['hello', 'world', 'hua', 'zhong'].map(function(s, index) {
  if (s.length > 4) {
    return s.length;
  } else if (index == 2) {
    return 'YES!!!!!';
  }
}) == [5, 5, 'YES!!!!!', 5]);
