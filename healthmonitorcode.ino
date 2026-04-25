#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int tempPin = A0;
const int pulsePin = A1;
const float TEMP_OFFSET = 26.3; 

float dcFilter = 0;
float pulseWave = 0;
float previousPulseWave = 0; 
unsigned long lastBeatTime = 0;
int displayBpm = 0;
boolean firstRun = true;
float smoothedTempC = 32.0;

int currentX = 0;
int lastPulseY = 38;
unsigned long lastSampleTime = 0;

// --- CUSTOM GRAPHICS ---
const unsigned char heartBitmap [] PROGMEM = {
  0b00000000,
  0b01100110,
  0b11111111,
  0b11111111,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000
};

void setup() {
  Wire.setClock(100000); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); 
  }

  Wire.setWireTimeout(25000, true);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(10, 25); 
  display.println("SYSTEM LOADING...");
  
  display.display();
  delay(1500);
  
  display.clearDisplay();
  
  // BPM and Temp values
  display.setCursor(0, 0);
  display.print("BPM:");
  display.setCursor(65, 0);      
  display.print("TEMP:");
  
  // Single-line separator
  display.drawLine(0, 10, 128, 10, WHITE);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSampleTime >= 20) {
    lastSampleTime = currentMillis;

    // Pulse sensor 
    int rawPulse = 0;
    for(int i = 0; i < 4; i++){
      rawPulse += analogRead(pulsePin);
    }
    rawPulse = rawPulse / 4; 

    if (firstRun) { dcFilter = rawPulse; firstRun = false; }
    
    dcFilter = (0.95 * dcFilter) + (0.05 * rawPulse); 
    float acSignal = rawPulse - dcFilter;
    pulseWave = (0.85 * pulseWave) + (0.15 * acSignal); 

    // BPM Calculation
    if (pulseWave > 1.0 && previousPulseWave <= 1.0 && (currentMillis - lastBeatTime) > 300) { 
      int ibi = currentMillis - lastBeatTime;
      lastBeatTime = currentMillis;
      int bpm = 60000 / ibi;
      
      if (bpm > 45 && bpm < 150) { 
          displayBpm = bpm; 
      }
    }
    previousPulseWave = pulseWave;

    // Wave Mapping 
    int pulseY = map(pulseWave, 3, -3, 20, 60); 
    pulseY = constrain(pulseY, 12, 63);


    // Temperature Sensor 
    int tempRaw = analogRead(tempPin);
    float instantTempC = ((tempRaw * 500.0) / 1024.0) - TEMP_OFFSET;
    smoothedTempC = (0.95 * smoothedTempC) + (0.05 * instantTempC);


    // Dashboard text
    display.setTextColor(WHITE, BLACK); 
    
    // BPM Text
    display.setCursor(26, 0);
    if (currentMillis - lastBeatTime > 4000) { 
      display.print("-- "); 
    } else { 
      if (displayBpm < 100) display.print(" "); 
      display.print(displayBpm); 
    }

    // Flashing heart
    if (currentMillis - lastBeatTime < 150) {
      display.drawBitmap(48, 0, heartBitmap, 8, 8, WHITE);
    } else {
      display.fillRect(48, 0, 8, 8, BLACK); 
    }

    // Temp text
    display.setCursor(95, 0);
    display.print(smoothedTempC, 1); 
    display.print("C ");


    // Wave 
    display.fillRect(currentX + 1, 11, 8, 53, BLACK);

    // Draw the new segment
    if (currentX > 0) { 
      display.drawLine(currentX - 1, lastPulseY, currentX, pulseY, WHITE);
    }

    display.display();

    // Move forward
    lastPulseY = pulseY;
    currentX++;

    // Wrap around to the left edge
    if (currentX >= SCREEN_WIDTH) {
      currentX = 0;
      display.fillRect(0, 11, 8, 53, BLACK); 
    }
    
    Wire.clearWireTimeoutFlag();
  }
}
