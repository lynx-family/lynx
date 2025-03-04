//Support Array.prototype.filter

let words = ['spray', 'limit', 'elite', 'exuberant', 'destruction', 'present'];

let result = words.filter(function f(x) {
  return x.length > 6;
});

Assert(result == ['exuberant', 'destruction', 'present']);
Assert(['spray', 'limit', 'elite', 'exuberant', 'destruction', 'present'].filter(function f(x) {
  return x.length > 6;
  }) == ['exuberant', 'destruction', 'present']);


function f(element, index, array) {
  if (element[index] > 'i') {
    return 1;
  } else {
    return 0;
  }
}
result = words.filter(f);
print(result);
Assert(result == ['spray', 'destruction', 'present']);
Assert(['spray', 'limit', 'elite', 'exuberant', 'destruction', 'present'].filter(f) == ['spray', 'destruction', 'present']);

result = words.filter(function(x) {
  x.length > 6;
});
Assert(result == []);
Assert(['spray', 'limit', 'elite', 'exuberant', 'destruction', 'present'].filter(function(x) {
  x.length > 6;
}) == []);