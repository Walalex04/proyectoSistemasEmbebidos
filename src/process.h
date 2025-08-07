#ifndef PROCESS
#define PROCESS

#include "matrix.h"
#include "web-sever.h"
#include "sensores.h"


#include "xtensa/core-macros.h"
#include <driver/timer.h>

//Configure handls
TaskHandle_t handleShowMessage = NULL;


//Configure task
void TestHwm(char *taskName);
void TaskShow(void *pvParameters);
void TaskServer(void *pvParameters);
void TaskReadSensors(void *pvParameters);
void TaskShowData(void *pvParameters);
void TashShowled(void *pvParameters);
void TaskCheckTareas(void *pvParameters);
void TaskPlayAudio(void *pvPaRarameters);

void TaskShow(void *pvParameters){
    String message = *(String *) pvParameters;
    while(1){
        handleNewMessages(message);
    }
    
}

void TaskServer(void *pvParameters){
    intentoconexion("espgroup1", "12341243");

    //this section the esp32 have conection
    vTaskDelete(handleShowMessage);
    
    //Show init all 
    xTaskCreatePinnedToCore(
        TaskShowData,
        "TaskShowData",
        2048,
        NULL,
        1,
        NULL,
        APP_CPU
    );

    xTaskCreatePinnedToCore(
        TaskReadSensors,
        "TaskReadSensors",
        8192,
        NULL,
        1,
        NULL,
        APP_CPU
    );
    
    xTaskCreatePinnedToCore(
        TaskCheckTareas,
        "TaskCheckTareas",
        8192,
        NULL,
        1,
        NULL,
      
        APP_CPU
    );
    
    xTaskCreatePinnedToCore(
        TaskPlayAudio,
        "TaskPlayAudio",
        8192,
        NULL,
        1,
        NULL,
      
        APP_CPU
    );
     
  

    while(1){
        loopAP(); //handle
    }
}

void TaskReadSensors(void *pvParameters){
    char contador = 0;
    while(1){
        getCurrentTimestamp();
        temp = getTemperature();
        hum = getHumidity();
        ppm = getCO2();
        

        char id[32];
        snprintf(id, sizeof(id), "%04d%02d%02d_%02d%02d00", year, month, day, hour, minutes);
        
        Serial.println(id);
        contador++;
        if(contador >= 3){
            String datos = "{\"temperatura\": " + String(temp) +
            ",\"ppm\": " + String(ppm) +
             ", \"humedad\":"  + hum + "}";
            //Serial.println(datos);
            String ruta = "/sensores/"+String(id)+"/datos.json";
            bool exito = sendDataToFirebase(ruta.c_str(), datos.c_str(), false); // PUT

            if (exito) {
                Serial.println("Datos enviados con éxito");
            } else {
                Serial.println("Error enviando datos");
            }
            contador = 0;
        }
        
        
        vTaskDelay(1000*60*1);

        
    }
    
}
void TaskShowData(void *pvParameters){
    while(1){
        
        if(!digitalRead(PINCHANGE)){
            switch (ActualState)
            {
            case TIME:
                ActualState = TEMPERATURE;
                break;
            case TEMPERATURE:
                ActualState = HUMIDITY;
                break;
            case HUMIDITY:
                ActualState = CO2;
                break;
            case CO2:
                ActualState = TIME;
                break;
            }
            vTaskDelay(300);
        }

        
        switch (ActualState)
            {
            case TIME:
                mostrarHora(hour/10,hour%10,minutes/10,minutes%10);
                break;
            case TEMPERATURE:
                mostrarNumero(temp);
                break;
            case HUMIDITY:
                mostrarNumero(hum);
                break;
            case CO2:
                mostrarNumero(ppm);
                break;
            }
    }
  
}

void TashShowled(void *pvParameters){
    char contador = 0;
    while(1){
        if(!digitalRead(PINCHANGECOLOR)){
            ledRojo = colores[contador%3][1];
            ledVerde = colores[contador%3][2];
            ledAzul = colores[contador%3][3];
            contador++;
            vTaskDelay(200);
        }
        if(mode){
            ledcWrite(CHANNEL1, ledRojo);
            ledcWrite(CHANNEL2, ledAzul);
            ledcWrite(CHANEEL3, ledVerde);
        }else if(mode == 0){
            ledcWrite(CHANNEL1, ledRojo);
            ledcWrite(CHANNEL2, ledAzul);
            ledcWrite(CHANEEL3, ledVerde);
            vTaskDelay(200);
            ledcWrite(CHANNEL1, 0);
            ledcWrite(CHANNEL2, 0);
            ledcWrite(CHANEEL3, 0);
            vTaskDelay(200);
        }
        
    }
}
void TaskCheckTareas(void *pvParameters){
    bool stateActive = 0; 
    bool passAlarm = 0;
    bool saveColors = 0;
    while(1){
        if(currentTarea.time != ""){
            int pos = currentTarea.time.indexOf(':');
            String horasStr = currentTarea.time.substring(0, pos);
            horasStr.replace("\"", "");
            String minutosStr = currentTarea.time.substring(pos + 1);
            int min = minutes;
            int hor = hour;
            int index1 = currentTarea.RGB.indexOf('-');
            int index2 = currentTarea.RGB.lastIndexOf('-');  
            String rStr = currentTarea.RGB.substring(0, index1);
            String gStr = currentTarea.RGB.substring(index1 + 1, index2);
            String bStr = currentTarea.RGB.substring(index2 + 1);

            if(horasStr.toInt() == hor && minutosStr.toInt() == min && !stateActive){
                if (saveColors == 0)
                {
                    lastRED = ledRojo;
                    lastBlue = ledAzul;
                    lastGreen = ledVerde;
                    saveColors = 1;
                }
                
                
                
                ledRojo = rStr.toInt();
                ledVerde = gStr.toInt();
                ledAzul = bStr.toInt();
                mode = 0;
                playAudio = 1;
            }else if((minutosStr.toInt()+1) == min ||  (horasStr.toInt()+1) == hor){
                mode = 1;
                passAlarm = 1;
                playAudio = 0;
                saveColors = 0;
                ledRojo = lastRED;
                ledVerde = lastGreen;
                ledAzul = lastBlue;
            }

            if(!digitalRead(PINSTOPALARM) && mode == 0){
                stateActive = 1; 
                playAudio = 0;
                mode = 1;
                ledRojo = lastRED;
                ledVerde = lastGreen;
                ledAzul = lastBlue;
                saveColors = 0;
                vTaskDelay(300);
            }

            if(passAlarm == 1){
                stateActive = 0;
                passAlarm = 0;

                //Elimina de la base de datos la tarea actual y trae el otro 
                deleteRegister(currentTarea.TareaId);
                //Se elimina el archivo
                if (LittleFS.exists("/" + currentTarea.filename)) {
                    if (LittleFS.remove("/" + currentTarea.filename)) {
                        Serial.println("Archivo eliminado correctamente");
                    } else {
                        Serial.println("Error al eliminar el archivo");
                    }
                } else {
                    Serial.println("El archivo no existe");
                }

                //obtiene la siguiente tarea
                char id[16];
                snprintf(id, sizeof(id), "%04d%02d%02d", year, month, day);
                String params = "?orderBy=\"$key\"&startAt=\"" + String(id) + "_\"&endAt=\"" + String(id) + "_\\uFFFF\"&limitToFirst=1";
                String response = getDatabase(params);
                Serial.println(response);

                currentTarea = parseTarea(response);
                Serial.println(currentTarea.RGB);
                if(currentTarea.filename != ""){
                    Serial.println("Si hay otra tarea");
                    String fileURL = "https://xxviwfoyyskhnxqznlly.supabase.co/storage/v1/object/public/appembebidos/" + currentTarea.filename;
                    bool success = downloadFileToFS(fileURL, "/" + currentTarea.filename);

                    if (success)
                    {
                        Serial.println("audio cargado");

                    }else{
                        Serial.println("Ocurrio un problema al cargar el audio");
                    }
                }
                Serial.println(currentTarea.time);
                //load the file
                
            }
        }
         
    }
    
}


void preciseDelayCycles(uint32_t cycles) {
  uint32_t start = XTHAL_GET_CCOUNT();
  while ((XTHAL_GET_CCOUNT() - start) < cycles);
}


// ===== VARIABLES GLOBALES =====
hw_timer_t *timer = NULL;
File audioFile;

uint8_t buffer[BUFFER_SIZE];
volatile int audioIndex = 0;
volatile int bufferLen = 0;
volatile bool bufferPlaying = false;
volatile bool audioDone = false;

// ===== INTERRUPCIÓN DEL TIMER =====
void IRAM_ATTR onTimer() {
  if (audioIndex < bufferLen) {
    dacWrite(DAC_PIN, buffer[audioIndex++]);
  } else {
    timerAlarmDisable(timer);  // Detiene la interrupción
    timerStop(timer);
    bufferPlaying = false;     // Marca que se terminó el bloque
  }
  timerWrite(timer, 0); // Reinicia el contador
}

// ===== INICIALIZACIÓN DEL TIMER =====
void setupTimer() {
  timer = timerBegin(TIMER_NUMBER, TIMER_DIVIDER, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIMER_TICKS - 22, true);
  timerAlarmDisable(timer);
}

// ===== INICIAR LA REPRODUCCIÓN DEL BLOQUE ACTUAL =====
void startPlayback() {
  audioIndex = 0;
  bufferPlaying = true;
  timerStart(timer);
  timerAlarmEnable(timer);
}

// ===== DETENER LA REPRODUCCIÓN =====
void stopPlayback() {
  timerAlarmDisable(timer);
  timerStop(timer);
  bufferPlaying = false;

}

void TaskPlayAudio(void *pvPaRarameters){
    Serial.println("Se ejecuta desde play Audio");
    while(1){
        if(playAudio == 1){
            String file = currentTarea.filename;
            Serial.println("alarm!");
             File audioFile = LittleFS.open("/" + currentTarea.filename, "r");
            if (!audioFile) {
                Serial.println("No se pudo abrir /" + currentTarea.filename);
                return;
            }
             Serial.println("Reproduciendo audio...");

            // Duración entre muestras, en microsegundos
            const int sampleRate = 22050;  // frecuencia en Hz
            int sampleDelay = 1000000 / sampleRate;
            size_t bytesRead = 0;
            size_t filesize = audioFile.size();  // Tamaño en bytes
            Serial.print("Tamaño del archivo: ");
            Serial.print(filesize);
            Serial.println(" bytes");
            
            //const int bufferSize = 5120;
            //uint8_t buffer[bufferSize];
            
            /*
            unsigned long t0 = micros();
            while (audioFile.available()) {
                /*
                uint8_t sample = audioFile.read();
                uint8_t sample2 = sample*65535/255;
                dacWrite(DACPIN, sample);
                delayMicroseconds(50); //change 80 
                if(playAudio == 0) break;   
                

                int len = audioFile.read(buffer, bufferSize);
                for (int i = 0; i < len; i++) {
                    dacWrite(25, buffer[i]);  // GPIO25
                    //dac_output_voltage(DAC_CHANNEL_1, buffer[i]);
                    //delayMicroseconds(8);
                    preciseDelayCycles(250);
                }

            }
*/
             while (audioFile.available()) {
                bufferLen = audioFile.read(buffer, BUFFER_SIZE);

                if (bufferLen <= 0) {
                audioDone = true;
                break;
                }

                startPlayback();

                // Espera hasta que se termine de reproducir el bloque
                while (bufferPlaying) {
                delay(1);
                }
            }

            stopPlayback();
            audioFile.close();
            Serial.println("Reproducción finalizada");
            audioDone = true;
        }
                    
        
        
    }
}                                                                                                           


#endif