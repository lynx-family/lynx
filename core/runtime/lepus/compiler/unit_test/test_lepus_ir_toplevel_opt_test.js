var a = 100;
var b = 200;
function check() {
    return a + b;
}

print("a: ", a);
print("b: ", b);
print("check(): ", check());

Assert(a == 100);
Assert(b == 200);
Assert(check() == 300);

a = 500;
print("a after set: ", a);
print("check() after set: ", check());

Assert(a == 500);
Assert(check() == 700);
