# **MBKM Aeroponik documentation** :seedling:
Dokumentasi untuk sistem **MBKM Aeroponik**, mencakup seluruh aspek dari sistem, desain, hingga pemrograman yang digunakan dalam implementasi proyek ini.

![Ilustrasi Sistem Aeoroponik dalam rancangan ini](https://github.com/Fathihzhafran/MBKM_Aeroponik_documentation/blob/f4ab73c94ae08fb66da3e35dbf9a9b10b971b886/Image/Aeroponik6.JPG)

_by fathih_

## Daftar isi Dokumentasi Sistem Aeroponik 

1. [Sistem Keseluruhan](#1-sistem-keseluruhan)  
2. [Desain dari Sistem](#2-desain-dari-sistem)  
3. [Panduan Lengkap Program Panel Slave](#3-panduan-lengkap-program-panel-slave)

## 1. Sistem Keseluruhan 

_(Bagian ini berisi penjelasan umum sistem aeroponik: perangkat keras yang digunakan, komunikasi antar panel, aliran data, dsb.)_

## 2. Desain dari sistem

_(Bagian ini memuat skema desain sistem aeroponik, diagram blok, relasi antar modul, serta desain topologi komunikasi.)_

## 3. Panduan Lengkap Program Panel Slave

Bagian ini menjelaskan program Arduino yang dijalankan pada **board ATmega128** dari **Outseal Mikrokontroler (PLC Outseal)**. Program ini digunakan untuk:

- Membaca data dari sensor-sensor menggunakan protokol **Modbus RTU**.
- Mengendalikan beberapa **output digital** seperti relay (untuk pompa, lampu, dll).
- Membaca **input digital** dari sensor eksternal.
- Berkomunikasi dua arah dengan **panel master (PC/komputer)** melalui serial, dalam format **JSON**.

### Daftar Isi Penjelasan Program

1. [Pendahuluan Program](#1-pendahuluan-program)  
2. [Inklusi Library dan Definisi Global](#2-inklusi-library-dan-definisi-global)  
3. [Fungsi Kontrol Transmisi Modbus](#3-fungsi-kontrol-transmisi-modbus)  
4. [Fungsi `setup()`](#4-fungsi-setup)  
5. [Fungsi `updateTriggers()`](#5-fungsi-updatetriggers)  
6. [Fungsi `readSendDatasensor()`](#6-fungsi-readsenddatasensor)  
7. [Fungsi `loop()`](#7-fungsi-loop)  
8. [Kesimpulan Program](#8-kesimpulan-program)
---

### 1. Pendahuluan Program

Program ini mengubah Arduino ATmega128 menjadi sebuah sistem kontrol dan monitoring. Kemampuan utamanya meliputi:
-   Akuisisi data dari maksimal 6 sensor Modbus RTU.
-   Pengendalian hingga 4 output digital (relay untuk pompa atau lampu).
-   Pembacaan 2 input digital (sensor CWT-TH04S dengan modbus RS485).
-   Komunikasi dua arah dengan sistem host/panel master (PC/Personal Computer) melalui port serial, menggunakan perintah teks dan mengirimkan data dalam format JSON.

---

### 2. Inklusi Library dan Definisi Global

Bagian awal kode mencakup library yang digunakan dan variabel global yang diperlukan dalam program:

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
- `PinoutOutsealAtmega128.h`: File header kustom yang mendefinisikan nama-nama pin (seperti `DM`, `R1`, `S1`, dll.) agar sesuai dengan papan Outseal ATmega128 Anda. Keberadaan dan keakuratan file ini sangat penting.
- `ModbusMaster.h`: Library yang menyediakan fungsionalitas untuk menjadikan Arduino sebagai perangkat Master dalam jaringan Modbus RTU, memungkinkannya meminta data dari perangkat Slave (sensor).
- `ArduinoJson.h`: Library serbaguna untuk memudahkan pembuatan dan parsing (pembacaan) data dalam format JSON, yang umum digunakan untuk pertukaran data terstruktur.
- `nodes[6]`: Sebuah array yang akan menyimpan 6 objek `ModbusMaster`. Setiap objek akan mengelola komunikasi dengan satu sensor Modbus.
- `sensorIDs[6]`: Array konstanta integer yang menyimpan alamat (ID) Modbus dari keenam sensor yang akan dihubungi. Urutan ID di sini harus sesuai dengan urutan objek `nodes`.
- `trig[2]`: Array integer untuk menyimpan status dari dua input digital (`S1` dan `S2`). Biasanya, nilai `1` menandakan input aktif (misalnya, tombol ditekan atau sensor terpicu menjadi `LOW`), dan `0` menandakan tidak aktif.
- `OUT_1`, `OUT_2`, `OUT_3`, `OUT_4`: Variabel integer yang digunakan untuk menyimpan status yang diinginkan (ON/OFF, direpresentasikan sebagai 1/0) untuk output digital. `OUT_4` dideklarasikan tetapi dalam logika `loop()` saat ini tidak secara eksplisit mengontrol output fisik.
- `IN_1`, `IN_2`: Variabel integer untuk menyimpan hasil pembacaan langsung dari pin input digital `S1` dan `S2`.
- `holdingRegisterOutput[3]`, `holdingRegisterInput[2]`, `holdingRegister[20]`: Array-array ini berfungsi sebagai buffer data internal. Mereka dapat digunakan untuk memetakan status I/O ke format register Modbus jika Arduino juga dirancang untuk berperan sebagai perangkat Slave Modbus, atau hanya sebagai cara terstruktur untuk mengelola data state internal.
- `jsonDoc`: Objek dari library ArduinoJson yang digunakan untuk membangun struktur data JSON sebelum dikirim melalui serial. Ukuran `512` byte adalah kapasitas memori yang dialokasikan untuk dokumen JSON ini; jika data JSON Anda lebih besar, ukuran ini perlu ditingkatkan.

### 3. Fungsi Kontrol Transmisi Modbus

Untuk komunikasi RS485, yang bersifat half-duplex (mengirim atau menerima secara bergantian pada jalur yang sama), diperlukan kontrol arah transmisi. Pin `DM` (Driver Master, sesuai definisi di `PinoutOutsealAtmega128.h`) digunakan untuk tujuan ini, biasanya terhubung ke pin `DE` (Driver Enable) dan RE (Receiver Enable) pada chip transceiver RS485 dengan MAX485.

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
- `preTransmission()`: Fungsi ini secara otomatis dipanggil oleh library `ModbusMaster` tepat sebelum Arduino mengirimkan permintaan Modbus ke sensor. Dengan mengatur pin `DM` ke `HIGH`, transceiver RS485 diaktifkan untuk mode transmisi (mengirim data).
- `postTransmission()`: Fungsi ini dipanggil secara otomatis setelah Arduino selesai mengirimkan permintaan Modbus. Dengan mengatur pin `DM` ke `LOW`, transceiver RS485 kembali ke mode penerimaan, siap untuk menangkap respons dari sensor.

### 4. Fungsi setup

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

Penjelasan terkait `setup()`;
- Pin `DM` diinisialisasi sebagai `OUTPUT` dan disetel ke `LOW` (mode terima default untuk RS485).
- `Serial.begin(9600)`: Mengaktifkan port serial standar (biasanya terhubung ke komputer melalui USB) untuk debugging, menerima perintah, dan mengirim data JSON.
- `Serial1.begin(9600)`: Mengaktifkan port serial perangkat keras kedua (`Serial1` pada ATmega128, biasanya pin TX1 dan RX1) yang didedikasikan untuk komunikasi Modbus RTU dengan sensor-sensor. Sangat penting bahwa baud rate ini sesuai dengan konfigurasi semua sensor Modbus di jaringan.
- Loop Inisialisasi Node Modbus: Loop `for` berjalan sebanyak jumlah sensor (6 kali). Di setiap iterasi:
    - `nodes[i].begin(sensorIDs[i], Serial1)`: Menginisialisasi objek `ModbusMaster` ke-`i`. Ini memberitahu library ID slave Modbus yang akan diajak bicara (`sensorIDs[i]`) dan port serial mana yang akan digunakan (`Serial1`).
    - `nodes[i].preTransmission(preTransmission)` dan `nodes[i].postTransmission(postTransmission)`: Mendaftarkan fungsi-fungsi callback yang telah kita buat sebelumnya untuk menangani kontrol arah transceiver RS485 secara otomatis setiap kali objek `nodes[i]` melakukan transaksi Modbus.
- Inisialisasi Variabel Status: Variabel `OUT_x` (status output) diatur ke `0` (OFF), dan `IN_x` (status input) juga diatur ke `0` sebagai nilai awal.
- Konfigurasi pinMode():
    - Pin-pin yang terhubung ke relay (`R1`, `R2`, `R3`, `R4`) dikonfigurasi sebagai `OUTPUT`.
    - Pin-pin yang terhubung ke input digital (`S1`, `S2`) dikonfigurasi sebagai `INPUT`. Pertimbangkan penggunaan `INPUT_PULLUP` jika input Anda adalah saklar atau sensor kontak yang terhubung ke ground saat aktif, untuk menghindari kebutuhan resistor pull-up eksternal.

### 5. Fungsi UpdateTriggers

Fungsi ini bertanggung jawab untuk membaca status terkini dari input digital (`S1` dan `S2`) dan memperbarui variabel-variabel global yang menyimpan status pemicu (trigger) ini. Ini memastikan bahwa representasi internal dari status input selalu sinkron dengan kondisi fisik input.

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

- `trig[0] = digitalRead(S1) == LOW`;: Baris ini membaca level logika pada pin `S1`. Jika levelnya `LOW` (yang seringkali berarti aktif untuk sensor atau tombol yang terhubung ke ground), maka `digitalRead(S1) == LOW` akan menghasilkan `true` (yang dikonversi menjadi `1` saat disimpan ke `trig[0]`). Jika `HIGH`, hasilnya `false` (dikonversi menjadi `0`). Proses serupa terjadi untuk `S2` dan `trig[1]`.
- Status dari `trig[]` kemudian disalin ke `holdingRegisterInput[]` dan juga ke elemen-elemen tertentu (`holdingRegister[2]` dan `holdingRegister[3]`) dari array `holdingRegister[]` utama. Ini menyediakan beberapa cara untuk mengakses status input yang sama, yang mungkin berguna tergantung pada bagaimana data digunakan atau diekspos.

### 6. Fungsi ReadSendDatasensor

Fungsi ini melakukan iterasi melalui semua sensor yang terkonfigurasi, membaca data dari masing-masing sensor menggunakan protokol Modbus, memformat data tersebut ke dalam array JSON, dan akhirnya mengirimkan array JSON ini melalui port `Serial` ke host. Kode di bawah ini mencerminkan logika asli dari program yang Anda berikan.

```cpp
// --- Fungsi untuk Membaca Sensor dan Mengirim Data via JSON ---
void readSendDatasensor() {
  jsonDoc.clear(); // Mengosongkan dokumen JSON dari data sebelumnya untuk memastikan data segar.
  JsonArray dataArray = jsonDoc.to<JsonArray>(); // Membuat struktur array JSON di dalam dokumen jsonDoc.

  // Loop untuk membaca data dari setiap sensor yang terdaftar dalam array 'nodes'.
  for (int i = 0; i < 6; i++) {
    uint8_t result; // Variabel untuk menyimpan kode status hasil dari operasi Modbus (sukses/gagal).


    // Logika pembacaan khusus jika ID sensor adalah 3 atau 6.
    if (sensorIDs[i] == 3 || sensorIDs[i] == 6) {
      // Membaca register 0x0001. Parameter kedua menggunakan sensorIDs[i] sebagai jumlah register.
      result = nodes[i].readInputRegisters(0x0001, sensorIDs[i]); 
      if (result == nodes[i].ku8MBSuccess) { 
        float sensorValue = float(nodes[i].getResponseBuffer(0x00) / 10.0F);
        dataArray.add(sensorValue == 0.0 ? 0 : sensorValue);
      } else {
        dataArray.add(nullptr); 
      }

      // Membaca register 0x0003. Parameter kedua menggunakan sensorIDs[i] sebagai jumlah register.
      result = nodes[i].readInputRegisters(0x0003, sensorIDs[i]); 
      if (result == nodes[i].ku8MBSuccess) {
        float sensorValue = float(nodes[i].getResponseBuffer(0x00) / 10.0F);
        dataArray.add(sensorValue == 0.0 ? 0 : sensorValue);
      } else {
        dataArray.add(nullptr);
      }

      // Membaca register 0x0008. Parameter kedua menggunakan sensorIDs[i] sebagai jumlah register.
      result = nodes[i].readInputRegisters(0x0008, sensorIDs[i]); 
      if (result == nodes[i].ku8MBSuccess) {
        float sensorValue = float(nodes[i].getResponseBuffer(0x00) / 10.0F);
        dataArray.add(sensorValue == 0.0 ? 0 : sensorValue);
      } else {
        dataArray.add(nullptr);
      }
    } else { // Logika untuk sensor lainnya (yang ID-nya bukan 3 atau 6).
      // Membaca dari alamat 0x0000. Parameter kedua menggunakan sensorIDs[i] sebagai jumlah register.
      result = nodes[i].readInputRegisters(0x0000, sensorIDs[i]); 
      if (result == nodes[i].ku8MBSuccess) {
        float tempValue = float(nodes[i].getResponseBuffer(1) / 10.0F); 
        float humidValue = float(nodes[i].getResponseBuffer(0) / 10.0F);
        
        dataArray.add(tempValue == 0.0 ? 0 : tempValue);
        dataArray.add(humidValue == 0.0 ? 0 : humidValue);
      } else {
        dataArray.add(nullptr); 
        dataArray.add(nullptr); 
      }
    }
    delay(50); 
  }
  updateTriggers(); 
  dataArray.add(trig[0]);
  dataArray.add(trig[1]);
  serializeJson(dataArray, Serial);
  Serial.println(); 
}
```
Penjelasan Rinci `readSendDatasensor()`:

- `jsonDoc.clear()` dan `JsonArray dataArray = jsonDoc.to<JsonArray>()`: Mempersiapkan objek JSON untuk diisi data baru.
- Loop Pembacaan Sensor: Iterasi untuk setiap sensor.
  - Logika Kondisional (ID 3 & 6 vs Lainnya): Sensor dengan ID 3 atau 6 dibaca dari tiga alamat register berbeda (`0x0001`, `0x0003`, `0x0008`). Sensor lainnya dibaca dari alamat `0x0000`.
  - `nodes[i].readInputRegisters(alamat_register, sensorIDs[i])`: Fungsi Modbus ini dipanggil dengan `alamat_register` sebagai alamat awal, dan `sensorIDs[i]` (nilai ID sensor, misal 2, 3, 7, dst.) sebagai parameter jumlah register yang akan dibaca. Penting untuk memverifikasi bahwa penggunaan `sensorIDs[i]` sebagai jumlah register ini sesuai dengan bagaimana data diharapkan oleh sensor Anda dan bagaimana library `ModbusMaster` menginterpretasikannya. Jika sensor hanya menyediakan satu atau dua register pada alamat tersebut, namun `sensorIDs[i]` nilainya lebih besar, ini bisa menyebabkan pembacaan yang tidak sesuai atau error, tergantung implementasi library dan perangkat Modbus slave.
  - `result == nodes[i].ku8MBSuccess`: Pemeriksaan status keberhasilan transaksi Modbus.
  - `nodes[i].getResponseBuffer(indeks_buffer)`: Mengambil data dari buffer respons. Indeks yang digunakan (`0x00` atau `0` dan `1`) harus sesuai dengan data yang diharapkan dari sensor untuk pembacaan tersebut.
  - `dataArray.add(...)`: Menambahkan data sensor atau `nullptr` (jika gagal) ke array JSON.
  - `delay(50)`: Jeda antar pembacaan sensor.
- `updateTriggers()` (Kedua): Memperbarui status trigger sebelum ditambahkan ke JSON.
- Menambahkan Status Trigger ke JSON: Status `trig[0]` dan `trig[1]` ditambahkan.
- Serialisasi dan Pengiriman JSON: Data JSON dikirim melalui `Serial`.

Alamat register yang dijelaskan pada program/penjelasan diatas menyesuaikan pada datasheet ketiga sensor tersebut;
1. **Untuk CWT-TH04S**

![Addres yang digunakan pada sensor CWT-TH04S](https://github.com/Fathihzhafran/MBKM_Aeroponik_documentation/blob/e7a1e4e433fcecf48962778f1d58d0741bd61c6e/Image/Screenshot%202025-05-30%20162419.png)

2. **Untuk CWT-Soil-THC-S**

![Addres yang digunakan pada sensor CWT-Soil-THC-S](https://github.com/Fathihzhafran/MBKM_Aeroponik_documentation/blob/e7a1e4e433fcecf48962778f1d58d0741bd61c6e/Image/Screenshot%202025-05-30%20162102.png)


### 7. Fungsi loop

Fungsi loop() adalah inti dari program Arduino yang berjalan terus-menerus setelah fungsi setup() selesai dieksekusi.

```cpp
// --- Fungsi Loop Utama ---
// Fungsi loop() berjalan berulang kali selamanya setelah setup() selesai.
void loop() {
  // 1. Baca status input digital S1 dan S2 secara langsung.
  IN_1 = digitalRead(S1) == LOW;
  IN_2 = digitalRead(S2) == LOW;

  trig[0] = IN_1; // Sinkronisasi trig dengan pembacaan langsung.
  trig[1] = IN_2;

  // 2. Periksa apakah ada data (perintah) yang masuk dari port Serial (dari host).
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim(); 

    switch (input.charAt(0)) {
      case 'P': // Perintah untuk PUMP (Pompa/Output).
        if (input == "PUMP1_ON") {
          OUT_1 = 1; 
        } else if (input == "PUMP1_OFF") {
          OUT_1 = 0; 
        } else if (input == "PUMP2_ON") {
          OUT_2 = 1;
        } else if (input == "PUMP2_OFF") {
          OUT_2 = 0;
        } else if (input == "PUMP3_ON") {
          OUT_3 = 1; // OUT_3 akan mengontrol Relay R4.
        } else if (input == "PUMP3_OFF") {
          OUT_3 = 0;
        } else {
          Serial.println("Unknown PUMP command: " + input);
        }
        break; 

      case 'R': // Perintah untuk READ (Baca Data).
        if (input == "READ_DATA") {
          for (int i = 0; i < 5; i++) {
            readSendDatasensor();
          }
        } else {
          Serial.println("Unknown READ command: " + input);
        }
        break; 

      case 'A': // Perintah untuk ALARM.
        if (input == "ALARM1_ON") {
          digitalWrite(R3, HIGH); // Langsung mengaktifkan Relay R3.
                                  // Asumsi di sini: HIGH = ON.
        } else if (input == "ALARM1_OFF") {
          digitalWrite(R3, LOW);  // Langsung menonaktifkan Relay R3.
                                  // Asumsi di sini: LOW = OFF.
        } else {
          Serial.println("Unknown ALARM command: " + input);
        }
        break;  

      default: 
        Serial.println("Unknown Command: " + input);
        break;
    }
  }

  // 3. Perbarui array-array holdingRegister dengan status output dan input saat ini.
  holdingRegisterOutput[0] = OUT_1; 
  holdingRegisterOutput[1] = OUT_2; 
  holdingRegisterOutput[2] = OUT_3; 

  holdingRegisterInput[0] = IN_1; 
  holdingRegisterInput[1] = IN_2;

  holdingRegister[0] = holdingRegisterOutput[0]; 
  holdingRegister[1] = holdingRegisterOutput[1]; 
  holdingRegister[2] = holdingRegisterInput[0];  
  holdingRegister[3] = holdingRegisterInput[1];  
  holdingRegister[4] = holdingRegisterOutput[2]; 

  // 4. Tulis status output dari holdingRegister ke pin-pin relay fisik.
  digitalWrite(R1, holdingRegister[0]); 
  digitalWrite(R2, holdingRegister[1]); 
  digitalWrite(R4, holdingRegister[4]);                                     

  // 5. Panggil updateTriggers() lagi di akhir loop.
  updateTriggers();

  // 6. Beri jeda singkat pada akhir setiap iterasi loop.
  delay(50); 
}
```
Penjelasan Rinci `loop()`:
1. Pembacaan Input Awal: Status pin `S1` dan `S2` dibaca dan disimpan ke `IN_1`, `IN_2`, serta `trig[]`.
2. Pemrosesan Perintah Serial:
   - Mengecek data masuk via `Serial.available()`.
   - Membaca perintah sebagai `String` hingga newline.
   - `switch` statement memproses perintah berdasarkan karakter pertama:
     - `case 'P'` (PUMP): Mengubah status `OUT_1`, `OUT_2`, atau `OUT_3` berdasarkan perintah `PUMPx_ON`/`PUMPx_OFF`. `OUT_3` terhubung ke `R4`.
     - `case 'R'` (READ): Jika `"READ_DATA"`, memanggil `readSendDatasensor()` 5 kali.
     - `case 'A'` (ALARM): Mengontrol relay `R3` secara langsung dengan `digitalWrite()`.
     - `default`: Menangani perintah tidak dikenal.
3. Pembaruan `holdingRegister`: Menyinkronkan status `OUT_x` dan `IN_x` ke dalam array `holdingRegisterOutput`, `holdingRegisterInput`, dan `holdingRegister`.
4. Pengaktifan Relay Fisik: `digitalWrite()` digunakan untuk mengatur pin relay `R1`, `R2`, dan `R4` sesuai nilai di `holdingRegister`. `R3` dikontrol terpisah.
5. `updateTriggers()` (Akhir Loop): Memastikan status input termutakhir.
