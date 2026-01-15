// Test unary operators + and -

let y = '2147483648';
let x = +y + ~1;
Assert(x == 2147483646);

y = '36283.28392';
x = -y + 8398452;
Assert(x == 8362168.71608);

x = 1;
Assert(!x == false);
