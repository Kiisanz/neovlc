# Arsitektur NeoVLC

## Modul dan dependensi

```text
app ──> backend/vlc ──> Qt Core + Qt Gui + libVLC
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

`VlcContext`, `VlcMediaPlayer`, dan `VlcVideoOutput` dibuat sebelum engine QML.
Karena destruksi stack berjalan terbalik, engine QML dihancurkan lebih dulu,
lalu sink video, lalu pemutar, lalu instance libVLC, dan terakhir aplikasi.
Ketiganya diekspos ke QML sebagai context property `vlcBackend`, `vlcPlayer`,
dan `vlcVideo`. Pemilik pemutar harus selalu menghancurkannya sebelum konteks,
dan sink video harus dihancurkan sebelum pemutarnya.

## Backend VLC

Target `neovlc_backend_vlc` mempunyai alias `NeoVLC::Vlc`. Backend memakai
namespace `neovlc::backend` dan menyediakan header melalui
`<vlc/VlcContext.h>`. Dependensi publiknya Qt Core dan Qt Gui (untuk `QImage`
yang membawa frame); header libVLC dan konfigurasi pkg-config berada pada
implementasi privat.

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

`VlcMediaPlayer::openFile()` menerima `QUrl` file lokal, memvalidasi bahwa
path merujuk ke file yang dapat dibaca, lalu membuat `libvlc_media_t` dengan
`libvlc_media_new_path()` dan memasangnya ke pemutar. Referensi media lokal
dilepas segera setelah dipasang karena pemutar menyimpan referensinya sendiri.
Path yang berhasil dibuka tersedia melalui properti `mediaPath`; kegagalan
memperbarui `errorString` dan tidak mengubah media yang sedang dimuat. URL
non-lokal ditolak sampai dukungan sumber jaringan ditambahkan. `openFile()`
mengembalikan `true` setelah descriptor media terpasang; nilai ini belum
menjamin isi file dapat diputar karena parsing dan validasi playback terjadi
secara asinkron di libVLC.

`play()`, `pause()`, `stop()`, `seek()`, dan `setVolume()` mengendalikan
pemutar melalui API libVLC. Kontrol transport memerlukan media yang sudah
dibuka dan mengembalikan `false` beserta pesan pada `errorString` bila belum
ada media. Volume disimpan di backend dalam rentang 0–200 dan diterapkan saat
pemutaran dimulai. `seek()` menerima waktu dalam milidetik; karena
`libvlc_media_player_set_time()` tidak mengembalikan status, keberhasilan seek
baru dapat dipastikan melalui event posisi pada langkah berikutnya.

Event libVLC (`Opening`, `Playing`, `Paused`, `Stopped`, `EndReached`,
`EncounteredError`, `TimeChanged`, `LengthChanged`) dipasang saat pemutar
dibuat dan dilepas sebelum pembongkaran. Callback dari thread VLC hanya
menyalin data event lalu memposting pemrosesan ke thread Qt. Setiap event
membawa identitas sesi media saat callback diterima; event yang sudah masuk
antrean dari media sebelumnya diabaikan setelah `openFile()` atau `stop()`
membuka sesi baru, sehingga tidak menimpa state media terbaru. Properti
`playbackState`, `playbackStateName`, `position`, dan `duration` diperbarui di
thread Qt. State `error` dipertahankan meskipun libVLC mengirim `Stopped`
setelah kegagalan, sampai media baru dibuka atau `stop()` dipanggil.

`VlcVideoOutput` memakai video callbacks libVLC untuk merender ke memori
(`RV32`). Buffer dialokasikan dengan pitch yang selaras 32 byte; mutex menjaga
buffer tetap stabil selama callback lock/unlock dan saat frame disalin.
Callback berjalan di thread video VLC. Salinan frame disimpan saat unlock,
tetapi baru dipublikasikan saat callback display agar waktu presentasi mengikuti
libVLC. Sink hanya menyimpan frame terbaru dan memakai satu notifikasi queued
yang dapat digabung, sehingga antrean frame tidak tumbuh saat thread Qt
tertinggal. Setiap frame membawa generasi media; frame dari sesi lama diabaikan
setelah media berganti. Ukuran yang diajukan vout dapat berisi baris padding,
sehingga callback format memakai ukuran track media yang terlihat dan
mempertahankan orientasi hasil rotasi vout. Destruktor menghentikan pemutaran,
lalu melepas callback vmem dengan nilai null karena libVLC 3 tidak menyediakan
API pelepasan khusus; pemutar yang dipakai ulang tidak akan memanggil sink yang
sudah dihancurkan. Sink wajib dihancurkan sebelum `VlcMediaPlayer`. Pendekatan
callbacks ini portabel dan menjaga presentasi di dalam scene graph, tetapi
menyalin frame ke memori CPU dan dapat menonaktifkan akselerasi hardware
libVLC; embedding window native dapat dipertimbangkan bila performa menjadi
kendala. Jangan memasukkan logika codec atau pemanggilan API VLC langsung ke
file QML.

## UI QML

`ui/Ui.cmake` mendaftarkan modul QML `NeoVLC` pada executable. Alias resource
menjaga alamat entry point `qrc:/NeoVLC/Main.qml` tetap eksplisit.

Kontrak yang tersedia saat ini adalah context property `vlcBackend` dengan
properti `available`, `version`, dan `errorString`, `vlcPlayer` dengan properti
`available`, `errorString`, `mediaPath`, `volume`, `playbackState`,
`playbackStateName`, `position`, dan `duration`, plus metode `openFile(url)`,
`play()`, `pause()`, `stop()`, `seek(milidetik)`, dan `setVolume(persen)`,
serta `vlcVideo` berupa `VlcVideoOutput`. `VideoItem` adalah elemen QML C++
yang didaftarkan melalui `QML_ELEMENT` di modul `NeoVLC`; properti `output`
menerima `vlcVideo`, dan `updatePaintNode()` mengunggah frame terbaru sebagai
tekstur dengan letterbox sesuai rasio sumber. Item memanggil `clear()` saat
`mediaPath` berubah agar frame lama tidak tertinggal.

`Main.qml` menyediakan tombol Open File, Play, Pause, dan Stop, `FileDialog`,
slider volume 0–200%, serta footer berisi slider seek, label waktu saat
ini/total, dan label state. `mediaPath` tampil di header dan `errorString` di
bagian bawah saat tidak kosong. QML menangani presentasi dan interaksi
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
proses sebelum fallback tersebut berjalan. Uji asap pembukaan file memastikan
file lokal valid termuat, `mediaPath` dan sinyalnya benar, serta file hilang,
URL kosong, dan URL non-lokal ditolak melalui `errorString`. Uji asap kontrol
pemutaran memutar file contoh, menerima play/pause/resume, seek, dan stop,
menolak kontrol tanpa media, serta menguji pembatasan volume 0–200. Uji asap
state memverifikasi transisi opening/playing/paused/stopped/ended, pembaruan
posisi dan durasi, posisi setelah seek, serta state error dan pesan kesalahan
saat media gagal diputar. Uji asap video memastikan frame `RV32` diterima dari
libVLC dengan dimensi yang benar untuk beberapa ukuran sumber, termasuk video
dengan metadata rotasi, dan memverifikasi orientasi piksel atas/bawah pada
video uji dua warna. Presentasi tekstur diuji ujung ke ujung dengan renderer
scene graph software offscreen: frame digambar dengan letterbox hitam pada
jendela yang rasio aspeknya berbeda, dan warna atas/bawah tetap benar. Impor
QML dan pemuatan `Main.qml` diverifikasi dengan `qmllint` serta eksekusi
offscreen; interaksi slider dan dialog file belum diuji.

Referensi mekanisme finalisasi:
[Qt: qt_add_executable](https://doc.qt.io/qt-6/qt-add-executable.html).
