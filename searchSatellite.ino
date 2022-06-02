#include <Camera.h>
#include <SDHCI.h>
// https://github.com/TE-YoshinoriOota/Spresense-LowPower-EdgeAI/tree/main/Libraries からzipをダウンロードしてarduion IDEの機能でインストールする
#include "Adafruit_ILI9341.h"

#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240

#define TFT_DC 9
#define TFT_CS 10

Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

SDClass SD;

void printCameraError(enum CamErr err) {
  Serial.print("Camera Error: ");
  switch (err)
    {
      case CAM_ERR_NO_DEVICE:
        Serial.println("No Device");
        break;
      case CAM_ERR_ILLEGAL_DEVERR:
        Serial.println("Illegal device error");
        break;
      case CAM_ERR_ALREADY_INITIALIZED:
        Serial.println("Already initialized");
        break;
      case CAM_ERR_NOT_INITIALIZED:
        Serial.println("Not initialized");
        break;
      case CAM_ERR_NOT_STILL_INITIALIZED:
        Serial.println("Still picture not initialized");
        break;
      case CAM_ERR_CANT_CREATE_THREAD:
        Serial.println("Failed to create thread");
        break;
      case CAM_ERR_INVALID_PARAM:
        Serial.println("Invalid parameter");
        break;
      case CAM_ERR_NO_MEMORY:
        Serial.println("No memory");
        break;
      case CAM_ERR_USR_INUSED:
        Serial.println("Buffer already in use");
        break;
      case CAM_ERR_NOT_PERMITTED:
        Serial.println("Operation not permitted");
        break;
      default:
        break;
    }
}

char fname[16] = {0};

void takePhoto() {
  static int g_counter = 0;
  sprintf(fname, "%03d.jpg", g_counter);
  if (SD.exists(fname)) {
    SD.remove(fname);
  }
  File imageFile = SD.open(fname, FILE_WRITE);
  CamImage img = theCamera.takePicture();
  if (img.isAvailable()) {
    imageFile.write(img.getImgBuff(), img.getImgSize());
    imageFile.close();
    Serial.println("Saved an image as " + String(fname));
  } else {
    Serial.println("Camera is not available");
  }
  ++g_counter;
}

#define PIXEL_THRESHOLD 150
#define SQUARE_SIZE_THRESHOLD 400

void CamCB(CamImage img) {
  if (!img.isAvailable()) {
    return;
  }
  uint16_t *yuvImageBuff = (uint16_t *)img.getImgBuff();
  int16_t offsetX = 0;
  int16_t widthOfRegion = 0;
  int16_t offsetY = 0;
  int16_t heightOfRegion = 0;
  bool success;
  success = getOffsetXAndWidthOfRegion(yuvImageBuff, PIXEL_THRESHOLD, &offsetX, &widthOfRegion);
  if (!success) {
    Serial.println("x detection error");
    goto result;
  }
  success = getOffsetYAndHeightOfRegion(yuvImageBuff, PIXEL_THRESHOLD, &offsetY, &heightOfRegion);
  if (!success) {
    Serial.println("y detection error");
    goto result;
  }
  
result:
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);
  uint16_t *rgbImageBuff = (uint16_t *)img.getImgBuff();
  if (success && widthOfRegion * heightOfRegion > SQUARE_SIZE_THRESHOLD) {
    digitalWrite(LED0, HIGH);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    drawBox(rgbImageBuff, offsetX, offsetY, widthOfRegion, heightOfRegion, 2, ILI9341_RED);
    takePhoto();
  } else {
    digitalWrite(LED0, LOW);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
  }
  display.drawRGBBitmap(0, 0, rgbImageBuff, CAMERA_WIDTH, CAMERA_HEIGHT);
}

void setup() {
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  Serial.begin(115200);
  display.begin();
  display.setRotation(3);
  CamErr err = theCamera.begin();
  if (err != CAM_ERR_SUCCESS) {
    printCameraError(err);
  }
  while (!SD.begin()) {
    Serial.println("Insert SD Card");
  }
  theCamera.startStreaming(true, CamCB);
  theCamera.setStillPictureImageFormat(
     CAM_IMGSIZE_QUADVGA_H,
     CAM_IMGSIZE_QUADVGA_V,
     CAM_IMAGE_PIX_FMT_JPG);
}

void loop() {
}
