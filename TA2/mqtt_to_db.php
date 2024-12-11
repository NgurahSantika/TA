<?php
require('Bluerhinos/phpMQTT.php');

// Konfigurasi broker MQTT
$server = '192.168.0.108'; // Alamat IP Mosquitto
$port = 1883;              // Port Mosquitto
$username = 'santika';      // Username MQTT
$password = 'santika123';   // Password MQTT
$client_id = 'phpMQTT-subscriber'; // Client ID unik

// Koneksi ke database MySQL
$host = 'localhost';
$db = 'mqtt_data_db';
$user = 'santika';
$pass = 'santika123'; // Ganti dengan password MySQL kamu

// Koneksi ke database menggunakan PDO
try {
    $pdo = new PDO("mysql:host=$host;dbname=$db", $user, $pass);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
} catch (PDOException $e) {
    die("Error: " . $e->getMessage());
}

// Koneksi ke broker MQTT
$mqtt = new bluerhinos\phpMQTT($server, $port, $client_id);
if (!$mqtt->connect(true, NULL, $username, $password)) {
    exit(1); // Jika gagal koneksi, keluar
}

// Subscribe ke topik yang diinginkan
$topics['IOT/STATUS'] = array('qos' => 0, 'function' => 'procMsg');
$mqtt->subscribe($topics, 0);

// Callback function ketika pesan baru diterima dari topik
function procMsg($topic, $msg) {
    global $pdo;

    // Set timezone ke GMT+8 (WITA atau Asia/Makassar)
    date_default_timezone_set('Asia/Makassar');
    
    // Log pesan yang diterima untuk debugging
    echo "Topic: $topic, Message: $msg\n";

    //jam
    $timestamp = date('Y-m-d H:i:s');

    // Simpan ke file log jika diperlukan
    file_put_contents('mqtt_log.txt', "[$timestamp] Topic: $topic, Message: $msg\n", FILE_APPEND);

    // Simpan pesan ke database
    $sql = "INSERT INTO mqtt_messages (topic, message, received_at) VALUES (:topic, :message, NOW())";
    try {
        $stmt = $pdo->prepare($sql);
        $stmt->execute([
            ':topic' => $topic,
            ':message' => $msg,
        ]);
        echo "Pesan berhasil disimpan ke database\n";
    } catch (PDOException $e) {
        echo "Error saat menyimpan ke database: " . $e->getMessage() . "\n";
    }
}

// Loop terus-menerus untuk mendengarkan pesan baru dengan delay 5 detik
while ($mqtt->proc()) {
    // Jika ada pesan baru, procMsg akan dijalankan
    sleep(5); // Delay 5 detik
}

// Tutup koneksi jika selesai (ini tidak akan pernah dipanggil kecuali loop dihentikan)
$mqtt->close();
?>
