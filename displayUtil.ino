#define DISPLAY_WIDTH (320)
#define DISPLAY_HEIGHT (240)

/**
 * スクリーンの下部に文字列を表示する
 */
void putStringOnLcd(String str, int color) {
  int len = str.length();
  display.fillRect(0, 224, DISPLAY_WIDTH, DISPLAY_HEIGHT, ILI9341_BLACK);
  display.setTextSize(2);
  int sx = 160 - len/2 * 12;
  if (sx < 0) {
    sx = 0;
  }
  display.setCursor(sx, 225);
  display.setTextColor(color);
  display.println(str);
}

/**
 * 画像バッファーに四角形の枠を描写する
 * 
 * @param imgBuf 画像バッファ
 * @param offset_x 枠の左上からのオフセット
 * @param offset_y 枠の左上からのオフセット
 * @param width 枠の幅
 * @param height 枠の高さ
 * @param thickness 枠線の幅
 * @param color 枠線の色
 */
void drawBox(uint16_t *imgBuf, int offset_x, int offset_y, int width, int height, int thickness, int color) {
  // 横向きの枠線を描く
  for (int x = offset_x; x < offset_x + width; ++x) {
    for (int n = 0; n < thickness; ++n) {
      int top =  DISPLAY_WIDTH * (offset_y + n) + x; // ポインタをDISPLAY_WIDTHだけずらせば一つ下に移動する。DISPLAY_WIDTH * (offset_y + n)だけ移動すれば枠の上からのオフセットと枠線の幅だけ下に移動する。それにxを足せば横に移動する。 
      if (0 <= top && top < DISPLAY_WIDTH * DISPLAY_HEIGHT) {
        imgBuf[top] = color; 
      }
      int bottom = DISPLAY_WIDTH * (offset_y + height - 1 - n) + x; // DISPLAY_WIDTH * (offset_y + height - 1 - n)だけ移動すれば枠の上からのオフセット+枠の高さ-枠線の幅だけ下に移動する。それにxを足せば横に移動する。 
      if (0 <= bottom && bottom < DISPLAY_WIDTH * DISPLAY_HEIGHT) {
        imgBuf[bottom] = color;
      }
    }
  }
  // 縦向きの枠線を描く
  for (int y = offset_y; y < offset_y + height; ++y) {
    for (int n= 0; n < thickness; ++n) {
      int left = DISPLAY_WIDTH * y + offset_x + n;
      if (0 <= left && left < DISPLAY_WIDTH * DISPLAY_HEIGHT) {
        imgBuf[left] = color;
      }
      int right = DISPLAY_WIDTH * y + offset_x + width - 1 - n;
      if (0 <= right && right <= DISPLAY_WIDTH * DISPLAY_HEIGHT) {
        imgBuf[right] = color;
      }
    }
  }
}
