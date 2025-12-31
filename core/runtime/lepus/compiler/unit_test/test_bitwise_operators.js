
var a = 5;  // 0101
var b = 3;  // 0011

Assert((a & b) == 1);   // 0001
Assert((a | b) == 7);   // 0111
Assert((a ^ b) == 6);   // 0110
Assert((~a) == -6);     // -(a+1)

var x = 5;
x &= 3;
Assert(x == 1);

var y = 5;
y |= 3;
Assert(y == 7);

var z = 10;
Assert((z & z) == z);
Assert((z | z) == z);
Assert((z ^ z) == 0);
