#ifndef _WLC_GEOMETRY_H_
#define _WLC_GEOMETRY_H_

#include "wlc.h"
#include "macros.h"
#include <string.h>
#include <assert.h>
#include <math.h>

static const struct wlc_origin wlc_origin_zero = { 0, 0 };
static const struct wlc_size wlc_size_zero = { 0, 0 };
static const struct wlc_geometry wlc_geometry_zero = { { 0, 0 }, { 0, 0 } };

static inline void
wlc_origin_min(const struct wlc_origin *a, const struct wlc_origin *b, struct wlc_origin *out)
{
   assert(a && b && out);
   out->x = fmin(a->x, b->x);
   out->y = fmin(a->y, b->y);
}

static inline void
wlc_origin_max(const struct wlc_origin *a, const struct wlc_origin *b, struct wlc_origin *out)
{
   assert(a && b && out);
   out->x = fmax(a->x, b->x);
   out->y = fmax(a->y, b->y);
}

static inline void
wlc_size_min(const struct wlc_size *a, const struct wlc_size *b, struct wlc_size *out)
{
   assert(a && b && out);
   out->w = fmin(a->w, b->w);
   out->h = fmin(a->h, b->h);
}

static inline void
wlc_size_max(const struct wlc_size *a, const struct wlc_size *b, struct wlc_size *out)
{
   assert(a && b && out);
   out->w = fmax(a->w, b->w);
   out->h = fmax(a->h, b->h);
}

static inline bool
wlc_origin_equals(const struct wlc_origin *a, const struct wlc_origin *b)
{
   assert(a && b);
   return !memcmp(a, b, sizeof(struct wlc_origin));
}

static inline bool
wlc_size_equals(const struct wlc_size *a, const struct wlc_size *b)
{
   assert(a && b);
   return !memcmp(a, b, sizeof(struct wlc_size));
}

static inline bool
wlc_geometry_equals(const struct wlc_geometry *a, const struct wlc_geometry *b)
{
   assert(a && b);
   return !memcmp(a, b, sizeof(struct wlc_geometry));
}

static inline bool
wlc_geometry_contains(const struct wlc_geometry *a, const struct wlc_geometry *b)
{
   assert(a && b);
   return (a->origin.x <= b->origin.x && a->origin.y <= b->origin.y &&
           a->origin.x + (int32_t)b->size.w >= b->origin.x + (int32_t)b->size.w && a->origin.y + (int32_t)a->size.h >= b->origin.y + (int32_t)b->size.h);
}

#endif /* _WLC_GEOMETRY_H_ */
