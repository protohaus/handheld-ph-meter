#include "lcdml_utils.h"

LcdmlUtils::LcdmlUtils(LCDMenuLib2& lcdml, U8G2& u8g2, Config config)
    : lcdml_(lcdml), u8g2_(u8g2), config_(config) {}

void LcdmlUtils::information(uint8_t param) {
  if (lcdml_.FUNC_setup())  // ****** SETUP *********
  {
    // setup function
    u8g2_.setFont(config_.disp_font);
    u8g2_.firstPage();
    do {
      u8g2_.drawStr(0, (config_.disp_font_h * 1), "To close this");
      u8g2_.drawStr(0, (config_.disp_font_h * 2), "function press");
      u8g2_.drawStr(0, (config_.disp_font_h * 3), "any button or use");
      u8g2_.drawStr(0, (config_.disp_font_h * 4), "back button");
    } while (u8g2_.nextPage());
  }

  if (lcdml_.FUNC_loop())  // ****** LOOP *********
  {
    // loop function, can be run in a loop when LCDML_DISP_triggerMenu(xx) is
    // set the quit button works in every DISP function without any checks; it
    // starts the loop_end function
    if (lcdml_.BT_checkAny())  // check if any button is pressed (enter, up,
                               // down, left, right)
    {
      // LCDML_goToMenu stops a running menu function and goes to the menu
      lcdml_.FUNC_goBackToMenu();
    }
  }

  if (lcdml_.FUNC_close())  // ****** STABLE END *********
  {
    // you can here reset some global vars or do nothing
  }
}

void LcdmlUtils::timer_info(uint8_t param) {
  if (lcdml_.FUNC_setup())  // ****** SETUP *********
  {
    timer_info_ = 20;  // reset and set timer

    char buf[20];
    sprintf(buf, "wait %d seconds", timer_info_);

    u8g2_.setFont(config_.disp_font);
    u8g2_.firstPage();
    do {
      u8g2_.drawStr(0, (config_.disp_font_h * 1), buf);
      u8g2_.drawStr(0, (config_.disp_font_h * 2), "or press back button");
    } while (u8g2_.nextPage());

    lcdml_.FUNC_setLoopInterval(100);  // starts a trigger event for the loop
                                       // function every 100 milliseconds

    lcdml_.TIMER_msReset(timer_1_);
  }

  if (lcdml_.FUNC_loop())  // ****** LOOP *********
  {
    // loop function, can be run in a loop when LCDML_DISP_triggerMenu(xx) is
    // set the quit button works in every DISP function without any checks; it
    // starts the loop_end function

    // reset screensaver timer
    lcdml_.SCREEN_resetTimer();

    // this function is called every 100 milliseconds

    // this method checks every 1000 milliseconds if it is called
    if (lcdml_.TIMER_ms(timer_1_, 1000)) {
      timer_1_ = millis();
      timer_info_--;  // increment the value every second
      char buf[20];
      sprintf(buf, "wait %d seconds", timer_info_);

      u8g2_.setFont(config_.disp_font);
      u8g2_.firstPage();
      do {
        u8g2_.drawStr(0, (config_.disp_font_h * 1), buf);
        u8g2_.drawStr(0, (config_.disp_font_h * 2), "or press back button");
      } while (u8g2_.nextPage());
    }

    // this function can only be ended when quit button is pressed or the time
    // is over check if the function ends normally
    if (timer_info_ <= 0) {
      // leave this function
      lcdml_.FUNC_goBackToMenu();
    }
  }

  if (lcdml_.FUNC_close())  // ****** STABLE END *********
  {
    // you can here reset some global vars or do nothing
  }
}

void LcdmlUtils::p2(uint8_t param) {
  if (lcdml_.FUNC_setup())  // ****** SETUP *********
  {
    // setup function
    // print LCD content
    char buf[17];
    sprintf(buf, "count: %d of 3", 0);

    u8g2_.setFont(config_.disp_font);
    u8g2_.firstPage();
    do {
      u8g2_.drawStr(0, (config_.disp_font_h * 1), "press a or w button");
      u8g2_.drawStr(0, (config_.disp_font_h * 2), buf);
    } while (u8g2_.nextPage());

    // Reset Button Value
    button_value = 0;

    // Disable the screensaver for this function until it is closed
    lcdml_.FUNC_disableScreensaver();
  }

  if (lcdml_.FUNC_loop())  // ****** LOOP *********
  {
    // loop function, can be run in a loop when LCDML_DISP_triggerMenu(xx) is
    // set the quit button works in every DISP function without any checks; it
    // starts the loop_end function

    // the quit button works in every DISP function without any checks; it
    // starts the loop_end function
    if (lcdml_.BT_checkAny())  // check if any button is pressed (enter, up,
                               // down, left, right)
    {
      if (lcdml_.BT_checkLeft() ||
          lcdml_.BT_checkUp())  // check if button left is pressed
      {
        lcdml_.BT_resetLeft();  // reset the left button
        lcdml_.BT_resetUp();    // reset the left button
        button_value++;

        // update LCD content
        char buf[20];
        sprintf(buf, "count: %d of 3", button_value);

        u8g2_.setFont(config_.disp_font);
        u8g2_.firstPage();
        do {
          u8g2_.drawStr(0, (config_.disp_font_h * 1), "press a or w button");
          u8g2_.drawStr(0, (config_.disp_font_h * 2), buf);
        } while (u8g2_.nextPage());
      }
    }

    // check if button count is three
    if (button_value >= 3) {
      lcdml_.FUNC_goBackToMenu();  // leave this function
    }
  }

  if (lcdml_.FUNC_close())  // ****** STABLE END *********
  {
    // you can here reset some global vars or do nothing
  }
}

void LcdmlUtils::screensaver(uint8_t param) {
  if (lcdml_.FUNC_setup())  // ****** SETUP *********
  {
    // setup function
    u8g2_.setFont(config_.disp_font);
    u8g2_.firstPage();
    do {
      u8g2_.drawStr(0, (config_.disp_font_h * 1), "screensaver");
      u8g2_.drawStr(0, (config_.disp_font_h * 2), "press any key");
      u8g2_.drawStr(0, (config_.disp_font_h * 3), "to leave it");
    } while (u8g2_.nextPage());

    lcdml_.FUNC_setLoopInterval(100);  // starts a trigger event for the loop
                                       // function every 100 milliseconds
  }

  if (lcdml_.FUNC_loop())  // ****** LOOP *********
  {
    if (lcdml_.BT_checkAny())  // check if any button is pressed (enter, up,
                               // down, left, right)
    {
      lcdml_.FUNC_goBackToMenu();  // leave this function
    }
  }

  if (lcdml_.FUNC_close())  // ****** STABLE END *********
  {
    // The screensaver go to the root menu
    lcdml_.MENU_goRoot();
  }
}

void LcdmlUtils::back(uint8_t param) {
  if (lcdml_.FUNC_setup())  // ****** SETUP *********
  {
    // end function and go an layer back
    lcdml_.FUNC_goBackToMenu(1);  // leave this function and go a layer back
  }
}

void LcdmlUtils::goToRootMenu(uint8_t param) {
  if (lcdml_.FUNC_setup())  // ****** SETUP *********
  {
    // go to root and display menu
    lcdml_.MENU_goRoot();
  }
}

// *********************************************************************
void LcdmlUtils::para(uint8_t param)
// *********************************************************************
{
  if (lcdml_.FUNC_setup())  // ****** SETUP *********
  {
    char buf[20];
    sprintf(buf, "parameter: %d", param);

    // setup function
    u8g2_.setFont(config_.disp_font);
    u8g2_.firstPage();
    do {
      u8g2_.drawStr(0, (config_.disp_font_h * 1), buf);
      u8g2_.drawStr(0, (config_.disp_font_h * 2), "press any key");
      u8g2_.drawStr(0, (config_.disp_font_h * 3), "to leave it");
    } while (u8g2_.nextPage());

    lcdml_.FUNC_setLoopInterval(100);  // starts a trigger event for the loop
                                       // function every 100 milliseconds
  }

  if (lcdml_.FUNC_loop())  // ****** LOOP *********
  {
    // For example
    switch (param) {
      case 10:
        // do something
        break;

      case 20:
        // do something
        break;

      case 30:
        // do something
        break;

      default:
        // do nothing
        break;
    }

    if (lcdml_.BT_checkAny())  // check if any button is pressed (enter, up,
                               // down, left, right)
    {
      lcdml_.FUNC_goBackToMenu();  // leave this function
    }
  }

  if (lcdml_.FUNC_close())  // ****** STABLE END *********
  {
    // you can here reset some global vars or do nothing
  }
}

void LcdmlUtils::dyn_para(uint8_t line) {
  // check if this function is active (cursor stands on this line)
  if (line == lcdml_.MENU_getCursorPos()) {
    // make only an action when the cursor stands on this menu item
    // check Button
    if (lcdml_.BT_checkAny()) {
      if (lcdml_.BT_checkEnter()) {
        // this function checks returns the scroll disable status (0 = menu
        // scrolling enabled, 1 = menu scrolling disabled)
        if (lcdml_.MENU_getScrollDisableStatus() == 0) {
          // disable the menu scroll function to catch the cursor on this point
          // now it is possible to work with BT_checkUp and BT_checkDown in this
          // function this function can only be called in a menu, not in a menu
          // function
          lcdml_.MENU_disScroll();
        } else {
          // enable the normal menu scroll function
          lcdml_.MENU_enScroll();
        }

        // do something
        // ...

        lcdml_.BT_resetEnter();
      }

      // This check have only an effect when MENU_disScroll is set
      if (lcdml_.BT_checkUp()) {
        dyn_param++;
        lcdml_.BT_resetUp();
      }

      // This check have only an effect when MENU_disScroll is set
      if (lcdml_.BT_checkDown()) {
        dyn_param--;
        lcdml_.BT_resetDown();
      }

      if (lcdml_.BT_checkLeft()) {
        dyn_param++;
        lcdml_.BT_resetLeft();
      }

      if (lcdml_.BT_checkRight()) {
        dyn_param--;
        lcdml_.BT_resetRight();
      }
    }
  }

  char buf[20];
  sprintf(buf, "dynValue: %d", dyn_param);

  // setup function
  u8g2_.drawStr(
      config_.disp_box_x0 + config_.disp_font_w + config_.disp_cur_space_behind,
      (config_.disp_font_h * (1 + line)), buf);
}
