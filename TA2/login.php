<?php
// Koneksi ke database
$host = 'localhost';
$dbname = 'tugas_akhir';
$username = 'santika';  // Sesuaikan dengan username database Anda
$password = 'santika123';      // Sesuaikan dengan password database Anda

try {
    $pdo = new PDO("mysql:host=$host;dbname=$dbname", $username, $password);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
} catch (PDOException $e) {
    die("Koneksi ke database gagal: " . $e->getMessage());
}

// Proses login
$message = '';
if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    $id_karyawan = $_POST['id_karyawan'];
    $password = $_POST['password'];

    // Query untuk memeriksa login
    $query = "SELECT * FROM login WHERE id_karyawan = :id_karyawan AND password = :password";
    $stmt = $pdo->prepare($query);
    $stmt->bindParam(':id_karyawan', $id_karyawan);
    $stmt->bindParam(':password', $password);
    $stmt->execute();

    if ($stmt->rowCount() > 0) {
        // Login berhasil, mulai sesi dan redirect
        session_start();
        $_SESSION['logged_in'] = true;
        $_SESSION['id_karyawan'] = $id_karyawan; // Simpan ID karyawan untuk digunakan di halaman lain

        header("Location: index.php");
        exit();
    } else {
        $message = "ID karyawan atau password salah.";
    }
}
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Login Karyawan</title>
</head>
<body>
    <h2>Login Karyawan</h2>

    <form method="post" action="">
        <label for="id_karyawan">ID Karyawan:</label>
        <input type="text" id="id_karyawan" name="id_karyawan" required><br><br>

        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required><br><br>

        <button type="submit">Login</button>
    </form>

    <p><?php echo $message; ?></p>
</body>
</html>
