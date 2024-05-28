#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>

RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);
int msg;

void setup() {
  Serial.begin(9600);
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
  if (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);
    network.read(header, &msg, sizeof(msg));
    Serial.print(header.type);
    Serial.print(" ");
    Serial.println(msg);
  }
}
