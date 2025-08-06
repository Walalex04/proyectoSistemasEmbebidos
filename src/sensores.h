#include <DHT.h>
#include "pines.h"

DHT dht(pinDHT, DHTTYPE);

void initSensors(){
    dht.begin();
}

float getTemperature(){
    return dht.readTemperature();
}

float getHumidity(){
    return dht.readHumidity();
}

float getCO2(){
    int adcValue = analogRead(ANALOGPINMQ);
    float Vout = adcValue * (3.3 / 4095.0);
    float Rs = (5.0 - Vout) * 10000 / Vout;
    float ratio = Rs / 3000;
    float ppm = pow(10, (log10(ratio) - 0.45) / -0.42);
    return ppm;
}