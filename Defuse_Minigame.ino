/**********************************************************************
** Arduino Defuse Puzzle Minigame:
** The premise of this minigame is to "defuse the bomb" by solving
** puzzles and pulling the correct wires before the timer runs out.
**
** Hardware Used: Arduino UNO R4 Minima, Adafruit 128x64 OLED
** display, SunFounder I2C LCD1602, SunFounder RGB LED module,
** SunFounder Touch Module, 4 removable jumper wires (Green, Yellow,
** Blue, and Red), and SunFounder Passive Buzzer Module
** 
** Pin Configuration:
** 2: Input Pullup, Green Wire to ground
** 3: Input Pullup, Yellow Wire to ground
** 4: Input Pullup, Blue Wire to ground
** 5: Input Pullup, Red Wire to ground
** 8: Input, Touch Sensor
** 9: Output, Passive Buzzer
** 10: Output, Red for RGB
** 11: Output, Green for RGB
** 12: Output, Blue for RGB
** A4: OLED SDA & LCD SDA
** A5: OLED SCK & LCD SCL
**********************************************************************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define GreenWire 0
#define YellowWire 1
#define BlueWire 2
#define RedWire 3

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define DEBUG true

// Constructors

  LiquidCrystal_I2C lcd(0x27, 16, 2);
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Global declarations
  int difficulty;
  int gameDuration;
  int currentWire = 0;
  int nWireCuts;
  bool ButtonPress = true;
  bool previousButtonPress = true;
  int pSeconds = 0;
  unsigned long startTime;
  int clueCase;
  int clue1;
  int clue2;
  int clue3;
  int clue4;
  int nBlinks;
  int LEDcolor;
  unsigned long lastBlinkTime = 0;
  int blinkCount = 0;
  bool ledState = false;
  const int blinkDelay = 300;
  const int longDelay = 1000;

// Array declarations
  const char* wireNames[4] = {"Green", "Yellow", "Blue", "Red"};
  const int playerWire[4] {2,3,4,5};
  bool playerWirePresent[4] {true,true,true,true};
  bool lastWirePresent[4] {true,true,true,true};
  int wirePullOrder[4] {GreenWire, YellowWire, BlueWire, RedWire};
  char serialNumber[6];

// Function Declarations
  int placeholder();
  bool wirePresent(int pin);
  int difficultySelection();
  void shuffleOrder();
  void wirePulled(int wire);
  void boom();
  void defuse();
  void detectButtonPress();
  void detectWirePull();
  void displayTimer();
  void levelCompleteJingle();
  void gameOverJingle();
  void updateLED();
  void blinkLED(int color);
  int generateFibonacci(int cutWire, int caseLED);
  int generateArithmetic(int cutWire, int caseLED);
  int generateGeometric(int cutWire, int caseLED);
  void generateMaze(int maze, int a);
  void generateSerialNumber();
  int accurateRandom(int n);

void setup() {
// Initialization

  // Initialize serial monitor for debug mode
    if (DEBUG) Serial.begin (9600);
    while (!Serial);
    delay(500);
    Serial.println("Serial is ready");

  // Initialize pin modes
    for (int i=0 ; i<4 ; i++) pinMode(playerWire[i], INPUT_PULLUP);   // Pullable jumper wires
    pinMode(8,INPUT);   // Touch Sensor Button
    pinMode(9,OUTPUT);  // Buzzer
    for (int i=10 ; i<=12 ; i++) pinMode(i,OUTPUT);   // RGB LED
  
  randomSeed(analogRead(A0));   // Initialize random() seed

  // Display Setup
    lcd.init();
    lcd.backlight();
    
    Wire.begin();
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    while(1);
    }
    display.clearDisplay();
    display.display();
  
  // Clear RGB LED
    for (int i=10 ; i<=12 ; i++) digitalWrite(i,0);

// Game Setup

  // Determine difficulty
    difficulty = difficultySelection();
    lcd.clear();
    lcd.setCursor(0,0);
    switch(difficulty){   // Set game duration and display selected difficulty depending on difficultySelection() output
      case 0: 
      lcd.print("Difficulty:");
      lcd.setCursor(0,1);
      lcd.print("Easy");
      gameDuration = 240000;
      break;
      case 1: 
      lcd.print("Difficulty:");
      lcd.setCursor(0,1);
      lcd.print("Medium");
      gameDuration = 120000;
      break;
      case 2: 
      lcd.print("Difficulty:");
      lcd.setCursor(0,1);
      lcd.print("Hard");
      gameDuration = 60000;
      break;
      case 3: 
      lcd.print("Difficulty:");
      lcd.setCursor(0,1);
      lcd.print("Insane");
      gameDuration = 30000;
      break;
    }

  // Wait for difficulty selection wire to be re-inserted
    bool pinReplaced = false;
    while(pinReplaced == false){
    playerWirePresent[difficulty] = wirePresent(playerWire[difficulty]);
    if ((playerWirePresent[difficulty] == true) && (playerWirePresent[difficulty] != lastWirePresent[difficulty])){
      pinReplaced = true;
    }
    lastWirePresent[difficulty] = playerWirePresent[difficulty];
  }

  lcd.clear();

  
  shuffleOrder(); // Shuffle wire pull order

  // Print wire order in debug mode
    Serial.print("Wire Order: ");
    for (int j=0 ; j<4 ; j++){
      Serial.print(wireNames[wirePullOrder[j]]);
      if(j<3) Serial.print(", ");
    }
    Serial.println();

  // Determine # of pulls needed
    nWireCuts = accurateRandom(3)+1;
    Serial.print("Number of wires: ");
    Serial.println(nWireCuts);

  generateSerialNumber(); // Generate and display a randomized serial #

  // Set up OLED Puzzle
    int mazeType;
    int mazeCase;
    int mazeWire;
    if(serialNumber[5] % 2 == 0) mazeWire = 0;
    else mazeWire = 1;

  switch(wirePullOrder[mazeWire]){
    case 0:
      switch(wirePullOrder[3]){
        case 1:
          if (accurateRandom(2)){ mazeType=6; mazeCase=2; }
          else{ mazeType=5; mazeCase=1; } break;
        case 2: mazeType=7; mazeCase=1; break;
        case 3:
          if (accurateRandom(2)){ mazeType=4; mazeCase=2; }
          else{ mazeType=1; mazeCase=1; } break;
      } break;
    case 1:
      switch(wirePullOrder[3]){
        case 0: if (accurateRandom(2)){ mazeType=3; mazeCase=1; }
        else{ mazeType=6; mazeCase=1;} break;
        case 2: if (accurateRandom(2)){ mazeType=8; mazeCase=1; }
        else{ mazeType=9; mazeCase=2; } break;
        case 3: mazeType=2; mazeCase=1; break;
      } break;
    case 2:
      switch(wirePullOrder[3]){
        case 0: mazeType=3; mazeCase=2; break;
        case 1: mazeType=4; mazeCase=1; break;
        case 3: if (accurateRandom(2)){ mazeType=9; mazeCase=1; }
        else{ mazeType=2; mazeCase=2;} break;
      } break;
    case 3:
      switch(wirePullOrder[3]){
        case 0: if (accurateRandom(2)){ mazeType=7; mazeCase=2; }
        else{ mazeType=1; mazeCase=2;} break;
        case 1: mazeType=8; mazeCase=2; break;
        case 2: mazeType=5; mazeCase=2; break;
      } break;
  }

  generateMaze(mazeType,mazeCase);
  display.display();

  // Set up LED conditions

    int LEDclue = accurateRandom(2);
    if (LEDclue){
      nBlinks = nWireCuts;
      LEDcolor = accurateRandom(4);
      Serial.println("Blinks = Wire Cuts");
    }else{
      nBlinks = accurateRandom(3)+1;
      LEDcolor = wirePullOrder[nWireCuts-1];
      Serial.println("Color = Last Cut");
    }

  // Set up LCD Puzzle and conditions

    bool conditionMet = false;

    switch(accurateRandom(3)){
      case 0:
        while(conditionMet == false){
          if (generateArithmetic(wirePullOrder[!mazeWire],LEDclue)) conditionMet = true;
        }
        Serial.println("Arithmetic Sequence");
        break;
      case 1:
        while(conditionMet == false){
          if (generateGeometric(wirePullOrder[!mazeWire],LEDclue)) conditionMet = true;
        }
        Serial.println("Geometric Sequence");
        break;
      case 2:
        while(conditionMet == false){
          if (generateFibonacci(wirePullOrder[!mazeWire],LEDclue)) conditionMet = true;
        }
        Serial.println("Fibonacci Sequence");
        break;
    }

  // Initialize clock
    startTime = millis();
}

void loop() {
  // put your main code here, to run repeatedly:

  detectWirePull();

  detectButtonPress();

  displayTimer();

  updateLED();

}

/**********************************************************************
** difficultySelection(): Display a difficulty selection interface on 
** the LCD display and wait for a wire to be pulled, indicating the 
** selected difficulty
** Inputs: None
** Outputs: # corresponding to wire pulled
** Side Effects: Difficulty selection interface displayed on LCD
**********************************************************************/
int difficultySelection(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("G:Easy Y:Medium");
  lcd.setCursor(0,1);
  lcd.print("B:Hard R:Insane");

  while(1){
    for (int i=0 ; i<4 ; i++){
    playerWirePresent[i] = wirePresent(playerWire[i]);

    if ((playerWirePresent[i] == false) && (playerWirePresent[i] != lastWirePresent[i])){
      return i;
    }

    lastWirePresent[i] = playerWirePresent[i];
    }
  }
  
}
/**********************************************************************
** shuffleOrder(): Randomizes the order of elements in the array that
** indicates the correct order to pull the wires
** Inputs: None
** Outputs: None
** Side Effects: wirePullOrder[] array is shuffled
**********************************************************************/
void shuffleOrder (){
  for (int i=0 ; i<4 ; i++){
    int j = accurateRandom(4);
    int tempVal = wirePullOrder[i];
    wirePullOrder[i] = wirePullOrder[j];
    wirePullOrder[j] = tempVal;
  }
}

/************************************************************************
** wirePresent(): Polls the input pins for the jumper wires to determine
** if the wire is present or not.
** Inputs: Wire input pin
** Outputs: Wire status
** Side Effects: None
************************************************************************/
bool wirePresent(int pin){
  int pinState1 = digitalRead(pin);
  delay(5);
  int pinState2 = digitalRead(pin);
  return (pinState1 == LOW && pinState2 == LOW);
}
/************************************************************************
** wirePulled(): Checks if the pulled wire was correct, updates the
** current wire to be pulled and decrements remaining wire cuts. Ends the
** game if the pulled wire is incorrect.
** Inputs: Pulled wire
** Outputs: None
** Side Effects: Increments currentWire, decrements nWireCuts, calls
** boom() on an incorrect pull
************************************************************************/
void wirePulled(int wire){
  Serial.print(wireNames[wire]);
  Serial.print(" wire");
  Serial.println(" was pulled");
  if ((wire == wirePullOrder[currentWire]) && (nWireCuts > 0)){
    currentWire++;
    nWireCuts--;
  } else {
    boom();
  }
}
/************************************************************************
** boom(): Prints "BOOM!" on LCD and calls the game over jingle 
** function. End of program.
** Inputs: None
** Outputs: None
** Side Effects: Prints "BOOM!" on LCD and calls gameOverJingle()
************************************************************************/
void boom(){
  Serial.print("BOOM!");
  lcd.setCursor(0,0);
  lcd.clear();
  lcd.print("BOOM!");
  gameOverJingle();
  while(1);
}
/************************************************************************
** defuse(): Prints victory message on LCD and calls the victory jingle
** function. End of program.
** Inputs: None
** Outputs: None
** Side Effects: Prints "Bomb Defused!" on LCD and calls 
** levelCompleteJingle()
************************************************************************/
void defuse(){
  Serial.print("Bomb Defused!");
  lcd.setCursor(0,0);
  lcd.clear();
  lcd.print("Bomb Defused!");
  levelCompleteJingle();
  while(1);
}
/************************************************************************
** detectButtonPress(): Monitors for inputs from the touch sensor and checks
** for a successful defusal when a button press is detected.
** Inputs: None
** Outputs: None
** Side Effects: On a button input, calls defuse() if victory conditions
** are met and boom() if they are not
************************************************************************/
void detectButtonPress(){
  ButtonPress = digitalRead(8);

  if((ButtonPress == true) && (previousButtonPress == false)){
    if(nWireCuts == 0) defuse();
      else boom();
    }

  previousButtonPress = ButtonPress;
}
/************************************************************************
** detectWirePull(): Monitors the state of each wire and checks if any
** wires have been pulled. Calls wirePulled() if so.
** Inputs: None
** Outputs: None
** Side Effects: Calls wirePulled() if a wire is pulled
************************************************************************/
void detectWirePull(){
  for (int i=0 ; i<4 ; i++){
    playerWirePresent[i] = wirePresent(playerWire[i]);

    if ((playerWirePresent[i] == false) && (playerWirePresent[i] != lastWirePresent[i])){
      wirePulled(i);
    }

    lastWirePresent[i] = playerWirePresent[i];
  }
}
/************************************************************************
** displayTimer(): Updates and displays the game timer, and calls boom()
** to end the game if the timer runs out.
** Inputs: None
** Outputs: None
** Side Effects: Updates game timer on LCD, calls boom() if the timer
** runs out
************************************************************************/
void displayTimer(){
  unsigned long currentTime = millis() - startTime;
  long timeRemaining = gameDuration - currentTime;
  if (timeRemaining < 0) boom();

  lcd.setCursor(12,1);

  int minutes = timeRemaining / 60000;
  int seconds = (timeRemaining % 60000) / 1000;
  int secondsTens = (seconds / 10);
  int secondsOnes = (seconds % 10);

  if (seconds != pSeconds) tone(9,784,250);
  pSeconds = seconds;


  lcd.print(minutes);
  lcd.print(":");
  lcd.print(secondsTens);
  lcd.print(secondsOnes);
}
/************************************************************************
** levelCompleteJingle(): Plays an upbeat jingle on the buzzer to
** indicate a sucessful defusal.
** Inputs: None
** Outputs: None
** Side Effects: Plays jingle on buzzer
************************************************************************/
void levelCompleteJingle(){
  tone(9, 196, 120); delay(125); 
  tone(9, 262, 120); delay(125);
  tone(9, 330, 120); delay(125);
  tone(9, 392, 120); delay(125);
  tone(9, 523, 120); delay(125);
  tone(9, 659, 120); delay(125);
  tone(9, 784, (125*3-5)); delay(125*3);
  tone(9, 659, (125*3-5)); delay(125*3);

  tone(9, 208, 120); delay(125); 
  tone(9, 262, 120); delay(125);
  tone(9, 311, 120); delay(125);
  tone(9, 415, 120); delay(125);
  tone(9, 523, 120); delay(125);
  tone(9, 622, 120); delay(125);
  tone(9, 831, (125*3-5)); delay(125*3);
  tone(9, 622, (125*3-5)); delay(125*3);

  tone(9, 233, 120); delay(125); 
  tone(9, 294, 120); delay(125);
  tone(9, 349, 120); delay(125);
  tone(9, 466, 120); delay(125);
  tone(9, 587, 120); delay(125);
  tone(9, 698, 120); delay(125);
  tone(9, 932, (125*3-5)); delay(125*3);
  tone(9, 932, 120); delay(125);
  tone(9, 932, 120); delay(125);
  tone(9, 932, 120); delay(125);
  tone(9, 1046, (125*7-5)); delay(125*7);
}
/************************************************************************
** gameOverJingle(): Plays a jingle on the buzzer to indicate game over
** and a failed defusal.
** Inputs: None
** Outputs: None
** Side Effects: Plays jingle on buzzer
************************************************************************/
void gameOverJingle(){
  tone(9, 392, 180); delay(200);
  tone(9, 698, 150*3-20); delay(150*3);
  tone(9, 698, 130); delay(150);
  tone(9, 698, 180); delay(200);
  tone(9, 659, 180); delay(200);
  tone(9, 587, 180); delay(200);
  tone(9, 523, 150*2-20); delay(150*2);
  tone(9, 392, 150*2-20); delay(150*2);
  tone(9, 262, 150*2-20); delay(150*2);
}
/************************************************************************
** updateLED(): Check the time to see if the LED needs to be updated,
** and call blinkLED() if so.
** Inputs: None
** Outputs: None
** Side Effects: Calls blinkLED() at the necessary times
************************************************************************/
void updateLED(){
  unsigned long currentMillis = millis();
  if (blinkCount < nBlinks){
    if (currentMillis - lastBlinkTime >= blinkDelay){
      if (ledState == false){
        blinkLED(LEDcolor);
        ledState = true;
        lastBlinkTime = millis();
      }else{
        blinkLED(LEDcolor);
        ledState = false;
        lastBlinkTime = millis();
        blinkCount++;
      }
    } 
  }
  else if(currentMillis - lastBlinkTime >= longDelay){
    blinkCount = 0;
    lastBlinkTime = millis();
  }
}
/************************************************************************
** blinkLED(): Reverse the state of the RGB LED, turning it off it it
** was on, and on if it was off.
** Inputs: LED color
** Outputs: None
** Side Effects: Reverses state of LED
************************************************************************/
void blinkLED(int color){
  switch(color){
    case 0:
      if(!ledState) digitalWrite(11,HIGH);
      else digitalWrite(11,LOW);
      break;
    case 1:
      if(!ledState) digitalWrite(11,HIGH);
      else digitalWrite(11,LOW);
      if(!ledState) digitalWrite(10,HIGH);
      else digitalWrite(10,LOW);
      break;
    case 2:
      if(!ledState) digitalWrite(12,HIGH);
      else digitalWrite(12,LOW);
      break;
    case 3:
      if(!ledState) digitalWrite(10,HIGH);
      else digitalWrite(10,LOW);
      break;
  }
}
/************************************************************************
** generateGeometric(): Generate a random Geometric sequence with a
** random starting term and common ratio, and check if it fits
** the decoding requirements.
** Inputs: Wire solution, LED clue status
** Outputs: 1 on success, 0 on failure
** Side Effects: Prints sequence on LCD
************************************************************************/
int generateGeometric(int cutWire, int caseLED){

  lcd.setCursor(0,0);
  
  int term = (accurateRandom(7)+1)*(accurateRandom(2) ? 1 : -1);
  int comRat = (accurateRandom(5)+2)*(accurateRandom(2) ? 1 : -1);

  for(int i=1 ; i<4 ; i++){
    lcd.print(term); lcd.print(",");
    term = term*comRat;
  }
  lcd.print("_");
  int answer = term;
  int answerOnes = answer % 10;

  switch (cutWire){
    case 0: // Even and divisible by 4
      if (!(answer % 4)) break;
      else return 0;
    case 1: // Even and NOT divisible by 4
      if ((answer % 4) && !(answer % 2)) break;
      else return 0;
    case 2: // Odd and divisible by 3
      if((answer % 2) && !(answer % 3)) break;
      else return 0;
    case 3: // Odd and NOT divisible by 3
      if((answer % 2) && (answer % 3)) break;
      else return 0;
    default: return 0;
  }

  switch (caseLED){
    case 0:
      if (answerOnes < 5) {
        Serial.println(answer); 
        return 1;
      }else return 0;
    case 1:
      if (answerOnes > 5) {
        Serial.println(answer); 
        return 1;
      }else return 0;
    default: return 0;
  }
}
/************************************************************************
** generateArithmetic(): Generate a random Arithmetic sequence with a
** random starting term and common differential, and check if it fits
** the decoding requirements.
** Inputs: Wire solution, LED clue status
** Outputs: 1 on success, 0 on failure
** Side Effects: Prints sequence on LCD
************************************************************************/
int generateArithmetic(int cutWire, int caseLED){

  lcd.setCursor(0,0);

  int term1 = (accurateRandom(9)+1);
  int comDiff = (accurateRandom(9)+1);
  int alternating = accurateRandom(2);
  int term;
  int answer;
  int a=1;
  int b=-1;

  if(accurateRandom(2)){ term1 = -term1; comDiff=-comDiff; }
  if(accurateRandom(2)){ a=-1; b=1; }

  for(int i=0 ; i<4 ; i++){
    if(alternating) term = (term1+comDiff*i)*(i % 2 == 0 ? a : b);
    else term = term1+comDiff*i;
    lcd.print(term); lcd.print(",");
    if(i==3){
      lcd.print("_");
      if(alternating) answer = (term1+comDiff*(i+1))*((i+1) % 2 == 0 ? a : -b);
      else answer = term1+comDiff*(i+1);
    }
  }
  
  int answerOnes = answer % 10;


  switch (cutWire){
    case 0: // Even and divisible by 4
      if (!(answer % 4)) break;
      else return 0;
    case 1: // Even and NOT divisible by 4
      if ((answer % 4) && !(answer % 2)) break;
      else return 0;
    case 2: // Odd and divisible by 3
      if((answer % 2) && !(answer % 3)) break;
      else return 0;
    case 3: // Odd and NOT divisible by 3
      if((answer % 2) && (answer % 3)) break;
      else return 0;
    default: return 0;
  }

  switch (caseLED){
    case 0:
      if (answerOnes < 5) {
        Serial.println(answer); 
        return 1;
      }else return 0;
    case 1:
      if (answerOnes > 5) {
        Serial.println(answer); 
        return 1;
      }else return 0;
    default: return 0;
  }
}
/************************************************************************
** generateFibonacci(): Generate a random Fibonacci sequence with 2
** random starting terms and check if it fits the decoding requirements.
** Inputs: Wire solution, LED clue status
** Outputs: 1 on success, 0 on failure
** Side Effects: Prints sequence on LCD
************************************************************************/
int generateFibonacci(int cutWire, int caseLED){

  lcd.setCursor(0,0);

  int term1 = accurateRandom(9)+1;
  int term2 = accurateRandom(9)+1;
  int term = term1+term2;
  int pTerm;
  int temp;

  if(term1<term2){
    lcd.print(term1);
    lcd.print(",");
    lcd.print(term2);
    lcd.print(",");
    pTerm = term2;
  } else{
    lcd.print(term2);
    lcd.print(",");
    lcd.print(term1);
    lcd.print(",");
    pTerm = term1;
  }

  for(int i=0 ; i<3 ; i++){
    lcd.print(term);
    lcd.print(",");
    temp = term;
    term = term+pTerm;
    pTerm = temp;
  }

  lcd.print("_");

  int answer = term;
  int answerOnes = answer % 10;


  // Test if sequences satisfies the manual clues

  switch (cutWire){
    case 0: // Even and divisible by 4
      if (!(answer % 4)) break;
      else return 0;
    case 1: // Even and NOT divisible by 4
      if ((answer % 4) && !(answer % 2)) break;
      else return 0;
    case 2: // Odd and divisible by 3
      if((answer % 2) && !(answer % 3)) break;
      else return 0;
    case 3: // Odd and NOT divisible by 3
      if((answer % 2) && (answer % 3)) break;
      else return 0;
    default: return 0;
  }

  switch (caseLED){
    case 0:
      if (answerOnes < 5) {
        Serial.println(answer); 
        return 1;
      } else return 0;
    case 1:
      if (answerOnes > 5) {
        Serial.println(answer); 
        return 1;
      } else return 0;
    default: return 0;
  }
}
/**********************************************************************
** generateMaze(): Displays the start and end points of the maze. Start
** point is indicated by an X, end point is indicated by an O.
** Inputs: Type of maze, Direction case of maze
** Outputs: None
** Side Effects: Maze points are displayed on OLED display
**********************************************************************/
void generateMaze(int maze, int a){

  int xCoord = 44;
  int yCoord = 8;

  display.drawLine(40, 4, 40, 52, WHITE);
  display.drawLine(40, 4, 88, 4, WHITE);
  display.drawLine(88, 4, 88, 52, WHITE);
  display.drawLine(40, 52, 88, 52, WHITE);

  for(int i=0 ; i<6 ; i++){
    for(int j=0 ; j<6 ; j++){
      display.drawPixel(xCoord + 8*i,yCoord + 8*j,WHITE);
    }
  }

  display.display();  

  int x1;
  int y1;
  int x2;
  int y2;
  int startPoint=1;

  switch(maze){
    case 1:
      x1 = 44+8*0;
      y1 = 8+8*1;
      x2 = 44+8*5;
      y2 = 8+8*2;
      if(a==2) startPoint=2;
      break;
    case 2:
      x1 = 44+8*1;
      y1 = 8+8*3;
      x2 = 44+8*4;
      y2 = 8+8*1;
      if(a==2) startPoint=2;
      break;
    case 3:
      x1 = 44+8*3;
      y1 = 8+8*3;
      x2 = 44+8*5;
      y2 = 8+8*3;
      if(a==2) startPoint=2;
      break;
    case 4:
      x1 = 44+8*0;
      y1 = 8+8*0;
      x2 = 44+8*0;
      y2 = 8+8*3;
      if(a==1) startPoint=2;
      break;
    case 5:
      x1 = 44+8*4;
      y1 = 8+8*2;
      x2 = 44+8*3;
      y2 = 8+8*5;
      if(a==1) startPoint=2;
      break;
    case 6:
      x1 = 44+8*4;
      y1 = 8+8*0;
      x2 = 44+8*2;
      y2 = 8+8*4;
      if(a==1) startPoint=2;
      break;
    case 7:
      x1 = 44+8*1;
      y1 = 8+8*0;
      x2 = 44+8*1;
      y2 = 8+8*5;
      if(a==1) startPoint=2;
      break;
    case 8:
      x1 = 44+8*3;
      y1 = 8+8*0;
      x2 = 44+8*2;
      y2 = 8+8*3;
      if(a==1) startPoint=2;
      break;
    case 9:
      x1 = 44+8*2;
      y1 = 8+8*1;
      x2 = 44+8*0;
      y2 = 8+8*4;
      if(a==1) startPoint=2;
      break;
  }
  
  int color1;
  int color2;

  if(startPoint == 1){
    display.drawLine(x1+3,y1+3,x1-3,y1-3,WHITE);
    display.drawLine(x1-3,y1+3,x1+3,y1-3,WHITE);
    display.drawCircle(x2,y2,4,WHITE);
  } else {
    display.drawLine(x2+3,y2+3,x2-3,y2-3,WHITE);
    display.drawLine(x2-3,y2+3,x2+3,y2-3,WHITE);
    display.drawCircle(x1,y1,4,WHITE);
  }
}
/************************************************************************
** generateSerialNumber(): Randomly generate a serial number in the
** format A-0000 and display it on the LCD. 
** Inputs: None
** Outputs: None
** Side Effects: Randomizes and displays serialNumber[] on LCD, prints
** serialNumber[] in serial monitor for debugging
************************************************************************/
void generateSerialNumber(){
  serialNumber[0] = 65 + accurateRandom(25);
  serialNumber[1] = '-';
  serialNumber[2] = 48 + accurateRandom(10);
  serialNumber[3] = 48 + accurateRandom(10);
  serialNumber[4] = 48 + accurateRandom(10);
  serialNumber[5] = 48 + accurateRandom(10);
  
  Serial.print(serialNumber);
  Serial.print("\n");
  lcd.setCursor(0,1);
  for (int i=0 ; i<6 ; i++) lcd.print(serialNumber[i]);
}
/************************************************************************
** accurateRandom(): Replacement for arduino random() function, provides
** values that are closer to being truly random.
** Inputs: Number of possible outputs (n)
** Outputs: Randomly selected value between 0 and n
** Side Effects: None
************************************************************************/
int accurateRandom(int n){
  for (int i = n ; i>0 ; i--){
    if (random(10000) % i == 0) return i-1;
  }
}