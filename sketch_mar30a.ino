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

unsigned long int getCurrentTime(){ //timer using timer 1
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

void setup() {
  DDRD |= B11111111;  // Configure PORTD (pins 0-7) as output for LED array
  DDRB |= B00101110;  // RGB and buzzer output
  DDRB &= B11101111;  // Button input
  PORTB |= B00010000; // Button pull-up
  winConditionLED = randomLED(); // Set initial win condition
}

void loop() {
  if (gameState == IDLE && checkButtonPress()) { //First button press to start game
    winConditionLED = randomLED(); // Generate new win condition each game
    gameState = BLINKING; //Changes gamestate to show the win condition
  }

  if (gameState == BLINKING) {
    startGame(); //shows the win condition
  } else if (gameState == PLAYING) { //game is started and the LED is shifting
    if (!isFrozen) { //checks if player has made a move
      playGame(); 
    } else {
      checkWinCondition(); //checks if player has won
    }
  } else { //player is idle and hasnt started game
    int potValue = analogRead(potPin); //analog input from potentiometer
    shiftDelay = map(potValue, 0, 1023, 50, 500); //takes the information from ADC and maps it into speed of the LED shifting, increasing the difficulty of the game
    adjustColorBasedOnSpeed(); //color goes from none to white for difficulty indicator
    gameIdle(); //blinks the led before starting the game
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
    if (currentTime - lastBlinkTime >= 200) { //blinks for 200ms
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

  if (!firstLEDDisplayed) { //to make sure initial LED is turned on
    PORTD = currentLED;
    if (currentTime - lastShiftTime >= shiftDelay) {
      firstLEDDisplayed = true;
      lastShiftTime = currentTime;
    }
    return;
  }
  PORTD = currentLED;

  if (checkButtonPress()) { //freeze the LED when the player pressed the button for the certain LED PORTD position
    isFrozen = true;
    firstLEDDisplayed = false;
    return;
  }

  if (currentTime - lastShiftTime >= shiftDelay) {
    lastShiftTime = currentTime;

    if (direction) { //shifts forward
      currentLED <<= 1;
      if (currentLED == 0) {
        currentLED = B01000000;
        direction = false;
      }
    } else { //shifts backwards
      currentLED >>= 1;
      if (currentLED == 0) {
        currentLED = B00000010;
        direction = true;
      }
    }
  }
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

  //analogWrites the color green for winning
  analogWrite(redPin, 255);
  analogWrite(greenPin, 0);
  analogWrite(bluePin, 255);

  if (stage == 0) { //plays the first tone
    tone(buzzerPin, 1000);
    prevTime = currTime;
    stage = 1;
  } 
  else if (stage == 1 && currTime - prevTime >= 500) {
    noTone(buzzerPin);
    tone(buzzerPin, 1200); //plays the second tone after the first after 500ms
    prevTime = currTime;
    stage = 2;
  } 
  else if (stage == 2 && currTime - prevTime >= 500) {
    noTone(buzzerPin); //restarts the game after the buzzer has finished he tone for 500ms
    gameState = IDLE;
    isFrozen = false;
    stage = 0;
  }
}

void playLoseTone() {
  static unsigned long prevTime = 0;
  static int stage = 0;  
  unsigned long currTime = getCurrentTime();

  //writes the color red
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
  bool currentButtonState = (PINB & B00010000) ? HIGH : LOW; //the current button state shall be taken from PINB

  if (lastButtonState == HIGH && currentButtonState == LOW) { //returns true only if is on falling edge
    lastButtonState = currentButtonState;
    return true;
  }
  lastButtonState = currentButtonState;
  return false;
}

void adjustColorBasedOnSpeed() {
  int normalizedSpeed = map(shiftDelay, 50, 500, 0, 255);

  int rgb = map(normalizedSpeed, 0, 255, 0, 255);   

  analogWrite(redPin, rgb);
  analogWrite(greenPin, rgb);
  analogWrite(bluePin, rgb);
}

