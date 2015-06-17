#include <pebble.h>

// ============
// == colors ==
// ============
#define MAIN_COLOR (GColor8) { .argb = 0b11000111 }
#define SHADOW_COLOR (GColor8) { .argb = 0b11000010 }

// ===============
// == constants ==
// ===============
static const int WIDTH = 144;
static const int HEIGHT = 168;
static const int PADDING = 10;
static const int SHADOW_OFFSET = 1;
static const int TEXT_HEIGHT = 38;
static const int TEXT_OFFSET = 3;
static const int STROKE_WIDTH = 2;
static const int SMALL_CIRCLE_WIDTH = 6;

// ======================
// == static variables ==
// ======================
static Window *s_main_window;
// time layers
static TextLayer *s_time_layer;
static TextLayer *s_time_shadow_layer;
// circle layers
static Layer *s_circle_layer;
static Layer *s_circle_shadow_layer;
// small circle layers
static Layer *s_small_circle_layer;
static Layer *s_small_circle_shadow_layer;
// font
static GFont s_time_font;
// animations
static PropertyAnimation *s_small_circle_property_animation;
static PropertyAnimation *s_small_circle_shadow_property_animation;


// ========================================
// == calculate position of small circle ==
// ========================================
static GPoint calculate_small_circle_position(tm *tick_time) {
  GPoint pos;
  int angle = TRIG_MAX_ANGLE * tick_time->tm_sec / 60;
  pos.x = (sin_lookup(angle) * (WIDTH / 2 - PADDING - STROKE_WIDTH) / TRIG_MAX_RATIO) + WIDTH / 2;
  pos.y = (-cos_lookup(angle) * (WIDTH / 2 - PADDING - STROKE_WIDTH) / TRIG_MAX_RATIO) + HEIGHT / 2;
  return pos;
}

static void update_minute(struct tm *tick_time) {
  // =======================
  // == update time layer ==
  // =======================
  // create a long-lived buffer
  static char buffer[] = "00:00";
  // write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
  // set time on layer
  text_layer_set_text(s_time_layer, buffer);
  text_layer_set_text(s_time_shadow_layer, buffer);
}

static void update_second(struct tm *tick_time) {
  // ==========================
  // == animate small circle ==
  // ==========================
  GPoint pos = calculate_small_circle_position(tick_time);
  GRect small_circle_from_frame = layer_get_frame(s_small_circle_layer);
  GRect small_circle_to_frame = GRect(pos.x - SMALL_CIRCLE_WIDTH - STROKE_WIDTH, pos.y - SMALL_CIRCLE_WIDTH - STROKE_WIDTH, (SMALL_CIRCLE_WIDTH + STROKE_WIDTH) * 2, (SMALL_CIRCLE_WIDTH + STROKE_WIDTH) * 2);
  // animate small circle shadow
  GRect small_circle_shadow_from_frame = layer_get_frame(s_small_circle_shadow_layer);
  GRect small_circle_shadow_to_frame = GRect(pos.x - SMALL_CIRCLE_WIDTH - STROKE_WIDTH, pos.y - SMALL_CIRCLE_WIDTH - STROKE_WIDTH + SHADOW_OFFSET + 1, (SMALL_CIRCLE_WIDTH + STROKE_WIDTH) * 2, (SMALL_CIRCLE_WIDTH + STROKE_WIDTH) * 2);
  // create small circle animations
  s_small_circle_property_animation = property_animation_create_layer_frame(s_small_circle_layer, &small_circle_from_frame, &small_circle_to_frame);
  s_small_circle_shadow_property_animation = property_animation_create_layer_frame(s_small_circle_shadow_layer, &small_circle_shadow_from_frame, &small_circle_shadow_to_frame);
  // schedule small circle animations
  animation_set_curve((Animation*) s_small_circle_property_animation, AnimationCurveLinear);
  animation_set_curve((Animation*) s_small_circle_shadow_property_animation, AnimationCurveLinear);
  animation_schedule((Animation*) s_small_circle_property_animation);
  animation_schedule((Animation*) s_small_circle_shadow_property_animation);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if ((units_changed & SECOND_UNIT) == SECOND_UNIT) {
    update_second(tick_time);
  }
  if ((units_changed & MINUTE_UNIT) == MINUTE_UNIT) {
    update_minute(tick_time);
  }
}

// ===============================
// == small circle layer update ==
// ===============================
static void small_circle_layer_draw(Layer *layer, GContext *ctx) {
  if (layer == s_small_circle_shadow_layer) {
    graphics_context_set_fill_color(ctx, SHADOW_COLOR);
    graphics_context_set_stroke_color(ctx, SHADOW_COLOR);
  } else {
    graphics_context_set_fill_color(ctx, MAIN_COLOR);
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }
  graphics_context_set_stroke_width(ctx, STROKE_WIDTH);
  graphics_fill_circle(ctx, GPoint(SMALL_CIRCLE_WIDTH + STROKE_WIDTH / 2, SMALL_CIRCLE_WIDTH + STROKE_WIDTH / 2), SMALL_CIRCLE_WIDTH);
  graphics_draw_circle(ctx, GPoint(SMALL_CIRCLE_WIDTH + STROKE_WIDTH / 2, SMALL_CIRCLE_WIDTH + STROKE_WIDTH / 2), SMALL_CIRCLE_WIDTH);
}

// =========================
// == circle layer update ==
// =========================
static void circle_layer_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const int16_t half_w = bounds.size.w / 2;
  const int16_t half_h = bounds.size.h / 2;
  GPoint pos;
  if (layer == s_circle_shadow_layer) {
    graphics_context_set_stroke_color(ctx, SHADOW_COLOR);
    pos = GPoint(half_w, half_h + SHADOW_OFFSET);
  } else {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    pos = GPoint(half_w, half_h);
  }
  graphics_context_set_stroke_width(ctx, STROKE_WIDTH);
  graphics_draw_circle(ctx, pos, half_w - STROKE_WIDTH);
}

static void main_window_load(Window *window) {
  // ==================
  // == circle layer ==
  // ==================
  s_circle_layer = layer_create(GRect(PADDING, PADDING, WIDTH - PADDING * 2, HEIGHT - PADDING * 2));
  layer_set_update_proc(s_circle_layer, circle_layer_draw);
  // circle lasyer shadow
  s_circle_shadow_layer = layer_create(GRect(PADDING, PADDING + SHADOW_OFFSET, WIDTH - PADDING * 2, HEIGHT - PADDING * 2));
  layer_set_update_proc(s_circle_shadow_layer, circle_layer_draw);

  // ================
  // == time layer ==
  // ================
  s_time_layer = text_layer_create(GRect(0, HEIGHT / 2 - TEXT_HEIGHT / 2 - TEXT_OFFSET, WIDTH, TEXT_HEIGHT));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  // time layer shadow
  s_time_shadow_layer = text_layer_create(GRect(0, HEIGHT / 2 - TEXT_HEIGHT / 2 - TEXT_OFFSET + SHADOW_OFFSET + 1, WIDTH, TEXT_HEIGHT));
  text_layer_set_background_color(s_time_shadow_layer, GColorClear);
  text_layer_set_text_color(s_time_shadow_layer, SHADOW_COLOR);
  // font
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MONTSERRAT_36));
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_shadow_layer, s_time_font);
  text_layer_set_text_alignment(s_time_shadow_layer, GTextAlignmentCenter);

  // ========================
  // == small circle layer ==
  // ========================
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  GPoint pos = calculate_small_circle_position(tick_time);
  s_small_circle_layer = layer_create(GRect(pos.x - SMALL_CIRCLE_WIDTH - STROKE_WIDTH, pos.y - SMALL_CIRCLE_WIDTH - STROKE_WIDTH, (SMALL_CIRCLE_WIDTH + STROKE_WIDTH) * 2, (SMALL_CIRCLE_WIDTH + STROKE_WIDTH) * 2));
  layer_set_update_proc(s_small_circle_layer, small_circle_layer_draw);
  // small circle layer shadow
  s_small_circle_shadow_layer = layer_create(GRect(pos.x - SMALL_CIRCLE_WIDTH - STROKE_WIDTH, pos.y - SMALL_CIRCLE_WIDTH - STROKE_WIDTH + SHADOW_OFFSET + 1, (SMALL_CIRCLE_WIDTH + STROKE_WIDTH) * 2, (SMALL_CIRCLE_WIDTH + STROKE_WIDTH) * 2));
  layer_set_update_proc(s_small_circle_shadow_layer, small_circle_layer_draw);

  // ===================
  // == add to window ==
  // ===================
  layer_add_child(window_get_root_layer(window), s_circle_shadow_layer);
  layer_add_child(window_get_root_layer(window), s_small_circle_shadow_layer);
  layer_add_child(window_get_root_layer(window), s_circle_layer);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_shadow_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), s_small_circle_layer);
}

static void main_window_unload(Window *window) {
  // unload GFont
  fonts_unload_custom_font(s_time_font);

  // destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_time_shadow_layer);

  // destroy Circle Layer
  layer_destroy(s_circle_layer);
  layer_destroy(s_circle_shadow_layer);

  // destroy Small Circle Layer
  layer_destroy(s_small_circle_layer);
  layer_destroy(s_small_circle_shadow_layer);

  // destroy animations
  property_animation_destroy(s_small_circle_property_animation);
  property_animation_destroy(s_small_circle_shadow_property_animation);
}

static void init() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  // create main Window element and assign to pointer
  s_main_window = window_create();

  // set background color
  window_set_background_color(s_main_window, MAIN_COLOR);

  // set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // make sure the time is displayed from the start
  update_second(tick_time);
  update_minute(tick_time);
}

static void deinit() {
  // destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
