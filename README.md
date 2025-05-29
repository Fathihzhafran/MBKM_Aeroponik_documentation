# MBKM_Aeroponik_documentation
berisi terkait dokumentasi MBKM Aeroponik dari kesleuruhan sistem/design/program 

## Panduan Lengkap Program Panel Slave

Pada bagian ini membberikan penjelasan terkait program arduino yang dirancang untuk board ATmega128 dalam Outseal Mikrokontroler from PLC Outseal. yang mana berfungsi untuk membaca data dari beberapa sensor dengan menggunakan modbus RTU, yang mengendalikan output digital (relay) serta berkomunikasi dengan panel master melalui antarmuka serial menggunakan format JSON.

### Daftar Isi
1.  [Pendahuluan](###pendahuluan-program)
2.  [Inklusi Library dan Definisi Global](###inklusi-library-dan-definisi-global)
3.  [Fungsi Kontrol Transmisi Modbus](###fungsi-kontrol-transmisi-modbus)
4.  [Fungsi `setup()`](###fungsi-setup)
5.  [Fungsi `updateTriggers()`](###fungsi-updatetriggers)
6.  [Fungsi `readSendDatasensor()`](###fungsi-readsenddatasensor)
7.  [Fungsi `loop()`](###fungsi-loop)
8.  [Kesimpulan](###kesimpulan-program)

---

### 1. Pendahuluan Program

Program ini mengubah Arduino ATmega128 menjadi sebuah sistem kontrol dan monitoring. Kemampuan utamanya meliputi:
-   Akuisisi data dari maksimal 6 sensor Modbus RTU.
-   Pengendalian hingga 4 output digital (relay untuk pompa atau lampu).
-   Pembacaan 2 input digital (sensor CWT-TH04S dengan modbus RS485).
-   Komunikasi dua arah dengan sistem host/panel master (PC/Personal Computer) melalui port serial, menggunakan perintah teks dan mengirimkan data dalam format JSON.

---

### 2. Inklusi Library dan Definisi Global

Bagian awal kode berisi deklarasi library yang dibutuhkan dan variabel global yang akan digunakan di seluruh program.

```cpp
// Bagian ini adalah tempat kita menyertakan library-library yang dibutuhkan.
// Library adalah kumpulan kode yang sudah jadi untuk melakukan tugas tertentu.
#include <PinoutOutsealAtmega128.h> // Library khusus untuk pinout papan Outseal ATmega128 Anda.
                                     // Pastikan file ini ada dan benar.
#include <ModbusMaster.h>            // Library untuk komunikasi Modbus sebagai Master.
#include <ArduinoJson.h>             // Library untuk membuat dan membaca data format JSON.

// --- Deklarasi Variabel Global dan Objek ---
// Variabel global bisa diakses dari fungsi manapun dalam program.

// Membuat array objek ModbusMaster. Kita punya 6 sensor, jadi ada 6 objek.
ModbusMaster nodes[6];
// Array untuk menyimpan ID Modbus dari masing-masing sensor.
const int sensorIDs[6] = {7, 2, 3, 4, 5, 6};

// Array untuk menyimpan status pemicu (trigger) dari input digital S1 dan S2.
int trig[2];

// Variabel untuk menyimpan status output (misalnya untuk Pompa).
// 0 = OFF, 1 = ON.
int OUT_1, OUT_2, OUT_3, OUT_4; // OUT_4 dideklarasikan tapi belum digunakan secara aktif.
// Variabel untuk menyimpan status input digital S1 dan S2.
int IN_1, IN_2;

// Register penampung untuk Modbus (jika Anda menggunakannya sebagai slave Modbus juga)
// atau hanya sebagai buffer internal.
uint16_t holdingRegisterOutput[3]; // Untuk status output.
uint16_t holdingRegisterInput[2];  // Untuk status input.
uint16_t holdingRegister[20];      // Array holding register serbaguna.

// Objek untuk membuat dokumen JSON. Ukuran 512 byte dialokasikan.
// Sesuaikan ukurannya jika data JSON Anda lebih besar.
StaticJsonDocument<512> jsonDoc;
```
Penjelasan Variabel Global:
- PinoutOutsealAtmega128.h: File header kustom yang mendefinisikan nama-nama pin (seperti DM, R1, S1, dll.) agar sesuai dengan papan Outseal ATmega128 Anda. Keberadaan dan keakuratan file ini sangat penting.
- ModbusMaster.h: Library yang menyediakan fungsionalitas untuk menjadikan Arduino sebagai perangkat Master dalam jaringan Modbus RTU, memungkinkannya meminta data dari perangkat Slave (sensor).
- ArduinoJson.h: Library serbaguna untuk memudahkan pembuatan dan parsing (pembacaan) data dalam format JSON, yang umum digunakan untuk pertukaran data terstruktur.
- nodes[6]: Sebuah array yang akan menyimpan 6 objek ModbusMaster. Setiap objek akan mengelola komunikasi dengan satu sensor Modbus.
- sensorIDs[6]: Array konstanta integer yang menyimpan alamat (ID) Modbus dari keenam sensor yang akan dihubungi. Urutan ID di sini harus sesuai dengan urutan objek nodes.
- trig[2]: Array integer untuk menyimpan status dari dua input digital (S1 dan S2). Biasanya, nilai 1 menandakan input aktif (misalnya, tombol ditekan atau sensor terpicu menjadi LOW), dan 0 menandakan tidak aktif.
- OUT_1, OUT_2, OUT_3, OUT_4: Variabel integer yang digunakan untuk menyimpan status yang diinginkan (ON/OFF, direpresentasikan sebagai 1/0) untuk output digital. OUT_4 dideklarasikan tetapi dalam logika loop() saat ini tidak secara eksplisit mengontrol output fisik.
- IN_1, IN_2: Variabel integer untuk menyimpan hasil pembacaan langsung dari pin input digital S1 dan S2.
- holdingRegisterOutput[3], holdingRegisterInput[2], holdingRegister[20]: Array-array ini berfungsi sebagai buffer data internal. Mereka dapat digunakan untuk memetakan status I/O ke format register Modbus jika Arduino juga dirancang untuk berperan sebagai perangkat Slave Modbus, atau hanya sebagai cara terstruktur untuk mengelola data state internal.
- jsonDoc: Objek dari library ArduinoJson yang digunakan untuk membangun struktur data JSON sebelum dikirim melalui serial. Ukuran 512 byte adalah kapasitas memori yang dialokasikan untuk dokumen JSON ini; jika data JSON Anda lebih besar, ukuran ini perlu ditingkatkan.

### Fungsi Kontrol Transmisi Modbus

Untuk komunikasi RS485, yang bersifat half-duplex (mengirim atau menerima secara bergantian pada jalur yang sama), diperlukan kontrol arah transmisi. Pin DM (Driver Master, sesuai definisi di PinoutOutsealAtmega128.h) digunakan untuk tujuan ini, biasanya terhubung ke pin DE (Driver Enable) dan RE (Receiver Enable) pada chip transceiver RS485 dengan MAX485.

```cpp
// --- Fungsi untuk Kontrol Transmisi RS485 ---

// Fungsi ini akan dipanggil SEBELUM library ModbusMaster mengirim data.
// Berguna untuk mengaktifkan mode transmisi pada driver RS485 (misalnya MAX485).
void preTransmission() {
  digitalWrite(DM, HIGH); // DM adalah pin yang terhubung ke Driver Enable (DE) pada MAX485.
}

// Fungsi ini akan dipanggil SETELAH library ModbusMaster selesai mengirim data.
// Mengembalikan driver RS485 ke mode menerima.
void postTransmission() {
  digitalWrite(DM, LOW); // Mengatur DM ke LOW.
}
```

Penjelasan
- preTransmission(): Fungsi ini secara otomatis dipanggil oleh library ModbusMaster tepat sebelum Arduino mengirimkan permintaan Modbus ke sensor. Dengan mengatur pin DM ke HIGH, transceiver RS485 diaktifkan untuk mode transmisi (mengirim data).
- postTransmission(): Fungsi ini dipanggil secara otomatis setelah Arduino selesai mengirimkan permintaan Modbus. Dengan mengatur pin DM ke LOW, transceiver RS485 kembali ke mode penerimaan, siap untuk menangkap respons dari sensor.

### Fungsi setup()

Fungsi yang mana diinisiasikan sekali dalam program arduino, dengan melakukan konfigurasi dan inialisasi awal yang diperlukan agar program dapat berjalan dengan lancar dan benar.

```cpp
// --- Fungsi Setup: Inisialisasi Awal ---
// Fungsi setup() hanya berjalan sekali ketika Arduino pertama kali dinyalakan atau di-reset.
void setup() {
  // Mengatur pin DM sebagai OUTPUT dan awalnya LOW (mode menerima untuk RS485).
  pinMode(DM, OUTPUT);
  digitalWrite(DM, LOW);

  // Memulai komunikasi serial untuk debugging atau mengirim data JSON ke PC/host.
  // 9600 adalah baud rate (kecepatan komunikasi dalam bit per detik).
  Serial.begin(9600);
  // Memulai komunikasi serial (Serial1) untuk Modbus yang terhubung ke sensor.
  Serial1.begin(9600); // Pastikan baud rate ini sama dengan semua sensor Modbus Anda.

  // Inisialisasi setiap node Modbus (sensor).
  for (int i = 0; i < 6; i++) {
    // Mengaitkan setiap objek 'nodes[i]' dengan ID sensor dan port Serial1.
    nodes[i].begin(sensorIDs[i], Serial1); 
    // Menetapkan fungsi callback untuk kontrol transmisi RS485.
    nodes[i].preTransmission(preTransmission);
    nodes[i].postTransmission(postTransmission);
  }

  // Mengatur kondisi awal untuk semua variabel status output ke OFF (0).
  OUT_1 = OUT_2 = OUT_3 = OUT_4 = 0;
  // Mengatur kondisi awal untuk variabel status input (akan diperbarui oleh pembacaan aktual).
  IN_1 = IN_2 = 0;

  // Mengatur pin-pin yang terhubung ke relay (R1-R4) sebagai OUTPUT.
  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(R3, OUTPUT);
  pinMode(R4, OUTPUT);
  // Mengatur pin-pin yang terhubung ke sensor input (S1-S2) sebagai INPUT.
  // Jika menggunakan resistor pull-up internal Arduino untuk input (agar tidak floating saat tidak terhubung):
  // pinMode(S1, INPUT_PULLUP);
  // pinMode(S2, INPUT_PULLUP);
  // Jika tidak, pastikan ada resistor pull-up atau pull-down eksternal pada rangkaian input
  // untuk mendefinisikan level logika yang jelas saat input tidak aktif.
  pinMode(S1, INPUT); 
  pinMode(S2, INPUT); 
}
```

Penjelasan terkait setup();
- Pin DM diinisialisasi sebagai OUTPUT dan disetel ke LOW (mode terima default untuk RS485).
- Serial.begin(9600): Mengaktifkan port serial standar (biasanya terhubung ke komputer melalui USB) untuk debugging, menerima perintah, dan mengirim data JSON.
- Serial1.begin(9600): Mengaktifkan port serial perangkat keras kedua (Serial1 pada ATmega128, biasanya pin TX1 dan RX1) yang didedikasikan untuk komunikasi Modbus RTU dengan sensor-sensor. Sangat penting bahwa baud rate ini sesuai dengan konfigurasi semua sensor Modbus di jaringan.
- Loop Inisialisasi Node Modbus: Loop for berjalan sebanyak jumlah sensor (6 kali). Di setiap iterasi:
    - nodes[i].begin(sensorIDs[i], Serial1): Menginisialisasi objek ModbusMaster ke-i. Ini memberitahu library ID slave Modbus yang akan diajak bicara (sensorIDs[i]) dan port serial mana yang akan digunakan (Serial1).
    - nodes[i].preTransmission(preTransmission) dan nodes[i].postTransmission(postTransmission): Mendaftarkan fungsi-fungsi callback yang telah kita buat sebelumnya untuk menangani kontrol arah transceiver RS485 secara otomatis setiap kali objek       - nodes[i] melakukan transaksi Modbus.
- Inisialisasi Variabel Status: Variabel OUT_x (status output) diatur ke 0 (OFF), dan IN_x (status input) juga diatur ke 0 sebagai nilai awal.
- Konfigurasi pinMode():
    - Pin-pin yang terhubung ke relay (R1, R2, R3, R4) dikonfigurasi sebagai OUTPUT.
    - Pin-pin yang terhubung ke input digital (S1, S2) dikonfigurasi sebagai INPUT. Pertimbangkan penggunaan INPUT_PULLUP jika input Anda adalah saklar atau sensor kontak yang terhubung ke ground saat aktif, untuk menghindari kebutuhan resistor pull-up eksternal.

### Fungsi UpdateTriggers
