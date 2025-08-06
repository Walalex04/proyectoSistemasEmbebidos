


#ifndef MATRIZ
#define MATRIZ

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>    //  max7219 library
#include "pines.h"


// Uncomment according to your hardware type
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW


Max72xxPanel matrix = Max72xxPanel(pinCS, 3, 1);


// Un patrón 3x5 para dígitos
byte numbers[10][5] = {
  {B111, B101, B101, B101, B111}, // 0
  {B010, B110, B010, B010, B111}, // 1
  {B111, B001, B111, B100, B111}, // 2
  {B111, B001, B111, B001, B111}, // 3
  {B101, B101, B111, B001, B001}, // 4
  {B111, B100, B111, B001, B111}, // 5
  {B111, B100, B111, B101, B111}, // 6
  {B111, B001, B010, B010, B010}, // 7
  {B111, B101, B111, B101, B111}, // 8
  {B111, B101, B111, B001, B111}  // 9
};

// Dos puntos :
byte colon[5] = {
  B000,
  B010,
  B000,
  B010,
  B000
};

// Punto '.'
byte punto[5] = {
  B000,
  B000,
  B000,
  B000,
  B010
};

// Signo '-'
byte menos[5] = {
  B000,
  B000,
  B111,
  B000,
  B000
};

void initMatriz(){
    matrix.setIntensity(7); // Set brightness between 0 and 15
}

void handleNewMessages(String tx) {


    String text = tx;

    //Draw scrolling text
    int spacer = 1;                            // This will scroll the string
    int width = 7 + spacer;                    // The font width is 5 pixels
    for ( int i = 0 ; i < width * text.length() + width + 2; i++ ) {
      matrix.fillScreen(0);

      int letter = i / width;
      int x = (matrix.width() - 1) - i % width;
      int y = (matrix.height() - 8) / 2; // center the text vertically

      while ( x + width - spacer >= 0 && letter >= 0 ) {
        if ( letter < text.length() ) {
          matrix.drawChar(x, y, text[letter], 1, 0, 1);
        }
        letter--;
        x -= width;
      }
      matrix.write(); // Send bitmap to display

      delay(100);
    }
  
}


// Dibuja un dígito en (x,y)
void drawDigit(byte digit[5], int x, int y) {
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      if (digit[row] & (1 << (2 - col))) {
        matrix.drawPixel(x + col, y + row, HIGH);
      }
    }
  }
}

void mostrarHora(int h1, int h2, int m1, int m2) {
  matrix.fillScreen(LOW);

  int totalWidth = 19;
  int panelWidth = 24;
  int offset = (panelWidth - totalWidth) / 2; // -> 2 px

  int x = offset;

  drawDigit(numbers[h1], x, 1);
  x += 4;
  drawDigit(numbers[h2], x, 1);
  x += 4;
  drawDigit(colon, x, 1);
  x += 4;
  drawDigit(numbers[m1], x, 1);
  x += 4;
  drawDigit(numbers[m2], x, 1);

  matrix.write();
}

void mostrarNumero(float valor) {
  matrix.fillScreen(LOW);

  char buffer[8];
  dtostrf(valor, 0, 1, buffer); // máximo 1 decimal

  int len = strlen(buffer);

  // Cada símbolo = 3 px + 1 px espacio
  int totalWidth = len * 3 + (len - 1) * 1; 
  int panelWidth = 24;
  int offset = (panelWidth - totalWidth) / 2;

  int x = offset;

  for (int i = 0; i < len; i++) {
    char c = buffer[i];
    if (c >= '0' && c <= '9') {
      drawDigit(numbers[c - '0'], x, 1);
    } else if (c == '.') {
      drawDigit(punto, x, 1);
    } else if (c == '-') {
      drawDigit(menos, x, 1);
    }
    x += 4; // 3 ancho + 1 espacio
  }

  matrix.write();
}


#endif