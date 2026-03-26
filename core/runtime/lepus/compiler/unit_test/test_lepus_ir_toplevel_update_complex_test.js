var a = 1;
var b = 2;
var c = a + b;

function test() {
    return a + b + c;
}

print("a: ", a);
print("b: ", b);
print("c: ", c);
print("test(): ", test());

Assert(a == 1);
Assert(b == 2);
Assert(c == 3);
Assert(test() == 6);

a = 10;
b = 20;
c = a + b;

Assert(a == 10);
Assert(b == 20);
Assert(c == 30);
Assert(test() == 60);
