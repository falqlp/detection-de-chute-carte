  #include "painlessMesh.h"
  #include <NimBLEDevice.h>
  #include <chrono>
  #include <ctime>

  #define MESH_PREFIX     "CotonLePlusBeau"
  #define MESH_PASSWORD   "somethingSneaky"
  #define MESH_PORT       5555

  #define BUTTON_PIN      1  // GPIO 1

  Scheduler userScheduler; // to control your personal task
  painlessMesh mesh;

  bool active = false;
  int count = 0;
  String chambre = "chambre 208";

  void sendMessage() {
    String msg = "chute";
    msg += ";";
    msg += chambre;
    Serial.println(msg);
    mesh.sendBroadcast(msg);
    String request = "countRequest;";
    request += chambre;
    Serial.println(request);
  }

  void receivedCallback(uint32_t from, String &msg) {
    Serial.println(msg.c_str());
  }

  void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  }

  void changedConnectionCallback() {
    Serial.printf("Changed connections\n");
  }

  void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
  }

  // BLE callbacks
class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
      Serial.println("Client connected");
      Serial.print("countRequest;");
      Serial.println(chambre);
    };

    void onDisconnect(NimBLEServer* pServer) {
      Serial.println("Client disconnected");
    }
};

class MyCallbacks : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic) {
        char strValue[10];
        sprintf(strValue, "%d", count);  // Convertir l'entier en chaîne de caractères
        pCharacteristic->setValue(count); 
    }
    void onWrite(NimBLECharacteristic* pCharacteristic) {
      String msg = pCharacteristic->getValue();
      msg += ";";
      msg += chambre;
      Serial.println(msg);
    }
};

  void setup() {
    Serial.begin(115200);
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Configurer la broche du bouton comme entrée avec pull-up interne

    NimBLEDevice::init("chambre 208");
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService *pService = pServer->createService("4fafc201-1fb5-459e-8fcc-c5c9c331914b");

    NimBLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                            "beb5483e-36e1-4688-b7f5-ea07361b26a8",
                                            NIMBLE_PROPERTY::READ |
                                            NIMBLE_PROPERTY::WRITE
                                          );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->("Hello BLE");
    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    Serial.println("Waiting for a client connection to notify...");

    //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
    mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    
    Serial.println("countRequest;"+chambre);
  }

  void loop() {
    // it will run the user scheduler as well
    mesh.update();

    // Vérifier l'état du bouton
    if (digitalRead(BUTTON_PIN) == HIGH && !active) {
      sendMessage();  // Envoyer un message lorsque le bouton est pressé
      active = true;
    }else if(digitalRead(BUTTON_PIN) != HIGH){
      active = false;
    }
    if (Serial.available() > 0) {
      String incomingMessage = Serial.readStringUntil('\n');
      Serial.println(incomingMessage);
      if (incomingMessage.startsWith("count")) {
        Serial.println("Message starts with 'count'");
        // Extraire le reste du message après "count"
        String countStr = incomingMessage.substring(5);
        countStr.trim();  // Remove any leading/trailing whitespace
        count = countStr.toInt(); 
      }
    }
    delay(100); 
  }
