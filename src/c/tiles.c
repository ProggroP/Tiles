// Tiles -- Kachel-Watchface fuer Emery und Gabbro.
//
// Raster aus sechs farbigen Kacheln: Datum oben links, Uhrzeit in der weissen
// Mittelkachel, vier frei belegbare Datenkacheln (Wetter, Puls, aktive
// Minuten, Batterie, Schritte, Distanz, Schlaf, Kalenderwoche). Die
// Datenquelle je Kachel und das Farbschema kommen per Clay. canvas_update ist
// der einzige Zeichen-Einstiegspunkt.

#include <pebble.h>

// ---- Datenquellen (Werte muessen zu config.js passen) ---------------------
enum {
  SRC_WEATHER  = 0,
  SRC_HEART    = 1,   // ohne Pulssensor (gabbro): aktive Minuten
  SRC_ACTIVE   = 2,
  SRC_BATTERY  = 3,
  SRC_STEPS    = 4,
  SRC_DISTANCE = 5,
  SRC_SLEEP    = 6,
  SRC_WEEK     = 7,
  SRC_COUNT
};

// Kacheln in Speicher- und Zeichenreihenfolge
enum { T_TR = 0, T_MR, T_BL, T_BR, T_COUNT };

#define DEFAULT_TILE_TR    SRC_WEATHER
#define DEFAULT_TILE_MR    SRC_HEART
#define DEFAULT_TILE_BL    SRC_BATTERY
#define DEFAULT_TILE_BR    SRC_STEPS

// ---- Farben -----------------------------------------------------------------
// Ein Schema je Zeile, Hex-Werte der 64er-Palette. Reihenfolge der Schemata
// muss zur Auswahlliste in config.js passen.
typedef struct {
  uint32_t bg;
  uint32_t date_bg, date_fg;
  uint32_t time_bg, time_fg;
  uint32_t tile_bg[T_COUNT];
  uint32_t tile_fg[T_COUNT];
} Scheme;

static const Scheme SCHEMES[] = {
  // Classic: Grau, Orange, Rot, Blau, Gruen
  { 0x000000, 0x555555, 0xAAAAAA, 0xFFFFFF, 0x000000,
    { 0xFFAA55, 0xFF5555, 0x5555FF, 0x55AA55 },
    { 0x000000, 0x000000, 0x000000, 0x000000 } },
  // Ocean: Petrol, Tuerkis, Blau, Himmelblau, Seegruen
  { 0x000000, 0x005555, 0x55FFFF, 0xFFFFFF, 0x000000,
    { 0x00AAAA, 0x0055FF, 0x55AAFF, 0x00AA55 },
    { 0x000000, 0xFFFFFF, 0x000000, 0x000000 } },
  // Sunset: Weinrot, Gold, Orange, Magenta, Indigo
  { 0x000000, 0x550000, 0xFF5555, 0xFFFFFF, 0x000000,
    { 0xFFAA00, 0xFF5500, 0xAA0055, 0x5500AA },
    { 0x000000, 0x000000, 0xFFFFFF, 0xFFFFFF } },
  // Mono: Grautoene im Schachbrett
  { 0x000000, 0xAAAAAA, 0x000000, 0xFFFFFF, 0x000000,
    { 0x555555, 0xAAAAAA, 0x555555, 0xAAAAAA },
    { 0xFFFFFF, 0x000000, 0xFFFFFF, 0x000000 } },
};
#define SCHEME_COUNT       ((int)ARRAY_LENGTH(SCHEMES))

// ---- Layout -----------------------------------------------------------------
#define TILE_RADIUS        14

typedef struct {
  GRect box;               // Kachelflaeche
  GRect label;             // Beschriftung (Kleinschrift)
  GRect value;             // Wert (Grossschrift)
  GTextAlignment align;
} TileLayout;

#ifdef PBL_PLATFORM_EMERY
// 200 x 228, eckig
#define RES_FONT_TIME      RESOURCE_ID_FONT_TIME_E_46
#define RES_FONT_VALUE     RESOURCE_ID_FONT_VALUE_E_28
#define RES_FONT_LABEL     RESOURCE_ID_FONT_LABEL_E_15
#define FALLBACK_FONT_KEY  FONT_KEY_GOTHIC_18_BOLD
#define BOX_DATE           GRect(0, 0, 94, 70)
#define RECT_DAY           GRect(6, 4, 80, 32)
#define RECT_WDAY          GRect(6, 30, 80, 32)
#define BOX_TIME           GRect(0, 76, 134, 76)
#define RECT_TIME          GRect(0, 86, 134, 52)
static const TileLayout TILES[T_COUNT] = {
  [T_TR] = { {{100, 0},   {100, 70}}, {{108, 8},   {86, 18}}, {{108, 24},  {86, 32}}, GTextAlignmentLeft },
  [T_MR] = { {{140, 76},  {60, 76}},  {{140, 88},  {60, 18}}, {{140, 104}, {60, 32}}, GTextAlignmentCenter },
  [T_BL] = { {{0, 158},   {88, 70}},  {{6, 194},   {76, 18}}, {{6, 162},   {76, 32}}, GTextAlignmentRight },
  [T_BR] = { {{94, 158},  {106, 70}}, {{102, 194}, {92, 18}}, {{102, 162}, {92, 32}}, GTextAlignmentLeft },
};
#else
// 260 x 260, rund: die Kreiskante schneidet die aeusseren Ecken ab, die
// Texte sitzen deshalb zur Mitte hin
#define RES_FONT_TIME      RESOURCE_ID_FONT_TIME_G_58
#define RES_FONT_VALUE     RESOURCE_ID_FONT_VALUE_G_32
#define RES_FONT_LABEL     RESOURCE_ID_FONT_LABEL_G_17
#define FALLBACK_FONT_KEY  FONT_KEY_GOTHIC_24_BOLD
#define BOX_DATE           GRect(0, 0, 122, 84)
#define RECT_DAY           GRect(30, 14, 84, 36)
#define RECT_WDAY          GRect(20, 44, 94, 36)
#define BOX_TIME           GRect(0, 90, 176, 80)
#define RECT_TIME          GRect(0, 96, 176, 64)
static const TileLayout TILES[T_COUNT] = {
  [T_TR] = { {{128, 0},   {132, 84}}, {{136, 18},  {100, 20}}, {{136, 34},  {110, 36}}, GTextAlignmentLeft },
  [T_MR] = { {{182, 90},  {78, 80}},  {{182, 100}, {74, 20}},  {{182, 116}, {74, 36}},  GTextAlignmentCenter },
  [T_BL] = { {{0, 176},   {112, 84}}, {{10, 214},  {94, 20}},  {{10, 180},  {94, 36}},  GTextAlignmentRight },
  [T_BR] = { {{118, 176}, {142, 84}}, {{126, 214}, {100, 20}}, {{126, 180}, {120, 36}}, GTextAlignmentLeft },
};
#endif

// ---- Wetter -------------------------------------------------------------------
#define WEATHER_INTERVAL_MIN 30
#define WEATHER_STALE_SEC    (2 * 60 * 60)

// ---- Beispielwerte fuer Store-Screenshots (nur per DEMO-Nachricht, nicht
// persistent) --------------------------------------------------------------------
#define DEMO_TEMP          12
#define DEMO_HEART         72
#define DEMO_ACTIVE_MIN    47
#define DEMO_BATTERY       86
#define DEMO_STEPS         8642
#define DEMO_METERS        6430
#define DEMO_SLEEP_MIN     (7 * 60 + 12)

// ---- AppMessage ----------------------------------------------------------------
// Clay schickt alle Keys in einer Nachricht; ist die Inbox zu klein, verwirft
// die Firmware die ganze Nachricht stumm. Faustformel: 1 + 7 je Key + Nutzdaten.
#define INBOX_SIZE         256
#define OUTBOX_SIZE        64

// ---- Einstellungen ---------------------------------------------------------------
// Das Struct wird als Byte-Dump gespeichert, die Feldreihenfolge ist also Teil
// des Speicherformats. Neue Felder NUR am Ende anhaengen.
#define SETTINGS_KEY       1
#define PKEY_TEMP          2
#define PKEY_TEMP_TIME     3
#define SETTINGS_VERSION   1

typedef struct {
  uint8_t version;
  uint8_t scheme;
  uint8_t tile[T_COUNT];
} Settings;

static Window *s_window;
static Layer *s_canvas;
static GFont s_font_time, s_font_value, s_font_label, s_font_fallback;
static Settings s_settings;
static int s_temp;
static time_t s_temp_time;     // 0 = noch nie empfangen
static bool s_demo;

// ---------------------------------------------------------------------------
// Einstellungen
// ---------------------------------------------------------------------------
static void settings_defaults(void) {
  s_settings.version = SETTINGS_VERSION;
  s_settings.scheme = 0;
  s_settings.tile[T_TR] = DEFAULT_TILE_TR;
  s_settings.tile[T_MR] = DEFAULT_TILE_MR;
  s_settings.tile[T_BL] = DEFAULT_TILE_BL;
  s_settings.tile[T_BR] = DEFAULT_TILE_BR;
}

static void settings_load(void) {
  settings_defaults();
  if (persist_exists(SETTINGS_KEY)) {
    persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  }
  if (s_settings.scheme >= SCHEME_COUNT) s_settings.scheme = 0;
  for (int i = 0; i < T_COUNT; i++) {
    if (s_settings.tile[i] >= SRC_COUNT) s_settings.tile[i] = SRC_STEPS;
  }
  if (persist_exists(PKEY_TEMP_TIME)) {
    s_temp = persist_read_int(PKEY_TEMP);
    s_temp_time = (time_t)persist_read_int(PKEY_TEMP_TIME);
  }
}

static void settings_save(void) {
  s_settings.version = SETTINGS_VERSION;
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static bool uses_source(int src) {
  for (int i = 0; i < T_COUNT; i++) {
    if (s_settings.tile[i] == src) return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Daten
// ---------------------------------------------------------------------------
static void upcase(char *s) {
  for (; *s; s++) if (*s >= 'a' && *s <= 'z') *s -= 'a' - 'A';
}

static int health_today(HealthMetric metric) {
  HealthServiceAccessibilityMask m = health_service_metric_accessible(
      metric, time_start_of_today(), time(NULL));
  if (!(m & HealthServiceAccessibilityMaskAvailable)) return 0;
  return (int)health_service_sum_today(metric);
}

// ISO-8601-Kalenderwoche
static int weeks_in_year(int y) {
  int p  = (y + y / 4 - y / 100 + y / 400) % 7;
  int y1 = y - 1;
  int p1 = (y1 + y1 / 4 - y1 / 100 + y1 / 400) % 7;
  return (p == 4 || p1 == 3) ? 53 : 52;
}

static int iso_week(const struct tm *t) {
  int wday = t->tm_wday == 0 ? 7 : t->tm_wday;   // Mo = 1 ... So = 7
  int year = t->tm_year + 1900;
  int week = (t->tm_yday + 1 - wday + 10) / 7;
  if (week < 1) return weeks_in_year(year - 1);
  if (week > weeks_in_year(year)) return 1;
  return week;
}

static void tile_text(int src, const struct tm *t,
                      char *label, size_t llen, char *value, size_t vlen) {
#ifndef PBL_PLATFORM_EMERY
  if (src == SRC_HEART) src = SRC_ACTIVE;   // kein Pulssensor
#endif
  switch (src) {
    case SRC_WEATHER: {
      snprintf(label, llen, "TEMP");
      bool fresh = s_temp_time != 0 && time(NULL) - s_temp_time < WEATHER_STALE_SEC;
      int temp = s_demo ? DEMO_TEMP : s_temp;
      if (temp < -99) temp = -99;
      if (temp > 999) temp = 999;
      if (fresh || s_demo) snprintf(value, vlen, "%d°", temp);
      else                 snprintf(value, vlen, "--°");
      break;
    }
#ifdef PBL_PLATFORM_EMERY
    case SRC_HEART: {
      snprintf(label, llen, "BPM");
      int bpm = s_demo ? DEMO_HEART
                       : (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
      if (bpm > 0) snprintf(value, vlen, "%d", bpm);
      else         snprintf(value, vlen, "--");
      break;
    }
#endif
    case SRC_ACTIVE: {
      snprintf(label, llen, "MIN");
      int min = s_demo ? DEMO_ACTIVE_MIN : health_today(HealthMetricActiveSeconds) / 60;
      snprintf(value, vlen, "%d", min);
      break;
    }
    case SRC_BATTERY: {
      BatteryChargeState b = battery_state_service_peek();
      snprintf(label, llen, b.is_charging ? "CHG" : "BAT");
      snprintf(value, vlen, "%d%%", s_demo ? DEMO_BATTERY : b.charge_percent);
      break;
    }
    case SRC_STEPS: {
      snprintf(label, llen, "STEP");
      int steps = s_demo ? DEMO_STEPS : health_today(HealthMetricStepCount);
      if (steps > 99999) steps = 99999;
      snprintf(value, vlen, "%d", steps);
      break;
    }
    case SRC_DISTANCE: {
      int m = s_demo ? DEMO_METERS : health_today(HealthMetricWalkedDistanceMeters);
      bool imperial = health_service_get_measurement_system_for_display(
          HealthMetricWalkedDistanceMeters) == MeasurementSystemImperial;
      int tenths = imperial ? (int)((int64_t)m * 10 / 1609) : m / 100;
      if (tenths > 9999) tenths = 9999;
      snprintf(label, llen, imperial ? "MI" : "KM");
      snprintf(value, vlen, "%d.%d", tenths / 10, tenths % 10);
      break;
    }
    case SRC_SLEEP: {
      snprintf(label, llen, "SLEEP");
      int min = s_demo ? DEMO_SLEEP_MIN : health_today(HealthMetricSleepSeconds) / 60;
      if (min > 99 * 60) min = 99 * 60;
      snprintf(value, vlen, "%d:%02d", min / 60, min % 60);
      break;
    }
    case SRC_WEEK:
    default:
      snprintf(label, llen, "WEEK");
      snprintf(value, vlen, "%d", iso_week(t));
      break;
  }
}

// ---------------------------------------------------------------------------
// Zeichnen
// ---------------------------------------------------------------------------
static void fill_tile(GContext *ctx, GRect box, uint32_t hex) {
  graphics_context_set_fill_color(ctx, GColorFromHEX(hex));
  graphics_fill_rect(ctx, box, TILE_RADIUS, GCornersAll);
}

// Zu breite Werte (etwa Schritte in der schmalen Kachel) fallen auf die
// kleinere Systemschrift zurueck, statt abgeschnitten zu werden.
static void draw_fit(GContext *ctx, const char *text, GFont font, GRect r,
                     GTextAlignment align, uint32_t hex) {
  GSize sz = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, 400, r.size.h), GTextOverflowModeFill, align);
  if (sz.w > r.size.w) {
    font = s_font_fallback;
    r.origin.y += 4;
  }
  graphics_context_set_text_color(ctx, GColorFromHEX(hex));
  graphics_draw_text(ctx, text, font, r, GTextOverflowModeFill, align, NULL);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  const Scheme *sc = &SCHEMES[s_settings.scheme];
  GRect b = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorFromHEX(sc->bg));
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // Datum
  static char day_buf[4];
  static char wday_buf[8];
  snprintf(day_buf, sizeof(day_buf), "%d", t->tm_mday);
  strftime(wday_buf, sizeof(wday_buf), "%a", t);
  upcase(wday_buf);
  fill_tile(ctx, BOX_DATE, sc->date_bg);
  draw_fit(ctx, day_buf, s_font_value, RECT_DAY, GTextAlignmentRight, sc->date_fg);
  draw_fit(ctx, wday_buf, s_font_value, RECT_WDAY, GTextAlignmentRight, sc->date_fg);

  // Uhrzeit
  static char time_buf[8];
  strftime(time_buf, sizeof(time_buf), clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  const char *time_txt = (!clock_is_24h_style() && time_buf[0] == '0') ? time_buf + 1 : time_buf;
  fill_tile(ctx, BOX_TIME, sc->time_bg);
  graphics_context_set_text_color(ctx, GColorFromHEX(sc->time_fg));
  graphics_draw_text(ctx, time_txt, s_font_time, RECT_TIME,
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Datenkacheln
  static char label[8];
  static char value[12];
  for (int i = 0; i < T_COUNT; i++) {
    const TileLayout *L = &TILES[i];
    tile_text(s_settings.tile[i], t, label, sizeof(label), value, sizeof(value));
    fill_tile(ctx, L->box, sc->tile_bg[i]);
    draw_fit(ctx, label, s_font_label, L->label, L->align, sc->tile_fg[i]);
    draw_fit(ctx, value, s_font_value, L->value, L->align, sc->tile_fg[i]);
  }
}

// ---------------------------------------------------------------------------
// Ereignisse
// ---------------------------------------------------------------------------
static void request_weather(void) {
  if (!uses_source(SRC_WEATHER)) return;
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
    app_message_outbox_send();
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  layer_mark_dirty(s_canvas);
  if (tick_time->tm_min % WEATHER_INTERVAL_MIN == 0) request_weather();
}

static void health_handler(HealthEventType event, void *context) {
  if (event != HealthEventSleepUpdate || uses_source(SRC_SLEEP)) {
    layer_mark_dirty(s_canvas);
  }
}

static void battery_handler(BatteryChargeState state) {
  if (uses_source(SRC_BATTERY)) layer_mark_dirty(s_canvas);
}

static void bt_handler(bool connected) {
  if (connected) request_weather();
}

// Clay schickt Auswahllisten als Zeichenkette, emu_config.py als Zahl
static int tuple_int(const Tuple *t) {
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  switch (t->length) {
    case 1:  return t->type == TUPLE_INT ? t->value->int8  : t->value->uint8;
    case 2:  return t->type == TUPLE_INT ? t->value->int16 : t->value->uint16;
    default: return t->value->int32;
  }
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  bool changed = false;
  Tuple *t;
  if ((t = dict_find(iter, MESSAGE_KEY_SCHEME))) {
    s_settings.scheme = (uint8_t)tuple_int(t);
    changed = true;
  }
  const uint32_t keys[T_COUNT] = {
    MESSAGE_KEY_TILE_TR, MESSAGE_KEY_TILE_MR, MESSAGE_KEY_TILE_BL, MESSAGE_KEY_TILE_BR
  };
  for (int i = 0; i < T_COUNT; i++) {
    if ((t = dict_find(iter, keys[i]))) {
      s_settings.tile[i] = (uint8_t)tuple_int(t);
      changed = true;
    }
  }
  if ((t = dict_find(iter, MESSAGE_KEY_DEMO))) {
    s_demo = tuple_int(t) != 0;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_TEMP))) {
    s_temp = tuple_int(t);
    s_temp_time = time(NULL);
    persist_write_int(PKEY_TEMP, s_temp);
    persist_write_int(PKEY_TEMP_TIME, (int32_t)s_temp_time);
  }
  if (changed) {
    settings_save();
    settings_load();
  }
  layer_mark_dirty(s_canvas);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
}

static void init(void) {
  settings_load();
  s_font_time     = fonts_load_custom_font(resource_get_handle(RES_FONT_TIME));
  s_font_value    = fonts_load_custom_font(resource_get_handle(RES_FONT_VALUE));
  s_font_label    = fonts_load_custom_font(resource_get_handle(RES_FONT_LABEL));
  s_font_fallback = fonts_get_system_font(FALLBACK_FONT_KEY);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  health_service_events_subscribe(health_handler, NULL);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_handler,
  });

  app_message_register_inbox_received(inbox_received);
  AppMessageResult r = app_message_open(INBOX_SIZE, OUTBOX_SIZE);
  if (r != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open: %d", (int)r);
  }
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  health_service_events_unsubscribe();
  connection_service_unsubscribe();
  window_destroy(s_window);
  fonts_unload_custom_font(s_font_time);
  fonts_unload_custom_font(s_font_value);
  fonts_unload_custom_font(s_font_label);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
