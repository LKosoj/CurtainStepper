#include "soc/rtc_wdt.h"
#include <esp_task_wdt.h>
#include <driver/dac.h>
#include <driver/touch_sensor.h>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <EEPROM.h>
#include <ESPAsyncWiFiManager.h>
#include <SPIFFSEditor.h>

#include "CurtainStepper.h"

#ifdef USE_UPDATE_OTA
#include <ArduinoOTA.h>
#endif


void stopService(void) {
  timerAlarmDisable(timer);
}

void startService(void) {
//  timerAlarmDisable(timer);
  timerAlarmWrite(timer, stepper.stepTime, true);
  timerAlarmEnable(timer);
}

void IRAM_ATTR StepperTicker(void) {
  portENTER_CRITICAL_ISR(&timerMux);
  StepperMoving = stepper.tick();
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {

  Serial.begin(115200);

  touch_pad_intr_disable();

  // Configure the Prescaler at 80 the quarter of the ESP32 is cadence at 80Mhz
  // 80000000 / 80 = 1000000 tics / seconde
  timer = timerBegin(2, 80, true);
  timerAttachInterrupt(timer, &StepperTicker, true);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  
  // можно установить скорость
  stepper.setSpeed(DRIVER_STEPS);    // в шагах/сек

  // режим следования к целевй позиции
  stepper.setRunMode(FOLLOW_POS);

  // установка макс. скорости в шагах/сек
  stepper.setMaxSpeed(DRIVER_STEPS);

  // установка ускорения в шагах/сек/сек
  //stepper.setAcceleration(100);

  // отключать мотор при достижении цели
  stepper.autoPower(true);

  stopService();
  stepper.brake();
  stepper.disable();

  //StepperMoving = false;
  //delay(2000);

  //Подключаемся к WI-FI
  //AsyncWiFiManagerParameter custom_blynk_token("blynk", "blynk token", SamSetup.blynkauth, 33, "blynk token");
  AsyncWiFiManager wifiManager(&server, &dns);
  wifiManager.setConfigPortalTimeout(180);
  //wifiManager.setSaveConfigCallback(saveConfigCallback);
  //wifiManager.setAPCallback(configModeCallback);
  wifiManager.setDebugOutput(false);
  //wifiManager.addParameter(&custom_blynk_token);
  wifiManager.autoConnect("CurtainStepper");

  //wifiManager.resetSettings();

//  if (shouldSaveWiFiConfig) {
//    if (strlen(custom_blynk_token.getValue()) == 33) {
//      strcpy(SamSetup.blynkauth, custom_blynk_token.getValue());
//      save_profile();
//    }
//  }

  Serial.print(F("Connected to "));
  Serial.println(WiFi.SSID());
  Serial.print(F("IP address: "));
  String StIP = WiFi.localIP().toString();

  Serial.println(StIP);

  if (!MDNS.begin(Curt_host)) {  //http://curt_stepper.local
    Serial.println("Error setting up MDNS responder!");
  } else {
#ifdef __SAMOVAR_DEBUG
    Serial.println("mDNS responder started");
#endif
  }

#ifdef USE_UPDATE_OTA
  //Send OTA events to the browser
  ArduinoOTA.onStart([]() {
    events.send("Update Start", "ota");
  });
  ArduinoOTA.onEnd([]() {
    events.send("Update End", "ota");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress / (total / 100)));
    events.send(p, "ota");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if (error == OTA_BEGIN_ERROR)
      events.send("Begin Failed", "ota");
    else if (error == OTA_CONNECT_ERROR)
      events.send("Connect Failed", "ota");
    else if (error == OTA_RECEIVE_ERROR)
      events.send("Recieve Failed", "ota");
    else if (error == OTA_END_ERROR)
      events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname(Curt_host);
  ArduinoOTA.begin();
#endif

  WebServerInit();
  read_config();
  curt_go_zero();
}

void curt_go_up(){
  curt_go_pos(100);
}

void curt_go_down(){
  curt_go_pos(0);
}

void curt_go_pos(byte s){
  s = 100 - s;
  uint16_t pos = map(s, 0, 100, 0, CurtSetup.step);
  Serial.println("go_pos=" + (String)pos);
  Curt_Status = MOVE;
  stepper.setTarget(pos);
  Serial.println("current=" + (String)stepper.getCurrent());
  Serial.println("target=" + (String)stepper.getTarget());
  startService();
}

void curt_stop(){
  stopService();
  if (Curt_Status == CALIBRATE) {
    CurtSetup.step = stepper.getCurrent();
    EEPROM.put(0, CurtSetup);
    EEPROM.commit();
  }
  if (Curt_Status == TO_ZERO) {
    curt_set_zero();
  }
  stepper.brake();
  stepper.disable();
  Curt_Status = STOP;
  Serial.println("stop");
}

void curt_go_zero(){
  curt_stop();
  stepper.setCurrent(99999999);
  stepper.setTarget(0);
  Serial.println("current=" + (String)stepper.getCurrent());
  Serial.println("target=" + (String)stepper.getTarget());
  startService();
  Curt_Status = TO_ZERO;
  Serial.println("to zero");
}

void curt_calibrate(){
  curt_stop();
  if (stepper.getCurrent() != 0) {
    curt_go_zero();
    return;
  }
  stepper.setTarget(99999999);
  Serial.println("current=" + (String)stepper.getCurrent());
  Serial.println("target=" + (String)stepper.getTarget());
  startService();
  Curt_Status = CALIBRATE;
  Serial.println("calibrate");
}

void curt_change_dir(){
  if (CurtSetup.direction > 0 ) {
    CurtSetup.direction = -1;
    stepper.reverse(true);
  } else {
    CurtSetup.direction = 1;
    stepper.reverse(false);
  }
  EEPROM.put(0, CurtSetup);
  EEPROM.commit();
}

byte get_curt_status(){
  uint16_t crt = stepper.getCurrent();
  byte s = map(crt, 0, CurtSetup.step, 0, 100);
  if (crt == stepper.getTarget() && Curt_Status != STOP) curt_stop();
  return s;
   //TargetStepps = stepper.getTarget();
}

void curt_set_zero(){
   stepper.setCurrent(0);
}

void read_config() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, CurtSetup);
  if (CurtSetup.direction > 0 ) {
    stepper.reverse(false);
  } else {
    stepper.reverse(true);
  }
//  CurtSetup.step = 12000;
}

void loop() {
#ifdef USE_UPDATE_OTA
  ArduinoOTA.handle();
#endif

  ws.cleanupClients();

  curt_status = get_curt_status();
  //Serial.println("curt_status = " + (String)curt_status);
  
  delay(100);
}

void getjson(void) {

  DynamicJsonDocument jsondoc(1500);
  jsondoc["position"] = 100 - get_curt_status();
  jsonstr = "";
  serializeJson(jsondoc, jsonstr);
}
