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

module.exports = (Promise, setTimeout, clearTimeout) => {
  var DEFAULT_WHITELIST = [ReferenceError, TypeError, RangeError];

  var enabled = false;

  function disable() {
    enabled = false;
    Promise._onHandle = null;
    Promise._onReject = null;
  }

  function enable(options) {
    options = options || {};
    if (enabled) disable();
    enabled = true;
    var id = 0;
    var displayId = 0;
    var rejections = {};
    Promise._onHandle = function(promise) {
      if (
        promise._state === 2 && // IS REJECTED
        rejections[promise._rejectionId]
      ) {
        if (rejections[promise._rejectionId].logged) {
          onHandled(promise._rejectionId);
        } else {
          clearTimeout && clearTimeout(rejections[promise._rejectionId].timeout);
        }
        delete rejections[promise._rejectionId];
      }
    };
    Promise._onReject = function(promise, err) {
      if (promise._deferredState === 0) {
        // not yet handled
        promise._rejectionId = id++;
        rejections[promise._rejectionId] = {
          displayId: null,
          error: err,
          timeout: setTimeout(
            onUnhandled.bind(null, promise), 0),
          logged: false,
        };
      }
    };
    function onUnhandled(promise) {
      const id = promise._rejectionId;
      if (options.allRejections || matchWhitelist(rejections[id].error, options.whitelist || DEFAULT_WHITELIST)) {
        rejections[id].displayId = displayId++;
        if (options.onUnhandled) {
          rejections[id].logged = true;
          if (rejections[id].error && !(rejections[id].error instanceof Error)) {
            const error = new Error(JSON.stringify(rejections[id].error));
            error.stack = promise.__createStack;
            rejections[id].error = error;
          }
          options.onUnhandled(rejections[id].displayId, rejections[id].error);
        } else {
          rejections[id].logged = true;
          logError(rejections[id].displayId, rejections[id].error);
        }
      }
    }
    function onHandled(id) {
      if (rejections[id].logged) {
        if (options.onHandled) {
          options.onHandled(rejections[id].displayId, rejections[id].error);
        } else if (!rejections[id].onUnhandled) {
          console.warn('Promise Rejection Handled (id: ' + rejections[id].displayId + '):');
          console.warn(
            '  This means you can ignore any previous messages of the form "Possible Unhandled Promise Rejection" with id ' +
              rejections[id].displayId +
              '.'
          );
        }
      }
    }
    return Promise;
  }

  function logError(id, error) {
    console.warn('Possible Unhandled Promise Rejection (id: ' + id + '):');
    var errStr = (error && (error.stack || error)) + '';
    errStr.split('\n').forEach(function(line) {
      console.warn('  ' + line);
    });
  }

  function matchWhitelist(error, list) {
    return list.some(function(cls) {
      return error instanceof cls;
    });
  }
  return {
    enable,
    disable,
  };
};
