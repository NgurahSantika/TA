<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MQTT Data Display</title>
    <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
    <style>
        body {
            font-family: Arial, sans-serif;
            padding: 20px;
        }
        #data-container {
            margin-top: 20px;
            padding: 10px;
            border: 1px solid #ccc;
            background-color: #f9f9f9;
        }
        #data-container p {
            font-size: 18px;
        }
    </style>
</head>
<body>

    <h1>Data from MQTT Broker</h1>
    <div id="data-container">
        <p>Loading data...</p>
    </div>

    <script>
        // Fungsi untuk memanggil mqtt_to_db.php (agar broker berjalan otomatis)
        function runMQTTtoDB() {
            $.ajax({
                url: 'mqtt_to_db.php', // Panggil file php yang menghubungkan ke broker MQTT
                type: 'GET',
                success: function(data) {
                    console.log('MQTT to DB script executed', data);
                },
                error: function(xhr, status, error) {
                    console.error('Error running mqtt_to_db.php:', error);
                }
            });
        }

        // Fungsi untuk mengambil data dari getdata.php dan menampilkannya
        function fetchData() {
            $.ajax({
                url: 'getdata.php', // URL untuk mengambil data
                type: 'GET',
                dataType: 'json',
                success: function(data) {
                    // Cek apakah data yang diterima valid
                    if (data && data.message !== undefined) {
                        $('#data-container').html('<p><strong>Message:</strong> ' + data.message + '</p>' +
                                                  '<p><strong>Received At:</strong> ' + data.received_at + '</p>');
                    } else {
                        $('#data-container').html('<p>No data available</p>');
                    }
                },
                error: function(xhr, status, error) {
                    $('#data-container').html('<p>Error retrieving data: ' + error + '</p>');
                }
            });
        }

        // Ketika halaman dimuat
        $(document).ready(function() {
            // Jalankan mqtt_to_db.php untuk mulai menerima data dari broker MQTT
            runMQTTtoDB();

            // Ambil data dari getdata.php pertama kali
            fetchData();

            // Refresh data setiap 5 detik (5000 ms)
            setInterval(fetchData, 5000); // Mengambil data secara berkala
        });
    </script>

</body>
</html>
