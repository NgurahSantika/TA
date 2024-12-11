<?php
session_start();
require 'connectDB.php';
require 'phpMQTT.php'; // Memuat file phpMQTT

// Setelan MQTT
$server = '192.168.0.108'; // Ganti dengan IP server MQTT Anda
$port = 1883;
$username = 'santika';
$password = 'santika123';
$client_id = 'phpMQTT-subscriber';

// Membuat koneksi MQTT
$mqtt = new bluerhinos\phpMQTT($server, $port, $client_id);
if (!$mqtt->connect(true, NULL, $username, $password)) {
    die("Tidak dapat terhubung ke MQTT");
}

// Fungsi untuk menerbitkan pesan MQTT
function publishMessage($mqtt, $topic, $message) {
    $mqtt->publish($topic, $message, 0);
}

if (isset($_GET['FingerID'])) {
    $fingerID = $_GET['FingerID'];

    $sql = "SELECT * FROM users WHERE fingerprint_id=?";
    $stmt = mysqli_stmt_init($conn);
    if (!mysqli_stmt_prepare($stmt, $sql)) {
        echo "SQL_Error_Select_card";
        exit();
    } else {
        mysqli_stmt_bind_param($stmt, "s", $fingerID);
        mysqli_stmt_execute($stmt);
        $result = mysqli_stmt_get_result($stmt);
        if ($row = mysqli_fetch_assoc($result)) {
            // Fingerprint yang terdaftar ditemukan untuk Login atau Logout
            if ($row['username'] != "Name") {
                $Uname = $row['username'];
                $Number = $row['serialnumber'];

                $sql = "SELECT * FROM users_logs WHERE fingerprint_id=? AND checkindate=CURDATE() AND timeout=''";
                $stmt = mysqli_stmt_init($conn);
                if (!mysqli_stmt_prepare($stmt, $sql)) {
                    echo "SQL_Error_Select_logs";
                    exit();
                } else {
                    mysqli_stmt_bind_param($stmt, "s", $fingerID);
                    mysqli_stmt_execute($stmt);
                    $result = mysqli_stmt_get_result($stmt);

                    // Jika tidak ada log hari ini, berarti ini adalah login
                    if (!$row = mysqli_fetch_assoc($result)) {
                        $sql = "INSERT INTO users_logs (username, serialnumber, fingerprint_id, checkindate, timein, timeout) VALUES (?, ?, ?, CURDATE(), CURTIME(), ?)";
                        $stmt = mysqli_stmt_init($conn);
                        if (!mysqli_stmt_prepare($stmt, $sql)) {
                            echo "SQL_Error_Insert_login";
                            exit();
                        } else {
                            $timeout = "0";
                            mysqli_stmt_bind_param($stmt, "sdis", $Uname, $Number, $fingerID, $timeout);
                            mysqli_stmt_execute($stmt);
                            
                            // Publikasikan pesan login ke MQTT
                            publishMessage($mqtt, 'absensi/login', "Login: $Uname");
                            echo "login" . $Uname;
                            exit();
                        }
                    } else {
                        // Jika ada log hari ini, berarti ini adalah logout
                        $sql = "UPDATE users_logs SET timeout=CURTIME() WHERE fingerprint_id=? AND checkindate=CURDATE() AND timeout='0'";
                        $stmt = mysqli_stmt_init($conn);
                        if (!mysqli_stmt_prepare($stmt, $sql)) {
                            echo "SQL_Error_Update_logout";
                            exit();
                        } else {
                            mysqli_stmt_bind_param($stmt, "s", $fingerID);
                            mysqli_stmt_execute($stmt);
                            
                            // Publikasikan pesan logout ke MQTT
                            publishMessage($mqtt, 'absensi/logout', "Logout: $Uname");
                            echo "logout" . $Uname;
                            exit();
                        }
                    }
                }
            } else {
                // Fingerprint yang tersedia ditemukan
                $sql = "UPDATE users SET fingerprint_select=1 WHERE fingerprint_id=?";
                $stmt = mysqli_stmt_init($conn);
                if (!mysqli_stmt_prepare($stmt, $sql)) {
                    echo "SQL_Error_update";
                    exit();
                } else {
                    mysqli_stmt_bind_param($stmt, "s", $fingerID);
                    mysqli_stmt_execute($stmt);
                    echo "available";
                    exit();
                }
            }
        } else {
            // Fingerprint baru ditambahkan
            $Uname = "Name";
            $Number = "000000";
            $Email = "Email";
            $Timein = "00:00:00";
            $Gender = "Gender";

            $sql = "INSERT INTO users (username, serialnumber, gender, email, fingerprint_id, fingerprint_select, user_date, time_in, add_fingerid) VALUES (?, ?, ?, ?, ?, 1, CURDATE(), ?, 0)";
            $stmt = mysqli_stmt_init($conn);
            if (!mysqli_stmt_prepare($stmt, $sql)) {
                echo "SQL_Error_Insert_new_fingerprint";
                exit();
            } else {
                mysqli_stmt_bind_param($stmt, "sdssis", $Uname, $Number, $Gender, $Email, $fingerID, $Timein);
                mysqli_stmt_execute($stmt);
                echo "succesful1";
                exit();
            }
        }
    }
}

// Fungsi untuk mendapatkan FingerID
if (isset($_GET['Get_Fingerid']) && $_GET['Get_Fingerid'] == "get_id") {
    $sql = "SELECT fingerprint_id FROM users WHERE add_fingerid=1";
    $stmt = mysqli_stmt_init($conn);
    if (!mysqli_stmt_prepare($stmt, $sql)) {
        echo "SQL_Error_Select";
        exit();
    } else {
        mysqli_stmt_execute($stmt);
        $result = mysqli_stmt_get_result($stmt);
        if ($row = mysqli_fetch_assoc($result)) {
            echo "add-id" . $row['fingerprint_id'];
            exit();
        } else {
            echo "Nothing";
            exit();
        }
    }
}

// Fungsi untuk menghapus FingerID
if (isset($_GET['DeleteID']) && $_GET['DeleteID'] == "check") {
    $sql = "SELECT fingerprint_id FROM users WHERE del_fingerid=1";
    $stmt = mysqli_stmt_init($conn);
    if (!mysqli_stmt_prepare($stmt, $sql)) {
        echo "SQL_Error_Select";
        exit();
    } else {
        mysqli_stmt_execute($stmt);
        $result = mysqli_stmt_get_result($stmt);
        if ($row = mysqli_fetch_assoc($result)) {
            echo "del-id" . $row['fingerprint_id'];

            $sql = "DELETE FROM users WHERE del_fingerid=1";
            $stmt = mysqli_stmt_init($conn);
            if (!mysqli_stmt_prepare($stmt, $sql)) {
                echo "SQL_Error_Delete";
                exit();
            } else {
                mysqli_stmt_execute($stmt);
                exit();
            }
        } else {
            echo "nothing";
            exit();
        }
    }
}

// Tutup koneksi MQTT dan MySQL
$mqtt->close();
mysqli_stmt_close($stmt);
mysqli_close($conn);
?>
