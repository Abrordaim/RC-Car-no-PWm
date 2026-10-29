# Dokumentasi RC Car WiFi (Tanpa PWM)

ESP32 + DRV8833 · kontrol lewat halaman web di HP · diperbarui 29 Oktober 2026

## 1. Ringkasan

RC car 2WD yang PCB aslinya rusak dikendalikan ulang memakai board ESP32. ESP32 membuat jaringan WiFi sendiri, menyajikan halaman web berisi tombol, lalu mengubah perintah dari HP menjadi sinyal arah ke motor driver DRV8833.

Versi ini **tanpa PWM**: motor selalu berputar penuh sesuai tegangan baterai, jadi tidak ada pengaturan kecepatan. Keuntungannya, kode lebih sederhana dan tidak ada dengung PWM.

**Fitur**

- Maju, mundur, belok kiri, dan belok kanan dengan tombol tahan
- Penggerak dan steering bisa dipakai bersamaan (dua jari)
- Failsafe: mobil berhenti otomatis kalau perintah terputus
- Tanpa internet, router, atau aplikasi tambahan

**Di luar cakupan**: pengaturan kecepatan, kamera, dan remote fisik (lihat bagian 12).

**Diuji pada**: ESP32-S3 UNO WROOM-1 N16R8 dengan Arduino IDE.

## 2. Cara kerja

Alur sistem:

HP (browser) → WiFi RC-CAR → ESP32 (web server port 80) → fungsi motor → pin GPIO → DRV8833 → motor

Prinsip tombol tahan:

1. Saat tombol ditekan, browser langsung mengirim perintah (misalnya /maju) dan mengulanginya setiap 150 ms.
2. Saat tombol dilepas, browser mengirim /stop (penggerak) atau /lurus (steering).
3. Jika ESP32 tidak menerima perintah baru selama 400 ms, motor dihentikan otomatis. Ini melindungi saat WiFi putus, browser ditutup, atau layar HP terkunci.

Penggerak dan steering punya penghitung waktu failsafe masing-masing, jadi menahan tombol belok tidak membuat penggerak dianggap masih aktif.

Steering memakai motor DC biasa dengan pegas pengembali, sehingga hanya punya tiga keadaan: kiri, kanan, dan lurus (motor dilepas, lalu pegas meluruskan roda).

## 3. Kebutuhan hardware

### 3.1 Komponen minimal

| Komponen | Jumlah | Keterangan |
| --- | --- | --- |
| Board ESP32 | 1 | Lihat syarat minimal di 3.2 |
| Modul motor driver DRV8833 | 1 | Dua channel; bisa diganti TB6612 |
| Motor penggerak DC | 1 | Bawaan mobil |
| Motor steering DC | 1 | Bawaan mobil, dengan pegas pengembali |
| Baterai motor | 3 × AAA (4,5 V) | Bawaan mobil |
| Sumber daya ESP32 | Power bank 5 V | Terpisah dari baterai motor |
| Kapasitor elektrolit 470 µF | 1 | Di antara VM dan GND driver |
| Kapasitor keramik 100 nF | 2 | Satu di tiap terminal motor |
| Saklar on/off | 1 | Di jalur baterai motor |
| Kabel jumper / breadboard | secukupnya |  |

### 3.2 Syarat minimal board ESP32

| Syarat | Minimum | Catatan |
| --- | --- | --- |
| WiFi | 2,4 GHz dengan mode access point | Wajib |
| Pin output | 4 GPIO | Tidak perlu pin PWM, karena kode memakai digitalWrite |
| Flash | 4 MB | Sketch WiFi + web server memakai sekitar 1 MB (perkiraan; lihat ukuran sebenarnya di output compile) |
| PSRAM | Tidak perlu |  |
| Bluetooth | Tidak perlu |  |
| Level logika | 3,3 V | DRV8833 menerima sinyal 3,3 V |
| Arduino core | arduino-esp32 2.x atau 3.x | Kode tidak memakai fungsi ledc yang sintaksnya berbeda antar versi |
| Jalur upload | USB data atau modul USB-TTL |  |

### 3.3 Board yang cocok

- **Cocok**: ESP32 DevKit (WROOM-32), ESP32-S3 (termasuk DevKitC-1 dan UNO N16R8 yang diuji), ESP32-S2, ESP32-C3
- **Tidak cocok**: ESP32-H2 dan ESP32-P4, karena tidak punya WiFi bawaan
- **ESP8266**: tidak bisa langsung, perlu library lain (ESP8266WiFi dan ESP8266WebServer)

### 3.4 Pemilihan pin

| Fungsi | Pin di mobil ini (ESP32-S3) | Saran untuk ESP32 DevKit klasik |
| --- | --- | --- |
| Penggerak IN1 | GPIO 4 | GPIO 25 |
| Penggerak IN2 | GPIO 5 | GPIO 26 |
| Steering BIN1 | GPIO 6 | GPIO 27 |
| Steering BIN2 | GPIO 7 | GPIO 32 |

Aturan memilih pin di board lain:

- Harus bisa menjadi output (di ESP32 klasik, GPIO 34–39 hanya input)
- Hindari pin flash: GPIO 6–11 pada ESP32 klasik, dan GPIO 26–37 pada ESP32-S3 varian N16R8 (flash/PSRAM octal)
- Hindari pin strapping (GPIO 0, 2, 12, 15 pada ESP32 klasik; GPIO 0, 3, 45, 46 pada S3), karena levelnya saat boot menentukan mode start
- Pada S3, hindari GPIO 19 dan 20 jika USB native dipakai untuk upload
- Hindari pin yang memancarkan sinyal saat boot, supaya motor tidak berputar sesaat ketika board dinyalakan

## 4. Wiring

| Pin DRV8833 | Disambung ke |
| --- | --- |
| VM | Kutub + baterai motor (lewat saklar) |
| GND (satu sisi) | Kutub − baterai motor |
| GND (sisi lain) | GND ESP32 |
| AIN1 | GPIO penggerak IN1 |
| AIN2 | GPIO penggerak IN2 |
| BIN1 | GPIO steering BIN1 |
| BIN2 | GPIO steering BIN2 |
| STBY (nSLEEP) | 3V3 ESP32 (harus HIGH agar driver aktif) |
| AO1, AO2 | Motor penggerak |
| BO1, BO2 | Motor steering |
| NC | Dibiarkan kosong |

Catatan wiring:

- ESP32 dialiri dari power bank, bukan dari baterai motor, supaya lonjakan arus motor tidak mereset ESP32.
- Semua GND disatukan.
- Kapasitor 470 µF dipasang antara VM dan GND (kaki panjang ke VM); kapasitor 100 nF dipasang di antara dua terminal masing-masing motor.
- Kalau arah motor terbalik, tukar dua kabel motornya, atau tukar nama fungsi di kode.
- Jaga tegangan VM di sekitar 4,5–5 V. Pada 7,4 V motor berdengung tanpa bergerak, kemungkinan karena proteksi arus driver.


## 5. Penjelasan kode

### 5.1 Konfigurasi

| Konstanta | Fungsi |
| --- | --- |
| SSID, PASS | Nama dan sandi WiFi buatan ESP32 (sandi minimal 8 karakter) |
| IN1, IN2 | Pin penggerak ke AIN1 dan AIN2 |
| BIN1, BIN2 | Pin steering ke BIN1 dan BIN2 |
| TIMEOUT\_MS | Batas waktu failsafe, 400 ms |

### 5.2 Fungsi motor

| Fungsi | IN1 / BIN1 | IN2 / BIN2 | Efek |
| --- | --- | --- | --- |
| maju() | HIGH | LOW | Penggerak maju |
| mundur() | LOW | HIGH | Penggerak mundur |
| stopMotor() | LOW | LOW | Penggerak berhenti |
| kiri() | HIGH | LOW | Steering ke satu arah |
| kanan() | LOW | HIGH | Steering ke arah sebaliknya |
| lurus() | LOW | LOW | Motor steering dilepas, pegas meluruskan roda |

Variabel berjalan dan membelok mencatat apakah motor sedang aktif, supaya failsafe di loop() hanya bekerja saat diperlukan.

### 5.3 Rute HTTP

| Rute | Aksi |
| --- | --- |
| / | Menampilkan halaman kontrol |
| /maju | Penggerak maju, memperbarui lastDrive |
| /mundur | Penggerak mundur, memperbarui lastDrive |
| /stop | Penggerak berhenti |
| /kiri | Belok kiri, memperbarui lastSteer |
| /kanan | Belok kanan, memperbarui lastSteer |
| /lurus | Steering dilepas |

Semua rute perintah membalas teks "ok" supaya browser tidak menunggu tanpa kejelasan.

### 5.4 Halaman web

Halaman berupa satu string HTML (PROGMEM) berisi empat tombol. Fungsi tahan(id, path, stopPath) memakai pointer events (pointerdown, pointerup, pointercancel, pointerleave) supaya bekerja di layar sentuh maupun mouse, dan satu tombol tidak mengganggu tombol lain. Gaya touch-action: none dan user-select: none mencegah halaman bergulir, memperbesar, atau memilih teks saat tombol ditahan.

### 5.5 Loop dan failsafe

loop() memanggil server.handleClient() untuk melayani permintaan, lalu memeriksa dua hal: apakah penggerak aktif tanpa perintah baru lebih dari 400 ms (lalu stopMotor()), dan apakah steering aktif tanpa perintah baru lebih dari 400 ms (lalu lurus()).

## 6. Instalasi dan upload

1. Pasang Arduino IDE dan paket "esp32 by Espressif Systems" lewat Boards Manager.
2. Pilih board sesuai yang dipakai. Untuk ESP32-S3 UNO WROOM-1 N16R8: **ESP32S3 Dev Module** (bukan yang berlabel Octal WROOM2), Flash Size 16MB, PSRAM OPI PSRAM.
3. USB CDC On Boot: **Enabled** bila upload lewat USB native, **Disabled** bila lewat chip USB-serial atau modul USB-TTL.
4. Pilih port lalu klik Upload. Jika gagal, masuk mode download: sambungkan IO0 ke GND, tekan RESET, lalu lepas IO0 (board UNO ini hanya punya tombol RESET, tanpa tombol BOOT).
5. Di Linux, tambahkan user ke grup dialout (atau uucp), lalu logout dan login lagi.

Catatan untuk board ini: jika chip CH340C dinonaktifkan, upload dapat dilakukan lewat pin USB native IO19 (D−) dan IO20 (D+), atau lewat modul USB-TTL ke pin TXD dan RXD. Pin 4, 5, 6, dan 7 yang dipakai kode tidak bentrok dengan pin tersebut.

## 7. Cara memakai

1. Nyalakan ESP32 dari power bank, lalu nyalakan saklar baterai motor.
2. Di HP, sambungkan WiFi **RC-CAR** (sandi sesuai kode).
3. Buka browser dan ketik **http://192.168.4.1** (pakai http, bukan https).
4. Tahan tombol untuk bergerak, lepas untuk berhenti.

Jika halaman tidak terbuka di Android, matikan data seluler sementara dan pastikan HP tidak berpindah ke WiFi lain secara otomatis.

## 8. Pengujian

Urutan uji yang disarankan:

1. Roda diangkat; upload kode, lalu nyalakan saklar motor.
2. Tes penggerak: tahan MAJU, lepas, tahan MUNDUR, lepas.
3. Tes steering: tahan KIRI dan KANAN; roda harus kembali lurus saat dilepas.
4. Tes dua jari: tahan MAJU sambil menahan KIRI atau KANAN.
5. Tes failsafe: tahan MAJU lalu matikan WiFi HP atau tutup browser; mobil harus berhenti dalam sekitar setengah detik.
6. Tes di lantai dengan ruang kosong beberapa meter, dan perhatikan apakah ESP32 reset saat dua motor menyala bersamaan.
7. Raba chip DRV8833 dan motor setelah beberapa menit; hangat boleh, panas tidak.

## 9. Pemecahan masalah

| Gejala | Kemungkinan penyebab | Solusi |
| --- | --- | --- |
| Motor diam sama sekali | STBY belum HIGH, GND tidak bersama, atau saklar mati | Sambung STBY ke 3V3, satukan semua GND, cek saklar |
| Motor berdengung tanpa bergerak | VM terlalu tinggi sehingga proteksi arus driver aktif, atau baterai lemah | Turunkan VM ke sekitar 4,5–5 V, ganti baterai |
| ESP32 reset saat motor jalan | Daya ESP32 dan motor satu sumber | Pisahkan sumber, tambah kapasitor di VM |
| WiFi RC-CAR tidak muncul | Board tidak boot atau daya kurang | Cek power bank, lihat Serial Monitor (115200) |
| Halaman tidak terbuka | Memakai https atau data seluler mengganggu | Pakai http://192.168.4.1, matikan data seluler |
| Arah motor terbalik | Kabel motor tertukar | Tukar kabel motor atau nama fungsi |
| Roda tidak kembali lurus | Pegas steering lemah atau macet | Periksa mekanik steering |
| Upload gagal (permission denied) | User belum masuk grup serial | Tambahkan ke dialout atau uucp, lalu login ulang |
| Upload gagal (tidak ada data serial) | Belum masuk mode download | Hubungkan IO0 ke GND, tekan RESET, lepas IO0 |

## 10. Keselamatan dan batasan

- **Ganti sandi WiFi** dari 12345678 sebelum dipakai di tempat ramai; siapa pun yang tahu sandinya bisa mengendalikan mobil.
- Failsafe hanya bekerja lewat perangkat lunak. Kalau ESP32 hang, motor bisa tetap menyala, jadi saklar baterai motor harus mudah dijangkau.
- Jangan menahan belok lama saat roda sudah mentok, karena motor steering menarik arus besar.
- Jangan ubah arah penggerak secara mendadak; dengan tombol tahan, pergantian arah selalu lewat berhenti terlebih dulu.
- Ukur dulu output regulator atau buck converter dengan voltmeter sebelum disambung ke pin 5V board. Tegangan berlebih di pin yang salah dapat merusak chip USB-serial.
- Jangan menyambung USB dan suplai eksternal 5 V ke pin 5V secara bersamaan tanpa dioda pelindung.
- Jika memakai LiPo, pakai pelindung tegangan rendah, charger yang sesuai, dan sekering; jangan biarkan tegangan turun di bawah 3,0 V per sel.
- Kecepatan hanya satu (penuh). Tegangan VM menentukan seberapa cepat mobil berjalan.

