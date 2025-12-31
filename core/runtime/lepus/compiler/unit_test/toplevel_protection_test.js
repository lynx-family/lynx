var a = 1;
var b = 2;
var c = 3;

// d is not used in the script, but should be preserved
var d = 4; 

function test() {
    // a and b are used
    var sum = a + b;
    // c is updated
    c = a + b + c;
    return sum;
}

var result = test();
print("sum: ", result);
print("c: ", c);

Assert(result == 3);
Assert(c == 6);
Assert(a == 1);
Assert(b == 2);
Assert(d == 4); // Check if dead toplevel d is still correct
