#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "yourssid";
const char *password = "yourpsswd";
const char* domainToCheck = "localserveraddress1";
const char* domainToCheck1 = "localserveraddress2";
const char* ntfyURL = "ntfyservercheckinurl";
const char* ntfyURL1 = "ntfyserverdownalerturl";
bool server_down = false;

unsigned long lastCheckTime = 0;
unsigned long lastBlinkTime = 0;
bool ledState = false;

NetworkServer server(80);

void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);  
  pinMode(4, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  server.begin();
}

void loop() {
  NetworkClient client = server.accept();
  if (client) {                     
    String currentLine = "";       
    while (client.connected()) {    
      if (client.available()) {     
        char c = client.read();     
        Serial.write(c);            
        if (c == '\n') {           
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print("Click <a href=\"/H\">here</a> to turn the LED on.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn the LED off.<br>");
            client.print("Please leave the LED off when you're done.");
            client.print(" If you want to know more Info about this click <a href='https://dangrain.top/lampy.HTML'>here</a>.");
            client.println();
            break;
          } else {  
            currentLine = "";
          }
        } else if (c != '\r') {  
          currentLine += c;      
        }

        if (currentLine.endsWith("GET /H")) { 
          digitalWrite(5, HIGH); 
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(5, LOW); 
        }
      }
    }
    client.stop();
  }

  unsigned long now = millis();
  if (now - lastCheckTime >= 3600000UL || lastCheckTime == 0) {
    checkOnline();
    lastCheckTime = now;
  }

  if (server_down) {
    if (now - lastBlinkTime >= 500) {
      lastBlinkTime = now;
      ledState = !ledState;
      digitalWrite(4, ledState ? HIGH : LOW);
    }
  } else {
    digitalWrite(4, LOW);
  }
}

void postRequest(const char* url, const char* payload) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.POST(payload);
  http.end();
}

void checkOnline() {
  bool down = false;
  WiFiClient client1;

  if (client1.connect(domainToCheck, 80)) {
    postRequest(ntfyURL, "Prim_Server is up!");
    client1.stop();
  } else {
    down = true;
    postRequest(ntfyURL1, "Prim_Server is down! Fix It!");
  }

  if (client1.connect(domainToCheck1, 80)) { 
    postRequest(ntfyURL, "Sec_Server is up!");
    client1.stop(); 
  } else {
    down = true;
    postRequest(ntfyURL1, "Sec_Server is down! Fix It!");
  }

  server_down = down;
}
