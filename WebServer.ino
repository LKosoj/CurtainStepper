void web_command(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
String calibrateKeyProcessor(const String &var);
String indexKeyProcessor(const String &var);
void calibrate_command(AsyncWebServerRequest *request);
String setupKeyProcessor(const String &var);


void WebServerInit(void) {

  FS_init();  // Включаем работу с файловой системой

  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
  server.serveStatic("/", SPIFFS, "/index.htm").setTemplateProcessor(indexKeyProcessor);
  server.serveStatic("/index.htm", SPIFFS, "/index.htm").setTemplateProcessor(indexKeyProcessor);

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request) {
    //TempStr = temp;
    getjson();
    request->send(200, "text/html", jsonstr);
  });
  server.on("/command", HTTP_GET, [](AsyncWebServerRequest * request) {
    web_command(request);
  });

  server.onFileUpload([](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char *)data);
    if (final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index + len);
  });
  server.onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char *)data);
    if (index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*"); // CORS
  server.begin();
#ifdef __SAMOVAR_DEBUG
  Serial.println("HTTP server started");
#endif
}

String indexKeyProcessor(const String &var) {
  return String();
}

String setupKeyProcessor(const String &var) {
  return String();
}

String calibrateKeyProcessor(const String &var) {
  return String();
}

void handleSave(AsyncWebServerRequest *request) {
  /*
       int params = request->params();
        for(int i=0;i<params;i++){
          AsyncWebParameter* p = request->getParam(i);
          Serial.print(p->name().c_str());
          Serial.print("=");
          Serial.println(p->value().c_str());
        }
        //return;
  */

  AsyncWebServerResponse *response = request->beginResponse(301);
  response->addHeader("Location", "/");
  response->addHeader("Cache-Control", "no-cache");
  request->send(response);
}

void web_command(AsyncWebServerRequest *request) {
/*  
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      Serial.print(p->name().c_str());
      Serial.print("=");
      Serial.println(p->value().c_str());
    }
    //return;
*/  
  if (request->hasArg("go_pos")) {
      curt_go_pos(request->arg("go_pos").toInt());
  } else if (request->hasArg("go_up")) {
      curt_go_up();
  } else if (request->hasArg("go_down")) {
      curt_go_down();
  } else if (request->hasArg("stop")) {
      curt_stop();
  } else if (request->hasArg("zero")) {
      curt_set_zero();
  } else if (request->hasArg("calibrate")) {
      curt_calibrate();
  } else if (request->hasArg("ch_dir")) {
      curt_change_dir();
  }
  request->send(200, "text/plain", "OK");
}

void calibrate_command(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
}
