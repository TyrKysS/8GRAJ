#include "StateMachineLib.h"


#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>

RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);
int msg;

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

StateMachine stateMachine(4, 5);
Input input;

int incomingChar = 3;


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
  stateMachine.AddTransition(Color, Type, []() {
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
  Serial.print("incomingChar ");
  Serial.println(incomingChar);
  stateMachine.SetState(Type, false, true);
  //incomingChar = 0;




  while (!Serial) {
    // some boards need this because of native USB capability
  }
  // Set the nodeID to 0 for the master node
  mesh.setNodeID(0);
  Serial.print("Node ID - ");
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
  } else Serial.println("ready");
}


void loop() {
  mesh.update();
  mesh.DHCP();

  input = static_cast<Input>(readInput());
  stateMachine.Update();
}

int readInput() {
  if (stateMachine.GetState() == Type) {
    if (network.available()) {
      RF24NetworkHeader header;
      network.peek(header);
      network.read(header, &msg, sizeof(msg));
      if (header.type == 'I') incomingChar = 0;
      else incomingChar = 1;
      Serial.println();
    }
  }
  Input currentInput = Input::Unknown;
  switch (incomingChar) {
    case 0: currentInput = Input::Incident; break;
    case 1: currentInput = Input::Request; break;
    case 2: currentInput = Input::Forward; break;
    default: break;
  }
  return currentInput;
}

void outputType() {
  Serial.print("(Type) ->> ");
}
void outputBuzzer() {
  incomingChar = 2;
  Serial.print("(Buzzer) ->> ");
}
void outputIntensity() {
  incomingChar = 2;
  Serial.print("(Intensity) ->> ");
}
void outputColor() {
  Serial.print("(Color) ->> ");
}