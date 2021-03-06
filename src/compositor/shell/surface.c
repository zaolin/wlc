#include "surface.h"
#include "macros.h"

#include "compositor/view.h"
#include "compositor/surface.h"
#include "compositor/output.h"
#include "compositor/seat/seat.h"
#include "compositor/seat/pointer.h"

#include <stdlib.h>
#include <assert.h>

#include <wayland-server.h>

static void
wl_cb_shell_surface_pong(struct wl_client *wl_client, struct wl_resource *resource, uint32_t serial)
{
   (void)wl_client, (void)serial;
   STUB(resource);
}

static void
wl_cb_shell_surface_move(struct wl_client *wl_client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial)
{
   (void)wl_client, (void)resource, (void)serial;
   struct wlc_seat *seat = wl_resource_get_user_data(seat_resource);

   if (!seat->pointer || !seat->pointer->focus)
      return;

   seat->pointer->action = WLC_GRAB_ACTION_MOVE;
}

static void
wl_cb_shell_surface_resize(struct wl_client *wl_client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial, uint32_t edges)
{
   (void)wl_client, (void)resource, (void)serial;
   struct wlc_seat *seat = wl_resource_get_user_data(seat_resource);

   if (!seat->pointer || !seat->pointer->focus)
      return;

   seat->pointer->action = WLC_GRAB_ACTION_RESIZE;
   seat->pointer->action_edges = edges;
}

static void
wl_cb_shell_surface_set_toplevel(struct wl_client *wl_client, struct wl_resource *resource)
{
   (void)wl_client;
   struct wlc_view *view = wl_resource_get_user_data(resource);
   wlc_view_request_state(view, WLC_BIT_FULLSCREEN, false);
   view->shell_surface.fullscreen_mode = WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT;
}

static void
wl_cb_shell_surface_set_transient(struct wl_client *wl_client, struct wl_resource *resource, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags)
{
   (void)wl_client, (void)flags;
   struct wlc_view *view = wl_resource_get_user_data(resource);
   struct wlc_surface *surface = (parent_resource ? wl_resource_get_user_data(parent_resource) : NULL);
   wlc_view_set_parent(view, (surface ? surface->view : NULL));
   wlc_view_set_geometry(view, &(struct wlc_geometry){ { x, y }, view->pending.geometry.size });
}

static void
wl_cb_shell_surface_set_fullscreen(struct wl_client *wl_client, struct wl_resource *resource, uint32_t method, uint32_t framerate, struct wl_resource *output_resource)
{
   (void)wl_client, (void)method, (void)framerate;

   struct wlc_view *view = wl_resource_get_user_data(resource);

   if (output_resource || view->space) {
      struct wlc_output *output = (output_resource ? wl_resource_get_user_data(output_resource) : view->space->output);
      wlc_view_set_space(view, output->space);
   }

   view->shell_surface.fullscreen_mode = method;
   wlc_view_request_state(view, WLC_BIT_FULLSCREEN, true);
}

static void
wl_cb_shell_surface_set_popup(struct wl_client *wl_client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags)
{
   (void)wl_client, (void)seat, (void)serial, (void)parent, (void)x, (void)y, (void)flags;
   STUB(resource);
}

static void
wl_cb_shell_surface_set_maximized(struct wl_client *wl_client, struct wl_resource *resource, struct wl_resource *output_resource)
{
   (void)wl_client;

   struct wlc_view *view = wl_resource_get_user_data(resource);

   if (output_resource || view->space) {
      struct wlc_output *output = (output_resource ? wl_resource_get_user_data(output_resource) : view->space->output);
      wlc_view_set_space(view, output->space);
   }

   wlc_view_request_state(view, WLC_BIT_MAXIMIZED, true);
}

static void
wl_cb_shell_surface_set_title(struct wl_client *wl_client, struct wl_resource *resource, const char *title)
{
   (void)wl_client;
   struct wlc_view *view = wl_resource_get_user_data(resource);
   wlc_view_set_title(view, title);
}

static void
wl_cb_shell_surface_set_class(struct wl_client *wl_client, struct wl_resource *resource, const char *class_)
{
   (void)wl_client;
   struct wlc_view *view = wl_resource_get_user_data(resource);
   wlc_view_set_class(view, class_);
}

static const struct wl_shell_surface_interface wl_shell_surface_implementation = {
   .pong = wl_cb_shell_surface_pong,
   .move = wl_cb_shell_surface_move,
   .resize = wl_cb_shell_surface_resize,
   .set_toplevel = wl_cb_shell_surface_set_toplevel,
   .set_transient = wl_cb_shell_surface_set_transient,
   .set_fullscreen = wl_cb_shell_surface_set_fullscreen,
   .set_popup = wl_cb_shell_surface_set_popup,
   .set_maximized = wl_cb_shell_surface_set_maximized,
   .set_title = wl_cb_shell_surface_set_title,
   .set_class = wl_cb_shell_surface_set_class,
};

static void
wl_cb_shell_surface_destructor(struct wl_resource *resource)
{
   assert(resource);
   struct wlc_view *view = wl_resource_get_user_data(resource);
   view->shell_surface.resource = NULL;
   wlc_shell_surface_release(&view->shell_surface);
}

void
wlc_shell_surface_implement(struct wlc_shell_surface *shell_surface, struct wlc_view *view, struct wl_resource *resource)
{
   assert(shell_surface);

   if (shell_surface->resource == resource)
      return;

   if (shell_surface->resource)
      wl_resource_destroy(shell_surface->resource);

   shell_surface->resource = resource;
   wl_resource_set_implementation(shell_surface->resource, &wl_shell_surface_implementation, view, wl_cb_shell_surface_destructor);
}

void
wlc_shell_surface_release(struct wlc_shell_surface *shell_surface)
{
   assert(shell_surface);

   if (shell_surface->resource) {
      wl_resource_destroy(shell_surface->resource);
      shell_surface->resource = NULL;
   }

   wlc_string_release(&shell_surface->title);
   wlc_string_release(&shell_surface->_class);
}
