#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

// Initializing pin numbers
int buzzerPin = 9;
int buttons[] = {6, 5, A5, A4, A3, A2, A1, A0};
int recordButton = 8;
int nextSongButton = 2;
int playbackButton = 3;

// Setting tone frequency for musical notes:
// Do, Re, Mi, Fa, So, La, Si, Do2
int frequency[] = {262, 294, 330, 349, 392, 440, 494, 524};

// Root directory
File rootDir;

// Recording variables
unsigned long recordingOnTime = 0;
unsigned long recordingOffTime = 0;
int recordingFlag = 0;
File recordingFile;

// Playback variables
int playbackFlag = 0;
File playbackFile;
int nextSongFlag = 0;
int playbackFinished = 0;

int buttonDebounce = 70;

// Musical tones debounce variables
int tonePreviousState[] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned int tonePreviousPress[] = {0, 0, 0, 0, 0, 0, 0, 0};

// Next song button debounce variables
int previousState1 = HIGH;
unsigned int previousPress1;

// Playback button debounce variables
int previousState2 = HIGH;
unsigned int previousPress2;

// Record button debounce variables
int previousState3 = HIGH;
unsigned int previousPress3;

int i = 0;

void setup(){
  Serial.begin(9600);

  // Initialize buttons and buzzer
  pinMode(buttons[0], INPUT_PULLUP);
  pinMode(buttons[1], INPUT_PULLUP);
  pinMode(buttons[2], INPUT_PULLUP);
  pinMode(buttons[3], INPUT_PULLUP);
  pinMode(buttons[4], INPUT_PULLUP);
  pinMode(buttons[5], INPUT_PULLUP);
  pinMode(buttons[6], INPUT_PULLUP);
  pinMode(buttons[7], INPUT_PULLUP);
  pinMode(nextSongButton, INPUT_PULLUP);
  pinMode(playbackButton, INPUT_PULLUP);
  pinMode(recordButton, INPUT_PULLUP);

  pinMode(buzzerPin, OUTPUT);
  noTone(buzzerPin);

  // Initialize SD card
  Serial.println("Initializing SD card...");
  if(!SD.begin(10))
    Serial.println("initialization failed!");
  else
    Serial.println("initialization done.");

  rootDir = SD.open("/");

  // Initialize LCD screen
  lcd.init();
  lcd.clear();
  lcd.backlight();

  playbackFile = rootDir.openNextFile();
  if(!playbackFile){
    lcd.print("No files.");
  }
  else{
    lcd.print(playbackFile.name());
  }
}

void nextSong(){
  if(playbackFile)
    playbackFile.close();

  playbackFile = rootDir.openNextFile();
  if(!playbackFile){
    rootDir.close();
    rootDir = SD.open("/");

    playbackFile = rootDir.openNextFile();
  }

  if(playbackFile){
    nextSongFlag = 1;
    lcd.clear();
    lcd.print(playbackFile.name());
    Serial.println(playbackFile.name());
    playbackFinished = 0;
  }
}

void nextSongButtonLoop(){
    if((millis() - previousPress1) > buttonDebounce){
      previousPress1 = millis();

      if(digitalRead(nextSongButton) == LOW && previousState1 == HIGH){
        Serial.println("next song");
        nextSong();

        previousState1 = LOW;
      }
      
      else if(digitalRead(nextSongButton) == HIGH && previousState1 == LOW){
        previousState1 = HIGH;
      }
  }
}

void playbackButtonLoop(){
  // Playback button read
  if((millis() - previousPress2) > buttonDebounce){
    previousPress2 = millis();

    if(digitalRead(playbackButton) == LOW && previousState2 == HIGH){
      Serial.println("playback toggle");
      playbackFlag = !playbackFlag;

      previousState2 = LOW;
    }
    else if(digitalRead(playbackButton) == HIGH && previousState2 == LOW){
      previousState2 = HIGH;
    }
  }
}

void playTone(int button){
  tone(buzzerPin, frequency[button]);

  recordingOnTime = millis();
  if(i > 0){
    if(recordingFlag){
      recordingFile.print(recordingOnTime - recordingOffTime);
      recordingFile.println();
    }
  }
  while(digitalRead(buttons[button])==LOW);
  recordingOffTime = millis();

  if(recordingFlag){
    recordingFile.print(frequency[button]);
    recordingFile.print(" ");
    recordingFile.print(recordingOffTime - recordingOnTime);
    recordingFile.print(" ");
  }

  ++i;

  Serial.print("button ");
  Serial.print(button);
  Serial.print(" pressed");
  Serial.println();
}

void startRecording(){
  recordingFlag = 1;
  i = 0;

  char filename[] = "track00.txt";
  for(uint8_t j = 0; j < 100; ++j){
    filename[5] = j / 10 + '0';
    filename[6] = j % 10 + '0';
    if(!SD.exists(filename)){
      recordingFile = SD.open(filename, FILE_WRITE);
      break;
    }
  }

  Serial.println("started recording");
}

void stopRecording(){
  recordingFlag = 0;

  recordingFile.print(0);
  recordingFile.close();

  Serial.println("finished recording");
}

void recordingButtonLoop(){
  if((millis() - previousPress3) > buttonDebounce){
    previousPress3 = millis();

    if(digitalRead(recordButton) == LOW && previousState3 == HIGH){
      if(recordingFlag == 0)
        startRecording();
      else
        stopRecording();

      previousState3 = LOW;
    }  
    else if(digitalRead(recordButton) == HIGH && previousState3 == LOW){
      previousState3 = HIGH;
    }
  }
}

void pianoButtonsLoop(){
  for(int j = 0; j < 8; ++j){
    if((millis() - tonePreviousPress[j]) > buttonDebounce){
      tonePreviousPress[j] = millis();

      if(digitalRead(buttons[j]) == LOW && tonePreviousState[j] == HIGH){
        playTone(j);

        tonePreviousState[j] = LOW;
        break;
      }  
      else if(digitalRead(buttons[j]) == HIGH && tonePreviousState[j] == LOW){
        tonePreviousState[j] = HIGH;
      }
    }
  }

  noTone(buzzerPin);
}

void playbackLoop(){
  if(playbackFile.available()){
    int freq = playbackFile.parseInt();
    unsigned long onTime = playbackFile.parseInt();
    unsigned long offTime = playbackFile.parseInt();

    tone(buzzerPin, freq);
    delay(onTime);
    noTone(buzzerPin);
    delay(offTime);
  }
  else{
    if(playbackFinished == 0){
      lcd.clear();
      lcd.print(playbackFile.name());
      lcd.setCursor(0, 1);
      lcd.print("finished");
      playbackFinished = 1;
    }
  }
}

void loop(){
  nextSongButtonLoop();
  
  if(recordingFlag == 0)
    playbackButtonLoop();

  if(playbackFlag == 0){
    recordingButtonLoop();
    pianoButtonsLoop();
  }
  else{
    playbackLoop();
  }
}