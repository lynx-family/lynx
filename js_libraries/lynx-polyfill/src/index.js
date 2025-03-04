/**
 * @license
Copyright (c) 2014-2023 Denis Pushkarev

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
require('core-js/modules/es.symbol');
require('core-js/modules/es.symbol.description');
require('core-js/modules/es.symbol.async-iterator');
require('core-js/modules/es.symbol.has-instance');
require('core-js/modules/es.symbol.is-concat-spreadable');
require('core-js/modules/es.symbol.match');
require('core-js/modules/es.symbol.match-all');
require('core-js/modules/es.symbol.replace');
require('core-js/modules/es.symbol.search');
require('core-js/modules/es.symbol.species');
require('core-js/modules/es.symbol.split');
require('core-js/modules/es.symbol.to-primitive');
require('core-js/modules/es.symbol.to-string-tag');
require('core-js/modules/es.array.concat');
require('core-js/modules/es.array.filter');
require('core-js/modules/es.array.flat');
require('core-js/modules/es.array.flat-map');
require('core-js/modules/es.array.includes');
require('core-js/modules/es.array.iterator');
require('core-js/modules/es.array.map');
require('core-js/modules/es.array.reverse');
require('core-js/modules/es.array.slice');
require('core-js/modules/es.array.sort');
require('core-js/modules/es.array.species');
require('core-js/modules/es.array.splice');
require('core-js/modules/es.array.unscopables.flat');
require('core-js/modules/es.array.unscopables.flat-map');
require('core-js/modules/es.array-buffer.constructor');
require('core-js/modules/es.array-buffer.slice');
require('core-js/modules/es.date.to-json');
require('core-js/modules/es.date.to-primitive');
require('core-js/modules/es.function.has-instance');
require('core-js/modules/es.global-this');
require('core-js/modules/es.json.stringify');
require('core-js/modules/es.json.to-string-tag');
require('core-js/modules/es.map');
require('core-js/modules/es.math.to-string-tag');
require('core-js/modules/es.number.parse-float');
require('core-js/modules/es.object.entries');
require('core-js/modules/es.object.from-entries');
require('core-js/modules/es.object.get-own-property-descriptors');
require('core-js/modules/es.object.to-string');
require('core-js/modules/es.object.values');
require('core-js/modules/es.reflect.apply');
require('core-js/modules/es.reflect.construct');
require('core-js/modules/es.reflect.define-property');
require('core-js/modules/es.reflect.delete-property');
require('core-js/modules/es.reflect.get');
require('core-js/modules/es.reflect.get-own-property-descriptor');
require('core-js/modules/es.reflect.get-prototype-of');
require('core-js/modules/es.reflect.has');
require('core-js/modules/es.reflect.is-extensible');
require('core-js/modules/es.reflect.own-keys');
require('core-js/modules/es.reflect.prevent-extensions');
require('core-js/modules/es.reflect.set');
require('core-js/modules/es.reflect.set-prototype-of');
require('core-js/modules/es.regexp.constructor');
require('core-js/modules/es.regexp.exec');
require('core-js/modules/es.regexp.sticky');
require('core-js/modules/es.regexp.test');
require('core-js/modules/es.regexp.to-string');
require('core-js/modules/es.set');
require('core-js/modules/es.string.ends-with');
require('core-js/modules/es.string.includes');
require('core-js/modules/es.string.match');
require('core-js/modules/es.string.match-all');
require('core-js/modules/es.string.pad-end');
require('core-js/modules/es.string.pad-start');
require('core-js/modules/es.string.replace');
require('core-js/modules/es.string.search');
require('core-js/modules/es.string.split');
require('core-js/modules/es.string.starts-with');
require('core-js/modules/es.string.trim');
require('core-js/modules/es.string.trim-end');
require('core-js/modules/es.string.trim-start');
require('core-js/modules/es.typed-array.float32-array');
require('core-js/modules/es.typed-array.float64-array');
require('core-js/modules/es.typed-array.int8-array');
require('core-js/modules/es.typed-array.int16-array');
require('core-js/modules/es.typed-array.int32-array');
require('core-js/modules/es.typed-array.uint8-array');
require('core-js/modules/es.typed-array.uint8-clamped-array');
require('core-js/modules/es.typed-array.uint16-array');
require('core-js/modules/es.typed-array.uint32-array');
require('core-js/modules/es.typed-array.copy-within');
require('core-js/modules/es.typed-array.every');
require('core-js/modules/es.typed-array.fill');
require('core-js/modules/es.typed-array.filter');
require('core-js/modules/es.typed-array.find');
require('core-js/modules/es.typed-array.find-index');
require('core-js/modules/es.typed-array.for-each');
require('core-js/modules/es.typed-array.from');
require('core-js/modules/es.typed-array.includes');
require('core-js/modules/es.typed-array.index-of');
require('core-js/modules/es.typed-array.iterator');
require('core-js/modules/es.typed-array.join');
require('core-js/modules/es.typed-array.last-index-of');
require('core-js/modules/es.typed-array.map');
require('core-js/modules/es.typed-array.of');
require('core-js/modules/es.typed-array.reduce');
require('core-js/modules/es.typed-array.reduce-right');
require('core-js/modules/es.typed-array.reverse');
require('core-js/modules/es.typed-array.slice');
require('core-js/modules/es.typed-array.some');
require('core-js/modules/es.typed-array.sort');
require('core-js/modules/es.typed-array.to-string');
require('core-js/modules/es.weak-map');
require('core-js/modules/es.weak-set');
