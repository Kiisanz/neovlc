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

`VlcContext` dan `VlcMediaPlayer` dibuat sebelum engine QML. Karena destruksi
stack berjalan terbalik, engine QML dihancurkan lebih dulu, lalu pemutar, lalu
instance libVLC, dan terakhir aplikasi. Keduanya diekspos ke QML sebagai
context property `vlcBackend` dan `vlcPlayer`. Pemilik pemutar harus selalu
menghancurkannya sebelum konteks.

## Backend VLC

Target `neovlc_backend_vlc` mempunyai alias `NeoVLC::Vlc`. Backend memakai
namespace `neovlc::backend` dan menyediakan header melalui
`<vlc/VlcContext.h>`. Dependensi publiknya hanya Qt Core; header libVLC
dan konfigurasi pkg-config berada pada implementasi privat.

`VlcContext` memiliki instance libVLC melalui RAII. API ini menyediakan status
inisialisasi, versi, pesan kesalahan, dan akses instance bagi implementasi
pemutar C++. Backend tidak boleh mengimpor tipe dari `app` atau `ui`.

`VlcMediaPlayer` memiliki satu `libvlc_media_player_t` melalui RAII dan
menerima `VlcContext&` saat dibuat. Konstruktor mendaftarkan pemutar ke
konteks hanya setelah pembuatan berhasil; destruktor menghentikan pemutaran
lalu melepas referensinya. Konteks melacak pemutar yang masih hidup. Bila
konteks dihancurkan lebih dulu, build debug menghentikan proses melalui
`Q_ASSERT_X`; build release mencetak peringatan dan melepaskan kaitan pemutar
agar pembongkaran tidak mengakses konteks yang sudah mati. Pola ini adalah
jaring pengaman; urutan destruksi yang benar tetap tanggung jawab pemilik
pemutar.

Ketika playback ditambahkan, letakkan pengelola media, kontrol transport,
dan penanganan event VLC di backend. Jangan memasukkan logika codec atau
pemanggilan API VLC langsung ke file QML.

## UI QML

`ui/Ui.cmake` mendaftarkan modul QML `NeoVLC` pada executable. Alias resource
menjaga alamat entry point `qrc:/NeoVLC/Main.qml` tetap eksplisit.

Kontrak yang tersedia saat ini adalah context property `vlcBackend` dengan
properti `available`, `version`, dan `errorString`, serta `vlcPlayer` dengan
properti `available` dan `errorString`. QML menangani presentasi dan interaksi
pengguna; modul aplikasi memasok objek backend.

Tambahkan komponen UI dan resource saat dibutuhkan, lalu daftarkan secara
eksplisit di `Ui.cmake`. Modul domain baru diberi target library sendiri
saat sudah mempunyai implementasi dan batas dependensi yang jelas.

## Status validasi

Konfigurasi CMake dan kompilasi berhasil diverifikasi di lingkungan setup. Uji
asap siklus hidup memastikan pemutar dibuat dan dihancurkan dalam urutan yang
benar tanpa peringatan. Jalur pelanggaran urutan juga diuji pada build dengan
asertion nonaktif: konteks mencetak peringatan, melepaskan kaitan pemutar, dan
pemutar dibongkar tanpa crash. Pada build debug, `Q_ASSERT_X` menghentikan
proses sebelum fallback tersebut berjalan. Aplikasi berjalan sampai event loop
dengan platform offscreen; interaksi UI dan pemutaran media belum diuji.

Referensi mekanisme finalisasi:
[Qt: qt_add_executable](https://doc.qt.io/qt-6/qt-add-executable.html).
