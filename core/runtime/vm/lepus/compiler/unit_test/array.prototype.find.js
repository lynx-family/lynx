let inventory = [
  { name: 'apples', quantity: 2 },
  { name: 'bananas', quantity: 0 },
  { name: 'cherries', quantity: 5 },
];

function findCherries(fruit) {
  return fruit.name === 'cherries';
}
Assert(inventory.find(findCherries) == { name: 'cherries', quantity: 5 }); // { name: 'cherries', quantity: 5 }
Assert(
  [
    { name: 'apples', quantity: 2 },
    { name: 'bananas', quantity: 0 },
    { name: 'cherries', quantity: 5 },
  ].find(findCherries) == { name: 'cherries', quantity: 5 }
);

function isPrime(element, index, array) {
  let start = 2;
  while (start <= Math.sqrt(element)) {
    if (element % start++ < 1) {
      return false;
    }
  }
  return element > 1;
}

let arr1 = [4, 6, 8, 12];
Assert(arr1.find(isPrime) == null); // undefined, not found
Assert([4, 6, 8, 12].find(isPrime) == null); // undefined, not found
arr1 = [4, 5, 8, 12];
Assert(arr1.find(isPrime) == 5); // 5
Assert([4, 5, 8, 12].find(isPrime) == 5); // undefined, not found
