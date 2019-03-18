/*
  Heavily inspired by the SparkFun example by Nathan Seidle
  https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

   The MLX90640 requires some hefty calculations and larger arrays. You will need a microcontroller with 20,000 bytes or more of RAM.

  This relies on the driver written by Melexis and can be found at:
  https://github.com/melexis/mlx90640-library

  RGB matrix drivers from Adafruit [connected to Arduino through Adafruit RGB Matrix Shield]:
  https://github.com/adafruit/RGB-matrix-Panel

  Hardware Connections:
  Tested using a Metro M0 Express from Adafruit and a MLX90640 Thermal Camera Breakout from Pimoroni
*/

#include <RGBmatrixPanel.h>
#include <Wire.h>

#define CLK  8   // USE THIS ON ARDUINO UNO, ADAFRUIT METRO M0, etc.
//#define CLK A4 // USE THIS ON METRO M4 (not M0)
//#define CLK 11 // USE THIS ON ARDUINO MEGA
#define OE   9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

const byte MLX90640_address = 0x33; //Default 7-bit unshifted address of the MLX90640

#define TA_SHIFT 8 //Default shift for MLX90640 in open air

float mlx90640To[768];
paramsMLX90640 mlx90640;

int histogram_arr[32] = {0};

void setup()
{
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz

  /*
  Serial.begin(115200); //Fast serial as possible
  
  while (!Serial); //Wait for user to open terminal
  //Serial.println("MLX90640 IR Array Example");

  if (isConnected() == false)
  {
    Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
    while (1);
  }
  */
  //Get device parameters - We only have to do this once
  int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);

  
  //if (status != 0)
    //Serial.println("Failed to load system parameters");

  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  //if (status != 0)
    //Serial.println("Parameter extraction failed");
  
  //Once params are extracted, we can release eeMLX90640 array

  MLX90640_SetRefreshRate(MLX90640_address, 0x02); //Set rate to 2Hz
  //MLX90640_SetRefreshRate(MLX90640_address, 0x03); //Set rate to 4Hz
  //MLX90640_SetRefreshRate(MLX90640_address, 0x07); //Set rate to 64Hz

  matrix.begin();
}

void loop()
{
  long startTime = millis();
  for (byte x = 0 ; x < 2 ; x++)
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);

    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

    float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
    float emissivity = 0.95;

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
  }
  long stopTime = millis();
/*
  float min_T = 300, max_T = -40;

  for (int i = 0 ; i < 768 ; i++)
  {
    if (mlx90640To[i] < min_T) {
      min_T = mlx90640To[i];
    } else if (mlx90640To[i] > max_T) {
      max_T = mlx90640To[i];
    }
  }

  min_T = floor(min_T);
  max_T = ceil(max_T);

  float diff_T = max_T - min_T;
*/
  int      x_pixel, y_pixel, hue, testTemp;
  float    dx, dy, d;
  uint8_t  low_hue, high_hue, sat, val;
  uint16_t c;
  //Reset histogram array
  for (int h = 0; h < 32; h++) {
    histogram_arr[h] = 0;
  }
  
  //matrix.begin();
  
  sat = 100;
  val = 127;

  low_hue = 250;
  high_hue = 359;

  for (int j = 0 ; j < 768 ; j++)
  {
    //if(x % 8 == 0) Serial.println();
    //Serial.print(mlx90640To[j], 2);
    //Serial.print(",");
    //TODO - replace; convert to colour value
    testTemp = floor(mlx90640To[j]); 
    x_pixel = j % 32;
    y_pixel = floor(j/32);
    //hue = (floor(diff_T/(mlx90640To[j]-min_T)));
    //c = matrix.ColorHSV(hue, sat, val, true);
    //hue = ceil((diff_T/(mlx90640To[j]-min_T))*15);
    //c = matrix.Color444(hue,0,15);
    
    c = findColor(testTemp);
    matrix.drawPixel(x_pixel,y_pixel,c);
    amendHistogramArr(testTemp);
  }

  //Clear legend area
   matrix.fillRect(0, 25, 32, 8, matrix.Color333(0, 0, 0));

  //Draw legend
  matrix.drawPixel(0,31,matrix.Color333(1,1,1));
  matrix.drawPixel(1,31,matrix.Color333(0,0,7));
  matrix.drawPixel(2,31,matrix.Color333(0,1,7));
  matrix.drawPixel(3,31,matrix.Color333(0,2,7));
  matrix.drawPixel(4,31,matrix.Color333(0,3,7));
  matrix.drawPixel(5,31,matrix.Color333(0,4,7));
  matrix.drawPixel(6,31,matrix.Color333(0,5,7));
  matrix.drawPixel(7,31,matrix.Color333(0,6,7));
  matrix.drawPixel(8,31,matrix.Color333(0,7,7));
  matrix.drawPixel(9,31,matrix.Color333(1,7,6));
  matrix.drawPixel(10,31,matrix.Color333(2,7,5));
  matrix.drawPixel(11,31,matrix.Color333(3,7,4));
  matrix.drawPixel(12,31,matrix.Color333(4,7,3));
  matrix.drawPixel(13,31,matrix.Color333(5,7,2));
  matrix.drawPixel(14,31,matrix.Color333(7,7,0));
  matrix.drawPixel(15,31,matrix.Color333(5,7,0));
  matrix.drawPixel(16,31,matrix.Color333(3,7,0));
  matrix.drawPixel(17,31,matrix.Color333(0,7,0));
  matrix.drawPixel(18,31,matrix.Color333(4,5,1));
  matrix.drawPixel(19,31,matrix.Color333(5,4,1));
  matrix.drawPixel(20,31,matrix.Color333(6,3,1));
  matrix.drawPixel(21,31,matrix.Color333(7,2,1));
  matrix.drawPixel(22,31,matrix.Color333(7,1,0));
  matrix.drawPixel(23,31,matrix.Color333(7,0,0));
  matrix.drawPixel(24,31,matrix.Color333(7,0,1));
  matrix.drawPixel(25,31,matrix.Color333(7,0,2));
  matrix.drawPixel(26,31,matrix.Color333(7,0,3));
  matrix.drawPixel(27,31,matrix.Color333(7,0,4));
  matrix.drawPixel(28,31,matrix.Color333(7,0,5));
  matrix.drawPixel(29,31,matrix.Color333(7,0,6));
  matrix.drawPixel(30,31,matrix.Color333(7,0,7));
  matrix.drawPixel(31,31,matrix.Color333(7,7,7));
  /*
  char str [3];
  matrix.setCursor(1,25);
  matrix.setTextSize(1);

  matrix.setTextColor(matrix.ColorHSV(low_hue,sat,val,true));
  matrix.print(sprintf(str, "%g", min_T));

  matrix.setCursor(16,25);
  
  matrix.setTextColor(matrix.ColorHSV(high_hue,sat,val,true));
  matrix.print(sprintf(str, "%g", max_T));
  */
  //Serial.println("");

  //Draw histogram
  int hist_brightness = 5;
  for (int k=0; k < 32; k++) {
    if (histogram_arr[k] < 1) {
      //Draw nothing
    } else if (histogram_arr[k] < 128) {
      matrix.drawPixel(k,30,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
    } else if (histogram_arr[k] < 256) {
      matrix.drawPixel(k,29,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,30,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
    } else if (histogram_arr[k] < 384) {
      matrix.drawPixel(k,28,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,29,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,30,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
    } else if (histogram_arr[k] < 512) {
      matrix.drawPixel(k,27,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,28,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,29,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,30,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
    } else if (histogram_arr[k] < 640) {
      matrix.drawPixel(k,26,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,27,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,28,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,29,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,30,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
    } else {
      matrix.drawPixel(k,25,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,26,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,27,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,28,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,29,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
      matrix.drawPixel(k,30,matrix.Color333(hist_brightness, hist_brightness, hist_brightness));
    }
    
  }
}

uint16_t findColor(int temp) {
  int ret_temp;
  if (temp < 0) {
    ret_temp = matrix.Color333(1,1,1);
  } else if (temp < 3) {
    ret_temp = matrix.Color333(0,0,7);
  } else if (temp < 6) {
    ret_temp = matrix.Color333(0,1,7);
  } else if (temp < 9) {
    ret_temp = matrix.Color333(0,2,7);   
  } else if (temp < 12) {
    ret_temp = matrix.Color333(0,3,7);
  } else if (temp < 15) {
    ret_temp = matrix.Color333(0,4,7);
  } else if (temp < 18) {
    ret_temp = matrix.Color333(0,5,7);
  } else if (temp < 21) {
    ret_temp = matrix.Color333(0,6,7);
  } else if (temp < 24) {
    ret_temp = matrix.Color333(0,7,7);
  } else if (temp < 27) {
    ret_temp = matrix.Color333(1,7,6);
  } else if (temp < 30) {
    ret_temp = matrix.Color333(2,7,5);
  } else if (temp < 33) {
    ret_temp = matrix.Color333(3,7,4);
  } else if (temp < 36) {
    ret_temp = matrix.Color333(4,7,3);
  } else if (temp < 39) {
    ret_temp = matrix.Color333(5,7,2);
  } else if (temp < 42) {
    ret_temp = matrix.Color333(7,7,0);
  } else if (temp < 45) {
    ret_temp = matrix.Color333(5,7,0);
  } else if (temp < 48) {
    ret_temp = matrix.Color333(3,7,0);
  } else if (temp < 51) {
    ret_temp = matrix.Color333(0,7,0);
  } else if (temp < 54) {
    ret_temp = matrix.Color333(4,5,1);
  } else if (temp < 57) {
    ret_temp = matrix.Color333(5,4,1);
  } else if (temp < 60) {
    ret_temp = matrix.Color333(6,3,1);
  } else if (temp < 63) {
    ret_temp = matrix.Color333(7,2,1);
  } else if (temp < 66) {
    ret_temp = matrix.Color333(7,1,0);
  } else if (temp < 69) {
    ret_temp = matrix.Color333(7,0,0);
  } else if (temp < 72) {
    ret_temp = matrix.Color333(7,0,1);
  } else if (temp < 75) {
    ret_temp = matrix.Color333(7,0,2);
  } else if (temp < 78) {
    ret_temp = matrix.Color333(7,0,3);
  } else if (temp < 81) {
    ret_temp = matrix.Color333(7,0,4);
  } else if (temp < 84) {
    ret_temp = matrix.Color333(7,0,5);
  } else if (temp < 87) {
    ret_temp = matrix.Color333(7,0,6);
  } else if (temp < 90) {
    ret_temp = matrix.Color333(7,0,7);
  } else {
    ret_temp = matrix.Color333(7,7,7);
  }

  return ret_temp;
}
 
int findHue(int percent) {
  int hue;
  if (percent < 12) {
    hue = 180;
  } else if (percent < 24){ 
    hue = 225;
  } else if (percent < 36){ 
    hue = 270;
  } else if (percent < 48){ 
    hue = 315;
  } else if (percent < 60){ 
    hue = 0;
  } else if (percent < 72){ 
    hue = 45;
  } else if (percent < 84){ 
    hue = 90;
  } else if (percent < 96){ 
    hue = 135;
  } else {
    hue = 180;
  }

    return hue;
}

int findSat(int percent) {
  int sat;
  int test = floor(percent % 12);
  if (test < 0) {
    sat = 0;
  } else if (test < 3) {
    sat = 63;
  } else if (test < 6) {
    sat = 127;
  } else if (test < 9) {
    sat = 195;
  } else {
    sat = 255;
  }
  return sat;
}

int findVal(int percent) {
  int val;
  if (percent < 0) {
    val = 0;
  } else if (percent > 100) {
    val = 127;
  } else {
    val = 127;
  }
  return val;
}

void amendHistogramArr (int temp) {
    if (temp <= 0) {
      histogram_arr[0]++;
    } else if (temp < 3) {
      histogram_arr[1]++;
    } else if (temp < 6) {
      histogram_arr[2]++;
    } else if (temp < 9) {
      histogram_arr[3]++;
    } else if (temp < 12) {
      histogram_arr[4]++;
    } else if (temp < 15) {
      histogram_arr[5]++;
    } else if (temp < 18) {
      histogram_arr[6]++;
    } else if (temp < 21) {
      histogram_arr[7]++;
    } else if (temp < 24) {
      histogram_arr[8]++;
    } else if (temp < 27) {
      histogram_arr[9]++;
    } else if (temp < 30) {
      histogram_arr[10]++;
    } else if (temp < 33) {
      histogram_arr[11]++;
    } else if (temp < 36) {
      histogram_arr[12]++;
    } else if (temp < 39) {
      histogram_arr[13]++;
    } else if (temp < 42) {
      histogram_arr[14]++;
    } else if (temp < 45) {
      histogram_arr[15]++;
    } else if (temp < 48) {
      histogram_arr[16]++;
    } else if (temp < 51) {
      histogram_arr[17]++;
    } else if (temp < 54) {
      histogram_arr[18]++;
    } else if (temp < 57) {
      histogram_arr[19]++;
    } else if (temp < 60) {
      histogram_arr[20]++;
    } else if (temp < 63) {
      histogram_arr[21]++;
    } else if (temp < 66) {
      histogram_arr[22]++;
    } else if (temp < 69) {
      histogram_arr[23]++;
    } else if (temp < 72) {
      histogram_arr[24]++;
    } else if (temp < 75) {
      histogram_arr[25]++;
    } else if (temp < 78) {
      histogram_arr[26]++;
    } else if (temp < 81) {
      histogram_arr[27]++;
    } else if (temp < 84) {
      histogram_arr[28]++;
    } else if (temp < 87) {
      histogram_arr[29]++;
    } else if (temp < 90) {
      histogram_arr[30]++;
    } else {
      histogram_arr[31]++;
    } 
  }


//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected()
{
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);
}
