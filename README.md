# MBKM_Aeroponik_documentation
berisi terkait dokumentasi MBKM Aeroponik dari kesleuruhan sistem/design/program 

# Menyelami Kode: Kontroler Modbus dan I/O Arduino ATmega128

Selamat datang di dokumentasi kode untuk firmware Arduino ATmega128 ini. Di sini, kita akan membedah bagaimana sistem ini dirancang untuk berfungsi sebagai kontroler canggih, membaca data dari berbagai sensor Modbus RTU, mengelola input digital, dan mengendalikan output digital (relay). Seluruh interaksi dengan sistem host (seperti komputer atau Raspberry Pi) difasilitasi melalui antarmuka serial menggunakan format data JSON yang terstruktur.

## Daftar Isi Panduan Kode

- [Memahami Fungsionalitas Inti](#memahami-fungsionalitas-inti)
- [Persiapan Perangkat Keras yang Digunakan](#persiapan-perangkat-keras-yang-digunakan)
- [Dependensi Perangkat Lunak & Library Pendukung](#dependensi-perangkat-lunak--library-pendukung)
- [Arsitektur Kode: Struktur Program](#arsitektur-kode-struktur-program)
  - [File Kode Utama](#file-kode-utama)
  - [Rincian Fungsi-Fungsi Kunci](#rincian-fungsi-fungsi-kunci)
- [Panduan Konfigurasi Sistem](#panduan-konfigurasi-sistem)
  - [Penentuan ID Sensor Modbus](#penentuan-id-sensor-modbus)
  - [Definisi dan Penetapan Pin](#definisi-dan-penetapan-pin)
  - [Pengaturan Baud Rate Serial](#pengaturan-baud-rate-serial)
- [Mekanisme Komunikasi Serial](#mekanisme-komunikasi-serial)
  - [Perintah Masuk: Instruksi dari Host ke Arduino](#perintah-masuk-instruksi-dari-host-ke-arduino)
  - [Aliran Data Keluar: Informasi dari Arduino ke Host](#aliran-data-keluar-informasi-dari-arduino-ke-host)
- [Langkah Awal: Setup dan Operasional](#langkah-awal-setup-dan-operasional)
- [Poin Kritis dan Pertimbangan Penting](#poin-kritis-dan-pertimbangan-penting)

## Memahami Fungsionalitas Inti

Sistem ini dirancang dengan kemampuan sebagai berikut:

-   **Akuisisi Data Modbus**: Mampu membaca data dari maksimal 6 perangkat slave Modbus RTU secara simultan.
-   **Pembacaan Register Dinamis**: Mengimplementasikan pola pembacaan register Modbus yang bervariasi, disesuaikan berdasarkan ID unik masing-masing sensor.
-   **Kontrol Output Presisi**: Mengendalikan hingga 4 output digital yang terhubung ke relay.
-   **Pemantauan Input Digital**: Membaca status dari 2 input digital, yang berfungsi sebagai pemicu (trigger) untuk berbagai aksi.
-   **Antarmuka Serial Cerdas**: Menyediakan kanal komunikasi serial untuk menerima perintah operasional dan mengirimkan status serta data sensor dalam format JSON yang mudah diolah.
-   **Manajemen RS485**: Menggunakan pin kontrol arah (DE/RE) khusus untuk memastikan komunikasi RS485 yang handal dan bebas tabrakan.

## Persiapan Perangkat Keras yang Digunakan

Untuk menjalankan firmware ini, pastikan perangkat keras berikut tersedia dan terhubung dengan benar:

1.  **Unit Mikrokontroler**: Sebuah papan pengembangan berbasis ATmega128 (contohnya, papan Outseal ATmega128).
2.  **Modul Transceiver RS485**: Komponen ini krusial untuk komunikasi Modbus RTU, kecuali jika sudah terintegrasi pada papan utama. Pin `DM` dalam kode dialokasikan untuk fungsi kontrol arah (Driver Enable) pada transceiver.
3.  **Array Sensor Modbus**: Hingga 6 unit sensor yang mendukung protokol Modbus RTU. Konfigurasi ID default dalam kode adalah `{7, 2, 3, 4, 5, 6}`.
4.  **Modul Relay**: Setidaknya 4 channel relay dibutuhkan untuk output yang dikontrol (`R1`, `R2`, `R3`, dan `R4`). Detail penugasan:
    * `R1` dikendalikan melalui perintah `PUMP1_ON`/`PUMP1_OFF`.
    * `R2` dikendalikan melalui perintah `PUMP2_ON`/`PUMP2_OFF`.
    * `R3` dikendalikan melalui perintah `ALARM1_ON`/`ALARM1_OFF`.
    * `R4` dikendalikan melalui perintah `PUMP3_ON`/`PUMP3_OFF`.
5.  **Sumber Input Digital**: Dua jalur input, bisa berupa saklar fisik, sensor kontak, atau output digital dari perangkat lain (terhubung ke pin `S1` dan `S2`).
6.  **Jalur Komunikasi Serial**:
    * `Serial`: Digunakan untuk interaksi utama dengan PC atau sistem host (untuk debugging, pengiriman perintah, dan penerimaan data JSON).
    * `Serial1`: Didedikasikan untuk jalur komunikasi Modbus RTU dengan perangkat sensor.

## Dependensi Perangkat Lunak & Library Pendukung

Pastikan lingkungan pengembangan Arduino Anda dilengkapi dengan:

-   **Arduino IDE**: Versi terbaru direkomendasikan untuk kompilasi dan proses upload firmware.
-   **`PinoutOutsealAtmega128.h`**: Ini adalah file header kustom yang vital. File ini berisi pemetaan pin spesifik papan yang Anda gunakan (misalnya, `DM`, `R1`, `R2`, `R3`, `R4`, `S1`, `S2`). *Keberadaan dan keakuratan konfigurasi file ini sangat penting.*
-   **`ModbusMaster.h`**: Library inti yang menyediakan fungsionalitas master Modbus RTU. (Umumnya versi dari Doc Walker, yang dapat diinstal melalui Arduino Library Manager).
-   **`ArduinoJson.h`**: Library serbaguna untuk parsing dan serialisasi data dalam format JSON. (Karya Benoît Blanchon, juga tersedia via Arduino Library Manager).

## Arsitektur Kode: Struktur Program

Mari kita lihat bagaimana kode ini diorganisir.

### File Kode Utama
Seluruh logika operasional, definisi, dan fungsi terkandung dalam file sketch utama (`.ino`).

### Rincian Fungsi-Fungsi Kunci

Berikut adalah fungsi-fungsi utama yang menjalankan sistem:

-   **`void setup()`**:
    -   Blok inisialisasi utama. Mengatur mode pin (input/output) untuk semua pin yang digunakan.
    -   Memulai komunikasi pada `Serial` (untuk host) dan `Serial1` (untuk Modbus), keduanya pada 9600 bps.
    -   Menginisialisasi instance objek `ModbusMaster` untuk setiap sensor dalam array `nodes[]`.
    -   Menetapkan fungsi `preTransmission()` dan `postTransmission()` sebagai callback untuk mengelola pin `DM` (kontrol arah RS485) secara otomatis.

-   **`void loop()`**:
    -   Jantung operasional program yang berjalan terus-menerus.
    -   Membaca status terkini dari input digital `S1` dan `S2`.
    -   Memeriksa apakah ada data perintah yang masuk melalui `Serial` dari host.
    -   Memproses perintah yang diterima: mengontrol variabel state output (`OUT_x`) atau memicu siklus pembacaan data sensor.
    -   Memperbarui array `holdingRegister[]` yang mencerminkan status output dan input terkini.
    -   Mengaktifkan atau menonaktifkan relay fisik (`R1`, `R2`, `R4`) berdasarkan nilai dalam `holdingRegister[]`.
    -   Memanggil `updateTriggers()` untuk menyegarkan status pemicu.

-   **`void preTransmission()`**: Fungsi callback yang dipanggil oleh library ModbusMaster sebelum transmisi data Modbus dimulai. Fungsinya adalah mengatur pin `DM` ke `HIGH`, mengaktifkan mode transmisi pada transceiver RS485.

-   **`void postTransmission()`**: Fungsi callback yang dipanggil setelah transmisi data Modbus selesai. Mengatur pin `DM` kembali ke `LOW`, mengembalikan transceiver RS485 ke mode penerimaan.

-   **`void updateTriggers()`**:
    -   Bertugas membaca kondisi aktual pin input `S1` dan `S2`.
    -   Memperbarui array boolean `trig[]` (di mana `trig[0]` untuk `S1`, `trig[1]` untuk `S2`). Status `true` (atau `1`) jika input `LOW`.
    -   Menyalin status `trig[]` ke `holdingRegisterInput[]`.
    -   Memperbarui elemen `holdingRegister[2]` dan `holdingRegister[3]` dengan status input terbaru.

-   **`void readSendDatasensor()`**:
    -   Fungsi komprehensif untuk akuisisi dan pengiriman data sensor.
    -   Memulai dengan mengosongkan dan menyiapkan `JsonDocument` untuk data baru.
    -   Melakukan iterasi melalui setiap sensor yang terdaftar dalam `nodes[]`.
    -   Untuk setiap sensor, melakukan pembacaan register input Modbus:
        -   **Sensor ID 3 dan 6**: Membaca tiga alamat register yang berbeda (misalnya, 0x0001, 0x0003, dan 0x0008), masing-masing sebanyak satu word (16-bit).
        -   **Sensor Lainnya**: Membaca dua register 16-bit secara berurutan, dimulai dari alamat 0x0000 (umumnya digunakan untuk data seperti suhu dan kelembapan).
        -   ⚠️ **Penting**: Akurasi parameter `u16ReadQty` (jumlah register yang dibaca) pada pemanggilan `readInputRegisters()` sangat krusial dan harus sesuai dengan spesifikasi masing-masing sensor. Kesalahan umum adalah keliru menggunakan ID sensor sebagai parameter ini.
    -   Data mentah dari sensor kemudian diskalakan (nilai register dibagi 10.0) sebelum ditambahkan ke array JSON. Jika hasil pembacaan adalah 0.0, maka nilai 0 akan ditambahkan. Idealnya, jika pembacaan gagal, nilai `null` atau placeholder error lainnya yang ditambahkan (membutuhkan penyesuaian kode).
    -   Setelah semua data sensor terkumpul, status pemicu (`trig[0]` dan `trig[1]`) juga ditambahkan ke array JSON.
    -   Akhirnya, seluruh array JSON diserialisasi dan dikirim melalui `Serial`, diakhiri dengan karakter newline (`\n`).

## Panduan Konfigurasi Sistem

Beberapa parameter dalam kode dapat disesuaikan untuk kebutuhan spesifik Anda:

### Penentuan ID Sensor Modbus

ID unik untuk setiap perangkat sensor Modbus didefinisikan dalam array global `sensorIDs`:
```cpp
const int sensorIDs[6] = {7, 2, 3, 4, 5, 6};
