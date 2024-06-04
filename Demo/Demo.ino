#include "StateMachineLib.h"


#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <Wire.h>
#include <BH1750.h>

#define pinR 2
#define pinG 3
#define pinB 4

#define BUTTON_PIN 5

int potPin = A0;
int potProm = 0;
int ledProm = 0;

RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

BH1750 luxSenzor;


enum LocalSate {
  Off = 0,
  White = 1,
  Yellow = 2
};

enum Input {
  Forward = 0,
  Unknown = 1,
  Incident = 2,
  Request = 3,
  Backward = 4,
};


StateMachine localStateMachine(3, 3);
Input localInput;
Input currentLocalInput;

void setuplocalStateMachine() {
  localStateMachine.AddTransition(Off, White, []() {
    return localInput == Forward;
  });
  localStateMachine.AddTransition(White, Yellow, []() {
    return localInput == Forward;
  });
  localStateMachine.AddTransition(Yellow, Off, []() {
    return localInput == Forward;
  });

  localStateMachine.SetOnEntering(Off, outputOff);
  localStateMachine.SetOnEntering(White, outputWhite);
  localStateMachine.SetOnEntering(Yellow, outputYellow);

  localStateMachine.SetOnLeaving(Off, []() {
    Serial.print("(Off) -> ");
  });
  localStateMachine.SetOnLeaving(White, []() {
    Serial.print("(White) -> ");
  });
  localStateMachine.SetOnLeaving(Yellow, []() {
    Serial.print("(Yellow) -> ");
  });
}


enum RFState {
  Type = 0,
  Buzzer = 1,
  isLight = 2,
  Color = 3
};




struct Message {
  unsigned int R;
  unsigned int G;
  unsigned int B;
};
Message msg;

StateMachine RfstateMachine(4, 6);
Input Rfinput;

int incomingChar = 1;

void setupStateMachine() {
  RfstateMachine.AddTransition(Type, Buzzer, []() {
    return Rfinput == Incident;
  });
  RfstateMachine.AddTransition(Type, isLight, []() {
    return Rfinput == Request;
  });
  RfstateMachine.AddTransition(Buzzer, Color, []() {
    return Rfinput == Forward;
  });
  RfstateMachine.AddTransition(isLight, Color, []() {
    return Rfinput == Forward;
  });
  RfstateMachine.AddTransition(Color, Type, []() {
    return Rfinput == Forward;
  });
  RfstateMachine.AddTransition(isLight, Type, []() {
    return Rfinput == Backward;
  });


  RfstateMachine.SetOnEntering(Type, outputType);
  RfstateMachine.SetOnEntering(Buzzer, outputBuzzer);
  RfstateMachine.SetOnEntering(isLight, outputisLight);
  RfstateMachine.SetOnEntering(Color, outputColor);

  RfstateMachine.SetOnLeaving(Type, []() {
    Serial.print("(Type) -> ");
  });
  RfstateMachine.SetOnLeaving(Buzzer, []() {
    Serial.print("(Buzzer) -> ");
  });
  RfstateMachine.SetOnLeaving(isLight, []() {
    Serial.print("(isLight) -> ");
  });
  RfstateMachine.SetOnLeaving(Color, []() {
    Serial.print("(Color) ->");
  });
}

void setup() {
  Serial.begin(9600);
  luxSenzor.begin();

  Serial.println("Starting setuplocalStateMachine ...");
  setuplocalStateMachine();
  Serial.println("Start setuplocalStateMachine");

  localStateMachine.SetState(Off, false, true);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  currentLocalInput = Input::Unknown;


  Serial.println("Starting setupStateMachine ...");
  setupStateMachine();
  Serial.println("Start setupStateMachine");
  Serial.print("incomingChar ");
  Serial.println(incomingChar);
  RfstateMachine.SetState(Type, false, true);

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
  if (digitalRead(BUTTON_PIN) == 0) {
    localStateMachine.Update();
    currentLocalInput = Input::Forward;
    delay(500);
    Serial.println("BTN pressed");
  }
  potProm = analogRead(potPin);
  //Serial.println(potProm);
  ledProm = map(potProm, 0, 1023, 0, 255);



  mesh.update();
  mesh.DHCP();

  Rfinput = static_cast<Input>(readInput());
  RfstateMachine.Update();
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



int readInput() {

  if (RfstateMachine.GetState() == Type) {
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
  if (lux < 100) incomingChar = 2;
  else {
    for (int i = 0; i < 5; i++) {
      setRGB(0, 255, 0);
      delay(100);
      setRGB(0, 0, 0);
      delay(100);
    }
    incomingChar = 0;
  }

  //Serial.print("(isLight) ->> ");
}
void outputColor() {
  setRGB(msg.R, msg.G, msg.B);
  //Serial.print("(Color) ->> ");
}


void setRGB(int red, int green, int blue) {
  // nastavení všech barev na zvolené intenzity
  analogWrite(pinR, red);
  analogWrite(pinG, green);
  analogWrite(pinB, blue);
}
