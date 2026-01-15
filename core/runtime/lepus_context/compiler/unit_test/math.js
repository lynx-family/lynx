//Support
// & | ~ ^ **

// &=
//|=
//^=
//**=

let num = 10;
num |= 2;
Assert(num == 10);
num &= 2;
Assert(num == 2);
num ^= 4;
Assert(num == 6);
num **= 2;
Assert(num == 36);
num = ~num;
Assert(num == -37);

num = -2 & 3;
Assert(num == 2);
num = num | 12;
Assert(num == 14);

num = num ^ 10.01;
Assert(num == 4);

num = 10.02 ** 3;
Assert(num == 1006.012008);

// Chained assignment

let b;
num = b = 4 ** 5 + 10 / 3;
Assert(num == 1027.333333);
Assert(b == 1027.333333);
