// Pin definitions
const int redPin = 9;
const int greenPin = 10;
const int bluePin = 11;
const int potPin = A0;
const int buttonPin = 12;
const int buzzerPin = 13;

// LED shifting and state variables
int shiftDelay = 100;
bool isFrozen = false;
bool direction = true;
uint8_t currentLED = B00000001;
uint8_t winConditionLED;

// Define game states
enum GameState { IDLE, BLINKING, PLAYING };
GameState gameState = IDLE;

void setup() {
  DDRD |= B11111111;  // Configure PORTD (pins 0-7) as output for LED array
  DDRB |= B00101110;  // RGB and buzzer output
  DDRB &= B11101111;  // Button input
  PORTB |= B00010000; // Button pull-up
  winConditionLED = randomLED(); // Set initial win condition
}

void loop() {
  if (gameState == IDLE && checkButtonPress()) {
    winConditionLED = randomLED(); // Generate new win condition each game
    gameState = BLINKING;
    startGame();
  }

  if (gameState == BLINKING) {
    startGame();
  } else if (gameState == PLAYING) {
    if (!isFrozen) {
      playGame();
    } else {
      checkWinCondition();
    }
  } else {
    int potValue = analogRead(potPin);
    shiftDelay = map(potValue, 0, 1023, 50, 550);
    adjustColorBasedOnSpeed();
    gameIdle();
  }
}

void gameIdle() {
  static unsigned long int prevIdleDelay = 0;
  unsigned long int currIdleDelay = getCurrentTime();
  if(currIdleDelay-prevIdleDelay>=shiftDelay){
    PORTD = B10101010;
  }
  if(currIdleDelay-prevIdleDelay>=(2*shiftDelay)){
    PORTD = B01010101;
    prevIdleDelay=currIdleDelay;
  }
}

void startGame() {
  static int blinkIndex = 0;
  static unsigned long lastBlinkTime = 0;
  unsigned long currentTime = getCurrentTime();

  if (blinkIndex < 10) { //blinks 5 times
    if (currentTime - lastBlinkTime >= 200) {
      PORTD = (blinkIndex % 2 == 0) ? winConditionLED : 0;
      lastBlinkTime = currentTime;
      blinkIndex++;
    }
  } else {
    blinkIndex = 0;
    gameState = PLAYING;
    direction = true;
    currentLED = B00000001;
  }
}

uint8_t randomLED() {
  return 1 << random(0, 7);
}

void playGame() {
  static unsigned long lastShiftTime = 0;
  static bool firstLEDDisplayed = false;
  unsigned long currentTime = getCurrentTime();

  noTone(buzzerPin);

  if (!firstLEDDisplayed) {
    PORTD = currentLED;
    if (currentTime - lastShiftTime >= shiftDelay) {
      firstLEDDisplayed = true;
      lastShiftTime = currentTime;
    }
    return;
  }
  PORTD = currentLED;

  if (checkButtonPress()) {
    isFrozen = true;
    firstLEDDisplayed = false;
    return;
  }

  if (currentTime - lastShiftTime >= shiftDelay) {
    lastShiftTime = currentTime;

    if (direction) {
      currentLED <<= 1;
      if (currentLED == 0) {
        currentLED = B01000000;
        direction = false;
      }
    } else {
      currentLED >>= 1;
      if (currentLED == 0) {
        currentLED = B00000010;
        direction = true;
      }
    }
  }
}

unsigned long int getCurrentTime(){
    static bool timerStarted = false;
    static unsigned int multiplier = 0;

    if (!timerStarted) {
        TCCR1A = 0x00;
        TCCR1B = 0x00;
        TCCR1B |= B00000101;
        TCNT1 = 0;
        timerStarted = true;
    }

    if (TIFR1 & (1 << TOV1)) {
        TIFR1 |= (1 << TOV1);
        TCNT1 = 0;
        multiplier++;
    }

    return (TCNT1*(0.064))+(((unsigned long int)pow(2, sizeof(unsigned short int) * 8)*multiplier)*(0.064));
}

void checkWinCondition() {
  if (currentLED == winConditionLED) {
    playWinTone();
  } else {
    playLoseTone();
  }
}

void playWinTone() {
  static unsigned long prevTime = 0;
  static int stage = 0;  
  unsigned long currTime = getCurrentTime();

  analogWrite(redPin, 255);
  analogWrite(greenPin, 0);
  analogWrite(bluePin, 255);

  if (stage == 0) {
    tone(buzzerPin, 1000);
    prevTime = currTime;
    stage = 1;
  } 
  else if (stage == 1 && currTime - prevTime >= 500) {
    noTone(buzzerPin);
    tone(buzzerPin, 1200);
    prevTime = currTime;
    stage = 2;
  } 
  else if (stage == 2 && currTime - prevTime >= 500) {
    noTone(buzzerPin);
    gameState = IDLE;
    isFrozen = false;
    stage = 0;
  }
}

void playLoseTone() {
  static unsigned long prevTime = 0;
  static int stage = 0;  
  unsigned long currTime = getCurrentTime();

  analogWrite(redPin, 0);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 255);

  if (stage == 0) {
    tone(buzzerPin, 400);
    prevTime = currTime;
    stage = 1;
  } 
  else if (stage == 1 && currTime - prevTime >= 500) {
    noTone(buzzerPin);
    tone(buzzerPin, 300);
    prevTime = currTime;
    stage = 2;
  } 
  else if (stage == 2 && currTime - prevTime >= 500) {
    noTone(buzzerPin);
    gameState = IDLE;
    isFrozen = false;
    stage = 0;
  }
}

bool checkButtonPress() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = (PINB & B00010000) ? HIGH : LOW;

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    lastButtonState = currentButtonState;
    return true;
  }
  lastButtonState = currentButtonState;
  return false;
}

void adjustColorBasedOnSpeed() {
  int normalizedSpeed = map(analogRead(potPin), 0, 1023, 0, 255);

  int red = map(normalizedSpeed, 0, 255, 0, 255);   // Increase from 0 to 255
  int green = 255;                                    // Keep green at 0
  int blue = map(normalizedSpeed, 0, 255, 255, 0);  // Decrease from 255 to 0

  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

