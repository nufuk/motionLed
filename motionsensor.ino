#include <WS2812FX.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define IrSensor      D7
#define PIN           D8
#define NUMPIXELS     61

WS2812FX ws2812fx = WS2812FX(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
HTTPClient httpClientSunriseSunset;
HTTPClient httpClientWorldApi;

int movement = 0;
char* ssid = "<insertSSID";
char* password = "<insertPaswd>";
const int waitTimeMS = 20000;
const int red = 100;
const int green = 0;
const int blue = 0;

struct hourMinute {
  int hour;
  int minute;
};
typedef struct hourMinute HourMinute;

void setup() {
  Serial.begin(115200);

  //initilize the LED Stripe
  ws2812fx.init();
  ws2812fx.setBrightness(100);
  ws2812fx.setSpeed(200);
  ws2812fx.setMode(FX_MODE_RAINBOW_CYCLE);
  ws2812fx.start();
  ws2812fx.service();

  //Connect to the wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  //Initialize the IR Motion sensor
  pinMode(IrSensor, INPUT);
  Serial.println("Start");

  delay(10000);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setColor(0, 0, 0);
  ws2812fx.setBrightness(0);
  ws2812fx.service();
}

String getTwilight(String twilight) {
  // TODO https
  String uriString = "http://api.sunrise-sunset.org/json?lat=xx.xxxxxx&lng=xx.xxxxxx&date=today&formatted=0";
  httpClientSunriseSunset.begin(uriString);
  httpClientSunriseSunset.addHeader("Content-Type", "text/html");

  //TODO if != 200 error
  int httpCode = httpClientSunriseSunset.GET();
  
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 480;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(httpClientSunriseSunset.getString());
  const String result = root["results"][twilight];
  
  httpClientSunriseSunset.end();
  return result;
}

void printStruct(String caller, HourMinute timeStruct) {
  Serial.print(caller);
  Serial.print(": ");
  Serial.print(timeStruct.hour);
  Serial.print(" ");
  Serial.println(timeStruct.minute);
}

HourMinute getTwilightEnd() {
  HourMinute hourAndMinute;
  const String twilightEnd = getTwilight("civil_twilight_end");
  hourAndMinute.hour = twilightEnd.substring(11,13).toInt();
  hourAndMinute.minute = twilightEnd.substring(14,16).toInt();
  printStruct("getTwilightEnd", hourAndMinute);
  return hourAndMinute;
}

HourMinute getTwilightBegin() {
  HourMinute hourAndMinute;
  const String twilightBegin = getTwilight("civil_twilight_begin");
  hourAndMinute.hour = twilightBegin.substring(11,13).toInt();
  hourAndMinute.minute = twilightBegin.substring(14,16).toInt();
  printStruct("getTwilightBegin", hourAndMinute);
  return hourAndMinute;
}

HourMinute getCurrentTime() {
  HourMinute hourAndMinute;
  //Get current UTC time form API
  httpClientWorldApi.begin("http://worldtimeapi.org/api/timezone/Etc/UTC");
  httpClientWorldApi.addHeader("Content-Type", "text/html");
  int httpCode = httpClientWorldApi.GET();
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 370;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(httpClientWorldApi.getString());
  const String datetime = root["datetime"];
  hourAndMinute.hour = datetime.substring(11,13).toInt();
  hourAndMinute.minute = datetime.substring(14,16).toInt();
  
  printStruct("getCurrentTime", hourAndMinute);
  httpClientWorldApi.end();
  return hourAndMinute;
}

bool checkTime(HourMinute currentTime, HourMinute twillight, bool later) {
  if (later) {
    if(currentTime.hour > twillight.hour) {
      return true;
    } else if (currentTime.hour == twillight.hour) {
      return currentTime.minute >= twillight.minute;
    } else {
      return false;
    }
  } else {
    if(currentTime.hour < twillight.hour) {
      return true;
    } else if (currentTime.hour == twillight.hour) {
      return currentTime.minute <= twillight.minute;
    } else {
      return false;
    }
  }
}

void loop() {
  ws2812fx.service();
  movement = digitalRead(IrSensor);
  
  if (movement == 1) {
    Serial.println("movement");
    HourMinute currentTime = getCurrentTime();
    HourMinute twilightBegin = getTwilightBegin();
    HourMinute twilightEnd = getTwilightEnd();
    
    if(checkTime(currentTime, twilightEnd, true) ||  checkTime(currentTime, twilightBegin, false)) {
      Serial.println("checkTime");
      ws2812fx.setMode(FX_MODE_STATIC);
      ws2812fx.setColor(red, green, blue);
      ws2812fx.setBrightness(20);
      ws2812fx.service();
    }
    
    delay(waitTimeMS);
    ws2812fx.setMode(FX_MODE_STATIC);
    ws2812fx.setColor(0, 0, 0);
    ws2812fx.setBrightness(0);
    ws2812fx.service();
  }
}
