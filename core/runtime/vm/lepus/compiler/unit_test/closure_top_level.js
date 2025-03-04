let fun = null;
let xx = null;
{
    function test() {
        xx = "hello world";
        console.log("hello world");
    }
    fun = function b() {
        test();
    }
}

fun();
Assert(xx == "hello world");

