#include "StateMachineLib.h"

struct myStruct {
  unsigned long name;
  unsigned long intesity;
  unsigned long color;
};

enum State {
  Type = 0,
  Buzzer = 1,
  Intensity = 2,
  Color = 3
};

enum Input {
  Incident = 0,
  Request = 1,
  Forward = 2,
  Unknown = 3,
};

StateMachine stateMachine(4, 4);
Input input;

myStruct mystruct = { 1, 120, 540 };
Input currentInput;
int incomingChar;


void setupStateMachine() {
  stateMachine.AddTransition(Type, Buzzer, []() {
    return input == Incident;
  });
  stateMachine.AddTransition(Type, Intensity, []() {
    return input == Request;
  });
  stateMachine.AddTransition(Buzzer, Color, []() {
    return input == Forward;
  });
  stateMachine.AddTransition(Intensity, Color, []() {
    return input == Forward;
  });

  stateMachine.SetOnEntering(Type, outputType);
  stateMachine.SetOnEntering(Buzzer, outputBuzzer);
  stateMachine.SetOnEntering(Intensity, outputIntensity);
  stateMachine.SetOnEntering(Color, outputColor);

  stateMachine.SetOnLeaving(Type, []() {
    Serial.print("(Type) -> ");
  });
  stateMachine.SetOnLeaving(Buzzer, []() {
    Serial.print("(Buzzer) -> ");
  });
  stateMachine.SetOnLeaving(Intensity, []() {
    Serial.print("(Intensity) -> ");
  });
  stateMachine.SetOnLeaving(Color, []() {
    Serial.print("(Color) ->");
  });
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting State Machine ...");
  setupStateMachine();
  Serial.println("Start Machine Started");
  Serial.print("Input state: ");
  stateMachine.SetState(Type, false, true);
  currentInput = Input::Unknown;
  incomingChar = mystruct.name;
  Serial.println();
}

void loop() {
  input = static_cast<Input>(readInput());
  stateMachine.Update();
}

int readInput() {
  switch (incomingChar) {
    case 0: currentInput = Input::Incident; break;
    case 1: currentInput = Input::Request; break;
    case 2: currentInput = Input::Forward; break;
    default: break;
  }
  return currentInput;
}

void outputType() {
  Serial.print("(Type) -> ");
}
void outputBuzzer() {
  incomingChar = 2;
  Serial.print("(Buzzer) -> ");
}
void outputIntensity() {
  incomingChar = 2;
  Serial.print("(Intensity) -> ");
}
void outputColor() {
  Serial.print("(Color) -> ");
}