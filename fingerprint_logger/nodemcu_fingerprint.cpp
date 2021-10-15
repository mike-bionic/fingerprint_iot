/*
relay = 4
fingerprintscanner = rx = 8 tx = 7;
led = 13
button = 2
*/

// /// Esp setup ///
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>


IPAddress staticIP(192, 168, 1, 154); //ESP static ip
IPAddress gateway(192, 168, 1, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(8, 8, 8, 8);  //DNS

const char* ssid = "ssid";
const char* password = "password";
const char* deviceName = "Finger";
String serverUrl = "192.168.1.252:5000"; //url of server ? raspberrty pi
String payload;
String device_key = "finger_secret_key";

ESP8266WebServer server(80);
/////////////


#include <Adafruit_Fingerprint.h>
int getFingerprintIDez();
SoftwareSerial mySerial(15, 13);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
int door_button = 5;
int key_button = 5;
int led = 0;
int relay = 4;


void handlePong() {
 server.send(200, "text/html", device_key);
}


void setup()
{
   pinMode(relay, OUTPUT);
   pinMode(led, OUTPUT);
   pinMode(door_button, INPUT);
   pinMode(key_button, INPUT);

   digitalWrite(relay, 0);
   digitalWrite(led, 0);
  Serial.begin(115200);
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Sensor tapyldy!");delay(1000);
  } else {
    while(true)
    blink();
  }


  WiFi.begin(ssid, password);
  Serial.println("");
  WiFi.disconnect();
  WiFi.config(staticIP, subnet, gateway, dns);
  WiFi.begin(ssid, password);

  WiFi.mode(WIFI_STA);

  delay(500);
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print(WiFi.localIP());
  server.on("/ping/", handlePong);
  server.begin();

}
char query = '*';


void loop(){
  server.handleClient();
  long sec = millis();
  query = '*';
  if(Serial.available()>0){
    query = Serial.read();
  }
  while(digitalRead(door_button) == 1){
    open_door();
    send_door_opened_log_info("Button");
    if((sec + 5000) < millis()){
      query = '1';
      break;
    }
  }

  if (digitalRead(key_button) == 1){
    open_door();
    send_door_opened_log_info("Key");
  }

 switch(query){
  case '1':  Enroll();break;
  case '2':  nowdelete();break;
  case 'D':  deleteF();break;
  default : getFingerprintIDez();break;
 }
  delay(50);
  query = '*';
}


void open_door(){
  digitalWrite(relay,1);
  delay(1000);
  digitalWrite(relay,0);
}

void sendRequest(String path, String sendingData){
  if(WiFi.status()== WL_CONNECTED){
    String serverPath = path+sendingData;
    Serial.println(serverPath);
    payload = httpGETRequest(serverPath.c_str());
    Serial.println(payload);
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();
  String payload = "{}";
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return payload;
}

//--------------------------------------------------------------------------------------------------------
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  {return -1;}
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  {return -1;}

  if (p == FINGERPRINT_OK) {open_door();}

  send_finger_log_info(finger.fingerID);

  delay(500);
}

void send_finger_log_info(int id){
  String argument_data = "?device_key="+device_key+"&finger_id="+id;
  sendRequest("http://"+serverUrl+"/finger_logger/",argument_data);
}
void send_door_opened_log_info(String access_type){
  String argument_data = "?device_key="+device_key+"&access_type="+access_type;
  sendRequest("http://"+serverUrl+"/finger_logger/",argument_data);
}
// http://192.168.1.252:5000/finger_logger/?device_key=finger_seceret&finger_id=3

///////////////////////////////////////////////////////ENROLL////////////////////////////////////////

uint8_t k;
void Enroll(){
  digitalWrite(led,1);
  uint8_t id = 1;
  delay(500);
  id = Findempty();
  k = id;
  while (!getFingerprintEnroll(id));
  digitalWrite(led,0);
}

uint8_t getFingerprintEnroll(uint8_t id) {
  uint8_t p = -1;
  Serial.println("Garasmagynyzy hayys edyan");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Hasaba alyndy");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("barmagy goy");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Komunikasya yalnyslygy");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Surat dogry alynmady");
      Serial.println("217");
      break;
    default:
      Serial.println("Anykdal yalnyslyk");
      Serial.println("217");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Surat konwertirlendi");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Surat dusnuksiz");Serial.println("217");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Komunikasiya yalnyslygy");Serial.println("217");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Barmagyn cyzgylaryny tapmadym");Serial.println("217");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Barmagyn cyzgylaryny tapmadym");Serial.println("217");
      return p;
    default:
      Serial.println("Anykdal yalnyslyk");Serial.println("217");
      return p;
  }

  Serial.println("Barmagy ayyr");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  p = -1;
  Serial.println("Sol barmagy tazeden goy");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Surata alyndy");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Surata almak amala asmady");
      Serial.println("217");
      break;
    default:
      Serial.println("Anykdal yalnyslyk");
      Serial.println("217");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Surat alyndy");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Surat hapa");Serial.println("217");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Komunikasiya yalnyslygy");Serial.println("217");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");Serial.println("217");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");Serial.println("217");
      return p;
    default:
      Serial.println("Unknown error");Serial.println("stop");Serial.println("217");
      return p;
  }


  // OK converted!
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Barmaklar gabat geldi!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Yalnyslyk");Serial.println("217");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Barmaklar gabat gelmedi");Serial.println("217");
    return p;
  } else {
    Serial.println("Anykdal yalnyshlyk");Serial.println("217");
    return p;
  }

  p = finger.storeModel(k);
  if (p == FINGERPRINT_OK) {
    Serial.println("Yatda sakladym");delay(500);Serial.println(k);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Komunikasiya yalnyslygy");Serial.println("217");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Sol yerde yatda saklap bilmedim");Serial.println("217");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Yazylmak yalnyslygy");Serial.println("217");
    return p;
  } else {
    Serial.println("Anykdal yalnyslyk");Serial.println("217");
    return p;
  }

}


uint8_t a = 1;
uint8_t Findempty()
{
while(true){
 uint8_t p = finger.loadModel(a);
  switch (p) {
    case FINGERPRINT_OK:

     break;

    default:
           return a;
      break;
  }
  a++;}
  return 0;
}


void nowdelete(){
  uint8_t nul = -1;

while(true){
nul = -5;
  Serial.println("Pozmaly elementi yaz :");
  if(Serial.available() > 0){
  nul = Serial.parseInt();
    if(nul > 0){break;}
  }
  delay(300);

}

deleteFingerprint(nul);
}


uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Pozuldy!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}

void blink(){
  digitalWrite(led,1);
  delay(1000);
  digitalWrite(led,0);
  delay(1000);
}

uint8_t deleteF() {
  uint8_t p = -1;

  for( uint8_t x = 1; x<165;x++){
  p = finger.deleteModel(x);
  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
  }
}