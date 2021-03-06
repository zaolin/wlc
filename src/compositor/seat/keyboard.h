#ifndef _WLC_KEYBOARD_H_
#define _WLC_KEYBOARD_H_

#include <stdbool.h>
#include <wayland-util.h>

enum wl_keyboard_key_state;

struct wl_list;
struct wl_resource;
struct xkb_state;
struct wlc_keymap;
struct wlc_view;
struct wlc_client;
struct wlc_modifiers;

struct wlc_keyboard {
   struct wlc_compositor *compositor;
   struct xkb_state *state;
   struct wlc_view *focus;
   struct wl_array keys;

   struct {
      uint32_t depressed;
      uint32_t latched;
      uint32_t locked;
      uint32_t group;
   } mods;
};

bool wlc_keyboard_request_key(struct wlc_keyboard *keyboard, uint32_t time, const struct wlc_modifiers *mods, uint32_t key, enum wl_keyboard_key_state state);
bool wlc_keyboard_update(struct wlc_keyboard *keyboard, uint32_t key, enum wl_keyboard_key_state state);
void wlc_keyboard_key(struct wlc_keyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state);
void wlc_keyboard_focus(struct wlc_keyboard *keyboard, struct wlc_view *view);
void wlc_keyboard_remove_client_for_resource(struct wlc_keyboard *keyboard, struct wl_resource *resource);
bool wlc_keyboard_set_keymap(struct wlc_keyboard *keyboard, struct wlc_keymap *keymap);
void wlc_keyboard_free(struct wlc_keyboard *keyboard);
struct wlc_keyboard* wlc_keyboard_new(struct wlc_keymap *keymap, struct wlc_compositor *compositor);

#endif /* _WLC_KEYBOARD_H_ */
