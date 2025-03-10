<div align="center">

<p>
  <a href="https://lynxjs.org">
    <img width="500" alt="Lynx" src="https://github.com/user-attachments/assets/23e35f90-1506-4b1d-8114-6bb2b8b643e7" />
  </a>
</p>

[![Apache licesed](https://img.shields.io/badge/License-Apache--2.0-cyan?logo=apache)](https://github.com/lynx-family/lynx/blob/develop/LICENSE)
[![Latest release version](https://img.shields.io/github/v/release/lynx-family/lynx.svg)](https://github.com/lynx-family/lynx/releases)
[![CI status](https://img.shields.io/github/actions/workflow/status/lynx-family/lynx/ci.yml)](https://github.com/lynx-family/lynx/actions)
[![X (formerly Twitter) URL](https://img.shields.io/twitter/url?url=https%3A%2F%2Fx.com%2Flynxjs_org&style=social&label=Lynx)](https://x.com/lynxjs_org)

Memperkuat komunitas web dan mengundang lebih banyak orang untuk membangun lintas platform

</div>


## Content
- [Tentang Lynx](#About-Lynx)
- [Dokumentasi](#Documentation)
- [Bagaimana cara menggunakan Lynx](#How-to-Use-Lynx)
- [Bagaimana cara berkontribusi](#How-to-Contribute)
- [Diskusi](#Discussions)
- [Credits](#Credits)
- [Lisensi](#License)

## About Lynx

Lynx adalah _keluarga_ teknologi sumber terbuka yang mendorong para pengembang untuk menggunakan keterampilan web yang sudah ada untuk membuat UI yang benar-benar asli untuk seluler dan web dari satu basis kode, yang menampilkan kinerja pada skala dan kecepatan.
- **💫 Menulis sekali, Render dimana saja.** Nikmati rendering native di Android, iOS, dan Web, atau konsistensi piksel yang sempurna di seluler dan desktop melalui renderer khusus kami.
- **🌐 Desain Terinspirasi dari Web.** Manfaatkan pengetahuan Anda tentang CSS dan React. Kami merancang Lynx dengan mempertimbangkan pengetahuan dan pustaka web.
- **⚡ Kinerja pada skala besar.** Dapatkan waktu peluncuran yang cepat dan responsifitas UI yang halus melalui mesin multithread kami, baik yang berdiri sendiri maupun yang disematkan.

Repositori ini berisi **core engine** Lynx. Untuk repositori lain dalam keluarga Lynx, kunjungi [org homepage](https://github.com/lynx-family).


## Dokumentasi
Anda bisa menemukan dokumentasi untuk Lynx di [lynxjs.org](http://lynxjs.org).

## Bagaimana cara menggunakan Lynx
### Prasyarat
Aplikasi Lynx dapat menargetkan iOS 10 dan Android 5.0 (API 21) atau yang lebih baru.

Kami menyarankan untuk menggunakan macOS sebagai sistem operasi pengembangan. Windows dan Linux belum diverifikasi atau dijamin, sehingga Anda mungkin mengalami masalah. Jika Anda memerlukan bantuan, silakan ajukan masalah, dan kami akan dengan senang hati membantu Anda memperbaikinya.

### Panduan Memulai
- Cobalah Lynx dengan membuat [hello world](https://lynxjs.org/guide/start/quick-start.html)
- [Mengintegrasikan Lynx dengan Aplikasi yang Sudah Ada](https://lynxjs.org/guide/start/integrate-with-existing-apps.html)

## Bagaimana cara berkontribusi
### Dapatkan kode
Alih-alih langsung mengkloning repositori, pertimbangkan untuk mengambil langkah ekstra.

Untuk mengelola dependensi secara efektif menggunakan alat khusus kami, [Habitat](https://github.com/lynx-family/habitat), disarankan untuk membuat direktori tambahan. Sebagai contoh, Anda dapat menamai direktori ini `src`:

```
git clone https://github.com/lynx-family/lynx.git src/lynx
```

Sekarang, `src` hanya berisi `lynx`. Namun, ketika Anda melanjutkan proses untuk build Lynx, Anda akan melihat beberapa depedensi yang terinstall di sana.
This additional `src` directory helps keep your workplace organized.
Direktori `src` tambahan ini membantu menjaga workplace Anda terorganisir.

### [Code of Conduct (Kode Etik)][coc]
We are devoted to ensuring a positive, inclusive, and safe environment for all contributors. Please find our [Code of Conduct][coc] for detailed information.

Kami berdedikasi untuk memastika lingkungan yang positif, inklusif, dan lingkungan yang aman untuk semua kontributor. Silahkan baca [Code of Conduct][coc] untuk informasi lebih rinci.

[coc]: CODE_OF_CONDUCT.md

### [Panduan Kontribusi][contributing]
Kami menyambut baik Anda untuk bergabung dan menjadi bagian dari Lynx Authors. Orang seperti Andalah yang membuat proyek menjadi ini semakin baik.

Silahkan lihat [panduan berkontribusi][contributing] untuk informasi lebih rinci.

[contributing]: CONTRIBUTING.md

## Diskusi
Diskusi dan proposal besar dibahas dalam [Github Discussions](https://github.com/lynx-family/lynx/discussions)

## Credits
Lynx memanfaatkan beberapa third-party libraries dan mengambil inspirasi dari beberapa proyek. Kami ingin mengucapkan terima kasih yang sebesar-besarnya pada sumber-sumber ini.

### Third Party Libraries
Lynx menggabungkan third-party libraries berikut ini, yang secara signifikan berkontribusi pada fungsionalitasnya. Kami menghargai upaya para pengembang dan komunitas sumber terbukan di balik proyek-proyek ini:
- [aes](https://github.com/SergeyBel/AES)
- [benchmark](https://github.com/google/benchmark)
- [binding](https://chromium.googlesource.com/chromium/blink/+/refs/heads/main/Source/bindings)
- [checkstyle](https://github.com/checkstyle/checkstyle)
- [buildroot](https://github.com/flutter/buildroot)
- [double-conversion](https://github.com/google/double-conversion)
- [googletest](https://github.com/google/googletest)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- [modp_b64](https://github.com/Piasy/modp_b64)
- [node-addon-api](https://github.com/nodejs/node-addon-api)
- [NativeScript](https://github.com/NativeScript/NativeScript)
- [perfetto](https://android.googlesource.com/platform//external/perfetto/)
- [rapidjson](https://github.com/Tencent/rapidjson)
- [v8](https://chromium.googlesource.com/v8/v8.git)
- [xctestrunner](https://github.com/google/xctestrunner)
- [xhook](https://github.com/iqiyi/xHook.git)
- [zlib](https://chromium.googlesource.com/chromium/src/third_party/zlib)

### Referenced API Design and Implementations
Desain beberapa API dan beberapa implementasi di Lynx terinspirasi dan direferensikan dari proyek-proyek luar biasa berikut ini. Desain dan solusi inovatif mereka sangat berharga dalam membentuk Lynx:
- [chromium](https://chromium.googlesource.com/chromium/)
- [react-native](https://github.com/facebook/react-native)
- [flutter engine](https://github.com/flutter/engine)

Kami menghormati hak kekayaan intelektual dari semua proyek ini dan mematuhi lisensi sumber terbuka yang relevan serta pedoman penggunaan.

## [License][license]
Lynx berlisensi Apache licensed, yang dapat ditemukan pada file [LICENSE][license].

[license]: LICENSE