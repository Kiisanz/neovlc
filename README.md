# NeoVLC

Proyek video player desktop menggunakan C++17, Qt Quick/QML, dan libVLC.
NeoVLC dikembangkan bertahap dengan fokus pada antarmuka modern dan
pengalaman pemutaran media yang sederhana.

Repository: [Kiisanz/neovlc](https://github.com/Kiisanz/neovlc)

## Referensi desain

Arah desain proyek mengacu pada video
["I Redesigned VLC After 25 Years"](https://www.youtube.com/watch?v=ERFeOgqk324)
dari channel **Juxtopposed**. Video tersebut menjadi referensi visual dan
pengalaman pengguna untuk pengembangan antarmuka NeoVLC.

Penerjemahan referensi ke komponen QML masuk dalam roadmap; tampilan saat
ini masih berupa jendela placeholder.

## Status proyek

**Tahap saat ini: fondasi awal / pra-MVP.** Belum tersedia video player
yang dapat digunakan untuk memutar media.

| Area | Status |
| --- | --- |
| Struktur modular | Tersedia: aplikasi, backend VLC, dan UI QML |
| Konfigurasi CMake | Ditulis untuk Qt 6 dan libVLC melalui pkg-config |
| Dependensi libVLC | Versi 3.0.24, header, dan plugin terdeteksi di lingkungan penyiapan |
| Backend dasar | Pengelola instance, versi, status, dan pesan kesalahan tersedia di kode |
| Penghubung QML | Context property `vlcBackend` tersedia di kode |
| Antarmuka | Jendela placeholder; desain referensi belum diterapkan |
| Playback dan rendering | Belum diimplementasikan |
| Validasi | Pemeriksaan statis struktur dan referensi file selesai |
| Konfigurasi, build, dan runtime | Belum dijalankan untuk struktur saat ini |

Folder `build/` yang sudah ada berasal dari proyek sebelumnya dan diabaikan
oleh Git. Isinya bukan hasil validasi implementasi saat ini.

## Roadmap

Urutan berikut menjadi panduan pengembangan. Item selesai menandai kode
atau dokumentasi yang sudah disiapkan; validasi build dan runtime dicatat
sebagai pekerjaan terpisah.

### 1. Fondasi proyek — disiapkan

- [x] Menyiapkan kerangka Qt Quick/QML dan C++17.
- [x] Memisahkan modul aplikasi, backend VLC, dan UI.
- [x] Menghubungkan dependensi libVLC melalui CMake dan pkg-config.
- [x] Menambahkan pengelola instance libVLC dan status backend untuk QML.
- [x] Mendokumentasikan arsitektur dan referensi desain.

### 2. Backend pemutaran — berikutnya

- [ ] Menambahkan pengelola media player dengan masa hidup yang terkontrol.
- [ ] Membuka file video lokal.
- [ ] Menyediakan play, pause, stop, seek, dan volume.
- [ ] Menyinkronkan status, posisi, durasi, dan kesalahan ke QML.
- [ ] Mengintegrasikan output video libVLC ke tampilan Qt Quick.

Target tahap: video lokal dapat tampil dan dikontrol melalui backend.

### 3. Antarmuka MVP

- [ ] Menjabarkan referensi Juxtopposed menjadi layout dan komponen QML.
- [ ] Menyiapkan warna, tipografi, jarak, dan ikon yang konsisten.
- [ ] Membuat area video, pemilih file, dan kontrol pemutaran.
- [ ] Menambahkan timeline, indikator waktu, dan kontrol volume.
- [ ] Mendukung fullscreen, shortcut dasar, serta status kosong dan kesalahan.

Target tahap: pengguna dapat membuka dan menonton video melalui UI.

### 4. Fitur media lanjutan

- [ ] Playlist dan antrean pemutaran.
- [ ] Pemilihan subtitle dan track audio.
- [ ] Pengaturan kecepatan pemutaran.
- [ ] Riwayat media dan penyimpanan preferensi pengguna.
- [ ] Membuka sumber media melalui URL.

### 5. Validasi dan distribusi

- [ ] Menjalankan konfigurasi CMake dan build setelah diizinkan.
- [ ] Menguji playback, seek, audio, sinkronisasi UI, dan penutupan aplikasi.
- [ ] Memverifikasi format media serta plugin VLC yang dibutuhkan.
- [ ] Memvalidasi platform tambahan di luar lingkungan Linux penyiapan.
- [ ] Menyiapkan pengujian otomatis untuk perilaku backend yang relevan.
- [ ] Menyiapkan packaging, dependensi runtime, dan petunjuk menjalankan aplikasi.

Validasi dilakukan saat masing-masing fitur dikembangkan; tahap terakhir
merangkum kesiapan distribusi. Platform rilis dan jadwal ditentukan setelah
MVP terverifikasi.

## Prasyarat

- Qt 6.2 atau lebih baru dengan modul Quick dan Quick Controls 2
- CMake 3.16 atau lebih baru
- Compiler C++17
- pkg-config dan libVLC 3.x (minimal 3.0), termasuk header pengembangan
  dan plugin runtime VLC

## Dependensi libVLC

Di lingkungan CachyOS saat penyiapan, `pkg-config` mendeteksi libVLC 3.0.24.
CMake memakai target `PkgConfig::LIBVLC` untuk include dan linker flags.
Dependensi ini wajib; konfigurasi CMake akan gagal jika libVLC tidak ditemukan.

Untuk memeriksa instalasi tanpa build:

```bash
pkg-config --modversion libvlc
pkg-config --cflags --libs libvlc
```

Jika libVLC berada di prefix khusus, tambahkan direktori yang berisi
`libvlc.pc` ke `PKG_CONFIG_PATH`. Saat aplikasi dijalankan, shared library
dan plugin VLC juga harus tersedia. Konfigurasi saat ini menggunakan
pkg-config dari instalasi sistem dan belum mencakup bundling distribusi.

## Struktur modular

```text
neovlc/
├── CMakeLists.txt              # Versi proyek, standar C++, dependensi Qt
├── src/
│   ├── CMakeLists.txt          # Komposisi target aplikasi dan modul
│   ├── app/
│   │   ├── Application.cmake   # Source dan dependensi executable
│   │   ├── main.cpp            # Entry point dan metadata aplikasi
│   │   ├── Application.h
│   │   └── Application.cpp     # Inisialisasi backend, QML, dan event loop
│   ├── backend/vlc/
│   │   ├── CMakeLists.txt      # Library NeoVLC::Vlc dan dependensi libVLC
│   │   ├── VlcContext.h        # Antarmuka backend
│   │   └── VlcContext.cpp      # Implementasi libVLC
│   └── ui/
│       ├── Ui.cmake            # Registrasi modul dan resource QML
│       └── Main.qml            # Jendela utama
└── docs/
    └── architecture.md        # Batas modul dan aturan pengembangan
```

Backend dibangun sebagai library statis `NeoVLC::Vlc`. Modul aplikasi
menghubungkannya dengan UI; backend tidak bergantung pada QML atau Qt Quick.
UI dikemas di executable sebagai modul QML `NeoVLC`, dengan entry resource
`qrc:/NeoVLC/Main.qml`. Detail ada di [arsitektur](docs/architecture.md).

QML dapat membaca `vlcBackend.available`, `vlcBackend.version`, dan
`vlcBackend.errorString`. Backend membuat instance libVLC saat startup
dan melepaskannya saat aplikasi ditutup. Instance tersedia untuk backend
pemutar C++ melalui `VlcContext::instance()`; objek pemutar nantinya harus
dihancurkan sebelum `VlcContext`.

Referensi API: [VideoLAN LibVLC 3.0](https://videolan.videolan.me/vlc-3.0/group__libvlc__core.html).
