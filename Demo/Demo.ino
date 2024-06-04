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
  Off = 0,
  White = 1,
  Yellow = 2,
  Type = 3,
  Buzzer = 4,
  isLight = 5,
  Color = 6
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

StateMachine rfStateMachine(7, 6);
Input rfInput;

int incomingChar = 1;


void setuprfStateMachine() {
  rfStateMachine.AddTransition(Type, Buzzer, []() {
    return rfInput == Incident;
  });
  rfStateMachine.AddTransition(Type, isLight, []() {
    return rfInput == Request;
  });
  rfStateMachine.AddTransition(Buzzer, Color, []() {
    return rfInput == Forward;
  });
  rfStateMachine.AddTransition(isLight, Color, []() {
    return rfInput == Forward;
  });
  rfStateMachine.AddTransition(Color, Type, []() {
    return rfInput == Forward;
  });
  rfStateMachine.AddTransition(isLight, Type, []() {
    return rfInput == Backward;
  });


  rfStateMachine.SetOnEntering(Type, outputType);
  rfStateMachine.SetOnEntering(Buzzer, outputBuzzer);
  rfStateMachine.SetOnEntering(isLight, outputisLight);
  rfStateMachine.SetOnEntering(Color, outputColor);

  rfStateMachine.SetOnLeaving(Type, []() {
    Serial.print("(Type) -> ");
  });
  rfStateMachine.SetOnLeaving(Buzzer, []() {
    Serial.print("(Buzzer) -> ");
  });
  rfStateMachine.SetOnLeaving(isLight, []() {
    Serial.print("(isLight) -> ");
  });
  rfStateMachine.SetOnLeaving(Color, []() {
    Serial.print("(Color) ->");
  });
}




#define BUTTON_PIN 5

int potPin = A0;
int potProm = 0;
int ledProm = 0;

StateMachine btnStateMachine(3, 3);
Input btnInput;

Input currentInput;

void setupBtnStateMachine() {
  btnStateMachine.AddTransition(Off, White, []() {
    return btnInput == Forward;
  });
  btnStateMachine.AddTransition(White, Yellow, []() {
    return btnInput == Forward;
  });
  btnStateMachine.AddTransition(Yellow, Off, []() {
    return btnInput == Forward;
  });

  btnStateMachine.SetOnEntering(Off, outputOff);
  btnStateMachine.SetOnEntering(White, outputWhite);
  btnStateMachine.SetOnEntering(Yellow, outputYellow);

  btnStateMachine.SetOnLeaving(Off, []() {
    Serial.print("(Off) -> ");
  });
  btnStateMachine.SetOnLeaving(White, []() {
    Serial.print("(White) -> ");
  });
  btnStateMachine.SetOnLeaving(Yellow, []() {
    Serial.print("(Yellow) -> ");
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
  setuprfStateMachine();
  Serial.println("Start Machine Started");
  Serial.print("incomingChar ");
  Serial.println(incomingChar);
  rfStateMachine.SetState(Type, false, true);
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

  Serial.println("Starting State Machine...");
  setupBtnStateMachine();
  Serial.println("Start Machine Started");

  btnStateMachine.SetState(Off, false, true);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(potPin, INPUT);

  currentInput = Input::Unknown;
}


void loop() {
  if (digitalRead(BUTTON_PIN) == 0) {
    Serial.println("Button pressed ... ");
    btnStateMachine.Update();
    currentInput = Input::Forward;
    delay(500);
  } else {
    mesh.update();
    mesh.DHCP();
    rfInput = static_cast<Input>(readrfInput());
    rfStateMachine.Update();
  }
  potProm = analogRead(potPin);
  //Serial.println(potProm);
  ledProm = map(potProm, 0, 1023, 0, 255);
}

int readrfInput() {

  if (rfStateMachine.GetState() == Type) {
    if (network.available()) {
      RF24NetworkHeader header;
      network.peek(header);
      network.read(header, &msg, sizeof(msg));
      if (header.type == 'I') incomingChar = 2;
      else incomingChar = 1;
      Serial.println();
    }
  }
  Input currentrfInput = Input::Unknown;
  switch (incomingChar) {
    case 0: currentrfInput = Input::Forward; break;
    case 1: currentrfInput = Input::Request; break;
    case 2: currentrfInput = Input::Incident; break;
    case 3: currentrfInput = Input::Backward; break;
    default: break;
  }
  return currentrfInput;
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
  analogWrite(pinR, red);
  analogWrite(pinG, green);
  analogWrite(pinB, blue);
}