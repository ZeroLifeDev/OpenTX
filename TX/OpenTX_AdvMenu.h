#ifndef OPENTX_ADV_MENU_H
#define OPENTX_ADV_MENU_H

#include <stdint.h>

enum MenuItemType {
  MENU_INT = 0,
  MENU_TOGGLE = 1,
  MENU_ENUM = 2,
  MENU_ACTION = 3
};

struct MenuItem {
  const char *label;
  MenuItemType type;
  int16_t *value;
  int16_t minv;
  int16_t maxv;
  int16_t step;
  const char **options;
  uint8_t optionCount;
  void (*action)();
};

struct MenuPage {
  const char *title;
  MenuItem *items;
  uint8_t count;
};

// Render callback types (implemented in TX.ino)
typedef void (*MenuDrawRowFn)(int y, bool sel, const char *label, const char *value, bool edit);
typedef void (*MenuDrawTitleFn)(const char *title);

static inline int16_t menuClamp(int16_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline void menuAdjust(MenuItem &it, int dir) {
  if (it.type == MENU_TOGGLE) {
    *it.value = (*it.value == 0) ? 1 : 0;
    return;
  }
  if (it.type == MENU_ENUM) {
    int16_t v = *it.value + dir;
    if (v < 0) v = it.optionCount - 1;
    if (v >= it.optionCount) v = 0;
    *it.value = v;
    return;
  }
  if (it.type == MENU_INT) {
    int16_t v = *it.value + dir * it.step;
    *it.value = menuClamp(v, it.minv, it.maxv);
    return;
  }
}

static inline const char *menuValueText(const MenuItem &it, char *buf, int buflen) {
  if (it.type == MENU_TOGGLE) return (*it.value) ? "ON" : "OFF";
  if (it.type == MENU_ENUM && it.options && it.optionCount) {
    int16_t idx = *it.value;
    if (idx < 0) idx = 0;
    if (idx >= it.optionCount) idx = it.optionCount - 1;
    return it.options[idx];
  }
  if (it.type == MENU_ACTION) return ">";
  if (it.type == MENU_INT) {
    if (buflen > 0) {
      int n = 0;
      int16_t v = *it.value;
      if (v < 0 && buflen > 1) { buf[n++] = '-'; v = (int16_t)(-v); }
      char tmp[8];
      int t = 0;
      do { tmp[t++] = (char)('0' + (v % 10)); v /= 10; } while (v && t < 7);
      for (int i = t - 1; i >= 0 && n < buflen - 1; i--) buf[n++] = tmp[i];
      buf[n] = '\0';
      return buf;
    }
  }
  return "";
}

static inline void menuRender(const MenuPage &page, uint8_t index, uint8_t top, bool edit,
                              MenuDrawTitleFn drawTitle, MenuDrawRowFn drawRow) {
  drawTitle(page.title);
  if (index < top) top = index;
  if (index > top + 6) top = index - 6;
  char buf[12];
  for (uint8_t i = 0; i < 7; i++) {
    uint8_t idx = top + i;
    if (idx >= page.count) break;
    const MenuItem &it = page.items[idx];
    const char *val = menuValueText(it, buf, sizeof(buf));
    drawRow(24 + i * 14, idx == index, it.label, val, edit && idx == index);
  }
}

#endif // OPENTX_ADV_MENU_H
