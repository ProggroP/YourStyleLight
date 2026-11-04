// Your Style Light -- schlichtes Analog-Zifferblatt im Stil von "Your Style":
// roemische Ziffern bei 12/3/6/9, Striche dazwischen, dicke Zeiger mit
// kleiner Nabe. Innen oben das Wetter mit Tagestief und -hoch, links die
// Mondphase, rechts Wochentag und Datum, unten die Schritte.
//
// Gezeichnet wird einmal pro Minute und bei neuen Einstellungen/Wetterdaten.

#include <pebble.h>

// ---------------------------------------------------------------------------
// Layout je Plattform
// ---------------------------------------------------------------------------

#ifdef PBL_PLATFORM_EMERY
  // 200 x 228, eckig: Ziffern und Striche laufen am Rechteck entlang
  #define NUMERAL_INSET      22   // Abstand Ziffernmitte -> Displayrand
  #define TICK_INSET          4   // Abstand Strich-Aussenende -> Rand
  #define TICK_LENGTH        12
  #define TICK_WIDTH          3
  #define HOUR_LENGTH        58
  #define HOUR_WIDTH         15
  #define MINUTE_LENGTH      86
  #define MINUTE_WIDTH       11
  #define HOUR_INLAY_W        5   // Einlage im Zeiger
  #define MINUTE_INLAY_W      3
  #define HUB_RADIUS         10
  #define AXIS_RADIUS         3
  #define ICON_SIZE          40   // muss zu tools/make_icons.py passen
  #define WEATHER_Y          42   // Oberkante des Icons
  #define TEMP_HI_FONT       FONT_KEY_GOTHIC_28_BOLD
  #define TEMP_LO_FONT       FONT_KEY_GOTHIC_28
  #define TEMP_PAD           10   // Leerraum ueber den Ziffern der Schrift
  #define TEMP_DIGIT_H       20   // Ziffernhoehe
  #define TEMP_LINE_GAP       4   // Abstand hoch -> tief
  #define MOON_OFFSET        40   // Mondmitte links von der Mitte
  #define MOON_RADIUS        12
  // Wochentag ueber dem Tag: zwischen Nabe und "III" ist nur rund 50 px
  // Platz, "Wed 28" in grosser Schrift passt nicht in eine Zeile
  #define DATE_DAY_FONT      FONT_KEY_GOTHIC_24
  #define DATE_DAY_PAD        7   // Leerraum ueber der Schrift
  #define DATE_DAY_H         17   // Versalhoehe
  #define DATE_NUM_FONT      FONT_KEY_GOTHIC_28_BOLD
  #define DATE_OFFSET        40   // Datumsmitte rechts von der Mitte
  #define DATE_WIN_W         38   // Datumsfenster
  #define DATE_WIN_H         50
  #define STEPS_OFFSET       34   // Oberkante der Schritte unter der Mitte
  #define STEPS_BAR_W        46
#else
  // 260 x 260, rund
  #define NUMERAL_RADIUS    106   // Ziffernmitte
  #define TICK_OUTER        125
  #define TICK_LENGTH        13
  #define TICK_WIDTH          3
  #define HOUR_LENGTH        70
  #define HOUR_WIDTH         17
  #define MINUTE_LENGTH     102
  #define MINUTE_WIDTH       13
  #define HOUR_INLAY_W        7
  #define MINUTE_INLAY_W      5
  #define HUB_RADIUS         11
  #define AXIS_RADIUS         3
  #define ICON_SIZE          46
  #define WEATHER_Y          45
  #define TEMP_HI_FONT       FONT_KEY_GOTHIC_28_BOLD
  #define TEMP_LO_FONT       FONT_KEY_GOTHIC_28
  #define TEMP_PAD           10
  #define TEMP_DIGIT_H       20
  #define TEMP_LINE_GAP       4
  #define MOON_OFFSET        62
  #define MOON_RADIUS        17
  #define DATE_DAY_FONT      FONT_KEY_GOTHIC_28
  #define DATE_DAY_PAD       10
  #define DATE_DAY_H         20
  #define DATE_NUM_FONT      FONT_KEY_GOTHIC_28_BOLD
  #define DATE_OFFSET        56
  #define DATE_WIN_W         52
  #define DATE_WIN_H         56
  #define STEPS_OFFSET       34
  #define STEPS_BAR_W        56
#endif

#define ICON_GAP              4   // Icon -> Temperaturspalte
#define DATE_NUM_PAD         10   // Gothic 28 Bold: Leerraum ueber den Ziffern
#define DATE_NUM_H           20   // Ziffernhoehe
#define DATE_LINE_GAP         4   // Wochentag -> Tag
#define MOON_SEGMENTS        24   // Polygon-Aufloesung fuer Mondscheibe
#define MOON_RING_GAP         4   // Ring um den Mond
#define MINUTE_TICK_LENGTH    5
#define INLAY_HUB_GAP         5   // Einlage beginnt so weit ausserhalb der Nabe
#define INLAY_TIP_GAP         4   // und endet so weit vor der Spitze
#define STEPS_BAR_H           4
#define DATE_WIN_RADIUS       6

#define INLAY_NONE            0
#define INLAY_BG              1
#define INLAY_ACCENT          2

// ---------------------------------------------------------------------------
// Wetter
// ---------------------------------------------------------------------------

// Zustandscodes, Reihenfolge = Position in der Sprite-Leiste,
// muessen zu tools/make_icons.py und src/pkjs/index.js passen
#define COND_COUNT           12
#define COND_NONE           255

#define WEATHER_INTERVAL_MIN 30
#define WEATHER_STALE_SEC   (3 * 60 * 60)

#define INBOX_SIZE          512
#define OUTBOX_SIZE          64

// ---------------------------------------------------------------------------
// Einstellungen
// ---------------------------------------------------------------------------

#define PKEY_SETTINGS         1
#define PKEY_WEATHER          2

#define LANG_EN               0
#define LANG_DE               1

// Neue Felder nur ans Ende haengen: das Struct wird roh gespeichert
typedef struct {
  GColor bg;
  GColor dial;
  GColor hour;
  GColor minute;
  GColor hub;
  GColor text;
  GColor accent;
  GColor moon_lit;
  GColor moon_dark;
  bool show_weather;
  bool show_moon;
  bool show_date;
  bool show_steps;
  bool show_ticks;
  uint8_t language;
  bool minute_track;
  uint8_t inlay;
  bool date_window;
  bool moon_ring;
  bool steps_bar;
  uint16_t step_goal;
} Settings;

typedef struct {
  int16_t lo;
  int16_t hi;
  uint8_t cond;
  int32_t time;     // Zeitpunkt des Abrufs, fuer die Veraltet-Pruefung
} Weather;

static Settings s_settings;
static Weather s_weather = { .cond = COND_NONE };

static Window *s_window;
static Layer *s_canvas;
static GBitmap *s_icons;

// Standard: schwarz auf weiss wie das Vorbild
static void default_settings(void) {
  s_settings = (Settings) {
    .bg        = GColorWhite,
    .dial      = GColorBlack,
    .hour      = GColorBlack,
    .minute    = GColorBlack,
    .hub       = GColorBlack,
    .text      = GColorBlack,
    .accent    = GColorBlack,
    .moon_lit  = GColorWhite,
    .moon_dark = GColorBlack,
    .show_weather = true,
    .show_moon    = true,
    .show_date    = true,
    .show_steps   = true,
    .show_ticks   = true,
    .language     = LANG_EN,
    .minute_track = false,
    .inlay        = INLAY_NONE,
    .date_window  = true,
    .moon_ring    = true,
    .steps_bar    = true,
    .step_goal    = 10000,
  };
}

// ---------------------------------------------------------------------------
// Hilfen
// ---------------------------------------------------------------------------

static GPoint polar(GPoint c, int32_t angle, int32_t r) {
  return GPoint(c.x + (int16_t)(sin_lookup(angle) * r / TRIG_MAX_RATIO),
                c.y - (int16_t)(cos_lookup(angle) * r / TRIG_MAX_RATIO));
}

// Text so setzen, dass seine sichtbare Mitte auf p liegt. Die Gothic- und
// Bitham-Schriften haben oben Leerraum, deshalb wird die Inhaltsgroesse
// gemessen und der Kasten um den Oberrand der Schrift verschoben.
static void draw_centered(GContext *ctx, const char *s, GFont font, GPoint p,
                          int top_pad) {
  GRect box = GRect(p.x - 70, 0, 140, 60);
  GSize sz = graphics_text_layout_get_content_size(s, font, box,
                GTextOverflowModeFill, GTextAlignmentCenter);
  box.origin.y = p.y - top_pad - (sz.h - top_pad) / 2;
  graphics_draw_text(ctx, s, font, box, GTextOverflowModeFill,
                     GTextAlignmentCenter, NULL);
}

static int text_width(const char *s, GFont font) {
  return graphics_text_layout_get_content_size(s, font, GRect(0, 0, 200, 60),
           GTextOverflowModeFill, GTextAlignmentLeft).w;
}

// ---------------------------------------------------------------------------
// Zifferblatt
// ---------------------------------------------------------------------------

#ifdef PBL_PLATFORM_EMERY
// Punkt, an dem der Strahl vom Mittelpunkt das um inset verkleinerte
// Display-Rechteck trifft
static GPoint rect_point(GRect b, GPoint c, int32_t angle, int inset) {
  int32_t s = sin_lookup(angle), co = cos_lookup(angle);
  int32_t hw = b.size.w / 2 - inset, hh = b.size.h / 2 - inset;
  int32_t as = s < 0 ? -s : s, ac = co < 0 ? -co : co;
  // r = min(hw/|sin|, hh/|cos|), in Pixeln
  int32_t r;
  if (as * hh > ac * hw) r = hw * TRIG_MAX_RATIO / as;
  else                   r = hh * TRIG_MAX_RATIO / ac;
  return polar(c, angle, r);
}
#endif

// Strich der Laenge length vom Rand nach innen
static void draw_tick(GContext *ctx, GRect b, GPoint c, int32_t a, int length) {
#ifdef PBL_PLATFORM_EMERY
  GPoint outer = rect_point(b, c, a, TICK_INSET);
  int32_t r = (outer.x - c.x) * (outer.x - c.x) + (outer.y - c.y) * (outer.y - c.y);
  // Laenge des Strahls bis zum Rand, ganzzahlige Wurzel
  int32_t len = 0;
  while ((len + 1) * (len + 1) <= r) len++;
  GPoint inner = polar(c, a, len - length);
#else
  GPoint outer = polar(c, a, TICK_OUTER);
  GPoint inner = polar(c, a, TICK_OUTER - length);
#endif
  graphics_draw_line(ctx, inner, outer);
}

static void draw_dial(GContext *ctx, GRect b, GPoint c) {
  static const char *const numerals[4] = { "XII", "III", "VI", "IX" };
  GFont font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);

  graphics_context_set_text_color(ctx, s_settings.dial);
  for (int i = 0; i < 4; i++) {
    int32_t a = TRIG_MAX_ANGLE * i / 4;
#ifdef PBL_PLATFORM_EMERY
    GPoint p = rect_point(b, c, a, NUMERAL_INSET);
#else
    GPoint p = polar(c, a, NUMERAL_RADIUS);
#endif
    draw_centered(ctx, numerals[i], font, p, 9);
  }

  graphics_context_set_stroke_color(ctx, s_settings.dial);
  if (s_settings.minute_track) {
    graphics_context_set_stroke_width(ctx, 1);
    for (int m = 0; m < 60; m++) {
      if (m % 5 == 0) continue;
      draw_tick(ctx, b, c, TRIG_MAX_ANGLE * m / 60, MINUTE_TICK_LENGTH);
    }
  }
  if (s_settings.show_ticks) {
    graphics_context_set_stroke_width(ctx, TICK_WIDTH);
    for (int h = 1; h < 12; h++) {
      if (h % 3 == 0) continue;
      draw_tick(ctx, b, c, TRIG_MAX_ANGLE * h / 12, TICK_LENGTH);
    }
  }
}

// ---------------------------------------------------------------------------
// Wetter
// ---------------------------------------------------------------------------

static void tint_icons(void) {
  if (!s_icons) return;
  GColor *pal = gbitmap_get_palette(s_icons);
  if (!pal) return;
  // 2-Bit-Palette: die Deckung bleibt, nur die Farbe wird getauscht
  for (int i = 0; i < 4; i++) {
    pal[i].argb = (pal[i].argb & 0xC0) | (s_settings.text.argb & 0x3F);
  }
}

static void draw_weather(GContext *ctx, GPoint c) {
  if (s_weather.cond >= COND_COUNT) return;
  if (time(NULL) - s_weather.time > WEATHER_STALE_SEC) return;

  GFont hi_font = fonts_get_system_font(TEMP_HI_FONT);
  GFont lo_font = fonts_get_system_font(TEMP_LO_FONT);
  char hi[12], lo[12];
  snprintf(hi, sizeof(hi), "%d°", s_weather.hi);
  snprintf(lo, sizeof(lo), "%d°", s_weather.lo);
  int tw = text_width(hi, hi_font);
  int lw = text_width(lo, lo_font);
  if (lw > tw) tw = lw;

  // Icon und Temperaturspalte als Block mittig
  int total = ICON_SIZE + ICON_GAP + tw;
  int x = c.x - total / 2;
  int y = WEATHER_Y;

  gbitmap_set_bounds(s_icons, GRect(s_weather.cond * ICON_SIZE, 0, ICON_SIZE, ICON_SIZE));
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_icons, GRect(x, y, ICON_SIZE, ICON_SIZE));
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  // Hoch und tief uebereinander, der Ziffernblock mittig neben dem Icon
  int tx = x + ICON_SIZE + ICON_GAP;
  int block = 2 * TEMP_DIGIT_H + TEMP_LINE_GAP;
  int ty = y + (ICON_SIZE - block) / 2 - TEMP_PAD;
  int step = TEMP_DIGIT_H + TEMP_LINE_GAP;
  graphics_context_set_text_color(ctx, s_settings.accent);
  graphics_draw_text(ctx, hi, hi_font, GRect(tx, ty, tw + 4, TEMP_PAD + TEMP_DIGIT_H + 10),
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, s_settings.text);
  graphics_draw_text(ctx, lo, lo_font, GRect(tx, ty + step, tw + 4, TEMP_PAD + TEMP_DIGIT_H + 10),
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

// ---------------------------------------------------------------------------
// Mondphase (Rechnung aus Your Style uebernommen)
// ---------------------------------------------------------------------------

static void draw_moon(GContext *ctx, GPoint mc, int32_t radius) {
  // bekannter Neumond (6.1.2000 18:14 UTC) und synodischer Monat in Sekunden
  const int32_t reference_new_moon = 947182440;
  const int32_t synodic_seconds    = 2551443;

  int32_t age = ((int32_t)time(NULL) - reference_new_moon) % synodic_seconds;
  if (age < 0) age += synodic_seconds;
  int32_t angle = (int32_t)(((int64_t)age * TRIG_MAX_ANGLE) / synodic_seconds);

  int32_t phase_cos = cos_lookup(angle);
  int32_t term_x = (int32_t)(((int64_t)radius * phase_cos) / TRIG_MAX_RATIO);
  bool bright_right = angle < TRIG_MAX_ANGLE / 2;   // zunehmend: rechts hell

  // Grund dunkel, helle Haelfte darauf, dann die Terminator-Ellipse
  graphics_context_set_fill_color(ctx, s_settings.moon_dark);
  graphics_fill_circle(ctx, mc, radius);

  GPoint half[MOON_SEGMENTS / 2 + 1];
  int32_t start = bright_right ? 0 : TRIG_MAX_ANGLE / 2;
  for (int i = 0; i <= MOON_SEGMENTS / 2; i++) {
    half[i] = polar(mc, start + TRIG_MAX_ANGLE * i / MOON_SEGMENTS, radius);
  }
  GPath half_path = { .num_points = MOON_SEGMENTS / 2 + 1, .points = half };
  graphics_context_set_fill_color(ctx, s_settings.moon_lit);
  gpath_draw_filled(ctx, &half_path);

  GPoint ell[MOON_SEGMENTS];
  int32_t rx = term_x < 0 ? -term_x : term_x;
  for (int i = 0; i < MOON_SEGMENTS; i++) {
    int32_t a = TRIG_MAX_ANGLE * i / MOON_SEGMENTS;
    ell[i].x = mc.x + (int16_t)(sin_lookup(a) * rx / TRIG_MAX_RATIO);
    ell[i].y = mc.y - (int16_t)(cos_lookup(a) * radius / TRIG_MAX_RATIO);
  }
  GPath ell_path = { .num_points = MOON_SEGMENTS, .points = ell };
  // nahe Neumond (cos > 0) ist die Ellipse dunkel, nahe Vollmond hell
  graphics_context_set_fill_color(ctx, phase_cos > 0 ? s_settings.moon_dark
                                                     : s_settings.moon_lit);
  gpath_draw_filled(ctx, &ell_path);

  graphics_context_set_stroke_color(ctx, s_settings.text);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, mc, radius);

  if (s_settings.moon_ring) {
    graphics_context_set_stroke_color(ctx, s_settings.dial);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, mc, radius + MOON_RING_GAP);
  }
}

// ---------------------------------------------------------------------------
// Datum und Schritte
// ---------------------------------------------------------------------------

static void draw_date(GContext *ctx, GPoint p, const struct tm *now) {
  static const char *const days_en[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  static const char *const days_de[7] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };
  const char *day = (s_settings.language == LANG_DE ? days_de : days_en)[now->tm_wday % 7];
  char num[4];
  snprintf(num, sizeof(num), "%d", now->tm_mday);

  GFont day_font = fonts_get_system_font(DATE_DAY_FONT);
  GFont num_font = fonts_get_system_font(DATE_NUM_FONT);

  if (s_settings.date_window) {
    graphics_context_set_stroke_color(ctx, s_settings.dial);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_round_rect(ctx, GRect(p.x - DATE_WIN_W / 2, p.y - DATE_WIN_H / 2,
                                        DATE_WIN_W, DATE_WIN_H), DATE_WIN_RADIUS);
  }

  // Wochentag oben, Tag darunter, der Block mittig um p.y
  int top = p.y - (DATE_DAY_H + DATE_LINE_GAP + DATE_NUM_H) / 2;
  graphics_context_set_text_color(ctx, s_settings.text);
  graphics_draw_text(ctx, day, day_font,
                     GRect(p.x - 35, top - DATE_DAY_PAD, 70, DATE_DAY_PAD + DATE_DAY_H + 12),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  graphics_context_set_text_color(ctx, s_settings.accent);
  graphics_draw_text(ctx, num, num_font,
                     GRect(p.x - 35, top + DATE_DAY_H + DATE_LINE_GAP - DATE_NUM_PAD, 70,
                           DATE_NUM_PAD + DATE_NUM_H + 12),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void draw_steps(GContext *ctx, GPoint c) {
  HealthServiceAccessibilityMask mask =
      health_service_metric_accessible(HealthMetricStepCount,
                                       time_start_of_today(), time(NULL));
  if (!(mask & HealthServiceAccessibilityMaskAvailable)) return;

  int steps = (int)health_service_sum_today(HealthMetricStepCount);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", steps);
  const char *label = s_settings.language == LANG_DE ? "SCHRITTE" : "STEPS";

  int y = c.y + STEPS_OFFSET;
  graphics_context_set_text_color(ctx, s_settings.text);
  graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(c.x - 50, y - 4, 100, 18),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  graphics_context_set_text_color(ctx, s_settings.accent);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(c.x - 50, y + 4, 100, 28),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  if (s_settings.steps_bar && s_settings.step_goal > 0) {
    // Fortschritt zum Tagesziel unter der Zahl, voll ab Erreichen
    GRect bar = GRect(c.x - STEPS_BAR_W / 2, y + 32, STEPS_BAR_W, STEPS_BAR_H);
    int fill = steps >= s_settings.step_goal ? STEPS_BAR_W
             : STEPS_BAR_W * steps / s_settings.step_goal;
    graphics_context_set_stroke_color(ctx, s_settings.text);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_round_rect(ctx, bar, STEPS_BAR_H / 2);
    if (fill > 0) {
      graphics_context_set_fill_color(ctx, s_settings.accent);
      graphics_fill_rect(ctx, GRect(bar.origin.x, bar.origin.y, fill, STEPS_BAR_H),
                         STEPS_BAR_H / 2, GCornersAll);
    }
  }
}

// ---------------------------------------------------------------------------
// Zeiger
// ---------------------------------------------------------------------------

static void draw_hand(GContext *ctx, GPoint c, int32_t a, int length, int width,
                      int inlay_width, GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, width);
  graphics_draw_line(ctx, c, polar(c, a, length));

  if (s_settings.inlay == INLAY_NONE) return;
  GColor inlay = s_settings.inlay == INLAY_ACCENT ? s_settings.accent : s_settings.bg;
  // Akzent auf einem Zeiger in Akzentfarbe waere unsichtbar
  if (gcolor_equal(inlay, color)) inlay = s_settings.bg;
  graphics_context_set_stroke_color(ctx, inlay);
  graphics_context_set_stroke_width(ctx, inlay_width);
  graphics_draw_line(ctx, polar(c, a, HUB_RADIUS + INLAY_HUB_GAP),
                          polar(c, a, length - INLAY_TIP_GAP));
}

static void draw_hands(GContext *ctx, GPoint c, const struct tm *now) {
  int32_t min_a  = TRIG_MAX_ANGLE * now->tm_min / 60;
  int32_t hour_a = TRIG_MAX_ANGLE * ((now->tm_hour % 12) * 60 + now->tm_min) / 720;

  draw_hand(ctx, c, hour_a, HOUR_LENGTH, HOUR_WIDTH, HOUR_INLAY_W, s_settings.hour);
  draw_hand(ctx, c, min_a, MINUTE_LENGTH, MINUTE_WIDTH, MINUTE_INLAY_W, s_settings.minute);

  graphics_context_set_fill_color(ctx, s_settings.hub);
  graphics_fill_circle(ctx, c, HUB_RADIUS);
  graphics_context_set_fill_color(ctx, s_settings.bg);
  graphics_fill_circle(ctx, c, AXIS_RADIUS);
}

// ---------------------------------------------------------------------------
// Zeichnen
// ---------------------------------------------------------------------------

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  GPoint c = grect_center_point(&b);
  time_t t = time(NULL);
  struct tm *now = localtime(&t);

  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, s_settings.bg);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  draw_dial(ctx, b, c);
  if (s_settings.show_weather) draw_weather(ctx, c);
  if (s_settings.show_moon)    draw_moon(ctx, GPoint(c.x - MOON_OFFSET, c.y), MOON_RADIUS);
  if (s_settings.show_date)    draw_date(ctx, GPoint(c.x + DATE_OFFSET, c.y), now);
  if (s_settings.show_steps)   draw_steps(ctx, c);
  draw_hands(ctx, c, now);
}

// ---------------------------------------------------------------------------
// Ereignisse
// ---------------------------------------------------------------------------

static void request_weather(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
    app_message_outbox_send();
  }
}

static void tick_handler(struct tm *now, TimeUnits changed) {
  if (s_settings.show_weather && now->tm_min % WEATHER_INTERVAL_MIN == 0) {
    request_weather();
  }
  layer_mark_dirty(s_canvas);
}

static void read_color(DictionaryIterator *iter, uint32_t key, GColor *out) {
  Tuple *t = dict_find(iter, key);
  if (t) *out = GColorFromHEX(t->value->int32);
}

static void read_bool(DictionaryIterator *iter, uint32_t key, bool *out) {
  Tuple *t = dict_find(iter, key);
  if (t) *out = t->value->int32 != 0;
}

// Clay schickt Select-Werte als String
static void read_choice(DictionaryIterator *iter, uint32_t key, uint8_t *out) {
  Tuple *t = dict_find(iter, key);
  if (!t) return;
  *out = (t->type == TUPLE_CSTRING) ? (uint8_t)atoi(t->value->cstring)
                                    : (uint8_t)t->value->int32;
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  // Wetter
  Tuple *cond = dict_find(iter, MESSAGE_KEY_CONDITION);
  Tuple *lo   = dict_find(iter, MESSAGE_KEY_TEMP_LO);
  Tuple *hi   = dict_find(iter, MESSAGE_KEY_TEMP_HI);
  if (cond && lo && hi) {
    s_weather.cond = (uint8_t)cond->value->int32;
    s_weather.lo   = (int16_t)lo->value->int32;
    s_weather.hi   = (int16_t)hi->value->int32;
    s_weather.time = (int32_t)time(NULL);
    persist_write_data(PKEY_WEATHER, &s_weather, sizeof(s_weather));
    layer_mark_dirty(s_canvas);
  }

  // Einstellungen: nur wenn die Nachricht von Clay stammt
  if (!dict_find(iter, MESSAGE_KEY_COLOR_BG)) return;
  read_color(iter, MESSAGE_KEY_COLOR_BG,        &s_settings.bg);
  read_color(iter, MESSAGE_KEY_COLOR_DIAL,      &s_settings.dial);
  read_color(iter, MESSAGE_KEY_COLOR_HOUR,      &s_settings.hour);
  read_color(iter, MESSAGE_KEY_COLOR_MINUTE,    &s_settings.minute);
  read_color(iter, MESSAGE_KEY_COLOR_HUB,       &s_settings.hub);
  read_color(iter, MESSAGE_KEY_COLOR_TEXT,      &s_settings.text);
  read_color(iter, MESSAGE_KEY_COLOR_ACCENT,    &s_settings.accent);
  read_color(iter, MESSAGE_KEY_COLOR_MOON_LIT,  &s_settings.moon_lit);
  read_color(iter, MESSAGE_KEY_COLOR_MOON_DARK, &s_settings.moon_dark);
  read_bool(iter, MESSAGE_KEY_SHOW_WEATHER, &s_settings.show_weather);
  read_bool(iter, MESSAGE_KEY_SHOW_MOON,    &s_settings.show_moon);
  read_bool(iter, MESSAGE_KEY_SHOW_DATE,    &s_settings.show_date);
  read_bool(iter, MESSAGE_KEY_SHOW_STEPS,   &s_settings.show_steps);
  read_bool(iter, MESSAGE_KEY_SHOW_TICKS,   &s_settings.show_ticks);
  read_choice(iter, MESSAGE_KEY_LANGUAGE, &s_settings.language);
  read_choice(iter, MESSAGE_KEY_HAND_INLAY, &s_settings.inlay);
  read_bool(iter, MESSAGE_KEY_MINUTE_TRACK, &s_settings.minute_track);
  read_bool(iter, MESSAGE_KEY_DATE_WINDOW,  &s_settings.date_window);
  read_bool(iter, MESSAGE_KEY_MOON_RING,    &s_settings.moon_ring);
  read_bool(iter, MESSAGE_KEY_STEPS_BAR,    &s_settings.steps_bar);
  Tuple *goal = dict_find(iter, MESSAGE_KEY_STEP_GOAL);
  if (goal) s_settings.step_goal = (uint16_t)goal->value->int32;
  persist_write_data(PKEY_SETTINGS, &s_settings, sizeof(s_settings));
  tint_icons();
  layer_mark_dirty(s_canvas);
}

static void load_state(void) {
  default_settings();
  if (persist_exists(PKEY_SETTINGS)) {
    persist_read_data(PKEY_SETTINGS, &s_settings, sizeof(s_settings));
  }
  if (persist_exists(PKEY_WEATHER)) {
    persist_read_data(PKEY_WEATHER, &s_weather, sizeof(s_weather));
  }
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
  load_state();
  s_icons = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WEATHER);
  tint_icons();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_register_inbox_received(inbox_received);
  AppMessageResult res = app_message_open(INBOX_SIZE, OUTBOX_SIZE);
  if (res != APP_MSG_OK) APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open: %d", (int)res);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
  gbitmap_destroy(s_icons);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
