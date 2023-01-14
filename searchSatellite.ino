#include <stdlib.h>
#include <Camera.h>
#include <SDHCI.h>
#include <GNSS.h>
#include <LowPower.h>
#include <MPMutex.h>
// https://github.com/TE-YoshinoriOota/Spresense-LowPower-EdgeAI/tree/main/Libraries からzipをダウンロードしてarduion IDEの機能でインストールする
#include "Adafruit_ILI9341.h"

#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240

#define TFT_DC 9
#define TFT_CS 10

// GPS setting
#define STRING_BUFFER_SIZE  128       /**< %Buffer size */

#define RESTART_CYCLE       (60 * 5)  /**< positioning test term */

static SpGnss Gnss;                   /**< SpGnss object */

/**
 * @enum ParamSat
 * @brief Satellite system
 */
enum ParamSat {
  eSatGps,            /**< GPS                     World wide coverage  */
  eSatGlonass,        /**< GLONASS                 World wide coverage  */
  eSatGpsSbas,        /**< GPS+SBAS                North America        */
  eSatGpsGlonass,     /**< GPS+Glonass             World wide coverage  */
  eSatGpsBeidou,      /**< GPS+BeiDou              World wide coverage  */
  eSatGpsGalileo,     /**< GPS+Galileo             World wide coverage  */
  eSatGpsQz1c,        /**< GPS+QZSS_L1CA           East Asia & Oceania  */
  eSatGpsGlonassQz1c, /**< GPS+Glonass+QZSS_L1CA   East Asia & Oceania  */
  eSatGpsBeidouQz1c,  /**< GPS+BeiDou+QZSS_L1CA    East Asia & Oceania  */
  eSatGpsGalileoQz1c, /**< GPS+Galileo+QZSS_L1CA   East Asia & Oceania  */
  eSatGpsQz1cQz1S,    /**< GPS+QZSS_L1CA+QZSS_L1S  Japan                */
};

/* Set this parameter depending on your current region. */
// static enum ParamSat satType =  eSatGps;
static enum ParamSat satType =  eSatGpsQz1cQz1S; // みちびきとの接続
static void init_gps() {
  int error_flag = 0;
  /* Activate GNSS device */
  int result = Gnss.begin();
  if (result != 0)
  {
    Serial.println("Gnss begin error!!");
    error_flag = 1;
  }
  else
  {
    /* Setup GNSS
     *  It is possible to setup up to two GNSS satellites systems.
     *  Depending on your location you can improve your accuracy by selecting different GNSS system than the GPS system.
     *  See: https://developer.sony.com/develop/spresense/developer-tools/get-started-using-nuttx/nuttx-developer-guide#_gnss
     *  for detailed information.
    */
    switch (satType)
    {
    case eSatGps:
      Gnss.select(GPS);
      break;

    case eSatGpsSbas:
      Gnss.select(GPS);
      Gnss.select(SBAS);
      break;

    case eSatGlonass:
      Gnss.select(GLONASS);
      break;

    case eSatGpsGlonass:
      Gnss.select(GPS);
      Gnss.select(GLONASS);
      break;

    case eSatGpsBeidou:
      Gnss.select(GPS);
      Gnss.select(BEIDOU);
      break;

    case eSatGpsGalileo:
      Gnss.select(GPS);
      Gnss.select(GALILEO);
      break;

    case eSatGpsQz1c:
      Gnss.select(GPS);
      Gnss.select(QZ_L1CA);
      break;

    case eSatGpsQz1cQz1S:
      Gnss.select(GPS);
      Gnss.select(QZ_L1CA);
      Gnss.select(QZ_L1S);
      break;

    case eSatGpsBeidouQz1c:
      Gnss.select(GPS);
      Gnss.select(BEIDOU);
      Gnss.select(QZ_L1CA);
      break;

    case eSatGpsGalileoQz1c:
      Gnss.select(GPS);
      Gnss.select(GALILEO);
      Gnss.select(QZ_L1CA);
      break;

    case eSatGpsGlonassQz1c:
    default:
      Gnss.select(GPS);
      Gnss.select(GLONASS);
      Gnss.select(QZ_L1CA);
      break;
    }
    /* Start positioning */
    result = Gnss.start(COLD_START);
    if (result != 0)
    {
      Serial.println("Gnss start error!!");
      error_flag = 1;
    }
  }
}

/**
 * 
 * シリアルに毎回接続する仕方だと必ず失敗する。
 * @brief %Print position information.
 */
static void print_pos(SpNavData *pNavData)
{
  char StringBuffer[STRING_BUFFER_SIZE];

  /* print time */
  snprintf(StringBuffer, STRING_BUFFER_SIZE, "%04d/%02d/%02d ", pNavData->time.year, pNavData->time.month, pNavData->time.day);
  Serial.print(StringBuffer);

  snprintf(StringBuffer, STRING_BUFFER_SIZE, "%02d:%02d:%02d.%06ld, ", pNavData->time.hour, pNavData->time.minute, pNavData->time.sec, pNavData->time.usec);
  Serial.print(StringBuffer);

  /* print satellites count */
  snprintf(StringBuffer, STRING_BUFFER_SIZE, "numSat:%2d, ", pNavData->numSatellites);
  Serial.print(StringBuffer);

  /* print position data */
  if (pNavData->posFixMode == FixInvalid)
  {
    Serial.print("No-Fix, ");
  }
  else
  {
    Serial.print("Fix, ");
  }
  if (pNavData->posDataExist == 0)
  {
    Serial.print("No Position");
  }
  else
  {
    Serial.print("Lat=");
    Serial.print(pNavData->latitude, 6);
    Serial.print(", Lon=");
    Serial.print(pNavData->longitude, 6);
  }

  Serial.println("");
}

// display setting
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

// base64 util
const uint8_t base64Table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/**
 * https://gist.github.com/ksasao/89b2b83df153386026ea69d813fd584e
 */
void base64EncodeSerialWrite(uint8_t* data, size_t dataLength){
  while (dataLength > 0) {
    if (dataLength >= 3) {
      Serial.write(base64Table[data[0] >> 2]);
      Serial.write(base64Table[((data[0]&0x3) << 4) | (data[1] >> 4)]);
      Serial.write(base64Table[((data[1]&0xF) << 2) | (data[2] >> 6)]);
      Serial.write(base64Table[data[2] & 0x3F]);
      data += 3;
      dataLength -= 3;
    } else if (dataLength == 2) {
      Serial.write(base64Table[data[0] >> 2]);
      Serial.write(base64Table[((data[0]&0x3) << 4) | (data[1] >> 4)]);
      Serial.write(base64Table[(data[1]&0xF) << 2]);
      Serial.write('=');
      dataLength = 0;
    } else {
      Serial.write(base64Table[data[0] >> 2]);
      Serial.write(base64Table[(data[0]&0x3) << 4]);
      Serial.write('=');
      Serial.write('=');
      dataLength = 0;
    }
  }
  Serial.write('\n');
  Serial.flush();
}

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

// CamImage savedImg; これ二つあるとなぜかうまくいかない。
uint8_t *savedBuff = NULL;
size_t savedSize;
// 撮影時のGPSデータ
SpNavData NavData;
// カメラのスレッドとloopのスレッドは別なのでロックしないと画像が混じる
MPMutex sendingSerialMutex(MP_MUTEX_ID0);
void takePhoto() {
  if (sendingSerialMutex.Trylock() != 0) {
    return;
  }
  CamImage img = theCamera.takePicture();
  if (!img.isAvailable() && img.getImgSize() == 0) {
    Serial.println("Image is not available");
    return;
  }
  if (savedBuff != NULL) {
    free(savedBuff);
  }
  savedSize = img.getImgSize();
  savedBuff = malloc(sizeof(uint8_t) * savedSize);
  memcpy(savedBuff, img.getImgBuff(), sizeof(uint8_t) * savedSize);
  Gnss.getNavData(&NavData);
  sendingSerialMutex.Unlock();
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
    goto result;
  }
  success = getOffsetYAndHeightOfRegion(yuvImageBuff, PIXEL_THRESHOLD, &offsetY, &heightOfRegion);
  if (!success) {
    goto result;
  }
  
result:
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);
  uint16_t *rgbImageBuff = (uint16_t *)img.getImgBuff();
  if (success && widthOfRegion * heightOfRegion > SQUARE_SIZE_THRESHOLD) {
    digitalWrite(LED3, HIGH);
    drawBox(rgbImageBuff, offsetX, offsetY, widthOfRegion, heightOfRegion, 2, ILI9341_RED);
    takePhoto();
  } else {
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
  theCamera.startStreaming(true, CamCB);
  // 画像が大きすぎると送信に時間がかかる
  theCamera.setStillPictureImageFormat(
     CAM_IMGSIZE_QVGA_H,
     CAM_IMGSIZE_QVGA_V,
     CAM_IMAGE_PIX_FMT_JPG);
  init_gps();
  LowPower.begin();
}

#define LIST_LENGTH 100
int list[LIST_LENGTH];
int head = 0;
void push(int val) {
  list[head] = val;
  head = (head + 1) % LIST_LENGTH;
}

int decrement(int i) {
  return i == 0 ? LIST_LENGTH - 1 : i - 1;
}

void sendList() {
  for (int i = decrement(head); i != head; i = decrement(i)) {
    Serial.print(list[i]);
    Serial.print(",");
  }
  Serial.print("\n");
}

void send_serial() {
  if (savedSize > 0) {
    // カメラのスレッドとloopのスレッドは別
    // なので送信時にロックしないと画像が混じる
    while (sendingSerialMutex.Trylock() != 0) {}
    print_pos(&NavData);
    base64EncodeSerialWrite(savedBuff, savedSize);
    sendList();
    sendingSerialMutex.Unlock();
  } else {
    Serial.println("image not found");
  }
  digitalWrite(LED0, LOW);
  digitalWrite(LED1, LOW);
}

void loop() {
  static int LoopCount = 0;
  int inputchar = Serial.read();
  if (inputchar != -1) {
    if (inputchar == '>') {
      LoopCount++;
      digitalWrite(LED0, HIGH);
      digitalWrite(LED1, HIGH);
      send_serial();
    }
  }
  push(LowPower.getVoltage());
  
  if (LoopCount >= RESTART_CYCLE)
  {
    int error_flag = 0;

    /* Restart GNSS. */
    if (Gnss.stop() != 0)
    {
      Serial.println("Gnss stop error!!");
      error_flag = 1;
    }
    else if (Gnss.end() != 0)
    {
      Serial.println("Gnss end error!!");
      error_flag = 1;
    }

    if (Gnss.begin() != 0)
    {
      Serial.println("Gnss begin error!!");
      error_flag = 1;
    }
    else if (Gnss.start(HOT_START) != 0)
    {
      Serial.println("Gnss start error!!");
      error_flag = 1;
    }

    LoopCount = 0;

    /* Set error LED. */
    if (error_flag == 1)
    {
      exit(0);
    }
  }
}
