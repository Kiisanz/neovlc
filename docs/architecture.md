# Arsitektur NeoVLC

## Modul dan dependensi

```text
app ──> backend/vlc ──> Qt Core + libVLC
 │
 └────> ui ──────────> Qt Quick + Quick Controls 2
```

`src/CMakeLists.txt` menjadi tempat komposisi executable `neovlc`.
Konfigurasi source aplikasi dan modul QML dimuat dari file `.cmake`
masing-masing dalam scope target yang sama. Finalisasi executable dilakukan
setelah seluruh modul terpasang.

## Aplikasi

`app/main.cpp` hanya membuat `QGuiApplication`, mengatur metadata, dan
memanggil `neovlc::app::run()`. `Application.cpp` mengelola masa hidup layanan,
menyediakan backend ke QML, memuat jendela utama, dan menjalankan event loop.

Instance backend dibuat sebelum engine QML dan dihancurkan setelah engine.
Objek pemutar media yang ditambahkan kemudian harus dihancurkan sebelum
instance libVLC yang dipakainya.

## Backend VLC

Target `neovlc_backend_vlc` mempunyai alias `NeoVLC::Vlc`. Backend memakai
namespace `neovlc::backend` dan menyediakan header melalui
`<vlc/VlcContext.h>`. Dependensi publiknya hanya Qt Core; header libVLC
dan konfigurasi pkg-config berada pada implementasi privat.

`VlcContext` memiliki instance libVLC melalui RAII. API ini menyediakan status
inisialisasi, versi, pesan kesalahan, dan akses instance bagi implementasi
pemutar C++. Backend tidak boleh mengimpor tipe dari `app` atau `ui`.

Ketika playback ditambahkan, letakkan pengelola media, kontrol transport,
dan penanganan event VLC di backend. Jangan memasukkan logika codec atau
pemanggilan API VLC langsung ke file QML.

## UI QML

`ui/Ui.cmake` mendaftarkan modul QML `NeoVLC` pada executable. Alias resource
menjaga alamat entry point `qrc:/NeoVLC/Main.qml` tetap eksplisit.

Kontrak yang tersedia saat ini adalah context property `vlcBackend`, dengan
properti `available`, `version`, dan `errorString`. QML menangani presentasi
dan interaksi pengguna; modul aplikasi memasok objek backend.

Tambahkan komponen UI dan resource saat dibutuhkan, lalu daftarkan secara
eksplisit di `Ui.cmake`. Modul domain baru diberi target library sendiri
saat sudah mempunyai implementasi dan batas dependensi yang jelas.

## Status validasi

Struktur, referensi file, dan alamat resource diperiksa tanpa menjalankan
konfigurasi CMake, kompilasi, atau aplikasi. Integrasi runtime belum diuji.

Referensi mekanisme finalisasi:
[Qt: qt_add_executable](https://doc.qt.io/qt-6/qt-add-executable.html).
