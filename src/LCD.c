#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

// The auto-generated header file with resource handle definitions
#include "resource_ids.auto.h"

#define MY_UUID { 0xBD, 0xBF, 0x7B, 0xFD, 0x82, 0x09, 0x40, 0x8B, 0xB9, 0xB0, 0x7B, 0xAD, 0x32, 0x3E, 0xB0, 0xDA }
#define COOKIE_MY_TIMER 1

PBL_APP_INFO(MY_UUID,
             "LCD", "Matt Smith & Pepper Garretson",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

Layer parent; 					// Parent Layer
BmpContainer cursor_layer; 		// Seconds Layer

TextLayer text_layer_4; // Month Layer
TextLayer text_layer_5; // Date Layer
TextLayer text_layer_6; // AM/PM Layer

AppTimerHandle timer_handle;

#define TOTAL_IMAGE_SLOTS 4
#define NUMBER_OF_IMAGES 10
#define EMPTY_SLOT -1

const int IMAGE_RESOURCE_IDS[NUMBER_OF_IMAGES] = {
  RESOURCE_ID_IMAGE_NUM_0, RESOURCE_ID_IMAGE_NUM_1, RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3, RESOURCE_ID_IMAGE_NUM_4, RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6, RESOURCE_ID_IMAGE_NUM_7, RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

BmpContainer image_containers[TOTAL_IMAGE_SLOTS];
int image_slot_state[TOTAL_IMAGE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

unsigned short get_display_hour(unsigned short hour) {

  if (clock_is_24h_style()) {
    return hour;
  }

  unsigned short display_hour = hour % 12;

  // Converts "0" to "12"
  return display_hour ? display_hour : 12;

}

void load_digit_image_into_slot(int slot_number, int digit_value) {

  if ((slot_number < 0) || (slot_number >= TOTAL_IMAGE_SLOTS)) {
    return;
  }

  if ((digit_value < 0) || (digit_value > 9)) {
    return;
  }

  if (image_slot_state[slot_number] != EMPTY_SLOT) {
    return;
  }

  image_slot_state[slot_number] = digit_value;
  bmp_init_container(IMAGE_RESOURCE_IDS[digit_value], &image_containers[slot_number]);

  int x;
  int y;
  if(slot_number == 0) {
  	x = 4;
	y = 54;
  }
  if(slot_number == 1) {
	x = 34;
	y = 54;
  }
  if(slot_number == 2) {
	x = 74;
	y = 54;
  }
  if(slot_number == 3) {
	x = 105;
	y = 54;
  }
  
  image_containers[slot_number].layer.layer.frame.origin.x = x;
  image_containers[slot_number].layer.layer.frame.origin.y = y;
  
  layer_add_child(&window.layer, &image_containers[slot_number].layer.layer);
	
}

void unload_digit_image_from_slot(int slot_number) {
  /*

     Removes the digit from the display and unloads the image resource
     to free up RAM.

     Can handle being called on an already empty slot.
	 
   */

  if (image_slot_state[slot_number] != EMPTY_SLOT) {
    layer_remove_from_parent(&image_containers[slot_number].layer.layer);
    bmp_deinit_container(&image_containers[slot_number]);
    image_slot_state[slot_number] = EMPTY_SLOT;
  }
}

void display_value(unsigned short value, unsigned short row_number, bool show_first_leading_zero) {
  /*

     Displays a numeric value between 0 and 99 on screen.

     Rows are ordered on screen as:

       Row 0
       Row 1

     Includes optional blanking of first leading zero,
     i.e. displays ' 0' rather than '00'.

   */
  value = value % 100; // Maximum of two digits per row.

  // Column order is: | Column 0 | Column 1 |
  // (We process the columns in reverse order because that makes
  // extracting the digits from the value easier.)
  for (int column_number = 1; column_number >= 0; column_number--) {
    int slot_number = (row_number * 2) + column_number;
    unload_digit_image_from_slot(slot_number);
    if (!((value == 0) && (column_number == 0) && !show_first_leading_zero)) {
      load_digit_image_into_slot(slot_number, value % 10);
    }
    value = value / 10;
  }
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
	(void)ctx;
	(void)handle;
	
	if(cookie == COOKIE_MY_TIMER) {
		//Hide the Seconds Layer
		layer_set_hidden(&cursor_layer.layer.layer, true);
	}
}

void update_display(PblTm *tick_time) {

  static char month_text[] = "AAA";
  static char date_text[] = "00"; 
  static char am_pm_text[] = "P";
  
  string_format_time(month_text, sizeof(month_text), "%b", tick_time);
  string_format_time(date_text, sizeof(date_text), "%e", tick_time);
  string_format_time(am_pm_text, sizeof(am_pm_text), "%p", tick_time);
  
  text_layer_set_text(&text_layer_4, month_text);
  text_layer_set_text(&text_layer_5, date_text);  
  
  if (!clock_is_24h_style()) {
	text_layer_set_text(&text_layer_6, am_pm_text);  
  }
  
  display_value(get_display_hour(tick_time->tm_hour), 0, false);
  display_value(tick_time->tm_min, 1, true);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)t;
  (void)ctx;
  
  //Load the Colon Image
  layer_set_hidden(&cursor_layer.layer.layer, false);
  
  timer_handle = app_timer_send_event(ctx, 500 /* milliseconds */, COOKIE_MY_TIMER);
    
  if((t->units_changed & MINUTE_UNIT) != 0) {
	update_display(t->tick_time);
  }
  
}

void LayerSetup(PblTm *tick_time) {

  //21pt
  GFont custom_font21 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL21));
  
  //Init the parent layer at (0,0) and size (144 X 168)
  layer_init(&parent, GRect(0, 0, 144, 168));

  text_layer_init(&text_layer_4, GRect(46, 105, 60, 30));  	// Month
  text_layer_init(&text_layer_5, GRect(114, 105, 30, 30));  // Date
  text_layer_init(&text_layer_6, GRect(114, 48, 30, 30));  // AM/PM 

  text_layer_set_font(&text_layer_4, custom_font21);
  text_layer_set_font(&text_layer_5, custom_font21);
  text_layer_set_font(&text_layer_6, custom_font21);
  
  text_layer_set_background_color(&text_layer_4, GColorBlack);
  text_layer_set_background_color(&text_layer_5, GColorBlack);
  text_layer_set_background_color(&text_layer_6, GColorBlack);
  
  text_layer_set_text_color(&text_layer_4, GColorWhite);
  text_layer_set_text_color(&text_layer_5, GColorWhite);
  text_layer_set_text_color(&text_layer_6, GColorWhite);
  
  text_layer_set_text_alignment(&text_layer_4, GTextAlignmentRight);
  text_layer_set_text_alignment(&text_layer_5, GTextAlignmentLeft);
  text_layer_set_text_alignment(&text_layer_6, GTextAlignmentRight);
  
  
  layer_add_child(&parent, &text_layer_5.layer);
  layer_add_child(&parent, &text_layer_4.layer);
  layer_add_child(&parent, &text_layer_6.layer);
  
  layer_add_child(&window.layer, &parent);
    
  bmp_init_container(RESOURCE_ID_IMAGE_COLON, &cursor_layer);
  cursor_layer.layer.layer.frame.origin.x = 64;
  cursor_layer.layer.layer.frame.origin.y = 54;
  layer_add_child(&parent, &cursor_layer.layer.layer);

  
  update_display(tick_time);
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "LCD Window");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);
  
  // If you neglect to call this, all `resource_get_handle()` requests
  // will return NULL.
  resource_init_current_app(&LCDRESOURCES);
    
  // Avoids a blank screen on watch start.
  PblTm tick_time;

  get_time(&tick_time);
  
  LayerSetup(&tick_time);
}

void pbl_main(void *params) {

  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
	
	.timer_handler = &handle_timer,
	
	.tick_info = {
		.tick_handler = &handle_second_tick,
		.tick_units = SECOND_UNIT
	}
  };
  app_event_loop(params, &handlers);
}
