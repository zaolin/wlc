#include "wlc.h"
#include "keymap.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include <wayland-server.h>

const char* WLC_MOD_NAMES[WLC_MOD_LAST] = {
   XKB_MOD_NAME_SHIFT,
   XKB_MOD_NAME_CAPS,
   XKB_MOD_NAME_CTRL,
   XKB_MOD_NAME_ALT,
   "Mod2",
   "Mod3",
   XKB_MOD_NAME_LOGO,
   "Mod5",
};

const char* WLC_LED_NAMES[WLC_LED_LAST] = {
   XKB_LED_NAME_NUM,
   XKB_LED_NAME_CAPS,
   XKB_LED_NAME_SCROLL
};

static char*
csprintf(const char *fmt, ...)
{
   assert(fmt);

   va_list args;
   va_start(args, fmt);
   size_t len = vsnprintf(NULL, 0, fmt, args) + 1;
   va_end(args);

   char *buffer;
   if (!(buffer = calloc(1, len)))
      return NULL;

   va_start(args, fmt);
   vsnprintf(buffer, len, fmt, args);
   va_end(args);
   return buffer;
}

static int
set_cloexec_or_close(int fd)
{
   if (fd == -1)
      return -1;

   long flags = fcntl(fd, F_GETFD);
   if (flags == -1)
      goto err;

   if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
      goto err;

   return fd;

err:
   close(fd);
   return -1;
}

static int
create_tmpfile_cloexec(char *tmpname)
{
   int fd;

#ifdef HAVE_MKOSTEMP
   if ((fd = mkostemp(tmpname, O_CLOEXEC)) >= 0)
      unlink(tmpname);
#else
   if ((fd = mkstemp(tmpname)) >= 0) {
      fd = set_cloexec_or_close(fd);
      unlink(tmpname);
   }
#endif

   return fd;
}

static int
os_create_anonymous_file(off_t size)
{
   static const char template[] = "/loliwm-shared-XXXXXX";
   int fd;
   int ret;

   const char *path;
   if (!(path = getenv("XDG_RUNTIME_DIR")) || strlen(path) <= 0) {
      errno = ENOENT;
      return -1;
   }

   char *name;
   int ts = (path[strlen(path) - 1] == '/');
   if (!(name = csprintf("%s%s%s", path, (ts ? "" : "/"), template)))
      return -1;

   fd = create_tmpfile_cloexec(name);
   free(name);

   if (fd < 0)
      return -1;

#ifdef HAVE_POSIX_FALLOCATE
   if ((ret = posix_fallocate(fd, 0, size)) != 0) {
      close(fd);
      errno = ret;
      return -1;
   }
#else
   if ((ret = ftruncate(fd, size)) < 0) {
      close(fd);
      return -1;
   }
#endif

   return fd;
}

void
wlc_keymap_free(struct wlc_keymap *keymap)
{
   assert(keymap);

   if (keymap->keymap)
      xkb_map_unref(keymap->keymap);

   if (keymap->area)
      munmap(keymap->area, keymap->size);

   if (keymap->fd >= 0)
      close(keymap->fd);

   free(keymap);
}

struct wlc_keymap*
wlc_keymap_new(const struct xkb_rule_names *names, enum xkb_keymap_compile_flags flags)
{
   struct wlc_keymap *keymap;
   if (!(keymap = calloc(1, sizeof(struct wlc_keymap))))
      goto fail;

   struct xkb_context *context;
   if (!(context = xkb_context_new(XKB_CONTEXT_NO_FLAGS)))
      goto context_fail;

   if (!(keymap->keymap = xkb_map_new_from_names(context, names, flags)))
      goto keymap_fail;

   char *keymap_str;
   if (!(keymap_str = xkb_map_get_as_string(keymap->keymap)))
      goto string_fail;

   keymap->size = strlen(keymap_str) + 1;
   if ((keymap->fd = os_create_anonymous_file(keymap->size)) < 0)
      goto file_fail;

   if (!(keymap->area = mmap(NULL, keymap->size, PROT_READ | PROT_WRITE, MAP_SHARED, keymap->fd, 0)))
      goto mmap_fail;

   for (int i = 0; i < WLC_MOD_LAST; ++i)
      keymap->mods[i] = xkb_map_mod_get_index(keymap->keymap, WLC_MOD_NAMES[i]);

   for (int i = 0; i < WLC_LED_LAST; ++i)
      keymap->leds[i] = xkb_map_led_get_index(keymap->keymap, WLC_LED_NAMES[i]);

   keymap->format = WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1;
   strcpy(keymap->area, keymap_str);
   free(keymap_str);
   return keymap;

context_fail:
   wlc_log(WLC_LOG_WARN, "Failed to create xkb context");
   goto fail;
keymap_fail:
   wlc_log(WLC_LOG_WARN, "Failed to get xkb keymap");
   goto fail;
string_fail:
   wlc_log(WLC_LOG_WARN, "Failed to get keymap as string");
   goto fail;
file_fail:
   wlc_log(WLC_LOG_WARN, "Failed to create file for keymap");
   goto fail;
mmap_fail:
   wlc_log(WLC_LOG_WARN, "Failed to mmap keymap");
fail:
   if (keymap)
      wlc_keymap_free(keymap);
   return NULL;
}
