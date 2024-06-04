#include "StateMachineLib.h"


#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <Wire.h>
#include <BH1750.h>

RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

BH1750 luxSenzor;

#define pinR 2
#define pinG 3
#define pinB 4

enum State {
  Type = 0,
  Buzzer = 1,
  isLight = 2,
  Color = 3
};

enum Input {
  Forward = 0,
  Unknown = 1,
  Incident = 2,
  Request = 3,
  Backward = 4,
  
};


struct Message {
  unsigned int R;
  unsigned int G;
  unsigned int B;
};
Message msg;

StateMachine stateMachine(4, 6);
Input input;

int incomingChar = 1;


void setupStateMachine() {
  stateMachine.AddTransition(Type, Buzzer, []() {
    return input == Incident;
  });
  stateMachine.AddTransition(Type, isLight, []() {
    return input == Request;
  });
  stateMachine.AddTransition(Buzzer, Color, []() {
    return input == Forward;
  });
  stateMachine.AddTransition(isLight, Color, []() {
    return input == Forward;
  });
  stateMachine.AddTransition(Color, Type, []() {
    return input == Forward;
  });
  stateMachine.AddTransition(isLight, Type, []() {
    return input == Backward;
  });


  stateMachine.SetOnEntering(Type, outputType);
  stateMachine.SetOnEntering(Buzzer, outputBuzzer);
  stateMachine.SetOnEntering(isLight, outputisLight);
  stateMachine.SetOnEntering(Color, outputColor);

  stateMachine.SetOnLeaving(Type, []() {
    Serial.print("(Type) -> ");
  });
  stateMachine.SetOnLeaving(Buzzer, []() {
    Serial.print("(Buzzer) -> ");
  });
  stateMachine.SetOnLeaving(isLight, []() {
    Serial.print("(isLight) -> ");
  });
  stateMachine.SetOnLeaving(Color, []() {
    Serial.print("(Color) ->");
  });
}

void setup() {
  Serial.begin(9600);

  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);

  luxSenzor.begin();

  setRGB(255, 0, 0);
  delay(1000);
  setRGB(0, 255, 0);
  delay(1000);
  setRGB(0, 0, 255);
  delay(1000);
  setRGB(0, 0, 0);

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
      if (header.type == 'I') incomingChar = 2;
      else incomingChar = 1;
      Serial.println();
    }
  }
  Input currentInput = Input::Unknown;
  switch (incomingChar) {
    case 0: currentInput = Input::Forward; break;
    case 1: currentInput = Input::Request; break;
    case 2: currentInput = Input::Incident; break;
    case 3: currentInput = Input::Backward; break;
    default: break;
  }
  return currentInput;
}

void outputType() {
  //Serial.print("(Type) ->> ");
}
void outputBuzzer() {
  //TODO activate buzzer
  incomingChar = 0;
  //Serial.print("(Buzzer) ->> ");
}
void outputisLight() {
  uint16_t lux = luxSenzor.readLightLevel();
  Serial.print("Intenzita světla: ");
  Serial.println(lux);
  if (lux < 100) incomingChar = 0;
  else {
    for (int i = 0; i < 5; i++) {
      setRGB(0, 255, 0);
      delay(100);
      setRGB(0, 0, 0);
      delay(100);
    }
    incomingChar = 3;
  }
  
  //Serial.print("(isLight) ->> ");
}
void outputColor() {
  setRGB(msg.R, msg.G, msg.B);
  //Serial.print("(Color) ->> ");
}



void setRGB(int red, int green, int blue) {
  analogWrite(pinR, red);
  analogWrite(pinG, green);
  analogWrite(pinB, blue);
}