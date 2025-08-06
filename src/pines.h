
#ifndef PINES
#define PINES


#define pinCS 5
#define pinDHT 14
#define ANALOGPINMQ 36
#define PINCHANGE 15
#define PINCHANGECOLOR 21
#define PINSTOPALARM 3
#define DACPIN 25
#define PINROJO 16
#define PINVERDE 4
#define PINAZUL 2

#define CHANNEL1 0
#define CHANNEL2 1
#define CHANEEL3 2

#define DHTTYPE DHT11 

//Define conection with database
#define API_KEY "AIzaSyAE_2WBjUClxyiURTuwFRJ8HkyntFvHPFw"
#define DATABASE_URL "https://sistemasembebidos-c9b89-default-rtdb.firebaseio.com/"


//variables for RTOS
#define PRO_CPU 0
#define APP_CPU 1

#define NOAFF_CPU tskNO_AFFINITY

//defines stoage

#define SUPABASE_URL "xxviwfoyyskhnxqznlly.supabase.co"
#define SUPABASE_BUCKET "appembebidos"
#define SUPABASE_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Inh4dml3Zm95eXNraG54cXpubGx5Iiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc1MzQ4ODE4MiwiZXhwIjoyMDY5MDY0MTgyfQ.IF60E9IYYNebWVaZNUjKcqa0_w1sBYauOKKQerxyLfI"
#define ANON_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Inh4dml3Zm95eXNraG54cXpubGx5Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTM0ODgxODIsImV4cCI6MjA2OTA2NDE4Mn0.1wOugGNrbFtZUKdR2a_ilrVQ3ga4yTAmV2pSpolyezk"



//varaibles
enum AppState {
  INITCONFIG,
  TIME, 
  TEMPERATURE,
  HUMIDITY,
  CO2
};


static AppState ActualState = TIME;
static String message = "";
static char hour = 00;
static char minutes = 00;
static int year = 2025;
static int month = 0;
static int day = 0;


static float temp = 0.0;
static float hum = 0.0;
static float ppm = 000;
volatile bool flagActualizar = false;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;    // Ejemplo: Ecuador UTC-5
const int daylightOffset_sec = 0;         // Sin horario de verano


//leds
static char ledVerde = 150;
static char ledRojo = 150;
static char ledAzul = 150;

static volatile char mode = 1; //0 parpadea

static char colores[3][3] ={{100, 200, 255}, {255, 0, 120}, {25, 35,89}};

static String DatosCurrent = "";



struct Tarea {
    String date;       // Ejemplo: "10-10-10"
    String filename;   // Ejemplo: "top1.wav"
    String time;       // Ejemplo: "20:15"
    String tipoTarea;  // Ejemplo: "Universidad"
    String TareaId;
    String RGB;
};

static struct Tarea currentTarea = {""};
static volatile bool playAudio = 0;

static char lastRED = 0;
static char lastGreen = 0;
static char lastBlue = 0;
#endif