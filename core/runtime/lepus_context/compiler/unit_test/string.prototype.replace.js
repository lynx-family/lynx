function test() {
    let rich_text = "<em>小</em>醉";
    let temp = rich_text.replace(/em/g, "[b type=7 color=#FF6880");
    temp = temp.replace(/<\/em>/g, "[/b]");
    return temp;
}

Assert(test() == "<[b type=7 color=#FF6880>小</[b type=7 color=#FF6880>醉");


let test_replace0 = "56.0".replace("s", "asdf");
Assert(test_replace0 == "56.0");

let test_replace1 = "56.0".replace('.', '12.234*');
Assert(test_replace1 == "5612.234*0");

 let test_replace2 = "56.0".replace('.', '');
Assert(test_replace2 == "560");


 let test_replace3 = "56.0".replace('.', '-$$');
 Assert(test_replace3 == "56-$0");

 let test_replace4 = "56.0".replace('.', '-$&-');
 Assert(test_replace4, "56-.-0")


let test_replace5 = "56.0".replace(".", '-$`');
 Assert(test_replace4, "56-560 ");


let test_replace6 = "56.0".replace(".", '-$\'');
Assert(test_replace6 == "56-00");

let test_replace7 = "test$$test".replace("$$", '$test');
Assert(test_replace7 == "test$testtest");

let test_replace8 = "time,time".replace(/time/g, undefined);
Assert(test_replace8 == "null,null");

let test_replace9 = "time,time".replace(/time/g, null);
Assert(test_replace9 == "null,null");

let test_replace10 = "time,time".replace(/time/g, "");
Assert(test_replace10 == ",");

let test_replace11 = "time".replace(/time/g, "");
Assert(test_replace11 == "");

let test_replace12 = "99000.000".replace(/(?:\.0*|(\.\d+?)0+)$/, '$1');
Assert(test_replace12 == "99000");

let test_replace13 = "020-82228888-0357-4227865".replace(/(\d{3,4}-)?(\d{7,8})/, "1");
Assert(test_replace13 == "1-0357-4227865");