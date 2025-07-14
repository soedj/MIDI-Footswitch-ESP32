# MIDI Footswitch Controller DIY Berbasis ESP32



Controller MIDI 4-tombol yang dapat dikonfigurasi melalui web server, dirancang untuk mengontrol software musik seperti AmpliTube, Helix Native, dan DAW lainnya.


##  Fitur Utama

* 4 Tombol Preset per Bank dengan indikator LED
* Fungsi Bank Up/Down menggunakan kombinasi tombol (1+2 & 3+4)
* Konfigurasi via **Web Server** melalui WiFi
* Layar **OLED** untuk menampilkan status bank dan preset
* Mode MIDI yang dapat diubah (**Program Change** & **Control Change**) per tombol
* Output MIDI melalui Serial (kompatibel dengan Hairless MIDI Bridge)

##  Komponen yang Dibutuhkan

* ESP32 DevKitC
* 4x Footswitch Momentary
* 4x LED 5mm
* 4x Resistor (sesuaikan dengan LED Anda, misal 220 Ohm)
* Layar OLED SSD1306 0.96 inch
* Kabel dan Project Box/Enclosure

##  Setup & Instalasi

1.  **Hardware**: Rakit semua komponen sesuai dengan skematik yang ada di folder `/hardware`.
2.  **Firmware**:
    * Buka file `/firmware/code.ino` menggunakan Arduino IDE.
    * Instal semua pustaka yang dibutuhkan melalui Library Manager.
    * Ubah kredensial WiFi (SSID & Password) di dalam kode.
    * Upload firmware ke board ESP32 Anda.
3.  **Software PC**:
    * Hubungkan ESP32 ke PC.
    * Jalankan [Hairless MIDI<->Serial Bridge](https://projectgus.github.io/hairless-midiserial/) dan atur ke port COM ESP32 Anda dengan baud rate 115200.
    * Gunakan [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) untuk membuat port MIDI virtual sebagai output dari Hairless.
    * Buka DAW (Reaper, dll) atau software gitar Anda dan atur input MIDI ke port virtual dari loopMIDI.

##  Konfigurasi via Web Server

1.  Saat pertama kali dinyalakan, ESP32 akan membuat jaringan WiFi. Sambungkan HP atau laptop Anda.
2.  Buka browser dan masuk ke alamat IP(IP yang tertera di Serial Monitor jika terhubung).
3.  Atur nama, mode MIDI (PC/CC), dan nilai MIDI untuk setiap tombol dan bank.
4.  Klik "Save Bank".

