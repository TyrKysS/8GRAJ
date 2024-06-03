#include "StateMachineLib.h"

#define pinR 2
#define pinG 3
#define pinB 4

#define BUTTON_PIN 5

int potPin = A0;
int potProm = 0;
int ledProm = 0;


enum Sate {
  Off = 0,
  White = 1,
  Yellow = 2
};
enum Input {
  Forward = 0,
  Unknown = 1,
};

StateMachine stateMachine(3, 3);
Input input;

Input currentInput;

void setupStateMachine() {
  stateMachine.AddTransition(Off, White, []() {
    return input == Forward;
  });
  stateMachine.AddTransition(White, Yellow, []() {
    return input == Forward;
  });
  stateMachine.AddTransition(Yellow, Off, []() {
    return input == Forward;
  });

  stateMachine.SetOnEntering(Off, outputOff);
  stateMachine.SetOnEntering(White, outputWhite);
  stateMachine.SetOnEntering(Yellow, outputYellow);

  stateMachine.SetOnLeaving(Off, []() {
    Serial.print("(Off) -> ");
  });
  stateMachine.SetOnLeaving(White, []() {
    Serial.print("(White) -> ");
  });
  stateMachine.SetOnLeaving(Yellow, []() {
    Serial.print("(Yellow) -> ");
  });
}

void setup() {
  Serial.begin(9600);

  Serial.println("Starting State Machine...");
  setupStateMachine();
  Serial.println("Start Machine Started");

  stateMachine.SetState(Off, false, true);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  currentInput = Input::Unknown;
}

void loop() {
  
  
  if (digitalRead(BUTTON_PIN) == 0) {
    stateMachine.Update();
    currentInput = Input::Forward;
    delay(500);
  }
  potProm = analogRead(potPin);
  //Serial.println(potProm);
  ledProm = map(potProm, 0, 1023, 0, 255);
}

void outputOff() {
  Serial.print("(Off) ->> ");
  setRGB(0, 0, 0);
}

void outputWhite() {
  Serial.print("(White) ->> ");
  setRGB(potProm, potProm, potProm);
}

void outputYellow() {
  Serial.print("(Yellow) ->> ");
  setRGB(potProm, potProm, 0);
}
void setRGB(int red, int green, int blue) {
  // nastavení všech barev na zvolené intenzity
  analogWrite(pinR, red);
  analogWrite(pinG, green);
  analogWrite(pinB, blue);
}