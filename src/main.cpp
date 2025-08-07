#include <Arduino.h>

#include "process.h"



void setup() {
  Serial.begin(115200);
  initMatriz();
  initSensors();
  pinMode(PINCHANGE, INPUT);
  pinMode(PINCHANGECOLOR, INPUT);
  pinMode(PINSTOPALARM, INPUT);
  //INit pwm
  ledcSetup(CHANNEL1, 5000, 8);
  ledcSetup(CHANNEL2, 5000, 8);
  ledcSetup(CHANEEL3, 5000, 8);


  ledcAttachPin(PINROJO, CHANNEL1);
  ledcAttachPin(PINAZUL, CHANNEL2);
  ledcAttachPin(PINVERDE, CHANEEL3);
  //dac_output_enable(DAC_CHANNEL_1);
  setupTimer();

  
  //setupI2SDAC();
  xTaskCreatePinnedToCore(
            TashShowled,
            "TashShowled",
            2048,
            NULL,
            1,
            NULL,
            APP_CPU
        );
  xTaskCreatePinnedToCore(
    TaskServer,
    "TaskServer",
    8192,
    NULL,
    1,
    NULL,
    APP_CPU
  );



  static String messageConfig = "config...";
   xTaskCreatePinnedToCore(
        TaskShow,
        "TaskShow",
        2048,
        &messageConfig,
        1,
        &handleShowMessage,
        APP_CPU
    );
    
}


void loop(){};