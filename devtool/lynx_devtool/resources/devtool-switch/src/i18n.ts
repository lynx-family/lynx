import i18next from 'i18next';
import type { i18n } from 'i18next';

import enTranslation from './locales/en.json';
import zhTranslation from './locales/zh.json';

const localI18nInstance: i18n = i18next.createInstance();

localI18nInstance.init({
  lng: lynx.__globalProps?.appLocale ?? 'en',
  // The default JSON format needs `Intl.PluralRules` API, which is currently unavailable in Lynx.
  compatibilityJSON: 'v3',
  resources: {
    en: {
      translation: enTranslation,
    },
    zh: {
      translation: zhTranslation,
    },
  },
});

export { localI18nInstance as i18n };
