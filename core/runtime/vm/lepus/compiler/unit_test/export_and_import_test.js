// import
import { myFunction, myVariable, myClass } from 'parentModule.js'
function test() {
   let a = 1;
   let b = "test import statement";
   Assert(b == "test import statement");
}
test();
import { export1 , export2 as alias2 } from "module-name";
import { export as alias } from "module-name"

// export
let variable1 = 1;
let variable2 = 1;
export { variable1 as name1, variable2 as name2};
function name2() {
   console.log("asdf");
   return 4;
}
export {name2}
export {name2};
export default variable1;
let c = 1;
Assert(c == 1);
Assert(name2() == 4);


export default function test_export3() {
  return 3;
}
export let test_export1 = 1, test_export2 = 2;

Assert(test_export1 == 1);
Assert(test_export2 == 2);
Assert(test_export3() == 3);

