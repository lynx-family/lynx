let recordStartTime = ###TESTBENCH_REPLAY_TIME###;
let systemStartTime = Date.now();

const oldDateObj = Date;
const handler = {
    getPrototypeOf: function(target) {
        if(target.__proto__ === oldDateObj.prototype){
            return TestBenchDate.prototype;
        }else{
            return null;
        }
    },
    get: function(obj, prop) {
        if(prop in obj){
            // if this prop is a type of function.
            if(Object.prototype.toString.call(obj[prop]) === "[object Function]"){
                return function(...args){
                    return obj[prop](...args);
                }
            }else{
                // return original Date's prop
                return obj.prop;
            }
        }else{
            return undefined;
        }
    }
};
class TestBenchDate {
// new operator will get an proxy obj to call correct function and attributes
constructor(...args) {
    if(args.length == 0){
        var t1 = oldDateObj.now();
        var t2 = t1 - systemStartTime + recordStartTime;
        return new Proxy(new oldDateObj(t2), handler);
    }else{
        return new Proxy(new oldDateObj(...args), handler);
    }
}
// for static function
static now(){
    var t1 = oldDateObj.now();
    var t2 = t1 - systemStartTime + recordStartTime;
    return t2;
}
static parse(dateString){
    return oldDateObj.parse(dateString);
}
static UTC(year, month, ...others){
    return oldDateObj.UTC(year, month, ...others);
}

}
// replace original Date obj
var Global = (function () {
        if (typeof global === 'object') {
            return global;
        }
        else {
            return (0, eval)('this');
        }
    })();
Global.Date = TestBenchDate;
