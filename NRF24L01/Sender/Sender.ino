#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>

/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

#define nodeID 1
int msg = 1;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // some boards need this because of native USB capability
  }

  // Set the nodeID manually
  mesh.setNodeID(nodeID);

  // Set the PA Level to MIN and disable LNA for testing & power supply related issues
  radio.begin();
  radio.setPALevel(RF24_PA_MIN, 0);

  // Connect to the mesh
  Serial.println(F("Connecting to the mesh..."));
  if (!mesh.begin()) {
    if (radio.isChipConnected()) {
      do {
        // mesh.renewAddress() will return MESH_DEFAULT_ADDRESS on failure to connect
        Serial.println(F("Could not connect to network.\nConnecting to the mesh..."));
      } while (mesh.renewAddress() == MESH_DEFAULT_ADDRESS);
    } else {
      Serial.println(F("Radio hardware not responding."));
      while (1) {
        // hold in an infinite loop
      }
    }
  }
}

void loop() {
  mesh.update();
  // Send an 'M' type message containing the current data information
  if (!mesh.write(&msg, 'M', sizeof(msg))) {
    // If a write fails, check connectivity to the mesh network
    if (!mesh.checkConnection()) {
      //refresh the network address
      Serial.println("Renewing Address");
      if (mesh.renewAddress() == MESH_DEFAULT_ADDRESS) {
        //If address renewal fails, reconfigure the radio and restart the mesh
        //This allows recovery from most if not all radio errors
        mesh.begin();
      }
    } else {
      Serial.println("Send fail, Test OK");
    }
  } else {
    Serial.print("Send OK - ");
    Serial.println(msg);
    msg++;
  }
  delay(10000);
}
