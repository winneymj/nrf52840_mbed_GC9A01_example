/* mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "USBConsole.h"
#include <lvgl/lvgl.h>
#include <lv_drivers/display/GC9A01.h>

#define LVGL_TICK 5
// 5 milliseconds (5000 microseconds)
#define TICKER_TIME 1000 * LVGL_TICK

#define INSET 5
#define HOURS_RADIUS 10
#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

static int32_t sHours = 4;
static int32_t sMinutes = 45;

Ticker ticker;
DigitalOut led1(LED_RED);

static int32_t getAngleForHour(int hour) {
  return (hour * 360) / 12;
}

static int32_t getAngleForMinutes(int minutes) {
  return (minutes * 360) / 60;
}

void lvl_ticker_func()
{
  // printf("lvl_ticker_func: ENTER \r\n");
  //Call lv_tick_inc(x) every x milliseconds in a Timer or Task (x should be between 1 and 10). 
  //It is required for the internal timing of LittlevGL.
  lv_tick_inc(LVGL_TICK);

  //Call lv_task_handler() periodically every few milliseconds. 
  //It will redraw the screen if required, handle input devices etc.  
  // lv_task_handler();
}

void eventcb() {
  // printf("eventcb()\r\n");
  //Call lv_task_handler() periodically every few milliseconds. 
  //It will redraw the screen if required, handle input devices etc.  
  lv_task_handler();
}

//---------------------------------------------------------------------------------------
// WATCHFACE

lv_point_t pointFromPolar(lv_area_t &bounds, int angleInDegrees) {
  lv_point_t retVal;

  int w = lv_area_get_width(&bounds);
  int h = lv_area_get_height(&bounds);
  int radius = w / 2;
  lv_coord_t cx = bounds.x1 + (w / 2);
  lv_coord_t cy = bounds.y1 + (h / 2);
  // printf("pointFromPolar angleInDegrees=%d\r\n", angleInDegrees);
  // printf("pointFromPolar radius=%d\r\n", radius);
  // printf("pointFromPolar bounds=%d,%d,%d,%d\r\n", bounds.x1, bounds.y1, bounds.x2, bounds.y2);

  // // Convert from degrees to radians via multiplication by PI/180        
  // float x = (float)(radius * cos(angleInDegrees * M_PI / 180.0)) + bounds.x1;
  // float y = (float)(radius * sin(angleInDegrees * M_PI / 180.0)) + bounds.y1;
  // Convert from degrees to radians via multiplication by PI/180        
  float x = (float) cx + (radius * cos(angleInDegrees / (180.0 / M_PI)));
  float y = (float) cy + (radius * sin(angleInDegrees / (180.0 / M_PI)));
  // printf("pointFromPolar x=%f\r\n", x);
  // printf("pointFromPolar y=%f\r\n", y);
  retVal.x = (lv_coord_t)x;
  retVal.y = (lv_coord_t)y;
  return retVal;
}

lv_area_t bounds_inset(const lv_area_t &bounds, const lv_coord_t offset) {
  lv_area_t retVal = {bounds.x1 + offset, bounds.y1 + offset, bounds.x2 - offset, bounds.y2 - offset};
  return retVal;
}

lv_area_t layer_get_bounds(const lv_obj_t *obj) {
  lv_area_t cords_p;
  lv_obj_get_coords(obj, &cords_p);
  return cords_p;
}

void pebble_circle_watchface(void) {
  // Fill background with blue
  static lv_style_t style_screen;
  lv_style_copy(&style_screen, &lv_style_plain);
  style_screen.body.main_color = LV_COLOR_BLUE;
  style_screen.body.grad_color = LV_COLOR_BLUE;  // Comment this out to get a graduated blue->white
  lv_obj_set_style(lv_scr_act(), &style_screen);

  // Get bounds
  lv_area_t bounds = layer_get_bounds(lv_scr_act());

  // 12 hours only with maximim size
  sHours -= (sHours > 12) ? 12 : 0;

  // Minutes are expanding circle arc
  int minuteAngle = getAngleForMinutes(sMinutes);
  lv_area_t frame = bounds_inset(bounds, (4 * INSET));

  /*Create style for the Arcs*/
  static lv_style_t style;
  lv_style_copy(&style, &lv_style_plain);
  style.line.color = LV_COLOR_MAKE(0x88, 0x8A, 0xD3); /*Arc color These are 888 numbers */
  style.line.width = 20;                       /*Arc width*/
  printf("style.line.color.full=0x%hx\r\n", style.line.color.full);

  /*Create an Arc*/
  lv_obj_t * arc = lv_arc_create(lv_scr_act(), NULL);
  lv_arc_set_style(arc, LV_ARC_STYLE_MAIN, &style);          /*Use the new style*/
  lv_arc_set_angles(arc, minuteAngle - 180, 0);
  lv_obj_set_size(arc, frame.x2, frame.y2);
  lv_obj_align(arc, NULL, LV_ALIGN_CENTER, 0, 0);

  /*Create style for the dots*/
  static lv_style_t styleBlack;
  lv_style_copy(&styleBlack, &lv_style_plain);
  styleBlack.body.main_color = LV_COLOR_BLACK;
  styleBlack.body.grad_color = LV_COLOR_BLACK;
  styleBlack.body.radius = LV_RADIUS_CIRCLE;

  static lv_style_t styleWhite;
  lv_style_copy(&styleWhite, &styleBlack);
  styleWhite.body.main_color = LV_COLOR_WHITE;
  styleWhite.body.grad_color = LV_COLOR_WHITE;

  // create new smaller bounds object using the style margin
  lv_area_t newBounds = bounds_inset(bounds, (2.2 * HOURS_RADIUS));

  // Hours are dots
  for (int i = 0; i < 12; i++) {
    int hourPos = getAngleForHour(i);
    lv_point_t pos = pointFromPolar(newBounds, hourPos);
    // printf("pointFromPolar = %d, %d\r\n", pos.x, pos.y);

    // Create object to draw
    lv_obj_t *btn = lv_obj_create(lv_scr_act(), NULL);

    lv_obj_set_style(btn, i <= sHours ? &styleWhite : &styleBlack);
    lv_obj_set_size(btn, HOURS_RADIUS, HOURS_RADIUS);
    lv_obj_set_pos(btn, pos.x - (HOURS_RADIUS / 2), pos.y - (HOURS_RADIUS / 2));
  }

  lv_obj_t *btn2 = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_set_style(btn2, &styleBlack);
  lv_obj_set_size(btn2, HOURS_RADIUS, HOURS_RADIUS);
  lv_obj_set_pos(btn2, 115, 5);
}

//--------------------------------------------------------------------------------------

// main() runs in its own thread in the OS
int main()
{
  printf("main: ENTER\r\n");

  // Initalize the display driver GC9A01
  GC9A01_init();

  printf("main: GC9A01_init() done\r\n");

  lv_init();

  printf("main: lv_init() done\r\n");
  static lv_disp_buf_t disp_buf;
  static lv_color_t buf[LV_HOR_RES_MAX * 10];
  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
  printf("main: lv_disp_buf_init() done\r\n");

  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = GC9A01_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

	// GC9A01_fillScreen(WHITE_COLOUR);

  printf("main: lv_disp_drv_register() done\r\n");

  ticker.attach_us(callback(&lvl_ticker_func), TICKER_TIME);

	printf("main: ticker.attach() done\r\n");
  events::EventQueue queue;

  // Set callback for lv_task_handler to redraw the screen if necessary
  queue.call_every(10, callback(&eventcb));

  pebble_circle_watchface();

  queue.dispatch_forever();

  while (true) {
    printf("hello\r\n");
    // Blink LED and wait 0.5 seconds
    led1 = !led1;
    wait_us(1000 * 1000);
  }
}
