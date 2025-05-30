# **MBKM Aeroponik documentation** :seedling:
berisi terkait dokumentasi MBKM Aeroponik dari kesleuruhan sistem/design/program 

## Daftar isi Dokumentasi Sistem Aeroponik 

## 1. Sistem Keseluruhan 

## 2. Desain dari sistem

## 3. Panduan Lengkap Program Panel Slave

Pada bagian ini membberikan penjelasan terkait program arduino yang dirancang untuk board ATmega128 dalam Outseal Mikrokontroler from PLC Outseal. yang mana berfungsi untuk membaca data dari beberapa sensor dengan menggunakan modbus RTU, yang mengendalikan output digital (relay) serta berkomunikasi dengan panel master melalui antarmuka serial menggunakan format JSON.

### Daftar Isi Penjelasan Program
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

### 3. Fungsi Kontrol Transmisi Modbus

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

### 4. Fungsi setup()

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

### 5. Fungsi UpdateTriggers

Fungsi ini bertanggung jawab untuk membaca status terkini dari input digital (S1 dan S2) dan memperbarui variabel-variabel global yang menyimpan status pemicu (trigger) ini. Ini memastikan bahwa representasi internal dari status input selalu sinkron dengan kondisi fisik input.

```cpp
// --- Fungsi untuk Memperbarui Status Pemicu (Trigger) ---
// Fungsi ini membaca input digital S1 dan S2 dan memperbarui variabel terkait.
void updateTriggers() {
  // Membaca pin S1. Jika S1 dalam kondisi LOW (misalnya tombol ditekan atau sensor aktif LOW), 
  // maka ekspresi 'digitalRead(S1) == LOW' bernilai true (atau 1 dalam konteks integer).
  // Sebaliknya, jika HIGH, bernilai false (atau 0).
  trig[0] = digitalRead(S1) == LOW;
  // Hal yang sama berlaku untuk pin S2 dan variabel trig[1].
  trig[1] = digitalRead(S2) == LOW;

  // Menyimpan status trigger yang baru dibaca ke dalam array holdingRegisterInput.
  // Ini bisa berguna jika Anda ingin mengekspos status input ini melalui Modbus (jika Arduino jadi slave)
  // atau untuk logging internal yang terstruktur.
  holdingRegisterInput[0] = trig[0];
  holdingRegisterInput[1] = trig[1];

  // Juga memperbarui elemen-elemen spesifik dalam array holdingRegister umum yang dialokasikan untuk input.
  holdingRegister[2] = holdingRegisterInput[0]; // Merefleksikan status Input 1 (S1)
  holdingRegister[3] = holdingRegisterInput[1]; // Merefleksikan status Input 2 (S2)
}
```
Penjelasan:

- trig[0] = digitalRead(S1) == LOW;: Baris ini membaca level logika pada pin S1. Jika levelnya LOW (yang seringkali berarti aktif untuk sensor atau tombol yang terhubung ke ground), maka digitalRead(S1) == LOW akan menghasilkan true (yang dikonversi menjadi 1 saat disimpan ke trig[0]). Jika HIGH, hasilnya false (dikonversi menjadi 0). Proses serupa terjadi untuk S2 dan trig[1].
- Status dari trig[] kemudian disalin ke holdingRegisterInput[] dan juga ke elemen-elemen tertentu (holdingRegister[2] dan holdingRegister[3]) dari array holdingRegister[] utama. Ini menyediakan beberapa cara untuk mengakses status input yang sama, yang mungkin berguna tergantung pada bagaimana data digunakan atau diekspos.

### 6. Fungsi ReadSendDatasensor
  
### 7. Fungsi loop

Langsung saja-lah ya

```cpp
// --- Fungsi Loop Utama ---
// Fungsi loop() berjalan berulang kali selamanya setelah setup() selesai.
void loop() {
  // 1. Baca status input digital S1 dan S2 secara langsung.
  // Nilai ini disimpan ke IN_1 dan IN_2.
  // Pembaruan trig[] di sini mungkin tumpang tindih dengan updateTriggers(),
  // namun memastikan IN_1/IN_2 dan trig[] selalu up-to-date di awal loop.
  IN_1 = digitalRead(S1) == LOW;
  IN_2 = digitalRead(S2) == LOW;

  trig[0] = IN_1; // Sinkronisasi trig dengan pembacaan langsung.
  trig[1] = IN_2;

  // 2. Periksa apakah ada data (perintah) yang masuk dari port Serial (dari host).
  if (Serial.available() > 0) {
    // Jika ada data, baca seluruh string perintah sampai karakter newline ('\n') ditemukan.
    String input = Serial.readStringUntil('\n');
    input.trim(); // Hapus spasi putih (whitespace) di awal atau akhir string perintah.

    // Proses perintah berdasarkan karakter pertama dari string input.
    // Ini adalah cara sederhana untuk routing perintah.
    switch (input.charAt(0)) {
      case 'P': // Perintah yang dimulai dengan 'P' diasumsikan untuk PUMP (Pompa/Output).
        if (input == "PUMP1_ON") {
          OUT_1 = 1; // Set status target OUT_1 menjadi ON.
        } else if (input == "PUMP1_OFF") {
          OUT_1 = 0; // Set status target OUT_1 menjadi OFF.
        } else if (input == "PUMP2_ON") {
          OUT_2 = 1;
        } else if (input == "PUMP2_OFF") {
          OUT_2 = 0;
        } else if (input == "PUMP3_ON") {
          OUT_3 = 1; // OUT_3 akan mengontrol Relay R4.
        } else if (input == "PUMP3_OFF") {
          OUT_3 = 0;
        } else {
          // Jika perintah 'P' tidak dikenal, kirim pesan error.
          Serial.println("Unknown PUMP command: " + input);
        }
        break; // Akhir dari case 'P'.

      case 'R': // Perintah yang dimulai dengan 'R' diasumsikan untuk READ (Baca Data).
        if (input == "READ_DATA") {
          // Panggil fungsi readSendDatasensor() sebanyak 5 kali.
          // Ini berarti Arduino akan membaca semua sensor dan mengirim paket JSON
          // sebanyak 5 kali berturut-turut untuk satu perintah "READ_DATA".
          // Jika Anda hanya ingin satu kali pembacaan dan pengiriman per perintah,
          // hapus loop 'for' ini dan panggil readSendDatasensor() sekali saja.
          for (int i = 0; i < 5; i++) {
            readSendDatasensor();
          }
        } else {
          Serial.println("Unknown READ command: " + input);
        }
        break; // Akhir dari case 'R'.

      case 'A': // Perintah yang dimulai dengan 'A' diasumsikan untuk ALARM.
        if (input == "ALARM1_ON") {
          digitalWrite(R3, HIGH); // Langsung mengaktifkan Relay R3.
                                  // (Gunakan HIGH atau LOW tergantung cara relay Anda di-wire,
                                  //  misalnya jika relay aktif LOW, maka gunakan LOW untuk ON).
                                  // Asumsi di sini: HIGH = ON.
        } else if (input == "ALARM1_OFF") {
          digitalWrite(R3, LOW);  // Langsung menonaktifkan Relay R3.
                                  // Asumsi di sini: LOW = OFF.
        } else {
          Serial.println("Unknown ALARM command: " + input);
        }
        break; // Akhir dari case 'A'.  

      default: // Jika karakter pertama perintah tidak cocok dengan case manapun.
        Serial.println("Unknown Command: " + input);
        break;
    }
  }

  // 3. Perbarui array-array holdingRegister dengan status output dan input saat ini.
  // Ini menyatukan status dari berbagai sumber (perintah serial, pembacaan input)
  // ke dalam satu set buffer data.
  holdingRegisterOutput[0] = OUT_1; // Status target untuk PUMP1.
  holdingRegisterOutput[1] = OUT_2; // Status target untuk PUMP2.
  holdingRegisterOutput[2] = OUT_3; // Status target untuk PUMP3 (yang mengontrol R4).

  // holdingRegisterInput sudah diperbarui oleh updateTriggers() atau pembacaan langsung IN_1/IN_2 di atas.
  holdingRegisterInput[0] = IN_1; 
  holdingRegisterInput[1] = IN_2;

  // Salin nilai dari buffer input/output spesifik ke array holdingRegister utama.
  // Pemetaan ini menentukan bagaimana status I/O internal direpresentasikan dalam holdingRegister.
  holdingRegister[0] = holdingRegisterOutput[0]; // Map OUT_1 ke holdingRegister[0].
  holdingRegister[1] = holdingRegisterOutput[1]; // Map OUT_2 ke holdingRegister[1].
  holdingRegister[2] = holdingRegisterInput[0];  // Map IN_1 (status S1) ke holdingRegister[2].
  holdingRegister[3] = holdingRegisterInput[1];  // Map IN_2 (status S2) ke holdingRegister[3].
  holdingRegister[4] = holdingRegisterOutput[2]; // Map OUT_3 ke holdingRegister[4].

  // 4. Tulis status output dari holdingRegister ke pin-pin relay fisik.
  // Relay R3 dikontrol langsung oleh perintah 'A' (ALARM) dan tidak melalui sistem holdingRegister ini.
  digitalWrite(R1, holdingRegister[0]); // Relay R1 dikontrol oleh status OUT_1.
  digitalWrite(R2, holdingRegister[1]); // Relay R2 dikontrol oleh status OUT_2.
  digitalWrite(R4, holdingRegister[4]); // Relay R4 dikontrol oleh status OUT_3.
                                        // (karena holdingRegister[4] = holdingRegisterOutput[2] = OUT_3).

  // 5. Panggil updateTriggers() lagi di akhir loop.
  // Ini memastikan bahwa sebelum iterasi loop berikutnya dimulai, status pemicu
  // (trig[], holdingRegisterInput[], holdingRegister[2,3]) sudah yang paling baru.
  updateTriggers();

  // 6. Beri jeda singkat pada akhir setiap iterasi loop.
  // Ini bisa membantu dalam mengurangi beban CPU, memberikan waktu untuk proses lain (jika ada interrupt),
  // atau menyesuaikan dengan interval waktu tertentu jika program ini bagian dari sistem yang lebih besar.
  // Nilai 50ms adalah contoh; sesuaikan sesuai kebutuhan.
  delay(50); 
}
```
Penjelasan Rinci loop():
1. Pembacaan Input Awal: IN_1 = digitalRead(S1) == LOW; dan IN_2 = digitalRead(S2) == LOW; membaca status pin S1 dan S2. Hasilnya (1 jika LOW, 0 jika HIGH) disimpan ke IN_1 dan IN_2, lalu disalin ke trig[0] dan trig[1].

2. Pemrosesan Perintah Serial:
- if (Serial.available() > 0): Mengecek apakah ada data yang dikirim dari host melalui port Serial.
- String input = Serial.readStringUntil('\n');: Jika ada, membaca seluruh baris teks hingga karakter newline (\n) sebagai perintah.
- input.trim(): Menghapus spasi di awal/akhir perintah.
- switch (input.charAt(0)): Memproses perintah berdasarkan karakter pertamanya:
  - case 'P' (PUMP): Jika perintah diawali 'P', kode akan membandingkan input dengan string perintah lengkap seperti "PUMP1_ON", "PUMP1_OFF", dll. Jika cocok, variabel OUT_1, OUT_2, atau OUT_3 akan diubah (menjadi 1 untuk ON, 0 untuk OFF). Perhatikan bahwa OUT_3 nantinya akan mengontrol relay R4.
  - case 'R' (READ): Jika perintah adalah "READ_DATA", fungsi readSendDatasensor() akan dipanggil 5 kali berturut-turut. Ini berarti untuk satu perintah "READ_DATA", Arduino akan melakukan 5 siklus pembacaan semua sensor dan mengirim 5 paket JSON terpisah.
  - case 'A' (ALARM): Jika perintah "ALARM1_ON" atau "ALARM1_OFF", pin relay R3 akan langsung dikontrol menggunakan digitalWrite(). Ini berbeda dengan relay lain yang dikontrol melalui variabel OUT_x.
  - default: Jika karakter pertama tidak cocok dengan 'P', 'R', atau 'A', atau jika perintah lengkap tidak dikenal dalam sub-kondisi, pesan error akan dikirim kembali melalui Serial.

3. Pembaruan holdingRegister:
- Status variabel OUT_1, OUT_2, OUT_3 disalin ke holdingRegisterOutput[].
- Status IN_1, IN_2 disalin ke holdingRegisterInput[].
- Kemudian, nilai-nilai dari holdingRegisterOutput[] dan holdingRegisterInput[] dipetakan ke dalam array holdingRegister[] yang lebih besar. Ini menciptakan representasi terpusat dari status I/O sistem.
    - holdingRegister[0] untuk OUT_1
    - holdingRegister[1] untuk OUT_2
    - holdingRegister[2] untuk IN_1 (status S1)
    - holdingRegister[3] untuk IN_2 (status S2)
    - holdingRegister[4] untuk OUT_3

4. Pengaktifan Relay Fisik:
- digitalWrite(R1, holdingRegister[0]);: Mengatur kondisi pin R1 sesuai dengan nilai di holdingRegister[0] (yang berasal dari OUT_1).
- digitalWrite(R2, holdingRegister[1]);: Mengatur pin R2 sesuai OUT_2.
- digitalWrite(R4, holdingRegister[4]);: Mengatur pin R4 sesuai OUT_3.
- Relay R3 tidak diatur di sini karena sudah dikontrol langsung dalam case 'A'.

5. updateTriggers() (Akhir Loop): Memanggil fungsi ini lagi untuk memastikan bahwa sebelum loop berikutnya dimulai, semua variabel yang terkait dengan status input (trig[], holdingRegisterInput[], dll.) sudah mencerminkan kondisi fisik terkini dari S1 dan S2.
6. delay(50): Memberikan jeda 50 milidetik di akhir setiap iterasi loop(). Ini bisa berguna untuk stabilitas, mengurangi penggunaan CPU, atau memberikan waktu bagi proses lain (jika ada, seperti interrupt) untuk berjalan.
