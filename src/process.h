#ifndef PROCESS
#define PROCESS

#include "matrix.h"
#include "web-sever.h"
#include "sensores.h"





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
        4096,
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
            const int sampleRate = 8000;  // frecuencia en Hz
            int sampleDelay = 1000000 / sampleRate;
            size_t bytesRead = 0;
            size_t filesize = audioFile.size();  // Tamaño en bytes
            Serial.print("Tamaño del archivo: ");
            Serial.print(filesize);
            Serial.println(" bytes");
            while (audioFile.available()) {
                uint8_t sample = audioFile.read();
                dacWrite(DACPIN, sample);
                ets_delay_us(sampleDelay - 100); //change 80 
                if(playAudio == 0) break;

            }
            audioFile.close();
            Serial.println("Reproducción terminada.");  
        }
        
    }
}                                                                                                           


#endif