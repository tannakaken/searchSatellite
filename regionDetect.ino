/**
 * 全部のピクセルを検査していては遅すぎるので、適当に間引きしてピクセルを検査する。
 */
#define STEP 32

/**
 * yuvをグレイスケールに変換する。
 */
uint8_t grayScale(uint16_t yuv) {
   return (uint8_t)(((yuv & 0xf000) >> 8) | ((yuv & 0x00f0) >> 4));
}

/**
 * 横方向の最大領域の開始座標(x)と横幅を検出する
 * 
 * @params yuvImage YUVの画像
 * @params out_offsetX 最大領域の開始x座標を得る
 * @params out_regionWidth 最大領域の横幅を得る
 * @return 正常に終了したか
 */
bool getOffsetXAndWidthOfRegion(
  uint16_t *yuvImage, 
  uint8_t threshold,
  int16_t* out_offsetX,
  int16_t* out_regionWidth
  ) {
  // 引数チェック
  if (yuvImage == NULL) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }

  int16_t max_regionWidth = -1;  // 最大幅（水平方向）を格納
  int16_t max_endXCoord = -1; // 最大幅の終了座標
  for (int i = 0; i < CAMERA_HEIGHT; i += STEP) {
    int regionWidth = 0;
    for (int j = 0; j < CAMERA_WIDTH; j += STEP) {
      int n = i * CAMERA_WIDTH + j;
      // ピクセルの出力が閾値より上の場合は加算
      if (grayScale(yuvImage[n]) > threshold) {
        regionWidth += STEP;
        if (regionWidth > max_regionWidth) {
          max_regionWidth = regionWidth; // ずっと白の場所の横幅の最大値
          max_endXCoord = j; // 幅が最大になった場所のX座標値
        }
      } else {
        regionWidth = 0; // 閾値以下は0リセット
      }
    }
  }
  *out_offsetX = max_endXCoord - max_regionWidth;
  *out_regionWidth = max_regionWidth;
  if (*out_offsetX < 0) {
    return false;
  }
  return true;
}


/**
 * 縦方向の最大領域の開始座標(y)と縦幅を検出する
 * 
 * @params yuvImage YUVの画像
 * @params out_offsetY 最大領域の開始y座標を得る
 * @params out_regionHeight 最大領域の縦幅を得る
 * @return 正常に終了したか
 */
bool getOffsetYAndHeightOfRegion(
  uint16_t *yuvImage, 
  uint8_t threshold,
  uint16_t* out_offsetY,
  uint16_t* out_regionHeight) {
  
  // 引数チェック
  if (&yuvImage == NULL) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }
  

  int max_regionHeight = -1;  // 最大幅（高さ方向）を格納
  int max_endYCoord = -1;  // 最大高さの終了座標
  for (int j = 0; j < CAMERA_WIDTH; j += STEP) {
    int regionHeight = 0;
    for (int i = 0; i < CAMERA_HEIGHT; i += STEP) {
      int n = i * CAMERA_WIDTH + j;
      // ピクセルの出力が閾値より上の場合は加算     
      if (grayScale(yuvImage[n]) > threshold) {
        regionHeight += STEP;
        if (regionHeight > max_regionHeight) {
          max_regionHeight = regionHeight;  // ずっと白の場所の縦幅の最大値
          max_endYCoord = i; // // 幅が最大になった場所のY座標値
        }
      }
      else {
        regionHeight = 0;  // 閾値以下は0リセット
      }
    }
  }

  *out_offsetY = max_endYCoord - max_regionHeight; // 開始座標(y)
  *out_regionHeight = max_regionHeight; // 最大領域の高さ
  if (*out_offsetY < 0) {
    return false;
  }
  return true;
}
