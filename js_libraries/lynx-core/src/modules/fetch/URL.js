/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

function validateBaseUrl(url) {
    // from this MIT-licensed gist: https://gist.github.com/dperini/729294
    return /^(?:(?:(?:https?|ftp):)?\/\/)(?:(?:[1-9]\d?|1\d\d|2[01]\d|22[0-3])(?:\.(?:1?\d{1,2}|2[0-4]\d|25[0-5])){2}(?:\.(?:[1-9]\d?|1\d\d|2[0-4]\d|25[0-4]))|(?:(?:[a-z0-9\u00a1-\uffff][a-z0-9\u00a1-\uffff_-]{0,62})?[a-z0-9\u00a1-\uffff]\.)*(?:[a-z\u00a1-\uffff]{2,}\.?))(?::\d{2,5})?(?:[/?#]\S*)?$/.test(
      url,
    );
  }
  
export class URL {
    _url;
    _searchParamsInstance = null;
  
    constructor(url, base) {
      let baseUrl = null;
      if (!base || validateBaseUrl(url)) {
        this._url = url;
        if (!this._url.endsWith('/')) {
          this._url += '/';
        }
      } else {
        if (typeof base === 'string') {
          baseUrl = base;
          if (!validateBaseUrl(baseUrl)) {
            throw new TypeError(`Invalid base URL: ${baseUrl}`);
          }
        } else {
          baseUrl = base.toString();
        }
        if (baseUrl.endsWith('/')) {
          baseUrl = baseUrl.slice(0, baseUrl.length - 1);
        }
        if (!url.startsWith('/')) {
          url = `/${url}`;
        }
        if (baseUrl.endsWith(url)) {
          url = '';
        }
        this._url = `${baseUrl}${url}`;
      }
    }

    get href() {
      return this.toString();
    }
  
    get searchParams() {
      if (this._searchParamsInstance == null) {
        this._searchParamsInstance = new URLSearchParams();
      }
      return this._searchParamsInstance;
    }
  
    toJSON() {
      return this.toString();
    }
  
    toString() {
      if (this._searchParamsInstance === null) {
        return this._url;
      }
  
      const instanceString = this._searchParamsInstance.toString();
      const separator = this._url.indexOf('?') > -1 ? '&' : '?';
      return this._url + separator + instanceString;
    }
  }
  