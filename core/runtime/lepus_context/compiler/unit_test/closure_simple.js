let __globalProps = {};
function renderer() {
let i18n = {};
(function exx(exports) {
    'use strict';
    function t(key, options) {
      return "xx";
    }
    function getTextMapForUserItem() {
      // key is field from server, value is i18n key
      let RELATION_SHIP = {
        'friends': 'friends',
        'following': 'following',
        'facebook': 'pymk',
        'twitter': 'twitter_connection',
        'contacts': 'from_your_contacts',
        'Friends with': 'reason_friends_with',
        'Followed by': 'reason_followed_by',
        'Follows': 'reason_follows',
      };
      let keyMap = {
        'pm_main_live_entry_final': 'pm_main_live_entry_final',
      };
      Object.assign(keyMap, RELATION_SHIP);
      let result = {};
      Object.keys(keyMap).forEach(function (_key) {
        result[_key] = t(keyMap[_key]);
      });
      return result;
    }
    exports.t = t;
  })(i18n);

  Assert(i18n.t() == "xx");
}
