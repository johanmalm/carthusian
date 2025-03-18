#ifndef CARTHUSIAN_PANEL_H
#define CARTHUSIAN_PANEL_H
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <wayland-server-core.h>
#include <wlr/backend/wayland.h>
#include <wlr/interfaces/wlr_pointer.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <unistd.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-protocol.h"

struct frontend {
	struct server *server;
	struct wlr_seat *wlr_seat;

	struct wl_display *local_display;

	struct wlr_output *wlr_output;
	struct wlr_scene_output_layout *scene_layout;
	struct wlr_output_layout *output_layout;

	struct wlr_cursor *cursor;
	struct wlr_xcursor_manager *cursor_mgr;

	struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	struct wl_listener new_input;
	struct wl_listener request_cursor;
	struct wl_listener request_set_selection;
};

struct seat {
	struct server *server;

	struct wl_seat *wl_seat;
	char *name;

	struct wlr_pointer wlr_pointer;
};

struct backend {
	struct server *server;
	struct seat *seat;

	struct wlr_backend *wlr_backend;

	struct wl_display *remote_display;
	struct wl_compositor *compositor;
	struct wl_subcompositor *subcompositor;
	struct wl_shm *shm;
	struct wl_surface *main_surface;

	struct {
		struct wl_egl_window *window;
		struct wlr_egl_surface *surface;
		EGLDisplay display;
		EGLConfig config;
		EGLContext context;
	} egl;
};

struct toplevel {
	struct server *server;
	struct wlr_xdg_toplevel *xdg_toplevel;
	struct wlr_scene_tree *scene_tree;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener destroy;
	struct wl_list link; /* server.toplevels */
};

struct server {
	int width;
	int height;

	struct frontend *frontend;
	struct backend *backend;

	struct wlr_scene *scene;

	struct wlr_xdg_shell *xdg_shell;
	struct wl_listener new_xdg_toplevel;
	struct wl_list toplevels;
};

void render(struct server *server);
void xdg_shell_init(struct server *server, struct wl_display *local_display);

void backend_layer_shell_init(struct backend *backend);
void backend_init(struct server *server, struct backend *backend);
void backend_finish(struct backend *backend);

#endif /* CARTHUSIAN_PANEL_H */
