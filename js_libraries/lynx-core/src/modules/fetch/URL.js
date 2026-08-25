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
    _urlWithoutQuery;
    _search;
    _hash;
    _hasSearch;
    _searchParamsInstance = null;
  
    constructor(url, base) {
      let baseUrl = null;
      let resolvedUrl = String(url);
      if (!base || validateBaseUrl(url)) {
        this._setUrl(resolvedUrl);
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
        if (!resolvedUrl.startsWith('/')) {
          resolvedUrl = `/${resolvedUrl}`;
        }
        if (baseUrl.endsWith(resolvedUrl)) {
          resolvedUrl = '';
        }
        this._setUrl(`${baseUrl}${resolvedUrl}`);
      }
    }

    _setUrl(url) {
      const hashIndex = url.indexOf('#');
      this._hash = '';
      if (hashIndex !== -1) {
        this._hash = url.slice(hashIndex);
        url = url.slice(0, hashIndex);
      }

      const searchIndex = url.indexOf('?');
      this._search = '';
      this._hasSearch = false;
      if (searchIndex !== -1) {
        this._search = url.slice(searchIndex + 1);
        this._hasSearch = true;
        url = url.slice(0, searchIndex);
      }

      this._urlWithoutQuery = this._normalizeUrlWithoutQuery(url);
    }

    _normalizeUrlWithoutQuery(url) {
      if (/^(?:(?:https?|ftp):)?\/\/[^/?#]+$/.test(url)) {
        return `${url}/`;
      }
      return url;
    }

    get href() {
      return this.toString();
    }
  
    get searchParams() {
      if (this._searchParamsInstance == null) {
        this._searchParamsInstance = new URLSearchParams(this._search);
      }
      return this._searchParamsInstance;
    }
  
    toJSON() {
      return this.toString();
    }
  
    toString() {
      let searchString = this._search;
      let hasSearch = this._hasSearch;
      if (this._searchParamsInstance !== null) {
        searchString = this._searchParamsInstance.toString();
        hasSearch = searchString.length > 0;
      }

      return (
        this._urlWithoutQuery +
        (hasSearch ? `?${searchString}` : '') +
        this._hash
      );
    }
  }
  
