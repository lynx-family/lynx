function test() {
	let a = 1;
	if (true) {
		a++;
	} else {
		function test2() {
			a++;
		}
		test2();
	}
	Assert(a == 2);
	return a;
}
let b = test();
Assert(b == 2)


function test_closure_block() {
	let a = 1;
	{
		let a = 1000;
		function test_closure_block3() {
			let a = 4;
			a++;
			Assert(a == 5);
		}
		test_closure_block3();
	}
	{
		function test_closure_block2() {
			a++;
			Assert(a == 2);
		}
		test_closure_block2();
	}
	Assert(a == 2);
}
test_closure_block();


function test_closure_block_1() {
	let a = 1;
	{
		let a = 3;
		function test_closure_block3() {
			a++;
			Assert(a == 4);
		}
		test_closure_block3();
	}
	{
		function test_closure_block2() {
			Assert(a == 1);
		}
		test_closure_block2();
	}
}
test_closure_block_1();


function test_closure_block2(function2) {
	let a = function2();
	Assert(a == 2);
}
function test_closure_block3() {
	{
		let a = 1;
		{
			let a = 3;
		}
		{
			test_closure_block2(function () {
				a++;
				return a;
			});
		}
	}
}
test_closure_block3();


function render() {
	let __this = {};
	__this.lepusClsxLepus = function (args) {
		let str_ = '';
		args.forEach(function (tmp) {
			let x = __this.lepusToVal(tmp);
			str_ += x;
		});
		return str_;
	};

	__this.lepusToVal = function (mix) {
		let k, y;
		let str = '';
		if (true) {
			str = str + mix;
		} else {
			[].forEach(function () {
				if (str) {
					str += ' ';
				}
			});
		}
		return str;
	};
	return __this.lepusClsxLepus([
		'tux-dialog-text-action',
		'flex justify-center items-center h-48 w-full block text-center px-8',
	]);
}
Assert(render() == "tux-dialog-text-actionflex justify-center items-center h-48 w-full block text-center px-8");


function test_closure_block_2() {
	let str = 'ccccccc';
	if (true) {
		str = "aaaaaa";
	} else {
		function test() {
			str = "bbbbb";
		}
	}
	Assert(str == "aaaaaa");
	return str;
}
let test_block_result = test_closure_block_2();
Assert(test_block_result == "aaaaaa");


function test_closure_block_3() {
	let a = 1;
	if (true) {
		let a = 3;
		a++;
		console.log(a);
	} else {
		function test() {
			a++;
		}
		test();
		console.log(a);
	}
	{
		a++;
		console.log(a);
	}
	a++;
	console.log(a);
	{
		function test_closure_block2() {
			console.log(a);
		}
		test_closure_block2();
	}
}
test_closure_block_3();



function test_closure_block_4() {
	let a = 1;
	if (true) {
		let a = 3;
		a++;
		Assert(a == 4);
	} else {
		function test() {
			a++;
		}
		test();
	}
	{
		a++;
		Assert(a == 2);
	}
	a++;
	Assert(a == 3);
	{
		function test_closure_block2() {
			Assert(a == 3);
		}
		test_closure_block2();
	}
}
test_closure_block_4();


function test_closure_block_5() {
	let a = 1;
	if (true) {
		a++;
	} else {
		function test2() {
			a++;
		}
		test2();
	}
	Assert(a == 2);
	return a;
}
let res = test_closure_block_5();
Assert(res == 2);