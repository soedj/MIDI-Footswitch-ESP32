// ==========================================================
//              ESP32 MIDI Footswitch Controller
// ==========================================================


#include <Arduino.h>

// Pustaka untuk Fitur Tambahan
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------------- KONFIGURASI PENGGUNA ---------------
const char* ssid = "";
const char* password = "";
const int buttonPins[4] = {18, 19, 5, 17};
const int ledPins[4] = {25, 26, 27, 32};
const int NUM_BANKS = 3;

// --------------- VARIABEL GLOBAL ---------------
AsyncWebServer server(80);
Preferences preferences;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool lastButtonState[4] = {false};
bool comboUpState = false;
bool comboDownState = false;
int activePreset = -1;
int currentBank = 0;
String effectNames[4];
int midiModes[4];
int midiValues[4];

// --------------- PROTOTIPE FUNGSI ---------------
void kirimMIDI(int index);
void tampilkanOLED(int index);
void loadBank(int bank);
void saveBank(int bank);
void updateOLEDHomeScreen();
void setupServer();
void handleBankUp();
void handleBankDown();

// ==========================================================
//                           SETUP
// ==========================================================
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal!");
    while (true);
  }
 display.setRotation(2);
  Serial.println("Serial MIDI Footswitch Siap.");
  Serial.println("Kombinasi Tombol Aktif: 1+2=BankUp, 3+4=BankDown");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung! IP: ");
  Serial.println(WiFi.localIP());
  setupServer();
  server.begin();
  Serial.println("Async Web server dimulai.");
  
  loadBank(currentBank);
  updateOLEDHomeScreen();
}

// ==========================================================
//                            LOOP
// ==========================================================
void loop() {
  bool p1_pressed = !digitalRead(buttonPins[0]);
  bool p2_pressed = !digitalRead(buttonPins[1]);
  bool p3_pressed = !digitalRead(buttonPins[2]);
  bool p4_pressed = !digitalRead(buttonPins[3]);

  if (p1_pressed && p2_pressed) {
    if (!comboUpState) {
      handleBankUp();
      comboUpState = true;
    }
    return; 
  } else {
    comboUpState = false;
  }

  if (p3_pressed && p4_pressed) {
    if (!comboDownState) {
      handleBankDown();
      comboDownState = true;
    }
    return;
  } else {
    comboDownState = false;
  }

  bool currentButtonStates[4] = {p1_pressed, p2_pressed, p3_pressed, p4_pressed};
  for (int i = 0; i < 4; i++) {
    if (currentButtonStates[i] && !lastButtonState[i]) {
      activePreset = i;
      kirimMIDI(i);
      tampilkanOLED(i);
      for (int j = 0; j < 4; j++) {
        digitalWrite(ledPins[j], (j == activePreset) ? HIGH : LOW);
      }
      delay(500);
      updateOLEDHomeScreen();
    }
    lastButtonState[i] = currentButtonStates[i];
  }
  delay(20);
}


// ==========================================================
//                        FUNGSI-FUNGSI
// ==========================================================

void handleBankUp() {
  currentBank = (currentBank + 1) % NUM_BANKS;
  loadBank(currentBank);
  activePreset = -1;
  for(int i=0; i<4; i++) { digitalWrite(ledPins[i], LOW); }
  updateOLEDHomeScreen();
  delay(50);
}

void handleBankDown() {
  currentBank = (currentBank - 1 + NUM_BANKS) % NUM_BANKS;
  loadBank(currentBank);
  activePreset = -1;
  for(int i=0; i<4; i++) { digitalWrite(ledPins[i], LOW); }
  updateOLEDHomeScreen();
  delay(50);
}

void kirimMIDI(int index) {
  const int midiChannel = 1;
  int mode = midiModes[index];
  int value = midiValues[index];
  if (mode == 0) { // PC
    Serial.write(0xC0 | (midiChannel - 1));
    Serial.write(value);
  } else { // CC
    Serial.write(0xB0 | (midiChannel - 1));
    Serial.write(value);
    Serial.write(127);
  }
}

/**
 * @brief Menampilkan layar utama dan memberi highlight pada preset aktif.
 */
void updateOLEDHomeScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE); // Warna default untuk judul "BANK"
  display.setCursor(0, 0);
  display.printf("BANK %d", currentBank + 1);
  
  display.setTextSize(1);
  for(int i = 0; i < 4; i++) {
    int yPos = 20 + (i * 11);
    display.setCursor(0, yPos);

    // *** PERUBAHAN DI SINI ***
    // Cek apakah preset ini yang sedang aktif
    if (i == activePreset) {
      // Jika aktif, set warna menjadi teks hitam dengan background putih
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
      // Jika tidak aktif, set warna normal (teks putih)
      display.setTextColor(SSD1306_WHITE);
    }
    display.printf("%d: %s", i + 1, effectNames[i].c_str());
  }
  display.display();
}

/**
 * @brief Menampilkan detail preset dengan background layar penuh.
 */
void tampilkanOLED(int index) {
  display.clearDisplay();

  // *** PERUBAHAN DI SINI ***
  // 1. Gambar kotak putih di seluruh layar
  display.fillRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
  // 2. Set warna teks menjadi hitam
  display.setTextColor(SSD1306_BLACK);

  display.setTextSize(2);
  display.setCursor(0, 10);
  display.println(effectNames[index].c_str());
  
  display.setTextSize(1);
  display.setCursor(0, 40);
  if (midiModes[index] == 0) {
    display.printf("PC: %d", midiValues[index]);
  } else {
    display.printf("CC: %d", midiValues[index]);
  }
  display.display();
}

void loadBank(int bank) {
  preferences.begin("midiCtrl", true);
  for (int i = 0; i < 4; i++) {
    effectNames[i] = preferences.getString(("b"+String(bank)+"n"+String(i)).c_str(), "Preset "+String(i+1));
    midiModes[i] = preferences.getInt(("b"+String(bank)+"m"+String(i)).c_str(), 0);
    midiValues[i] = preferences.getInt(("b"+String(bank)+"v"+String(i)).c_str(), i);
  }
  preferences.end();
}

void saveBank(int bank) {
  preferences.begin("midiCtrl", false);
  for (int i = 0; i < 4; i++) {
    preferences.putString(("b"+String(bank)+"n"+String(i)).c_str(), effectNames[i]);
    preferences.putInt(("b"+String(bank)+"m"+String(i)).c_str(), midiModes[i]);
    preferences.putInt(("b"+String(bank)+"v"+String(i)).c_str(), midiValues[i]);
  }
  preferences.end();
}

void setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    int bankToShow = currentBank;
    if (request->hasParam("bank")) {
      bankToShow = request->getParam("bank")->value().toInt();
    }
    if (bankToShow < 0 || bankToShow >= NUM_BANKS) {
      bankToShow = currentBank;
    }
    String tempNames[4];
    int tempModes[4], tempValues[4];
    preferences.begin("midiCtrl", true);
    for(int i = 0; i < 4; i++){
        tempNames[i] = preferences.getString(("b"+String(bankToShow)+"n"+String(i)).c_str(), "Preset "+String(i+1));
        tempModes[i] = preferences.getInt(("b"+String(bankToShow)+"m"+String(i)).c_str(), 0);
        tempValues[i] = preferences.getInt(("b"+String(bankToShow)+"v"+String(i)).c_str(), i);
    }
    preferences.end();
    String html = R"rawliteral(
<!DOCTYPE html><html><head><title>MIDI Controller Config</title><meta name="viewport" content="width=device-width, initial-scale=1.0"><style>:root{--primary-bg:#1e1e1e;--secondary-bg:#2d2d2d;--font-color:#e0e0e0;--accent-color:#03a9f4;--border-color:#444}body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;background-color:var(--primary-bg);color:var(--font-color);margin:0;padding:15px}.container{max-width:800px;margin:0 auto}h2,h3{color:var(--accent-color);border-bottom:2px solid var(--border-color);padding-bottom:10px}.bank-nav{display:flex;gap:10px;margin-bottom:20px}.bank-nav a{padding:10px 15px;text-decoration:none;background-color:var(--secondary-bg);color:var(--font-color);border-radius:5px;border:1px solid var(--border-color);transition:all .2s ease-in-out}.bank-nav a.active,.bank-nav a:hover{background-color:var(--accent-color);color:#fff;box-shadow:0 0 10px var(--accent-color)}.preset-block{background-color:var(--secondary-bg);padding:15px;border-radius:8px;margin-bottom:15px;border:1px solid var(--border-color)}label{display:block;margin-bottom:5px;font-weight:bold;color:#ccc}input[type=text],input[type=number],select{width:100%;padding:12px;margin-bottom:10px;border-radius:5px;border:1px solid var(--border-color);background-color:var(--primary-bg);color:var(--font-color);box-sizing:border-box;font-size:1em}.submit-btn{width:100%;padding:15px;border:none;border-radius:5px;background-color:var(--accent-color);color:#fff;font-size:1.2em;font-weight:bold;cursor:pointer;transition:background-color .2s ease-in-out}.submit-btn:hover{background-color:#0288d1}</style></head><body><div class="container"><h2>MIDI Controller Config</h2><h3>Select Bank to Edit</h3><div class="bank-nav">
)rawliteral";
    for(int b=0; b<NUM_BANKS; b++){
      html += "<a href='/?bank=" + String(b) + "'";
      if (b == bankToShow) { html += " class='active'"; }
      html += ">Bank " + String(b+1) + "</a>";
    }
    html += R"rawliteral(</div><form action='/save' method='POST'><input type='hidden' name='bank' value='")rawliteral" + String(bankToShow) + R"rawliteral('>)rawliteral";
    for (int i = 0; i < 4; i++) {
      html += "<div class='preset-block'><h4>Preset " + String(i + 1) + "</h4>";
      html += "<label for='name" + String(i) + "'>Effect Name</label>";
      html += "<input type='text' id='name" + String(i) + "' name='name" + String(i) + "' value='" + tempNames[i] + "'>";
      html += "<label for='mode" + String(i) + "'>MIDI Mode</label>";
      html += "<select id='mode" + String(i) + "' name='mode" + String(i) + "'>";
      html += "<option value='0'" + String(tempModes[i] == 0 ? " selected" : "") + ">Program Change (PC)</option>";
      html += "<option value='1'" + String(tempModes[i] == 1 ? " selected" : "") + ">Control Change (CC)</option>";
      html += "</select>";
      html += "<label for='value" + String(i) + "'>MIDI Value (0-127)</label>";
      html += "<input type='number' id='value" + String(i) + "' name='value" + String(i) + "' min='0' max='127' value='" + String(tempValues[i]) + "'>";
      html += "</div>";
    }
    html += R"rawliteral(<input type='submit' class='submit-btn' value='Save Bank ")rawliteral" + String(bankToShow + 1) + R"rawliteral('"></form></div></body></html>)rawliteral";
    
    request->send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("bank", true)) {
      request->send(400, "text/plain", "Bad Request");
      return;
    }
    int bankToSave = request->getParam("bank", true)->value().toInt();

    for (int i = 0; i < 4; i++) {
      effectNames[i] = request->getParam("name" + String(i), true)->value();
      midiModes[i] = request->getParam("mode" + String(i), true)->value().toInt();
      midiValues[i] = request->getParam("value" + String(i), true)->value().toInt();
    }
    saveBank(bankToSave);
    
    if (bankToSave == currentBank) {
      loadBank(currentBank);
      updateOLEDHomeScreen();
    }

    request->redirect("/?bank=" + String(bankToSave));
  });
}