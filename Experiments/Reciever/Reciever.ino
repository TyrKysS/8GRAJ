#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include "StateMachineLib.h"

// struktura zprávy
struct MsgStruct {
  unsigned long intensity;
  unsigned long color;
};

// Stavy
enum State {
  Type = 0,
  Buzzer = 1,
  Intensity = 2,
  Color = 3
};

// Vstupy
enum Input {
  Incident = 0,
  Request = 1,
  Forward = 2,
  Unknown = 3,
};

// Úvodní nastavení konečného automatu (Počet stavů, počet přechodů)
StateMachine stateMachine(4, 5);
Input input;
MsgStruct msgStruct = { 0, 0 };
Input currentInput;
char incomingChar;

// Nastavení pinů NRF24L01 komunikačního modulu a zvolení mesh sítě
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

// nastavení přechodových funkcí
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
    Serial.print("(Color) -> ");
  });
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // some boards need this because of native USB capability
  }

  // Set the nodeID to 0 for the master node
  mesh.setNodeID(0);
  Serial.println(mesh.getNodeID());

  // Set the PA Level to MIN and disable LNA for testing & power supply related issues
  radio.begin();
  radio.setPALevel(RF24_PA_MIN, 0);

  // Connect to the mesh
  if (!mesh.begin()) {
    // if mesh.begin() returns false for a master node, then radio.begin() returned false.
    Serial.println(F("Radio hardware not responding."));
    while (1) {
      // hold in an infinite loop
    }
  }

  Serial.println("Starting State Machine ...");
  setupStateMachine();
  Serial.println("Start Machine Started");

  stateMachine.SetState(Type, false, true);
  currentInput = Input::Unknown;
  incomingChar = 'F';
}

void loop() {
  mesh.update();
  mesh.DHCP();
  input = static_cast<Input>(readInput());
  stateMachine.Update();
}

int readInput() {
  switch (incomingChar) {
    case 'I': currentInput = Input::Incident; break;
    case 'R': currentInput = Input::Request; break;
    case 'F': currentInput = Input::Forward; break;
    default: break;
  }
  return currentInput;
}

void outputType() {
  if (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);
    incomingChar = header.type;
    Serial.println(incomingChar);
  }
  Serial.println("(Type)");
}
void outputBuzzer() {
  Serial.println("(Buzzer)");
  incomingChar = 'F';
}
void outputIntensity() {
  Serial.println("(Intensity)");
  incomingChar = 'F';
}
void outputColor() {
  Serial.println("(Color)");
  incomingChar = 'F';
}