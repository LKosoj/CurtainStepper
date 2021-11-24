#define DRIVER_STEP_TIME 4
#define DRIVER_STEPS 500
#define EEPROM_SIZE 200

#define USE_LittleFS
#define USE_UPDATE_OTA


#ifdef __cplusplus
extern "C" {
#endif

uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

uint8_t temprature_sens_read();

char* Curt_host = "curt_stepper";

enum CURT_STATUS {TO_ZERO, STOP, MOVE, CALIBRATE};
CURT_STATUS Curt_Status;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

DNSServer dns;

#include <FS.h>
#ifdef USE_LittleFS
#pragma message ("USE LITTLEFS")
  //#define SPIFFS LITTLEFS
  //#include <LITTLEFS.h> 
  #define SPIFFS LittleFS
  #include <LittleFS.h> 
#else
#pragma message ("USE SPIFFS")
  #include <SPIFFS.h>
#endif


#include <GyverStepper.h>
GStepper<STEPPER4WIRE_HALF> stepper(2048, 23, 21, 22, 19);

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool StepperMoving = false;                            // Индикатор движущегося шагового двигателя
String jsonstr;                                                 // Строка json
byte curt_status;                                               // Текущая позиция шторы

struct SetupEEPROM {
byte direction;                                                 // Направление движения
uint16_t step;                                                  // Всего шагов от нулевой позиции
};

SetupEEPROM CurtSetup;