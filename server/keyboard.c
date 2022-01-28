#include "keyboard.h"

#include <sys/time.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include "seat.h"
#include "surface.h"
#include "util.h"

static void zwl_keyboard_protocol_release(struct wl_client *client,
                                          struct wl_resource *resource)
{
  UNUSED(client);
  UNUSED(resource);
}

static const struct wl_keyboard_interface zwl_keyboard_interface = {
    .release = zwl_keyboard_protocol_release,
};

void zwl_keyboard_send_keymap(struct zwl_keyboard *keyboard,
                              struct wl_resource *resource)
{
  if (keyboard->keymap_info.fd == -1) {
    return;
  }

  wl_keyboard_send_keymap(resource, keyboard->keymap_info.format,
                          keyboard->keymap_info.fd, keyboard->keymap_info.size);
}

void zwl_keyboard_send_enter(struct zwl_keyboard *keyboard, uint32_t serial,
                             struct zwl_surface *surface, struct wl_array *keys)
{
  struct wl_resource *resource;

  wl_resource_for_each(resource, &keyboard->resource_list)
  {
    wl_keyboard_send_enter(resource, serial, surface->resource, keys);
  }
}

void zwl_keyboard_send_leave(struct zwl_keyboard *keyboard, uint32_t serial,
                             struct zwl_surface *surface)
{
  struct wl_resource *resource;

  wl_resource_for_each(resource, &keyboard->resource_list)
  {
    wl_keyboard_send_leave(resource, serial, surface->resource);
  }
}

void zwl_keyboard_send_key(struct zwl_keyboard *keyboard, uint32_t serial,
                           uint32_t time, uint32_t key, uint32_t state)
{
  struct wl_resource *resource;

  wl_resource_for_each(resource, &keyboard->resource_list)
  {
    wl_keyboard_send_key(resource, serial, time, key, state);
  }
}

void zwl_keyboard_send_modifiers(struct zwl_keyboard *keyboard, uint32_t serial,
                                 uint32_t mods_depressed, uint32_t mods_latched,
                                 uint32_t mods_locked, uint32_t group)
{
  struct wl_resource *resource;

  wl_resource_for_each(resource, &keyboard->resource_list)
  {
    wl_keyboard_send_modifiers(resource, serial, mods_depressed, mods_latched,
                               mods_locked, group);
  }
}

static void zwl_keyboard_handle_destroy(struct wl_resource *resource)
{
  wl_list_remove(wl_resource_get_link(resource));
}

struct wl_resource *zwl_keyboard_add_resource(struct zwl_keyboard *keyboard,
                                              struct wl_client *client,
                                              uint32_t id)
{
  struct wl_resource *resource;

  resource = wl_resource_create(client, &wl_keyboard_interface, 7, id);
  if (resource == NULL) {
    wl_client_post_no_memory(client);
    return NULL;
  }

  wl_list_insert(&keyboard->resource_list, wl_resource_get_link(resource));

  wl_resource_set_implementation(resource, &zwl_keyboard_interface, keyboard,
                                 zwl_keyboard_handle_destroy);

  return resource;
}

static void zwl_keyboard_client_destroy_handler(struct wl_listener *listener,
                                                void *data)
{
  UNUSED(data);
  struct zwl_keyboard *keyboard;

  keyboard = wl_container_of(listener, keyboard, client_destroy_listener);

  zwl_keyboard_destroy(keyboard);
}

struct zwl_keyboard *zwl_keyboard_create(struct wl_client *client,
                                         struct zwl_seat *seat)
{
  struct zwl_keyboard *keyboard;

  keyboard = zalloc(sizeof *keyboard);
  if (keyboard == NULL) goto out;

  keyboard->client = client;
  keyboard->client_destroy_listener.notify =
      zwl_keyboard_client_destroy_handler;
  wl_client_add_destroy_listener(client, &keyboard->client_destroy_listener);

  wl_list_init(&keyboard->resource_list);
  wl_list_insert(&seat->keyboard_list, &keyboard->link);

  keyboard->keymap_info.fd = -1;

  return keyboard;

out:
  return NULL;
}

void zwl_keyboard_destroy(struct zwl_keyboard *keyboard)
{
  struct wl_resource *resource, *tmp;
  wl_list_remove(&keyboard->link);
  wl_list_remove(&keyboard->client_destroy_listener.link);
  wl_resource_for_each_safe(resource, tmp, &keyboard->resource_list)
  {
    wl_resource_set_destructor(resource, NULL);
    wl_resource_set_user_data(resource, NULL);
    wl_list_remove(wl_resource_get_link(resource));
  }
  free(keyboard);
}
