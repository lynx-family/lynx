let foo = null ?? 'default string';
Assert(foo == 'default string');
// expected output: "default string"

let baz = 0 ?? 42;
Assert(baz == 0);

let nullValue = null;
let emptyText = ""; // falsy
let someNumber = 42;

let valA = nullValue ?? "default for A";
let valB = emptyText ?? "default for B";
let valC = someNumber ?? 0;

Assert(valA == "default for A");
Assert(valB == "");
Assert(valC == 42);

let foo1 = 0;
let someDummyText = foo1 || 'Hello!';
Assert(someDummyText == 'Hello!');

let someDummyText1 = foo1 ?? 'Hello!';
Assert(someDummyText1 == 0)

function A() { console.log('A was called'); return undefined;}
function B() { console.log('B was called'); return false;}
function C() { console.log('C was called'); return "foo";}

console.log( A() ?? C() );
// logs "A was called" then "C was called" and then "foo"
// as A() returned undefined so both expressions are evaluated

console.log( B() ?? C() );
// logs "B was called" then "false"
// as B() returned false (and not null or undefined), the right
// hand side expression was not evaluated

// null || undefined ?? "foo";
// true || undefined ?? "foo";

let foo2 = { someFooProp: "hi" };

Assert(foo2.someFooProp?.length ?? "not available" == 2);
Assert(foo2.someBarProp?.length ?? "not available" == "not available");