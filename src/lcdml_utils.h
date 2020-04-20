#pragma once

#include <LCDMenuLib2.h>
#include <U8g2lib.h>

class LcdmlUtils {
 public:
  struct Config {
    const uint8_t* disp_font;
    const uint8_t disp_font_h;
    const uint8_t disp_font_w;
    const uint8_t disp_box_x0;
    const uint8_t disp_box_y0;
    const uint8_t disp_cur_space_behind;
  };

  LcdmlUtils(LCDMenuLib2& lcdml, U8G2& u8g2, Config config);
  
  void information(uint8_t param);
  void timer_info(uint8_t param);
  void p2(uint8_t param);
  void screensaver(uint8_t param);
  void back(uint8_t param);
  void goToRootMenu(uint8_t param);
  void para(uint8_t param);
  void dyn_para(uint8_t param);

 private:
  LCDMenuLib2& lcdml_;
  U8G2& u8g2_;
  Config config_;

  uint8_t timer_info_ = 0;
  unsigned long timer_1_ = 0;

  uint8_t button_value = 0;

  uint8_t dyn_param = 100;

};
