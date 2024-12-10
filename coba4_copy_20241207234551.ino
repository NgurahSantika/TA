#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Fingerprint.h>
#include <map>
#include <EEPROM.h>
#include <NTPClient.h> // Pustaka untuk NTP
#include <WiFiUdp.h>
#include <TimeLib.h>


// Informasi jaringan WiFi
#define WIFI_SSID "TRISKA_sunset"
#define WIFI_PASSWORD "10922901triska"

// Informasi MQTT broker
#define MQTT_SERVER "192.168.0.101"
#define MQTT_PORT 1883
#define MQTT_USER "santika"
#define MQTT_PASS "santika123"

// Topik MQTT
#define MQTT_TOPIC_STATUS "IOT/STATUS"
#define MQTT_TOPIC_FINGERPRINT "IOT/FINGERPRINT"
#define MQTT_TOPIC_TIME "IOT/TIME"


// Pin untuk sensor sidik jari
#define Finger_Rx 16
#define Finger_Tx 17

// Pin LED
#define LED_RED 21
#define LED_YELLOW 19
#define LED_GREEN 18

// Pin buzzer
#define BUZZER_PIN 32

// Objek untuk sensor sidik jari
HardwareSerial mySerial(2); // UART2 untuk komunikasi serial
Adafruit_Fingerprint finger(&mySerial);

// Objek WiFi dan MQTT
WiFiClient espClient;
PubSubClient client(espClient);

//Zona waktu bagian Wita +8 28800
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800); // 25200 untuk WIB (UTC+7)

// Fungsi untuk menghubungkan ke WiFi
void connectToWiFi() {
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_RED, HIGH);
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.update();
}

// Callback untuk menangani pesan MQTT masuk
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Pesan diterima di topik: ");
  Serial.println(topic);
  Serial.print("Pesan: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
}

void sendCurrentTime() {
  timeClient.update(); // Update time from NTP
  unsigned long epochTime = timeClient.getEpochTime();  // Get the Unix epoch time

  // Use TimeLib to convert epoch time to human-readable format
  setTime(epochTime);  // Set the time using TimeLib

  // Get the formatted time from TimeLib
  String formattedTime = String(hour()) + ":" + String(minute()) + ":" + String(second());

  // Get the date from TimeLib
  String formattedDate = String(day()) + "/" + String(month()) + "/" + String(year());

  // Publish the time and date to MQTT
  client.publish(MQTT_TOPIC_TIME, formattedDate.c_str()); // Send date
  client.publish(MQTT_TOPIC_TIME, formattedTime.c_str()); // Send time

  Serial.print("Current time sent to topic ");
  Serial.println(MQTT_TOPIC_TIME);
  Serial.println(formattedTime); // Print time
  Serial.println(formattedDate); // Print date
}



// Fungsi untuk menghubungkan ke MQTT broker
void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT Broker...");
    if (client.connect("ESP32_Client", MQTT_USER, MQTT_PASS)) {
      Serial.println("Terhubung");
      digitalWrite(LED_RED, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      client.subscribe(MQTT_TOPIC_STATUS);
      client.publish(MQTT_TOPIC_STATUS, "ESP32 siap terhubung!");
    } else {
      digitalWrite(LED_RED, HIGH);
      Serial.print("Gagal. Status: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}
// Fungsi untuk indikasi error
void error() {
  Serial.println("*****************************************************");
  digitalWrite(LED_RED, HIGH);
  for (int i = 0; i < 3; i++) { 
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100); 
    digitalWrite(BUZZER_PIN, LOW);
    delay(70); 
  }
  delay(500); 
  digitalWrite(LED_RED, HIGH);
}

// Fungsi untuk indikasi success
void success() {
  Serial.println("******************************************************");
  digitalWrite(LED_GREEN, HIGH);
  for (int i = 0; i < 1; i++) { 
    digitalWrite(BUZZER_PIN, HIGH);
    delay(250); 
    digitalWrite(BUZZER_PIN, LOW);
    delay(100); 
  }
  delay(500); 
  digitalWrite(LED_GREEN, LOW); 
}
////////////////////////////////////////////////////////////////////////
// Fungsi untuk mendapatkan ID sidik jari
int getFingerprintID() {
  int id = finger.getImage();
  if (id == FINGERPRINT_NOFINGER) {
    return -1;
  } else if (id != FINGERPRINT_OK) {
    Serial.println("Gagal membaca gambar sidik jari.");
    error();
    return -2;
  }

  id = finger.image2Tz();
  if (id != FINGERPRINT_OK) {
    Serial.println("Gagal mengonversi gambar ke template.");
    error();
    return -2;
  }

  id = finger.fingerFastSearch();
  if (id != FINGERPRINT_OK) {
    Serial.println("Sidik jari tidak ditemukan.");
    error();
    return -3;
  }
  success();
  Serial.print("Sidik jari ditemukan dengan ID: ");
  Serial.println(finger.fingerID);
  return finger.fingerID;
}


// Fungsi untuk enroll sidik jari
void enrollFingerprint() {
  int id = -1; // ID sidik jari
  String employeeID; // ID karyawan yang akan disimpan

  // Meminta input EmployeeID dari pengguna
  while (true) {
    Serial.println("Masukkan EmployeeID:");
    while (!Serial.available()) {
      delay(100); // Tunggu hingga pengguna mengetikkan EmployeeID
    }
    employeeID = Serial.readStringUntil('\n');  // Baca input hingga newline
    employeeID.trim();  // Hilangkan spasi atau karakter ekstra

    if (employeeID.length() == 0) {
      Serial.println("EmployeeID tidak boleh kosong. Silakan coba lagi.");
      error();
    } else {
      break; // Keluar dari loop jika EmployeeID valid
    }
  }

  // Meminta input ID untuk sidik jari
  do {
    Serial.println("Masukkan ID untuk sidik jari (1-127):");
    while (!Serial.available()) {
      delay(100); // Tunggu hingga pengguna mengetikkan ID
    }
    id = Serial.parseInt();  // Ambil input sebagai integer

    if (id < 1 || id > 127) {
      Serial.println("ID tidak valid, harus antara 1-127.");
      error();
      id = -1; // Reset ke nilai awal jika tidak valid
    }
  } while (id == -1); // Ulangi hingga ID valid dimasukkan

  // Proses pendaftaran sidik jari
  Serial.println("Letakkan jari Anda di sensor.");
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    Serial.println("Gagal memproses gambar.");
    return;
  }

  Serial.println("Hapus jari Anda, lalu letakkan lagi.");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(2) != FINGERPRINT_OK) {
    Serial.println("Gagal memproses gambar kedua.");
    error();
    return;
  }

  if (finger.createModel() != FINGERPRINT_OK) {
    Serial.println("Sidik jari tidak cocok.");
    error();
    return;
  }

  if (finger.storeModel(id) == FINGERPRINT_OK) {
    Serial.println("Sidik jari berhasil disimpan.");
  } else {
    Serial.println("Gagal menyimpan sidik jari.");
    error();
    return;
  }

  // Simpan EmployeeID ke EEPROM
  success();
  saveEmployeeIDToEEPROM(id, employeeID);
  Serial.print("Sidik jari dengan ID ");
  Serial.print(id);
  Serial.print(" berhasil didaftarkan untuk EmployeeID: ");
  Serial.println(employeeID);
}


// Fungsi untuk menghapus sidik jari berdasarkan ID
void deleteFingerprint() {
  Serial.println("Letakkan sidik jari untuk menghapus:");

  int id = -1;
  while (id < 0) {
    // Cek apakah ada sidik jari yang diletakkan
    id = getFingerprintID();  // Fungsi untuk mendapatkan ID sidik jari

    // Jika tidak ada sidik jari, beri tahu pengguna untuk meletakkan jari
    if (id == -1) {
      Serial.println("Sidik jari tidak terdeteksi. Silakan letakkan sidik jari.");
      delay(1000); // Beri sedikit waktu sebelum mencoba lagi
    } else {
      break;  // Keluar dari loop jika sidik jari terdeteksi
    }
  }

  // Setelah berhasil mendeteksi sidik jari, tampilkan ID sidik jari
  Serial.print("Sidik jari dengan ID ");
  Serial.print(id);
  Serial.println(" ditemukan. Menghapus...");

  // Mengonfirmasi penghapusan sidik jari
  Serial.print("Konfirmasi penghapusan sidik jari ID ");
  Serial.print(id);
  Serial.println("? (y/n):");

  // Tunggu input dari serial
  while (!Serial.available());  // Tunggu input
  char confirm = Serial.read();  // Baca konfirmasi
  if (confirm == 'y' || confirm == 'Y') {
    if (finger.deleteModel(id) == FINGERPRINT_OK) {
      Serial.println("Sidik jari berhasil dihapus.");

      // Hapus EmployeeID terkait dari EEPROM
      int addr = id * 10;  // Asumsi setiap ID memiliki alokasi 10 byte di EEPROM
      for (int i = 0; i < 10; i++) {
        EEPROM.write(addr + i, 0);  // Set 0 untuk setiap byte yang terkait
      }
      EEPROM.commit();  // Simpan perubahan ke EEPROM
      Serial.println("Data EmployeeID yang terkait juga berhasil dihapus.");
    } else {
      Serial.println("Gagal menghapus sidik jari.");
    }
  } else {
    Serial.println("Penghapusan dibatalkan.");
  }
}


// Fungsi untuk mengedit sidik jari
void editFingerprint() {
  Serial.println("Letakkan sidik jari untuk mengedit:");

  int id = -1;
  while (id < 0) {
    id = getFingerprintID();
    if (id == -1) {
      Serial.println("Sidik jari tidak terdeteksi. Silakan letakkan sidik jari.");
      delay(1000);
    } else if (id == -2) {
      Serial.println("Gagal membaca gambar sidik jari. Silakan coba lagi.");
      delay(1000);
    } else if (id == -3) {
      Serial.println("Sidik jari tidak ditemukan di database. Proses dibatalkan.");
      return; // Keluar dari fungsi jika tidak ditemukan
    } else if (id > 0) {
      break; // ID valid ditemukan
    }
  }

  Serial.print("Sidik jari dengan ID ");
  Serial.print(id);
  Serial.println(" ditemukan. Mengedit...");

  Serial.print("Konfirmasi pengeditan sidik jari ID ");
  Serial.print(id);
  Serial.println("? (y/n):");

  while (!Serial.available());
  char confirm = Serial.read();
  if (confirm == 'y' || confirm == 'Y') {
    if (finger.deleteModel(id) == FINGERPRINT_OK) {
      success();
      Serial.println("Data lama berhasil dihapus. Silakan enroll ulang:");

      // Hapus EmployeeID terkait dari EEPROM
      int addr = id * 10;
      for (int i = 0; i < 10; i++) {
        EEPROM.write(addr + i, 0); // Set 0 untuk setiap karakter
      }
      EEPROM.commit();
      Serial.println("EmployeeID lama berhasil dihapus dari EEPROM.");

      // Enroll ulang sidik jari dan EmployeeID
      enrollFingerprint();
    } else {
      Serial.println("Gagal menghapus data lama.");
    }
  } else {
    Serial.println("Pengeditan dibatalkan.");
  }
}


///////////////////////////////////////////////////////
// Fungsi untuk menampilkan total sidik jari yang terdaftar
void showTotalFingerprints() {
  int total = 0;
  for (int id = 1; id <= 127; id++) { // Cek ID dari 1 sampai 127
    if (finger.loadModel(id) == FINGERPRINT_OK) { // Coba load model untuk setiap ID
      total++; // Jika model berhasil dimuat, itu berarti ada sidik jari terdaftar
    }
  } // Mendapatkan jumlah sidik jari yang terdaftar

  if (total < 0) {
    Serial.println("Gagal mendapatkan jumlah sidik jari.");
  } else if (total == 0) {
    Serial.println("Tidak ada sidik jari yang terdaftar.");
  } else {
    Serial.print("Total sidik jari yang terdaftar: ");
    Serial.println(total);
  }
}
// Fungsi untuk menghapus semua sidik jari
void deleteAllFingerprints() {
  Serial.println("Menghapus semua data sidik jari...");
  if (finger.emptyDatabase() == FINGERPRINT_OK) {
    Serial.println("Semua sidik jari berhasil dihapus.");
       for (int i = 0; i < 2; i++) {   // Buzzer berbunyi 2 kali cepat
        digitalWrite(BUZZER_PIN, HIGH);
        delay(50);                 // Durasi bunyi buzzer
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);                 // Jeda antar bunyi
    } 
  } else {
    Serial.println("Gagal menghapus sidik jari.");
  }
}
// Fungsi untuk menyimpan employeeID ke EEPROM
// Fungsi untuk menyimpan employeeID ke EEPROM
void saveEmployeeIDToEEPROM(int fingerprintID, String employeeID) {
  if (employeeID.length() < 10) {
    while (employeeID.length() < 10) {
      employeeID += " "; // Tambahkan spasi untuk memastikan panjangnya 10
    }
  } else if (employeeID.length() > 10) {
    employeeID = employeeID.substring(0, 10); // Potong jika lebih dari 10
  }

  int addr = fingerprintID * 10;
  EEPROM.begin(512);

  // Menyimpan EmployeeID ke EEPROM
  for (int i = 0; i < 10; i++) {
    EEPROM.write(addr + i, employeeID[i]);
  }
  EEPROM.commit();

  // Verifikasi data yang disimpan
  String verify = "";
  for (int i = 0; i < 10; i++) {
    verify += (char)EEPROM.read(addr + i);
  }

  if (verify == employeeID) {
    Serial.println("EmployeeID berhasil disimpan ke EEPROM.");
  } else {
    Serial.println("Gagal menyimpan EmployeeID ke EEPROM.");
  }
}


void deleteAllEmployeeData() {
  Serial.println("Menghapus semua data EmployeeID di EEPROM...");
  
  // Iterasi melalui semua fingerprintID (1-127)
  for (int i = 1; i <= 127; i++) {
    int addr = i * 10;  // Mengakses alamat sesuai dengan fingerprintID
    
    // Set panjang data menjadi 0 untuk menandakan data tidak ada
    EEPROM.write(addr, 0);  // Menyimpan panjang 0 untuk menghapus data
    
    // Optional: Bersihkan karakter employeeID setelah panjangnya
    addr++;  // Pindah ke alamat berikutnya untuk karakter data
    for (int j = 0; j < 10; j++) {
      EEPROM.write(addr++, 0);  // Set karakter ke 0 (menghapus data)
    }
  }
  
  EEPROM.commit();  // Menyimpan perubahan ke EEPROM
  Serial.println("Semua data EmployeeID berhasil dihapus.");
}

// Fungsi untuk mencari employeeID berdasarkan fingerprintID
// Fungsi untuk mencari employeeID berdasarkan fingerprintID
String searchEmployeeID(int fingerprintID) {
  EEPROM.begin(512);

  int addr = fingerprintID * 10;
  String employeeID = "";

  Serial.print("Membaca data dari EEPROM untuk Fingerprint ID: ");
  Serial.println(fingerprintID);

  // Baca 10 karakter dari EEPROM
  for (int i = 0; i < 10; i++) {
    char c = (char)EEPROM.read(addr + i);
    employeeID += c;
  }

  Serial.print("Data mentah EEPROM: ");
  Serial.println(employeeID);

  employeeID.trim(); // Hilangkan spasi di awal/akhir

  if (employeeID.length() == 0 || employeeID == "          ") { // 10 spasi
    Serial.println("EmployeeID tidak ditemukan.");
    error();
    return "";
  }

  Serial.print("EmployeeID ditemukan: ");
  Serial.println(employeeID);
  return employeeID;
}

// Fungsi untuk mengirimkan EmployeeID ke MQTT
void sendToMQTT(String employeeID, int fingerprintID) {
  Serial.print("Sidik jari cocok dengan EmployeeID: ");
  Serial.println(employeeID);
  
  // Kirim pesan ke MQTT
  String message = employeeID;
  client.publish(MQTT_TOPIC_FINGERPRINT, message.c_str());
  
  // Konfirmasi bahwa pesan telah dikirim
  Serial.println("Pesan terkirim ke MQTT: " + message + " dengan ID " + fingerprintID);
}



void setup() {
  Serial.begin(115200);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  mySerial.begin(57600, SERIAL_8N1, Finger_Rx, Finger_Tx);
  if (finger.verifyPassword()) {
    Serial.println("Sensor sidik jari ditemukan.");
  } else {
    Serial.println("Sensor sidik jari tidak ditemukan. Cek koneksi!");
    while (true);
  }

  connectToWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttCallback);
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  int fingerprintID = getFingerprintID();

  if (fingerprintID > 0) {
    // Sidik jari terdeteksi dan cocok di database
    String employeeID = searchEmployeeID(fingerprintID);
    
    // Memberikan feedback jika fingerprint ditemukan
    if (employeeID != "") {
      sendToMQTT(employeeID, fingerprintID); // Kirim ke MQTT menggunakan fungsi baru
      sendCurrentTime();  // Kirim waktu saat ini ke MQTT
    } else {
    
      Serial.println("Sidik jari ditemukan, tetapi tidak ada data EmployeeID yang terkait.");
    }
  } else if (fingerprintID == -1) {
    // Tidak ada jari
    // Pesan sudah ditampilkan dalam getFingerprintID()
    Serial.println("Tidak ada jari yang terdeteksi.");
  } else if (fingerprintID == -3) {
    // Sidik jari tidak ditemukan di database
    // Pesan sudah ditampilkan dalam getFingerprintID()
    Serial.println("Sidik jari tidak dikenali dalam database.");
  }
  

  showTotalFingerprints();

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("enroll")) {
      Serial.println("Mulai proses enroll...");
      enrollFingerprint();
    } else if (command.equalsIgnoreCase("delete")) {
      Serial.println("Mulai proses delete...");
      deleteFingerprint();
    } else if (command.equalsIgnoreCase("edit")) {
      Serial.println("Mulai proses edit...");
      editFingerprint();
    } else if (command.equalsIgnoreCase("deleteall")) {
      Serial.println("Mulai proses delete all...");
      deleteAllFingerprints();
      deleteAllEmployeeData();  // Menghapus semua data EmployeeID di EEPROM
    }
  }

  delay(1000);
}
