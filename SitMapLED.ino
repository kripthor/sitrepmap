// This is for TTGO ESP8266 OLED 128x32 module 
// To use in other modules, adjust as needed
// You will need to change the SSID, PASSWORD and HTTP endpoint
// 
//
// kripthor

#include <NeoPixelBus.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define MAXTIME 600

const char* ssid = "YOUR-SSID";
const char* password = "YOUR-PASSWORD";
const char* HTTP_ENDPOINT = "http://YOURSERVER:YOURPORT/events.txt";

const uint8_t PixelPerLine = 30; 
const uint8_t Lines = 10; 
const uint16_t PixelCount = PixelPerLine * Lines; 
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, 2);


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_SDA 2
#define OLED_SCL 14
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int MaxLongitude = 180;
int MinLongitude = -180;
int MaxLatitude = 63;
int MinLatitude = -35;
unsigned long timeNow;

int LatLonToPixel(float lat, float lon) {
  int pix;
  int col = map(lon,MinLongitude,MaxLongitude,PixelPerLine-1,0);
  int row = map(lat,MinLatitude,MaxLatitude,Lines-1,0);
  if (row % 2 != 0) col = 29 - col;
  pix = row*PixelPerLine+col;
  return pix;
}

RgbColor timeToColor(int led, int secsElapsed) {
  RgbColor red(32, 0, 0);
  RgbColor green(0, 32, 0);

  int secsToEnd = MAXTIME - secsElapsed;
  if (secsToEnd <= 0) return RgbColor(0,0,0);
  float percent = secsElapsed*1.0/MAXTIME;

  RgbColor newc(128,0,0);
  if (secsElapsed > 10)  {
    newc = RgbColor::LinearBlend(red,green,percent+0.05);
  }
  if (secsToEnd < 30) newc = RgbColor(0,0,secsToEnd/2);
  RgbColor rc = strip.GetPixelColor(led);
  if (rc.R > newc.R) return rc;   
  return newc;
}


void setup()
{
    Serial.begin(115200);
    Serial.println("Initializing...");
    
    //OVERRIDE Wire.begin from Adafruit_SSD1306 to make this work on ESP8266 TTGO OLED
    Wire.begin( OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
      Serial.println("SSD1306 connection failed");
    }

    // Show initial display buffer contents on the screen
    // The library initializes this with an Adafruit splash screen.
    display.display();
    delay(500); // Pause 
    display.clearDisplay();
    display.setTextSize(1);      
    display.setTextColor(WHITE); 
    display.cp437(true);
      
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
     delay(1000);
     Serial.println("Connecting to "+String(ssid)+" ...");
     Serial.flush();
    }
    strip.Begin();
    strip.Show();
    Serial.println("Running...");
    
}



void loop()
{
  int oledLine = 0;
  int httpCode = 0;
   /*
   //TEST LOCATIONS
   int LX = LatLonToPixel(38.7,-9.1);
   int NY = LatLonToPixel(40.69,-74.25);
   int MOSCOW = LatLonToPixel(55.58,36.82);
   int SID = LatLonToPixel(-33.84,150.65);
   int SP = LatLonToPixel(-23.68,-46.87);
   int MX = LatLonToPixel(19.39,-99.28);
   int LA = LatLonToPixel(34.02,-118.69);
   int DAWSON = LatLonToPixel(63.48,-140.65);
   int BJ = LatLonToPixel(39.93,116.11);
   int PARIS = LatLonToPixel(48.85,2.271);
   int HCM = LatLonToPixel(10.75,106.411);
   strip.SetPixelColor(LX, RgbColor(0,128,0));
   strip.SetPixelColor(NY, RgbColor(0,0,64));
   strip.SetPixelColor(MOSCOW, RgbColor(0,0,64));
   strip.SetPixelColor(SID, RgbColor(0,0,64));
   strip.SetPixelColor(SP, RgbColor(0,0,64));
   strip.SetPixelColor(MX, RgbColor(0,0,64));
   strip.SetPixelColor(LA, RgbColor(0,0,64));
   strip.SetPixelColor(DAWSON, RgbColor(0,0,64));
   strip.SetPixelColor(BJ, RgbColor(0,0,64));
   strip.SetPixelColor(PARIS, RgbColor(0,0,64));
   strip.SetPixelColor(HCM, RgbColor(0,0,64));
   strip.Show();
   */ 
  
  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) { 
    String events;
    String evtItem;
    HTTPClient http; 
    //Specify request destination
    http.begin(HTTP_ENDPOINT);  
    httpCode = http.GET();                                                                 
     //Check the returning code
    if (httpCode == 200) {
     
    events = http.getString();   
     //PARSE response
    int startLine = 0;
    int endLine = events.indexOf('\n',startLine);
    timeNow = events.substring(startLine,endLine).toInt();
   
    startLine = endLine+1;
    endLine = events.indexOf('\n',startLine);
    while(endLine > 0) {
     evtItem = events.substring(startLine,endLine); 
    
     //PARSE EVENT TIME
     int endT = evtItem.indexOf(',');
     long evtTime = evtItem.substring(1,endT).toInt();
         
     //PARSE LOCATION
     int startLoc = evtItem.indexOf('(');
     int endLoc = evtItem.indexOf(')');
     int sepLoc = evtItem.indexOf(',',startLoc);
     String latS = evtItem.substring(startLoc+1,sepLoc);
     String lonS = evtItem.substring(sepLoc+2,endLoc);
     float lat =  latS.toFloat();
     float lon =  lonS.toFloat(); 
 
     startLine = endLine+1;
     endLine = events.indexOf('\n',startLine);
     int timeElapsed = timeNow-evtTime;
    
    //PARSE EVENT TYPE (TODO)

    //PARSE IP AND SEND TO OLED 
    int startIp = evtItem.indexOf('\'');
    int endIp = evtItem.indexOf('\'',startIp+1);
    String ip = evtItem.substring(startIp+1,endIp);
    
    if((timeElapsed < 10) && (oledLine <= 32)) {
      display.setCursor(0, oledLine);     // Start at top-left corner
      display.print(ip);
      display.setCursor(102, oledLine);     // Start at top-left corner
      display.print(String("> "+String(timeElapsed)+"s"));
      oledLine += 8;
      Serial.println(ip);
    }
  
    //SEND TO LED STRIP, INTENSITY BY TIME
    int led = LatLonToPixel(lat,lon);
    strip.SetPixelColor(led, timeToColor(led,timeElapsed));
    }
    display.display();
    strip.Show();
  }
  http.end();   //Close connection
  }

 
  Serial.println("Sleeping...");
  delay(3337);
  
  if (httpCode == 200) {
    //clear LED strip and OLED
    for (int k=0;k<PixelCount;k++) strip.SetPixelColor(k,RgbColor(0,0,0));
    display.clearDisplay();
  }
}
