function test(a, b) {
	console.log("hahaha");
	console.log("continue to run");
	let test_value = 1;
	Assert(test_value == 1);
}
test(1);
// default parameters check, program continues to run
function test2() {
	console.log("hahaha");
	test(1);
}
test2();