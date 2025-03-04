/**
 * @license
Copyright (c) 2014 Forbes Lindesay

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */
'use strict';

//This file contains the ES6 extensions to the core Promises/A+ API

module.exports = Promise => {
  /* Static Functions */

  var TRUE = valuePromise(true);
  var FALSE = valuePromise(false);
  var NULL = valuePromise(null);
  var UNDEFINED = valuePromise(undefined);
  var ZERO = valuePromise(0);
  var EMPTYSTRING = valuePromise('');

  function valuePromise(value) {
    var p = new Promise(Promise._noop);
    p._state = 1;
    p._value = value;
    return p;
  }
  Promise.resolve = function(value) {
    if (value instanceof Promise) return value;

    if (value === null) return NULL;
    if (value === undefined) return UNDEFINED;
    if (value === true) return TRUE;
    if (value === false) return FALSE;
    if (value === 0) return ZERO;
    if (value === '') return EMPTYSTRING;

    if (typeof value === 'object' || typeof value === 'function') {
      try {
        var then = value.then;
        if (typeof then === 'function') {
          return new Promise(then.bind(value));
        }
      } catch (ex) {
        return new Promise(function(resolve, reject) {
          reject(ex);
        });
      }
    }
    return valuePromise(value);
  };

  var iterableToArray = function(iterable) {
    if (typeof Array.from === 'function') {
      // ES2015+, iterables exist
      iterableToArray = Array.from;
      return Array.from(iterable);
    }

    // ES5, only arrays and array-likes exist
    iterableToArray = function(x) {
      return Array.prototype.slice.call(x);
    };
    return Array.prototype.slice.call(iterable);
  };

  Promise.all = function(arr) {
    var args = iterableToArray(arr);

    return new Promise(function(resolve, reject) {
      if (args.length === 0) return resolve([]);
      var remaining = args.length;
      function res(i, val) {
        if (val && (typeof val === 'object' || typeof val === 'function')) {
          if (val instanceof Promise && val.then === Promise.prototype.then) {
            while (val._state === 3) {
              val = val._value;
            }
            if (val._state === 1) return res(i, val._value);
            if (val._state === 2) reject(val._value);
            val.then(function(val) {
              res(i, val);
            }, reject);
            return;
          } else {
            var then = val.then;
            if (typeof then === 'function') {
              var p = new Promise(then.bind(val));
              p.then(function(val) {
                res(i, val);
              }, reject);
              return;
            }
          }
        }
        args[i] = val;
        if (--remaining === 0) {
          resolve(args);
        }
      }
      for (var i = 0; i < args.length; i++) {
        res(i, args[i]);
      }
    });
  };

  Promise.reject = function(value) {
    return new Promise(function(resolve, reject) {
      reject(value);
    });
  };

  Promise.race = function(values) {
    return new Promise(function(resolve, reject) {
      iterableToArray(values).forEach(function(value) {
        Promise.resolve(value).then(resolve, reject);
      });
    });
  };

  /* Prototype Methods */

  Promise.prototype['catch'] = function(onRejected) {
    return this.then(null, onRejected);
  };
  Promise.prototype.done = function(onFulfilled, onRejected) {
    var self = arguments.length ? this.then.apply(this, arguments) : this;
    self.then(null, function(err) {
      setTimeout(function() {
        throw err;
      }, 0);
    });
  };
  Promise.prototype.finally = function(f) {
    return this.then(
      function(value) {
        return Promise.resolve(f()).then(function() {
          return value;
        });
      },
      function(err) {
        return Promise.resolve(f()).then(function() {
          throw err;
        });
      }
    );
  };
  return Promise;
};
