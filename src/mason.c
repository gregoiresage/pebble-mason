#include <pebble.h>

#include "autoconfig.h"
#include "drawarc.h"

static Window *window;
static Layer *layer;
static InverterLayer *inverter_layer;

static GBitmap *image;
static GFont custom_font;
static char *days[7] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};

static uint8_t hours = 0;
static uint8_t minutes = 0;
static char date_str[7] = "XXX XX";

static GPoint center;

#define OUTER_RADIUS 65
#define OUTER_THICKNESS 4
#define INNER_RADIUS 55
#define DOT_RADIUS 6

static bool bluetooth_connected = false;

static AppTimer *timerDots;

static void timer_callback(void *data) {
  timerDots = NULL;
  layer_mark_dirty(layer);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  // Let Pebble Autoconfig handle received settings
  autoconfig_in_received_handler(iter, context);

  // Update display with new values
  layer_set_hidden(inverter_layer_get_layer(inverter_layer), !getInverted());
}

static void update_layer_callback(Layer *layer, GContext* ctx) {
  GRect bounds = layer_get_frame(layer);

  // minutes = 59;

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorWhite);

  // Draw minutes arc
  int32_t minutes_angle = 360 * minutes / 60;
  if(minutes_angle <= 90){
    graphics_draw_arc(ctx, center, INNER_RADIUS, INNER_RADIUS, 270, 270 + minutes_angle);
  }
  else {
    graphics_draw_arc(ctx, center, INNER_RADIUS, INNER_RADIUS, 270, 360);
    graphics_draw_arc(ctx, center, INNER_RADIUS, INNER_RADIUS, 0, minutes_angle - 90);
  }
  
  // Draw bitmap pattern
  graphics_context_set_compositing_mode(ctx, GCompOpAnd);
  graphics_draw_bitmap_in_rect(ctx, image, bounds);

  // Draw hours arc
  int32_t hours_angle = 360 * ((hours % 12) * 60 + minutes) / (12 * 60);
  if(hours_angle <= 90){
    graphics_draw_arc(ctx, center, OUTER_RADIUS, OUTER_THICKNESS, 270, 270 + hours_angle);
  }
  else {
    graphics_draw_arc(ctx, center, OUTER_RADIUS, OUTER_THICKNESS, 270, 360);
    graphics_draw_arc(ctx, center, OUTER_RADIUS, OUTER_THICKNESS, 0, hours_angle - 90);
  }
  graphics_draw_pixel(ctx, GPoint(center.x-1, center.y - OUTER_RADIUS + 2)); // round end effect
  graphics_draw_pixel(ctx, GPoint(center.x-1, center.y - OUTER_RADIUS + 3)); // round end effect

  GPoint hourDot;

  // draw hours dots if needed
  if(timerDots){
    for(int i=0; i<12; i++){
      hours_angle = TRIG_MAX_ANGLE * i / 12;
      hourDot.y = (int16_t)(-cos_lookup(hours_angle) * (OUTER_RADIUS - OUTER_THICKNESS/2 - 1) / TRIG_MAX_RATIO) + center.y;
      hourDot.x = (int16_t)(sin_lookup(hours_angle) * (OUTER_RADIUS - OUTER_THICKNESS/2 - 1) / TRIG_MAX_RATIO) + center.x;
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_circle(ctx, hourDot, DOT_RADIUS - 2);
    }
  
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_arc(ctx, center, OUTER_RADIUS + 5, 5, 0, 360);
  }

  // Draw hour dot
  hours_angle = TRIG_MAX_ANGLE * ((hours % 12) * 60 + minutes) / (12 * 60);
  hourDot.y = (int16_t)(-cos_lookup(hours_angle) * (OUTER_RADIUS - OUTER_THICKNESS/2) / TRIG_MAX_RATIO) + center.y;
  hourDot.x = (int16_t)(sin_lookup(hours_angle) * (OUTER_RADIUS - OUTER_THICKNESS/2) / TRIG_MAX_RATIO) + center.x;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, hourDot, DOT_RADIUS);
  if(hours >= 12){
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, hourDot, DOT_RADIUS - 2);
  }
  
  // Draw center dot if bluetooth connected
  if(bluetooth_connected){
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, center, DOT_RADIUS + 3);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, center, DOT_RADIUS);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, center, DOT_RADIUS - 2);
  }
  else {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(center.x - (DOT_RADIUS + 4), center.y - (DOT_RADIUS + 4), 2*(DOT_RADIUS + 4), 2*(DOT_RADIUS + 4)), 0, 0);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(center.x - DOT_RADIUS, center.y - DOT_RADIUS, 2*DOT_RADIUS, 2*DOT_RADIUS), 0, 0);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(center.x - (DOT_RADIUS - 2), center.y - (DOT_RADIUS - 2), 2*(DOT_RADIUS - 2), 2*(DOT_RADIUS - 2)), 0, 0);
  }

  // Draw dates
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx,
      date_str,
      custom_font,
      GRect(0, 142, 144, 23),
      GTextOverflowModeWordWrap,
      GTextAlignmentCenter,
      NULL);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  hours = tick_time->tm_hour;
  minutes = tick_time->tm_min;

  snprintf(date_str, sizeof(date_str), "%s %02d", days[tick_time->tm_wday], tick_time->tm_mday);

  layer_mark_dirty(layer);
}

void bluetooth_connection_handler(bool connected){
  if(bluetooth_connected != connected){
    bluetooth_connected = connected;
    layer_mark_dirty(layer);
  }
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction){
  if(!timerDots){
    timerDots = app_timer_register(3000, timer_callback, NULL);
    layer_mark_dirty(layer);
  }
}

int main(void) {
  autoconfig_init();

  app_message_register_inbox_received(in_received_handler);

  center = GPoint(72, 74);

  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  layer = layer_create(bounds);
  layer_set_update_proc(layer, update_layer_callback);
  layer_add_child(window_layer, layer);

  inverter_layer = inverter_layer_create(bounds);
  layer_set_hidden(inverter_layer_get_layer(inverter_layer), !getInverted());
  layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));

  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNSTEADY_OVERSTEER_22));
  image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PATTERN);

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  handle_minute_tick(tick_time, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  bluetooth_connection_service_subscribe(bluetooth_connection_handler);
  accel_tap_service_subscribe(accel_tap_handler);

  bluetooth_connected = bluetooth_connection_service_peek();

  app_event_loop();

  layer_destroy(layer);
  inverter_layer_destroy(inverter_layer);
  window_destroy(window);
  gbitmap_destroy(image);
  fonts_unload_custom_font(custom_font);
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  accel_tap_service_unsubscribe();

  if(!timerDots){
    app_timer_cancel(timerDots);
  }

  autoconfig_deinit();
}
