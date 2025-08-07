#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <LittleFS.h>


#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <Wire.h>
#include "WiFi.h"

#include "pines.h"


WebServer server(80);


static tm currentTime = {0};

String leerStringDeEEPROM(int direccion)
{
    String cadena = "";
    char caracter = EEPROM.read(direccion);
    int i = 0;
    while (caracter != '\0' && i < 100)
    {
        cadena += caracter;
        i++;
        caracter = EEPROM.read(direccion + i);
    }
    return cadena;
}

void escribirStringEnEEPROM(int direccion, String cadena)
{
    int longitudCadena = cadena.length();
    for (int i = 0; i < longitudCadena; i++)
    {
        EEPROM.write(direccion + i, cadena[i]);
    }
    EEPROM.write(direccion + longitudCadena, '\0'); // Null-terminated string
    EEPROM.commit();                                // Guardamos los cambios en la memoria EEPROM
}



bool sendDataToFirebase(const char* path, const char* jsonPayload, bool usePost = false) {
    WiFiClientSecure client;
    HTTPClient https;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi no conectado");
        return false;
    }

    // Prepara la URL con token auth
    String url = String(DATABASE_URL) + String(path);
    if (API_KEY && strlen(API_KEY) > 0) {
        url += "?auth=";
        url += API_KEY;
    }

    // Configura cliente y HTTP
    client.setInsecure();
    if (!https.begin(client, url)) {
        Serial.println("Error al iniciar HTTPS");
        return false;
    }

    https.addHeader("Content-Type", "application/json");

    int httpCode;
    if (usePost) {
        httpCode = https.POST(jsonPayload);
    } else {
        httpCode = https.PUT(jsonPayload);
    }

    Serial.printf("HTTP Code: %d\n", httpCode);

    https.end();

    return (httpCode > 0 && httpCode < 300); // true si fue exitoso
}

String getDatabase(const String& queryParams) {
  HTTPClient http;
  // Construimos query con orderBy y equalTo (escapamos comillas)
    
  String url = DATABASE_URL + String("/tareas.json") + queryParams;

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    http.end();
    return payload;
  } else {
    Serial.printf("Error HTTP: %d\n", httpCode);
    http.end();
    return "ERROR";
  }

}


String getCurrentTimestamp() {

configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "No se obtuvo la hora";  // No se pudo obtener hora
  }
  hour = timeinfo.tm_hour; //actualizo la hora del reloj
  minutes = timeinfo.tm_min;
  year = timeinfo.tm_year + 1900;
  month = timeinfo.tm_mon + 1;
  day  = timeinfo.tm_mday;

  

  char timestamp[25];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(timestamp);
}



void handleRoot()
{

    String json = "{";
    json += "\"nombre\":\"ESP32\",";
    json += "\"Temperatura\" : \"" + String(temp) + "\",";
    json += "\"Humedad\" : \"" + String(hum) + "\",";
    json += "\"CO2\" : \"" + String(ppm) + "\"";
    json += "}";

    server.send(200, "application/json", json);

    /*
    String html = "<html><body>";
    html += "<form method='POST' action='/wifi'>";
    html += "Red Wi-Fi: <input type='text' name='ssid'><br>";
    html += "Contraseña: <input type='password' name='password'><br>";
    html += "<input type='submit' value='Conectar'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
    */
}


bool downloadFileToFS(const String& fileURL, const String& savePath) {
  WiFiClientSecure client;
  client.setInsecure();  // Solo para pruebas, sin verificar certificado

  Serial.println("Conectando a: " + fileURL);

  HTTPClient https;
  if (!https.begin(client, fileURL)) {
    Serial.println("Error al iniciar conexión");
    return false;
  }

  int httpCode = https.GET();
  Serial.printf("HTTP GET code: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    File file = LittleFS.open(savePath, FILE_WRITE);
    if (!file) {
      Serial.println("No se pudo abrir archivo para escribir");
      https.end();
      return false;
    }

    WiFiClient* stream = https.getStreamPtr();
    uint8_t buffer[128];  // Buffer pequeño

    while (https.connected() && stream->available()) {
      int len = stream->readBytes(buffer, sizeof(buffer));
      file.write(buffer, len);
    }
    Serial.printf("Tamaño archivo guardado: %u bytes\n", file.size());

    file.close();
    
  } else {
    Serial.printf("HTTP Error: %d\n", httpCode);
  }

  https.end();

  return (httpCode == HTTP_CODE_OK);
}

void deleteRegister(const String& tareaId) {
  HTTPClient http;
    Serial.print("Se empieza a eliminar");
  String url = String(DATABASE_URL) + "/tareas/" + tareaId + ".json";

  http.begin(url);
  int httpCode = http.sendRequest("DELETE");  // Petición DELETE

  if (httpCode > 0) {
    Serial.printf("Código respuesta HTTP: %d\n", httpCode);
    String payload = http.getString();
    Serial.println("Respuesta:");
    Serial.println(payload);  // Normalmente Firebase devuelve null
  } else {
    Serial.printf("Error en la petición HTTP: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

// date1 < date 2 if return true
bool isEarlier(const struct tm* a, const struct tm* b) {
  if (a->tm_year != b->tm_year) return a->tm_year < b->tm_year;
  if (a->tm_mon  != b->tm_mon)  return a->tm_mon  < b->tm_mon;
  if (a->tm_mday != b->tm_mday) return a->tm_mday < b->tm_mday;
  if (a->tm_hour != b->tm_hour) return a->tm_hour < b->tm_hour;
  if (a->tm_min  != b->tm_min)  return a->tm_min  < b->tm_min;
  return a->tm_sec < b->tm_sec;
}

File uploadFile;

void handleUpload() {
    String typeTask = String(server.arg("type"));
    String date = String(server.arg("date"));
    String time = String(server.arg("time"));
    String nameFile = String(server.arg("nameFile"));
    String RGB = String(server.arg("rgb"));
    String date1 = date;
    String time1  = time;
    date1.replace("\"", "");
    time1.replace("\"", "");
    String dia = date1.substring(0, 2);
    String mes = date1.substring(3, 5);
    String anioCorto = date1.substring(6, 8);

    // Convertir año corto a año completo, asumiendo 2000+
    String anio = "20" + anioCorto;

    String hh = time1.substring(0, 2);
    String mm = time1.substring(3, 5);

    String id = anio + mes + dia + "_" + hh + mm +"00";
    Serial.println(id.c_str());
    //Almacenarlos en la base de datos
    //se envia a la base de datos
    String datos = "{\"tipoTarea\":\"" + typeTask  + 
             "\", \"rgb\": " + RGB +
            ", \"date\": " + date + ", \"time\": "  + time +
             ", \"filename\": \"" + nameFile +  "\"}";
            
    Serial.println(datos);
    
    //Agregando datos
    struct tm fechaData = {0};

    int firstDash = date.indexOf('-');
    int secondDash = date.indexOf('-', firstDash + 1);

    String diaStr = date.substring(1, firstDash);
    String mesStr = date.substring(firstDash + 1, secondDash);
    String anioStr = date.substring(secondDash + 1);

    int c1 = time.indexOf(':');
    int hora = time.substring(1, c1).toInt();
    int minuto = time.substring(c1 + 1).toInt();

    fechaData.tm_year = (anioStr.toInt() + 2000) - 1900;
    fechaData.tm_mon = mesStr.toInt() - 1;
    fechaData.tm_mday = diaStr.toInt();
    fechaData.tm_hour = hora;
    fechaData.tm_min = minuto;
    fechaData.tm_sec = 0;

    

    static bool state = "";
    //mktime(&fechaData) < mktime(&currentTime)
    if(currentTime.tm_year == 0 || isEarlier(&fechaData, &currentTime) ){
        currentTime = fechaData;
        DatosCurrent = datos;
        currentTarea.date = date;
        currentTarea.filename = nameFile;
        currentTarea.time = time;
        currentTarea.tipoTarea = typeTask;
        currentTarea.TareaId = id;
        currentTarea.RGB = RGB;
        String ruta = "/tareas/" + id + "/datos.json";
        state = sendDataToFirebase(ruta.c_str(), DatosCurrent.c_str(), false);
        if (LittleFS.exists("/" + nameFile)) {
            if (LittleFS.remove("/" + nameFile)) {
                Serial.println("Archivo eliminado correctamente");
            } else {
                Serial.println("Error al eliminar el archivo");
            }
        } else {
            Serial.println("El archivo no existe");
        }
        if(state){
            //get the file
            String fileURL = "https://xxviwfoyyskhnxqznlly.supabase.co/storage/v1/object/public/appembebidos/" + nameFile;

            bool success = downloadFileToFS(fileURL, "/" + nameFile);
            if (success)
            {
                Serial.println("audio cargado");
                Serial.println("Primer registro Agregado");

            }else{
                Serial.println("Ocurrio un problema al cargar el audio");
                server.send(300, "text/plain", "Ocurrio un problema");

                //Eliminar la peticion de tarea
                return;
            }
        
        }
        
        
    }else{
        String ruta = "/tareas/" + id + "/datos.json";
        state = sendDataToFirebase(ruta.c_str(),  datos.c_str(), false);
        Serial.println("no se cambio nada porque el segundo es mas tarde");

    }
    


    if(state){
        server.send(200, "text/plain", "Datos almacenados correctamente");
    }else{
        currentTarea.date = "";
        currentTarea.filename = "";
        currentTarea.time = time;
        currentTarea.tipoTarea = "";
        currentTarea.TareaId = "";
        currentTarea.RGB = "";
        server.send(300, "text/plain", "Ocurrio un problema");
    }

 
}


u_int16_t posW = 50;
void handleWifi()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");   
    Serial.print("Conectando a la red Wi-Fi ");
    Serial.println(ssid);
    Serial.print("Clave Wi-Fi ");
    Serial.println(password);
    Serial.print("...");
    WiFi.disconnect(); // Desconectar la red Wi-Fi anterior, si se estaba conectado
    WiFi.begin(ssid.c_str(), password.c_str(), 6);

    u_int16_t cnt = 0;
    while (WiFi.status() != WL_CONNECTED and cnt < 8)
    {
        vTaskDelay(1000);
        Serial.print(".");
        cnt++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        // guardar en memoria eeprom la ultima red conectada

        Serial.print("Guardando en memoria eeprom...");
/*        if (posW == 0)
            posW = 50;
        else
            posW = 0;*/
        String varsave = leerStringDeEEPROM(300);
        if (varsave == "a") {
            posW = 0;
            escribirStringEnEEPROM(300, "b");
        }
        else{
            posW=50;
            escribirStringEnEEPROM(300, "a");
        }
        escribirStringEnEEPROM(0 + posW, ssid);
        escribirStringEnEEPROM(100 + posW, password);
        // guardar en memoria eeprom la ultima red conectada
        

        Serial.println("Conexión establecida");
        server.send(200, "text/plain", WiFi.localIP().toString());

        Serial.println("Desvinculando de el appwifi");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);

        // Conectarse como cliente:
        WiFi.begin(ssid, password);
    }
    else
    {
        Serial.println("Conexión no establecida");
        server.send(200, "text/plain", "Conexión no establecida");
    }
}

void handleGetTask(){
    String response = getDatabase("");
    if(response == "ERROR"){
        server.send(300, "Ocurrio un problema al recibir las tareas");
        return;
    }

    server.send(200,"application/json", response.c_str());
}


void initServer(){
    server.on("/", handleRoot);
    server.on("/wifi", handleWifi);
    server.on("/upload", handleUpload);
    server.on("/task", handleGetTask);

    server.begin();
    Serial.println("Servidor web iniciado");
}   

bool lastRed()
{ // verifica si una de las 2 redes guardadas en la memoria eeprom se encuentra disponible
    // para conectarse en ese momento
    
    Serial.println("Inicia config");
    
    for (int psW = 0; psW <= 50; psW += 50)
    {
        String usu = leerStringDeEEPROM(0 + psW);
        String cla = leerStringDeEEPROM(100 + psW);
        usu = usu;
        Serial.println(usu);
        Serial.println(cla);
        WiFi.disconnect();
        WiFi.begin(usu.c_str(), cla.c_str(), 6);
        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED and cnt < 5)
        {
            vTaskDelay(1000);
            Serial.print(".");
            cnt++;
        }
        if (WiFi.status() == WL_CONNECTED){
            Serial.println("Conectado a Red Wifi");
            Serial.println(WiFi.localIP());
            break;
        }
        
    }
    if (WiFi.status() == WL_CONNECTED)
        return true;
    else
        return false;
}

void initAP(const char *apSsid, const char *apPassword)
{ // Nombre de la red Wi-Fi y  Contraseña creada por el ESP32
    Serial.begin(115200);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword);

    
    
}

void loopAP()
{


    server.handleClient();
}

Tarea parseTarea(String payload) {
  Tarea tarea = {""};

    int keyStart = payload.indexOf("\"") + 1;
    int keyEnd = payload.indexOf("\":{");
    tarea.TareaId = payload.substring(keyStart, keyEnd);

    int dateIndex = payload.indexOf("\"date\":\"");
    if (dateIndex != -1) {
        int valueStart = dateIndex + 8;
        int valueEnd = payload.indexOf("\"", valueStart);
        tarea.date = payload.substring(valueStart, valueEnd);
    }

    int fileIndex = payload.indexOf("\"filename\":\"");
    if (fileIndex != -1) {
        int valueStart = fileIndex + 12;
        int valueEnd = payload.indexOf("\"", valueStart);
        tarea.filename = payload.substring(valueStart, valueEnd);
    }

    int timeIndex = payload.indexOf("\"time\":\"");
    if (timeIndex != -1) {
        int valueStart = timeIndex + 8;
        int valueEnd = payload.indexOf("\"", valueStart);
        tarea.time = payload.substring(valueStart, valueEnd);
    }

    int tipoIndex = payload.indexOf("\"tipoTarea\":\"");
    if (tipoIndex != -1) {
        int valueStart = tipoIndex + 14;
        int valueEnd = payload.indexOf("\"", valueStart);
        tarea.tipoTarea = payload.substring(valueStart, valueEnd);
    }

    // Buscar campo "rgb" en minúscula
    int rgbIndex = payload.indexOf("\"rgb\":\"");
    if (rgbIndex != -1) {
        int valueStart = rgbIndex + 7;
        int valueEnd = payload.indexOf("\"", valueStart);
        tarea.RGB = payload.substring(valueStart, valueEnd);
    } else {
        tarea.RGB = "";
    }
  return tarea;
}




void intentoconexion(const char *apname, const char *appassword)
{
   

    Serial.begin(115200);
    EEPROM.begin(512);
    Serial.println("Se habilita el servidor");
    

    if (!lastRed())
    {                               // redirige a la funcion
        
        Serial.println("Conectarse desde su celular a la red creada");
        Serial.println("en el navegador colocar la ip:");
        Serial.println("192.168.4.1");
        initServer();
        initAP(apname, appassword); // nombre de wifi a generarse y contrasena
    }else{
        initServer();
    }
    
    while (WiFi.status() != WL_CONNECTED) // mientras no se encuentre conectado a un red
    {
        loopAP(); // genera una red wifi para que se configure desde la app movil
        
    }

    

    //monto el sistema de archivo
    /*
    if (!SPIFFS.begin(true)) {
        Serial.println("Error montando SPIFFS");
    }
        */
     if (!LittleFS.begin(false)) {  // intenta montar sin formatear
        Serial.println("No se pudo montar LittleFS, formateando...");
        if (LittleFS.format()) {      // formatea la partición
        Serial.println("Formateo exitoso");
        if (!LittleFS.begin(false)) {
            Serial.println("Error crítico al montar LittleFS luego del formateo");
            while (true); // detiene ejecución o maneja error
        }
        } else {
        Serial.println("Error formateando LittleFS");
        while (true); // detiene ejecución o maneja error
        }
    } else {
        Serial.println("LittleFS montado correctamente");
    }

    vTaskDelay(200);
    getCurrentTimestamp();
    //Obtengo la primera tarea si es q hay
    //String fechaActual = "20250728";
    char id[16];
    snprintf(id, sizeof(id), "%04d%02d%02d", year, month, day);
    String params = "?orderBy=\"$key\"&startAt=\"" + String(id) + "_\"&endAt=\"" + String(id) + "_\\uFFFF\"&limitToFirst=1";

    String response = getDatabase(params);
    Serial.println(response);

    currentTarea = parseTarea(response);
    if(currentTarea.date != ""){
        currentTime.tm_year = currentTarea.TareaId.substring(0, 4).toInt() - 1900;
        currentTime.tm_mon = currentTarea.TareaId.substring(4, 6).toInt() -1;
        currentTime.tm_mday = currentTarea.TareaId.substring(6, 8).toInt();
        currentTime.tm_hour = currentTarea.TareaId.substring(9, 11).toInt();
        currentTime.tm_min = currentTarea.TareaId.substring(11, 13).toInt();
        currentTime.tm_sec = 0;

    }
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
        
   

    

}



