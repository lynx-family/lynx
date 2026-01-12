let alpha = ['a', 'b', 'c'];
let numeric = [1, 2, 3];
Assert(alpha.concat(numeric) == ['a', 'b', 'c', 1, 2, 3]);
Assert(['a', 'b', 'c'].concat([1, 2, 3]) == ['a', 'b', 'c', 1, 2, 3]);

let num1 = [1, 2, 3],
  num2 = [4, 5, 6],
  num3 = [7, 8, 9];

let nums = num1.concat(num2, num3);
Assert(nums == [1, 2, 3, 4, 5, 6, 7, 8, 9]);
Assert([1, 2, 3].concat([4, 5, 6], [7, 8, 9]) == [1, 2, 3, 4, 5, 6, 7, 8, 9]);

let alphaNumeric = alpha.concat(1, [2, 3]);
Assert(alphaNumeric == ['a', 'b', 'c', 1, 2, 3]);
Assert(['a', 'b', 'c'].concat(1, [2, 3]) == ['a', 'b', 'c', 1, 2, 3]);

num1 = [[1]];
num2 = [2, [3]];
num3 = [5, [6]];
nums = num1.concat(num2);
Assert(nums == [[1], 2, [3]]);
Assert([[1]].concat([2, [3]]) == [[1], 2, [3]]);

let nums2 = num1.concat(4, num3);
Assert(nums2 == [[1], 4, 5, [6]]);
Assert([[1]].concat(4, [5, [6]]) == [[1], 4, 5, [6]]);
num1[0].push(4);
Assert(nums == [[1, 4], 2, [3]]);
Assert([4, [5, [6]]].concat([1].push(4)) == [4, [5, [6]], 2]);
