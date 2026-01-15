function isPrime(element, index, array) {
  let start = 2;
  while (start <= Math.sqrt(element)) {
    if (element % start++ < 1) {
      return false;
    }
  }
  return element > 1;
}

let a = [-1, 6, 8, 12];
Assert(a.findIndex(isPrime) == -1);
Assert([-1, 6, 8, 12].findIndex(isPrime) == -1);
a = [4, 6, 7, 12];
Assert(a.findIndex(isPrime) == 2);
Assert([4, 6, 7, 12].findIndex(isPrime) == 2);

function isPrime2(element) {
  let start = 2;
  while (start <= Math.sqrt(element)) {
    if (element % start++ < 1) {
      return false;
    }
  }
  return element > 1;
}

a = [-1, 6, 8, 12];
Assert(a.findIndex(isPrime2) == -1);
Assert([-1, 6 , 8, 12].findIndex(isPrime2) == -1);
a = [4, 6, 7, 12];
Assert(a.findIndex(isPrime2) == 2);
Assert([4, 6, 7, 12].findIndex(isPrime2) == 2);

