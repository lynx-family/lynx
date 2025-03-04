// MIT License

// Copyright (c) 2016 Jerry Bendy

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/**!
 * url-search-params-polyfill
 *
 * @author Jerry Bendy (https://github.com/jerrybendy)
 * @licence MIT
 */

export function URLSearchParamsPolyfill(self) {
    'use strict';
    const __URLSearchParams__ = "__URLSearchParams__";
/**
 * Make a URLSearchParams instance
 *
 * @param {object|string|URLSearchParams} search
 * @constructor
 */
function URLSearchParamsPolyfill(search) {
    search = search || "";

    // support construct object with another URLSearchParams instance
    if (search instanceof URLSearchParams) {
        search = search.toString();
    }
    this [__URLSearchParams__] = parseToDict(search);
}

const prototype = URLSearchParamsPolyfill.prototype;

/**
 * Appends a specified key/value pair as a new search parameter.
 *
 * @param {string} name
 * @param {string} value
 */
prototype.append = function(name, value) {
    appendTo(this [__URLSearchParams__], name, value);
};

/**
 * Deletes the given search parameter, and its associated value,
 * from the list of all search parameters.
 *
 * @param {string} name
 */
prototype['delete'] = function(name) {
    delete this [__URLSearchParams__] [name];
};

/**
 * Returns the first value associated to the given search parameter.
 *
 * @param {string} name
 * @returns {string|null}
 */
prototype.get = function(name) {
    var dict = this [__URLSearchParams__];
    return this.has(name) ? dict[name][0] : null;
};

/**
 * Returns all the values association with a given search parameter.
 *
 * @param {string} name
 * @returns {Array}
 */
prototype.getAll = function(name) {
    var dict = this [__URLSearchParams__];
    return this.has(name) ? dict [name].slice(0) : [];
};

/**
 * Returns a Boolean indicating if such a search parameter exists.
 *
 * @param {string} name
 * @returns {boolean}
 */
prototype.has = function(name) {
    return hasOwnProperty(this [__URLSearchParams__], name);
};

/**
 * Sets the value associated to a given search parameter to
 * the given value. If there were several values, delete the
 * others.
 *
 * @param {string} name
 * @param {string} value
 */
prototype.set = function set(name, value) {
    this [__URLSearchParams__][name] = ['' + value];
};

/**
 * Returns a string containing a query string suitable for use in a URL.
 *
 * @returns {string}
 */
prototype.toString = function() {
    var dict = this[__URLSearchParams__], query = [], i, key, name, value;
    for (key in dict) {
        name = encode(key);
        for (i = 0, value = dict[key]; i < value.length; i++) {
            query.push(name + '=' + encode(value[i]));
        }
    }
    return query.join('&');
};

prototype.polyfill = true;
prototype[Symbol.toStringTag] = 'URLSearchParams';

/**
 *
 * @param {function} callback
 * @param {object} thisArg
 */
prototype.forEach = function(callback, thisArg) {
    var dict = parseToDict(this.toString());
    Object.getOwnPropertyNames(dict).forEach(function(name) {
        dict[name].forEach(function(value) {
            callback.call(thisArg, value, name, this);
        }, this);
    }, this);
};

/**
 * Sort all name-value pairs
 */
prototype.sort = function() {
    var dict = parseToDict(this.toString()), keys = [], k, i, j;
    for (k in dict) {
        keys.push(k);
    }
    keys.sort();

    for (i = 0; i < keys.length; i++) {
        this['delete'](keys[i]);
    }
    for (i = 0; i < keys.length; i++) {
        var key = keys[i], values = dict[key];
        for (j = 0; j < values.length; j++) {
            this.append(key, values[j]);
        }
    }
};

/**
 * Returns an iterator allowing to go through all keys of
 * the key/value pairs contained in this object.
 *
 * @returns {function}
 */
prototype.keys = function() {
    var items = [];
    this.forEach(function(item, name) {
        items.push(name);
    });
    return makeIterator(items);
};

/**
 * Returns an iterator allowing to go through all values of
 * the key/value pairs contained in this object.
 *
 * @returns {function}
 */
prototype.values = function() {
    var items = [];
    this.forEach(function(item) {
        items.push(item);
    });
    return makeIterator(items);
};

/**
 * Returns an iterator allowing to go through all key/value
 * pairs contained in this object.
 *
 * @returns {function}
 */
prototype.entries = function() {
    var items = [];
    this.forEach(function(item, name) {
        items.push([name, item]);
    });
    return makeIterator(items);
};

prototype[Symbol.iterator] = prototype.entries;

Object.defineProperty(prototype, 'size', {
    get: function () {
        var dict = parseToDict(this.toString())
        if (prototype === this) {
            throw new TypeError('Illegal invocation at URLSearchParams.invokeGetter')
        }
        return Object.keys(dict).reduce(function (prev, cur) {
            return prev + dict[cur].length;
        }, 0);
    }
});

function encode(str) {
    var replace = {
        '!': '%21',
        "'": '%27',
        '(': '%28',
        ')': '%29',
        '~': '%7E',
        '%20': '+',
        '%00': '\x00'
    };
    return encodeURIComponent(str).replace(/[!'\(\)~]|%20|%00/g, function(match) {
        return replace[match];
    });
}

function decode(str) {
    return str
        .replace(/[ +]/g, '%20')
        .replace(/(%[a-f0-9]{2})+/ig, function(match) {
            return decodeURIComponent(match);
        });
}

function makeIterator(arr) {
    var iterator = {
        next: function() {
            var value = arr.shift();
            return {done: value === undefined, value: value};
        }
    };

    iterator[Symbol.iterator] = function() {
        return iterator;
    };

    return iterator;
}

function parseToDict(search) {
    var dict = {};

    if (typeof search === "object") {
        // if `search` is an array, treat it as a sequence
        if (isArray(search)) {
            for (var i = 0; i < search.length; i++) {
                var item = search[i];
                if (isArray(item) && item.length === 2) {
                    appendTo(dict, item[0], item[1]);
                } else {
                    throw new TypeError("Failed to construct 'URLSearchParams': Sequence initializer must only contain pair elements");
                }
            }

        } else {
            for (var key in search) {
                if (search.hasOwnProperty(key)) {
                    appendTo(dict, key, search[key]);
                }
            }
        }

    } else {
        // remove first '?'
        if (search.indexOf("?") === 0) {
            search = search.slice(1);
        }

        var pairs = search.split("&");
        for (var j = 0; j < pairs.length; j++) {
            var value = pairs [j],
                index = value.indexOf('=');

            if (-1 < index) {
                appendTo(dict, decode(value.slice(0, index)), decode(value.slice(index + 1)));

            } else {
                if (value) {
                    appendTo(dict, decode(value), '');
                }
            }
        }
    }

    return dict;
}

function appendTo(dict, name, value) {
    var val = typeof value === 'string' ? value : (
        value !== null && value !== undefined && typeof value.toString === 'function' ? value.toString() : JSON.stringify(value)
    );

    // #47 Prevent using `hasOwnProperty` as a property name
    if (hasOwnProperty(dict, name)) {
        dict[name].push(val);
    } else {
        dict[name] = [val];
    }
}

function isArray(val) {
    return !!val && '[object Array]' === Object.prototype.toString.call(val);
}

function hasOwnProperty(obj, prop) {
    return Object.prototype.hasOwnProperty.call(obj, prop);
}

self.URLSearchParams = self.URLSearchParams ?? URLSearchParamsPolyfill;

}