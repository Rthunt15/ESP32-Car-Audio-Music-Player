#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"

#define SCK_PIN   18  // Default SCK
#define MOSI_PIN  23  // Default MOSI
#define MISO_PIN  22  // Remapped MISO to GPIO22
#define RA8875_CS 5
#define RA8875_RESET 4

String songTitle = "";
String songArtist = "";
String songAlbum = "";
uint32_t totalPlaytimeMs = 0;
uint32_t currentPlaytimeMs = 0;

const int callPin = 2;
const int rewindPin = 2;
const int pausePin = 2;
const int forwardPin = 2;
const int volDownPin = 2;
const int volUpPin = 2;

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);
uint16_t tx, ty;

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

bool isPlaying = false;
unsigned long previousMillis = 0;  // For playtime updates

// Utility function to format time in minutes and seconds
String formatTime(uint32_t timeMs) {
  uint32_t totalSeconds = timeMs / 1000;
  uint32_t minutes = totalSeconds / 60;
  uint32_t seconds = totalSeconds % 60;
  char buffer[6];
  sprintf(buffer, "%02d:%02d", minutes, seconds);
  return String(buffer);
}

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  Serial.printf("==> AVRC metadata rsp: attribute id 0x%x, %s\n", id, text);

  switch (id) {
    case ESP_AVRC_MD_ATTR_TITLE:
      songTitle = String((char*)text);
      break;
    case ESP_AVRC_MD_ATTR_ARTIST:
      songArtist = String((char*)text);
      break;
    case ESP_AVRC_MD_ATTR_ALBUM:
      songAlbum = String((char*)text);
      break;
    case ESP_AVRC_MD_ATTR_PLAYING_TIME:
      totalPlaytimeMs = String((char*)text).toInt();
      break;
  }

  // Refresh the display with updated metadata
  updateDisplay();
}

void playbackStateCallback(esp_a2d_audio_state_t state, void* param) {
  if (state == ESP_A2D_AUDIO_STATE_STARTED) {
    Serial.println("Playing");
    isPlaying = true;
    updatePlayPauseStatus(true);
  } else if (state == ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND || state == ESP_A2D_AUDIO_STATE_STOPPED) {
    Serial.println("Paused");
    isPlaying = false;
    updatePlayPauseStatus(false);
  }
}

void updatePlayPauseStatus(bool playing) {
  // Clear previous status line and update
  tft.textSetCursor(10, 310);
  tft.textWrite("                ");  // Clear the previous text
  tft.textSetCursor(10, 310);
  tft.textWrite(playing ? "Status: Playing" : "Status: Paused");
}

void updateDisplay() {
  tft.fillScreen(RA8875_BLACK);  // Clear screen
  tft.textMode();  // Set text mode
  tft.textEnlarge(2);
  
  // Display metadata on the screen
  tft.textColor(RA8875_GREEN, RA8875_BLACK);  // Green text on black background

  tft.textSetCursor(10, 10);   // Line 1: Title
  tft.textWrite("Title: ");
  tft.textWrite(songTitle.c_str());

  tft.textSetCursor(10, 110);  // Line 2: Artist
  tft.textWrite("Artist: ");
  tft.textWrite(songArtist.c_str());

  tft.textSetCursor(10, 210);  // Line 3: Album
  tft.textWrite("Album: ");
  tft.textWrite(songAlbum.c_str());

  tft.textSetCursor(10, 310);  // Line 4: Status
  tft.textWrite(isPlaying ? "Status: Playing" : "Status: Paused");

  // Update playtime immediately
  updatePlaytime();
}

void updatePlaytime() {
  // If there's no total playtime, don't display the playtime yet
  if (totalPlaytimeMs == 0) {
    return;
  }

  // Calculate current playtime
  String currentTime = formatTime(currentPlaytimeMs);
  String totalTime = formatTime(totalPlaytimeMs);

  // Clear and update the playtime line
  tft.textSetCursor(10, 410);
  tft.textWrite("                ");  // Clear previous time
  tft.textSetCursor(10, 410);
  tft.textWrite((currentTime + " / " + totalTime).c_str());
}

void setup() {
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, RA8875_CS); 
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = 26;
  cfg.pin_ws = 25;
  cfg.pin_data = 19;
  i2s.begin(cfg);
  Serial.begin(115200);

  pinMode(callPin, INPUT);
  pinMode(rewindPin, INPUT);
  pinMode(pausePin, INPUT);
  pinMode(forwardPin, INPUT);
  pinMode(volDownPin, INPUT);
  pinMode(volUpPin, INPUT);

  // Initialize display
  Serial.println("RA8875 start");
  if (!tft.begin(RA8875_800x480)) {
    Serial.println("RA8875 Not Found!");
    while (1);
  }

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);
  tft.fillScreen(RA8875_BLACK);
  tft.textMode();
  tft.cursorBlink(32);

  // Initialize Bluetooth
  a2dp_sink.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_PLAYING_TIME);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.start("Explorer Audio");

  // Set playback state callback
  a2dp_sink.set_on_audio_state_changed(playbackStateCallback);

  // Display initial metadata
  updateDisplay();
}

void loop() {
  unsigned long currentMillis = millis();

  // If the song is playing, update the playtime every second
  if (isPlaying && totalPlaytimeMs > 0) {
    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;
      currentPlaytimeMs += 1000;  // Increment playtime by 1 second
      updatePlaytime();  // Update playtime on the display
    }
  }
}
