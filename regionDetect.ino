/**
 * spresenseはメモリが小さいので、機械学習モデルは非常に小さなものしか使えない。 よって、機械学習は使わず、敢えて原始的な方法で物体を認識している。
 * 原理としては、カメラからYUV形式で取得した画像データをビット演算でグレイスケール化し、適当な閾値で二値化している。
 * そして、白が続く最大の横幅及びその開始地点のx座標と、白が続く最大の縦幅及びその開始地点のy座標を計算している。
 * それによって赤い枠を画像に描いてディスプレイに出力している。
 * この手法は背景が黒に近いことを仮定している。デモ時は黒い垂れ幕などをカメラの前において、その垂れ幕とカメラの間に、明るい色の物体を通過させること。
 * 物体が正方形に近ければ、正しい枠が描かれるが、例えばブーメラン型の物体だと正しく描けない。
 * また、画面に映っている明るいもののなかで一番大きなものにのみ枠を描く。
 */

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
