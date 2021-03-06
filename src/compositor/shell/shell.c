#include "internal.h"
#include "macros.h"
#include "shell.h"
#include "surface.h"

#include "compositor/compositor.h"
#include "compositor/surface.h"
#include "compositor/output.h"
#include "compositor/client.h"
#include "compositor/view.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <wayland-server.h>

static void
wl_cb_shell_get_shell_surface(struct wl_client *wl_client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
   struct wlc_shell *shell = wl_resource_get_user_data(resource);
   struct wlc_surface *surface = wl_resource_get_user_data(surface_resource);

   struct wlc_client *client;
   if (!(client = wlc_client_for_client_with_wl_client_in_list(wl_client, &shell->compositor->clients))) {
      wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "Could not find wlc_client for wl_client");
      return;
   }

   struct wl_resource *shell_surface_resource;
   if (!(shell_surface_resource = wl_resource_create(wl_client, &wl_shell_surface_interface, wl_resource_get_version(resource), id))) {
      wl_resource_post_no_memory(resource);
      return;
   }

   if (!surface->view && !(surface->view = wlc_view_new(shell->compositor, client, surface))) {
      wl_resource_destroy(shell_surface_resource);
      wl_resource_post_no_memory(resource);
      return;
   }

   wlc_shell_surface_implement(&surface->view->shell_surface, surface->view, shell_surface_resource);
}

static const struct wl_shell_interface wl_shell_implementation = {
   .get_shell_surface = wl_cb_shell_get_shell_surface
};

static void
wl_shell_bind(struct wl_client *wl_client, void *data, unsigned int version, unsigned int id)
{
   struct wl_resource *resource;
   if (!(resource = wl_resource_create(wl_client, &wl_shell_interface, fmin(version, 1), id))) {
      wl_client_post_no_memory(wl_client);
      wlc_log(WLC_LOG_WARN, "Failed create resource or bad version (%u > %u)", version, 1);
      return;
   }

   wl_resource_set_implementation(resource, &wl_shell_implementation, data, NULL);
}

void
wlc_shell_free(struct wlc_shell *shell)
{
   assert(shell);

   if (shell->global)
      wl_global_destroy(shell->global);

   free(shell);
}

struct wlc_shell*
wlc_shell_new(struct wlc_compositor *compositor)
{
   struct wlc_shell *shell;
   if (!(shell = calloc(1, sizeof(struct wlc_shell))))
      goto out_of_memory;

   if (!(shell->global = wl_global_create(wlc_display(), &wl_shell_interface, 1, shell, wl_shell_bind)))
      goto shell_interface_fail;

   shell->compositor = compositor;
   return shell;

out_of_memory:
   wlc_log(WLC_LOG_WARN, "Out of memory");
   goto fail;
shell_interface_fail:
   wlc_log(WLC_LOG_WARN, "Failed to bind shell interface");
fail:
   if (shell)
      wlc_shell_free(shell);
   return NULL;
}
