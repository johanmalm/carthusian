#include "panel.h"

static struct zwlr_layer_surface_v1 *layer_surface;
static struct zwlr_layer_shell_v1 *layer_shell;
static struct wl_output *wl_output;

static struct wl_cursor_image *cursor_image;
static struct wl_surface *cursor_surface;
static struct wl_cursor_theme *cursor_theme;

static void
layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *layer_surface,
		uint32_t serial, uint32_t width, uint32_t height)
{
	struct server *server = data;
	server->width = width;
	server->height = height;
	if (server->backend->egl.window) {
		wl_egl_window_resize(server->backend->egl.window, width, height, 0, 0);
	}
	zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
	render(server);
}

static void
layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *layer_surface)
{
	zwlr_layer_surface_v1_destroy(layer_surface);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

void
backend_layer_shell_init(struct backend *backend)
{
	struct server *server = backend->server;

	layer_surface = zwlr_layer_shell_v1_get_layer_surface(layer_shell, backend->main_surface,
		wl_output, ZWLR_LAYER_SHELL_V1_LAYER_TOP, "carthusian");
	if (!layer_surface) {
		fprintf(stderr, "fatal: layer_surface\n");
		exit(EXIT_FAILURE);
	}
	zwlr_layer_surface_v1_set_size(layer_surface, server->width, server->height);
	zwlr_layer_surface_v1_set_anchor(layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
		| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
	zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, server->height);
	zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface,
		ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
	zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener, server);
	wl_surface_commit(backend->main_surface);
	wl_display_roundtrip(backend->remote_display);
}

static void
handle_wl_pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
		struct wl_surface *surface, wl_fixed_t surface_x,
		wl_fixed_t surface_y)
{
	struct seat *seat = data;
	struct server *server = seat->server;

	struct wl_cursor_image *image = cursor_image;
	wl_surface_attach(cursor_surface, wl_cursor_image_get_buffer(image), 0, 0);
	wl_surface_damage(cursor_surface, 1, 0, image->width, image->height);
	wl_surface_commit(cursor_surface);
	wl_pointer_set_cursor(wl_pointer, serial, cursor_surface,
		image->hotspot_x, image->hotspot_y);
}

static void
handle_wl_pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
		struct wl_surface *surface)
{
	// TODO
}

static void
handle_wl_pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time,
		wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct seat *seat = data;
	struct server *server = seat->server;

	struct wlr_pointer_motion_absolute_event event = {
		.pointer = &seat->wlr_pointer,
		.time_msec = time,
		.x = wl_fixed_to_double(surface_x) / (double)server->width,
		.y = wl_fixed_to_double(surface_y) / (double)server->height,
	};

	struct frontend *frontend = seat->server->frontend;
	wl_signal_emit_mutable(&frontend->cursor->events.motion_absolute, &event);
}

static void
handle_wl_pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
		uint32_t time, uint32_t button, uint32_t state)
{
	fprintf(stderr, "info: '%s()'\n", __func__);
	struct seat *seat = data;

	struct wlr_pointer_button_event event = {
		.pointer = &seat->wlr_pointer,
		.button = button,
		.state = state,
		.time_msec = time,
	};

	// From wlroots 0.19 we can do:
	// wlr_pointer_notify_button(&seat->wlr_pointer, &event);
	// but for the time being we'll stick with...
	//wl_signal_emit_mutable(&seat->wlr_pointer.events.button, &event);

	struct frontend *frontend = seat->server->frontend;
	wl_signal_emit_mutable(&frontend->cursor->events.button, &event);
}

static void
handle_wl_pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time,
		uint32_t axis, wl_fixed_t value)
{
	/* no-op */
}

static void
handle_wl_pointer_frame(void *data, struct wl_pointer *wl_pointer)
{
	struct seat *seat = data;
	wl_signal_emit_mutable(&seat->wlr_pointer.events.frame, &seat->wlr_pointer);
	struct frontend *frontend = seat->server->frontend;
	wl_signal_emit_mutable(&frontend->cursor->events.frame, &frontend->cursor);
}

static void
handle_wl_pointer_axis_source(void *data, struct wl_pointer *wl_pointer,
		uint32_t axis_source)
{
	/* no-op */
}

static void
handle_wl_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer, uint32_t time,
		uint32_t axis)
{
	/* no-op */
}

static void
handle_wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer,
		uint32_t axis, int32_t discrete)
{
	/* no-op */
}

struct wl_pointer_listener pointer_listener = {
	.enter = handle_wl_pointer_enter,
	.leave = handle_wl_pointer_leave,
	.motion = handle_wl_pointer_motion,
	.button = handle_wl_pointer_button,
	.axis = handle_wl_pointer_axis,
	.frame = handle_wl_pointer_frame,
	.axis_source = handle_wl_pointer_axis_source,
	.axis_stop = handle_wl_pointer_axis_stop,
	.axis_discrete = handle_wl_pointer_axis_discrete,
};

const struct wlr_pointer_impl wl_pointer_impl = {
	.name = "wl-pointer",
};

static void
init_seat_pointer(struct seat *seat, struct wl_pointer *wl_pointer)
{
	char name[64] = {0};
	snprintf(name, sizeof(name), "wayland-pointer-%s", seat->name ? : "");
	wlr_pointer_init(&seat->wlr_pointer, &wl_pointer_impl, name);
	wl_pointer_add_listener(wl_pointer, &pointer_listener, seat);
}

static void
seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
		enum wl_seat_capability caps)
{
	struct seat *seat = data;

	if ((caps & WL_SEAT_CAPABILITY_POINTER)) {
		fprintf(stderr, "info: get external seat pointer\n");
		struct wl_pointer *wl_pointer = wl_seat_get_pointer(wl_seat);
		init_seat_pointer(seat, wl_pointer);
	}
}

static void
seat_handle_name(void *data, struct wl_seat *wl_seat, const char *name)
{
	fprintf(stderr, "info: set backend seat name '%s'\n", name);
	struct seat *seat = data;
	free(seat->name);
	seat->name = strdup(name);
}

const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name = seat_handle_name,
};

static void
seat_init(struct server *server, struct wl_seat *wl_seat)
{
	struct seat *seat = calloc(1, sizeof(*seat));
	seat->server = server;
	seat->wl_seat = wl_seat;
	server->backend->seat = seat;
	wl_seat_add_listener(wl_seat, &seat_listener, seat);
}

static void
handle_global(void *data, struct wl_registry *registry, uint32_t name,
		const char *interface, uint32_t version)
{
	struct server *server = data;

	if (!strcmp(interface, wl_compositor_interface.name)) {
		server->backend->compositor = wl_registry_bind(registry, name,
			&wl_compositor_interface, 4);
	} else if (!strcmp(interface, wl_subcompositor_interface.name)) {
		server->backend->subcompositor = wl_registry_bind(registry, name,
			&wl_subcompositor_interface, 1);
	} else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name)) {
		layer_shell = wl_registry_bind(registry, name,
			&zwlr_layer_shell_v1_interface, 4);
	} else if (!strcmp(interface, wl_output_interface.name)) {
		wl_output = wl_registry_bind(registry, name,
			&wl_output_interface, 4);
	} else if (!strcmp(interface, wl_shm_interface.name)) {
		server->backend->shm = wl_registry_bind(registry, name,
			&wl_shm_interface, 1);
	} else if (!strcmp(interface, wl_seat_interface.name)) {
		/* Note: This is against the remote dipsplay */
		struct wl_seat *wl_seat = wl_registry_bind(registry, name,
			&wl_seat_interface, 7);
		seat_init(server, wl_seat);
	}
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	/* No-op */
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove,
};

static void
init_cursor(struct backend *backend)
{
	cursor_theme = wl_cursor_theme_load(NULL, 24, backend->shm);
	if (!cursor_theme) {
		fprintf(stderr, "fatal: no cursor theme\n");
		exit(EXIT_FAILURE);
	}
	struct wl_cursor *cursor = wl_cursor_theme_get_cursor(cursor_theme, "default");
	if (!cursor) {
		fprintf(stderr, "fatal: no cursor\n");
		exit(EXIT_FAILURE);
	}
	cursor_image = cursor->images[0];
	cursor_surface = wl_compositor_create_surface(backend->compositor);
	if (!cursor_surface) {
		fprintf(stderr, "fatal: no cursor surface\n");
		exit(EXIT_FAILURE);
	}
}

static void
init_egl(struct backend *backend)
{
	backend->egl.display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR,
		backend->remote_display, NULL);
	if (backend->egl.display == EGL_NO_DISPLAY) {
		fprintf(stderr, "fatal: failed to create EGL display\n");
		goto error;
	}

	if (eglInitialize(backend->egl.display, NULL, NULL) == EGL_FALSE) {
		fprintf(stderr, "fatal: failed to initialize EGL\n");
		goto error;
	}

	const EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE,
	};
	EGLint matched = 0;
	if (!eglChooseConfig(backend->egl.display, config_attribs,
			&backend->egl.config, 1, &matched)) {
		fprintf(stderr, "fatal: eglChooseConfig failed\n");
		goto error;
	}
	if (matched == 0) {
		fprintf(stderr, "fatal: failed to match an EGL config\n");
		goto error;
	}

	const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};
	backend->egl.context = eglCreateContext(backend->egl.display,
		backend->egl.config, EGL_NO_CONTEXT, context_attribs);
	if (backend->egl.context == EGL_NO_CONTEXT) {
		fprintf(stderr, "fatal: failed to create EGL context\n");
		goto error;
	}

	return;

error:
	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	if (backend->egl.display) {
		eglTerminate(backend->egl.display);
	}
	eglReleaseThread();
	exit(EXIT_FAILURE);
}

void
backend_init(struct server *server, struct backend *backend)
{
	server->backend = backend;
	backend->server = server;

	/*
	 * Register global bindings against the backend/remote wayland
	 * connection, which is typically on "wayland-0"
	 *
	 * https://wayland-book.com/registry/binding.html
	 */
	backend->remote_display = wl_display_connect(NULL);
	struct wl_registry *registry = wl_display_get_registry(backend->remote_display);
	wl_registry_add_listener(registry, &registry_listener, server);
	wl_display_roundtrip(backend->remote_display);

	init_cursor(backend);
	init_egl(backend);
}

void
backend_finish(struct backend *backend)
{
	wl_cursor_theme_destroy(cursor_theme);

	// TODO: more to clean up here
}
