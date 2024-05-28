#define pinR 3
#define pinG 4
#define pinB 5


#define BUTTON_PIN 2
int counter = 0;

int potPin = A0;
int potProm = 0;
int ledProm = 0;

enum States {
  OFF,
  WHITE,
  YELLOW
};

States state = States::OFF;

void setup() {
  Serial.begin(9600);
  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setRGB(255, 0, 0);
  delay(100);
  setRGB(0, 255, 0);
  delay(100);
  setRGB(0, 0, 255);
  delay(100);
  setRGB(255, 255, 255);
}

void loop() {
  if(digitalRead(BUTTON_PIN) == 0) {
    nextState();
    delay(500);
  }
  Serial.println(counter);
  potProm = analogRead(potPin);
  Serial.println(potProm);
  ledProm = map(potProm, 0, 1023, 0, 255);



  switch(state) {
    case States::OFF:
        setRGB(0, 0, 0);
      break;
    case States::WHITE:
        setRGB(potProm, potProm, potProm);
      break;
    case States::YELLOW:
      setRGB(potProm, potProm, 0);
      break;
  }
}

void nextState() {
  if(state == States::OFF) state = States::WHITE;
  else if(state == States::WHITE) state = States::YELLOW;
  else state = States::OFF;
}


void setRGB(int red, int green, int blue) {
  // nastavení všech barev na zvolené intenzity
  analogWrite(pinR, red);
  analogWrite(pinG, green);
  analogWrite(pinB, blue);
}