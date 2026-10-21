# NeoVLC

Kerangka aplikasi desktop menggunakan C++17 dan Qt Quick/QML.

Repository tujuan: https://github.com/Kiisanz/neovlc.git

## Prasyarat

- Qt 6.2 atau lebih baru dengan modul Quick dan Quick Controls 2
- CMake 3.16 atau lebih baru
- Compiler C++17

## Struktur

- `main.cpp`: entry point aplikasi dan pemuat QML.
- `qml/Main.qml`: jendela utama aplikasi.
- `CMakeLists.txt`: konfigurasi proyek Qt dan modul QML `NeoVLC`.

Proyek ini baru disiapkan sebagai kerangka QML. Belum ada integrasi VLC
atau fitur pemutaran media. Konfigurasi dan build belum dijalankan selama
penyiapan ini. Folder `build/` yang sudah ada berasal dari proyek sebelumnya
dan diabaikan oleh Git.
