#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"

#define SCK_PIN   18  // Default SCK
#define MOSI_PIN  23  // Default MOSI
#define MISO_PIN  22  // Remapped MISO to GPIO22
#define RA8875_CS 5
#define RA8875_RESET 4

ezButton buttonPrev(32);  
ezButton buttonRewind(14); 
ezButton buttonPause(13); 
ezButton buttonSkip(12); 
ezButton buttonVolDown(33);
ezButton buttonVolUp(27); 

const unsigned long debounceDelay = 50;
unsigned long lastDebounceTime[6] = {0};

bool lastButtonState[6] = {HIGH};

bool callPressed = false;
bool prevSongPressed = false;
bool playPausePressed = false;
bool nextSongPressed = false;
bool volumeDownPressed = false;
bool volumeUpPressed = false;

String songTitle = "";
String songArtist = "";
String songAlbum = "";
uint32_t currentPlaytimeMs = 0; // Current playtime in milliseconds
uint32_t totalPlaytimeMs = 0;   // Total playtime in milliseconds

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

bool isPlaying = false;

// Utility function to format time in minutes and seconds
String formatTime(uint32_t timeMs) {
  uint32_t totalSeconds = timeMs / 1000;
  uint32_t minutes = totalSeconds / 60;
  uint32_t seconds = totalSeconds % 60;
  char buffer[6];
  sprintf(buffer, "%02d:%02d", minutes, seconds);
  return String(buffer);
}

void avrc_rn_play_pos_callback(uint32_t play_pos) {
  currentPlaytimeMs = play_pos; // Update current playtime based on position
  Serial.printf("Current play position is %d (%d seconds)\n", play_pos, (int)round(play_pos / 1000.0));
}

////////////////////////////////// Metadata String Handling //////////////////////////////////

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
      totalPlaytimeMs = String((char*)text).toInt() * 1000; // Convert to milliseconds
      break;
  }

  // Refresh the display with updated metadata
  updateDisplay();
}



void playbackStateCallback(esp_a2d_audio_state_t state, void* param) {
  if (state == ESP_A2D_AUDIO_STATE_STARTED) {
    Serial.println("Playing");
    isPlaying = true;
  } else if (state == ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND || state == ESP_A2D_AUDIO_STATE_STOPPED) {
    Serial.println("Paused");
    isPlaying = false;
  }
}

////////////////////////////////// Update Display //////////////////////////////////

void updateDisplay() {
  tft.fillScreen(RA8875_BLACK);  // Clear screen
  tft.textMode();  // Set text mode
  tft.textEnlarge(2.5);
  
  // Display metadata on the screen
  tft.textColor(RA8875_GREEN, RA8875_BLACK);  // Green text on black background

  tft.textSetCursor(10, 50);   // Line 1: Title
 //tft.textWrite("Title: ");
  tft.textWrite(songTitle.c_str());

  tft.textSetCursor(10, 210);  // Line 2: Artist
 // tft.textWrite("Artist: ");
  tft.textWrite(songArtist.c_str());

  tft.textSetCursor(10, 370);  // Line 3: Album
  //tft.textWrite("Album: ");
  tft.textWrite(songAlbum.c_str());

 // tft.textSetCursor(10, 350);  // Line 4: Status
 // tft.textWrite(isPlaying ? ">" : "||");

  // Update playtime display
  tft.textSetCursor(10, 360);
  tft.textColor(RA8875_BLACK, RA8875_BLACK);
  tft.textWrite("");
  updatePlaytimeDisplay();
  delay(1000);
}





void updatePlaytimeDisplay() {
  String currentTime = formatTime(currentPlaytimeMs);
  String totalTime = formatTime(totalPlaytimeMs);

 // tft.textSetCursor(10, 410);  // Playtime line
 // tft.textWrite("                ");  // Clear previous time
 // tft.textSetCursor(10, 410);
 // tft.textWrite((currentTime + " / " + totalTime).c_str());
}

////////////////////////////////// Setup //////////////////////////////////

void setup() {
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, RA8875_CS); 
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = 26;
  cfg.pin_ws = 25;
  cfg.pin_data = 19;
  i2s.begin(cfg);
  Serial.begin(115200);

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
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_on_audio_state_changed(playbackStateCallback);
  a2dp_sink.set_avrc_rn_play_pos_callback(avrc_rn_play_pos_callback);
  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.start("Explorer Audio");

  pinMode(BUTTON_CALL, INPUT_PULLUP);
  pinMode(BUTTON_PREV_SONG, INPUT_PULLUP);
  pinMode(BUTTON_PLAY_PAUSE, INPUT_PULLUP);
  pinMode(BUTTON_NEXT_SONG, INPUT_PULLUP);
  pinMode(BUTTON_VOLUME_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_VOLUME_UP, INPUT_PULLUP);

  // Display initial metadata
  updateDisplay();
}

////////////////////////////////// Loop //////////////////////////////////

void loop() {
  // Keep looping to handle Bluetooth events
  delay(1000); // Add a delay to prevent rapid updates
  //updatePlaytimeDisplay(); // Update current playtime periodically
}

