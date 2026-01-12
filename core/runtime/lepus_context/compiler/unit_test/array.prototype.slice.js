let animals = ['ant', 'bison', 'camel', 'duck', 'elephant'];
let tmp = animals.slice(5);
console.log(tmp);
// Assert(tmp == ["camel", "duck", "elephant"]);
// expected output: Array ["camel", "duck", "elephant"]
Assert(animals.slice(2, 4) == ['camel', 'duck']);
Assert(animals.slice(2, 5) == ['camel', 'duck', 'elephant']);
Assert(animals.slice(2, 6) == ['camel', 'duck', 'elephant']);

console.log(['ant', 'bison', 'camel', 'duck', 'elephant'].slice(5));
Assert(['ant', 'bison', 'camel', 'duck', 'elephant'].slice(2, 4) == ['camel', 'duck']);
Assert(['ant', 'bison', 'camel', 'duck', 'elephant'].slice(2, 5) == ['camel', 'duck', 'elephant']);
Assert(['ant', 'bison', 'camel', 'duck', 'elephant'].slice(2, 6) == ['camel', 'duck', 'elephant']);

// expected output: Array ["camel", "duck"]
Assert(animals.slice(-2) == ['duck', 'elephant']);
Assert(['ant', 'bison', 'camel', 'duck', 'elephant'].slice(-2) == ['duck', 'elephant']);

let fruits = ['Banana', 'Orange', 'Lemon', 'Apple', 'Mango'];
let citrus = fruits.slice(1, 3);
Assert(fruits == ['Banana', 'Orange', 'Lemon', 'Apple', 'Mango']);
Assert(citrus == ['Orange', 'Lemon']);
Assert(['Banana', 'Orange', 'Lemon', 'Apple', 'Mango'].slice(1, 3) == ['Orange', 'Lemon']);

let myHonda = { color: 'red', wheels: 4, engine: { cylinders: 4, size: 2.2 } };
let myCar = [myHonda, 2, 'cherry condition', 'purchased 1997'];
let newCar = myCar.slice(0, 2);

// Output the color properties of myCar, newCar, and the myHonda objects they each reference.
Assert(
  myCar == [{ color: 'red', wheels: 4, engine: { cylinders: 4, size: 2.2 } }, 2, 'cherry condition', 'purchased 1997']
);
Assert(newCar == [{ color: 'red', wheels: 4, engine: { cylinders: 4, size: 2.2 } }, 2]);
Assert([myHonda, 2, 'cherry condition', 'purchased 1997'].slice(0, 2) == [{ color: 'red', wheels: 4, engine: { cylinders: 4, size: 2.2 } }, 2]);
Assert(myCar[0].color == 'red');
Assert(newCar[0].color == 'red');

// Change the color property of the myHonda object.
myHonda.color = 'purple';
Assert(myHonda.color == 'purple');

// Output the color property referenced by the myHonda object in myCar and newCar respectively.
Assert(myCar[0].color == 'purple');
Assert(newCar[0].color == 'purple');

let origin = [1,2,3];
let origin_copy = origin.slice();
origin_copy.push(4);
Assert(origin == [1,2,3]);