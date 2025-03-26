#include <assert.h>
#include "panel.h"

static struct wlr_scene_output *scene_output;
static struct wl_listener output_frame;

static struct
toplevel *toplevel_at(struct server *server, double lx, double ly,
		struct wlr_surface **surface, double *sx, double *sy)
{
	struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
	if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
		return NULL;
	}
	struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
	struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
	if (!scene_surface) {
		return NULL;
	}
	*surface = scene_surface->surface;
	struct wlr_scene_tree *tree = node->parent;
	while (tree && !tree->node.data) {
		tree = tree->node.parent;
	}
	return tree->node.data;
}

static void
output_handle_frame(struct wl_listener *listener, void *data)
{
	wlr_scene_output_commit(scene_output, NULL);
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

static void
frontend_new_input(struct wl_listener *listener, void *data)
{
	struct frontend *frontend = wl_container_of(listener, frontend, new_input);
	struct wlr_input_device *device = data;
	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		break;
	case WLR_INPUT_DEVICE_POINTER:
		wlr_cursor_attach_input_device(frontend->cursor, device);
		break;
	default:
		break;
	}
	wlr_seat_set_capabilities(frontend->wlr_seat, WL_SEAT_CAPABILITY_POINTER);
	fprintf(stderr, "frontend->wlr_seat->capabilities=%d\n", frontend->wlr_seat->capabilities);
}

static void
seat_request_cursor(struct wl_listener *listener, void *data)
{
	fprintf(stderr, "info: %s()\n", __func__);
	/* noop */
}

static void
seat_request_set_selection(struct wl_listener *listener, void *data)
{
	/* noop */
}

static void
process_cursor_motion(struct frontend *frontend, uint32_t time)
{
	double sx, sy;
	struct wlr_seat *seat = frontend->wlr_seat;
	struct wlr_surface *surface = NULL;

	struct toplevel *toplevel = toplevel_at(frontend->server,
		frontend->cursor->x, frontend->cursor->y, &surface, &sx, &sy);
	if (!toplevel) {
		wlr_cursor_set_xcursor(frontend->cursor, frontend->cursor_mgr, "default");
	}
	if (surface) {
		fprintf(stderr, "info: '%s()'; surface=%p\n",
			"wlr_seat_pointer_notify_enter", surface);
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	} else {
		wlr_seat_pointer_clear_focus(seat);
	}
}

static void
frontend_cursor_motion(struct wl_listener *listener, void *data)
{
	struct frontend *frontend = wl_container_of(listener, frontend, cursor_motion);
	struct wlr_pointer_motion_event *event = data;
	wlr_cursor_move(frontend->cursor, &event->pointer->base, event->delta_x, event->delta_y);
	process_cursor_motion(frontend, event->time_msec);
}

static void
frontend_cursor_motion_absolute(struct wl_listener *listener, void *data)
{
	struct frontend *frontend = wl_container_of(listener, frontend, cursor_motion_absolute);
	struct wlr_pointer_motion_absolute_event *event = data;
	wlr_cursor_warp_absolute(frontend->cursor, &event->pointer->base, event->x, event->y);
	process_cursor_motion(frontend, event->time_msec);
}

static void
frontend_cursor_button(struct wl_listener *listener, void *data)
{
	fprintf(stderr, "info: '%s()'\n", __func__);
	struct frontend *frontend = wl_container_of(listener, frontend, cursor_button);
	struct wlr_pointer_button_event *event = data;
	wlr_seat_pointer_notify_button(frontend->wlr_seat, event->time_msec,
		event->button, event->state);

	fprintf(stderr, "frontend->wlr_seat->pointer_state.focused_surface=%p\n",
		frontend->wlr_seat->pointer_state.focused_surface);

	double sx, sy;
	struct wlr_surface *surface = NULL;
	struct toplevel *toplevel = toplevel_at(frontend->server,
		frontend->cursor->x, frontend->cursor->y, &surface, &sx, &sy);

	fprintf(stderr, "info: 'sx=%f; sy=%f; app_id=%s; surface=%p\n", sx, sy,
		toplevel ? toplevel->xdg_toplevel->app_id : "n/a", surface);

	if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
		;
	} else {
		;
	}
}

static void
frontend_cursor_axis(struct wl_listener *listener, void *data)
{
	/* noop */
}

static void
frontend_cursor_frame(struct wl_listener *listener, void *data)
{
	struct frontend *frontend = wl_container_of(listener, frontend, cursor_frame);
	wlr_seat_pointer_notify_frame(frontend->wlr_seat);
}

static void
init_frontend(struct server *server, struct frontend *frontend)
{
	server->frontend = frontend;
	frontend->server = server;

	frontend->output_layout = wlr_output_layout_create(frontend->local_display);
	frontend->scene_layout =
		wlr_scene_attach_output_layout(server->scene, frontend->output_layout);

	struct wlr_output_layout_output *l_output =
		wlr_output_layout_add_auto(frontend->output_layout, frontend->wlr_output);
	scene_output = wlr_scene_output_create(server->scene, frontend->wlr_output);
	wlr_scene_output_layout_add_output(frontend->scene_layout, l_output, scene_output);

	wlr_cursor_attach_output_layout(frontend->cursor, frontend->output_layout);

	frontend->cursor_motion.notify = frontend_cursor_motion;
	wl_signal_add(&frontend->cursor->events.motion, &frontend->cursor_motion);

	frontend->cursor_motion_absolute.notify = frontend_cursor_motion_absolute;
	wl_signal_add(&frontend->cursor->events.motion_absolute, &frontend->cursor_motion_absolute);

	frontend->cursor_button.notify = frontend_cursor_button;
	wl_signal_add(&frontend->cursor->events.button, &frontend->cursor_button);

	frontend->cursor_axis.notify = frontend_cursor_axis;
	wl_signal_add(&frontend->cursor->events.axis, &frontend->cursor_axis);

	frontend->cursor_frame.notify = frontend_cursor_frame;
	wl_signal_add(&frontend->cursor->events.frame, &frontend->cursor_frame);

	frontend->request_cursor.notify = seat_request_cursor;
	wl_signal_add(&frontend->wlr_seat->events.request_set_cursor, &frontend->request_cursor);

	frontend->request_set_selection.notify = seat_request_set_selection;
	wl_signal_add(&frontend->wlr_seat->events.request_set_selection,
		&frontend->request_set_selection);
}

static void
spawn(const char *command)
{
	const char *shell = "/bin/sh";
	if (!fork()) {
		execl(shell, shell, "-c", command, (void *)NULL);
	}
}

int
main(int argc, char **argv)
{
	wlr_log_init(WLR_ERROR, NULL);

	struct server server = {0};
	server.height = 40;

	struct backend backend = {0};
	backend_init(&server, &backend);

	struct wl_display *local_display = wl_display_create();
	struct wl_event_loop *event_loop = wl_display_get_event_loop(local_display);

	backend.wlr_backend = wlr_wl_backend_create(event_loop, backend.remote_display);

	struct wlr_renderer *renderer = wlr_renderer_autocreate(backend.wlr_backend);
	wlr_renderer_init_wl_display(renderer, local_display);
	struct wlr_allocator *allocator = wlr_allocator_autocreate(backend.wlr_backend, renderer);
	struct wlr_compositor *wlr_compositor = wlr_compositor_create(local_display, 5, renderer);
	server.scene = wlr_scene_create();

	struct frontend frontend = {0};
	frontend.local_display = local_display;

	/* Need to rig up the new_input listener before starting the backend */
	frontend.cursor = wlr_cursor_create();
	frontend.cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
	if (!frontend.cursor_mgr) {
		fprintf(stderr, "fatal: no frontend cursor manager\n");
		exit(EXIT_FAILURE);
	}
	frontend.wlr_seat = wlr_seat_create(frontend.local_display, "seat0");
	fprintf(stderr, "info: create frontend seat '%s'\n", frontend.wlr_seat->name);
	frontend.new_input.notify = frontend_new_input;
	wl_signal_add(&backend.wlr_backend->events.new_input, &frontend.new_input);

	wlr_backend_start(backend.wlr_backend);

	backend.main_surface = wl_compositor_create_surface(backend.compositor);

	backend_layer_shell_init(&backend);

	backend.egl.window = wl_egl_window_create(backend.main_surface, server.width, server.height);
	backend.egl.surface = eglCreatePlatformWindowSurface(backend.egl.display,
		backend.egl.config, backend.egl.window, NULL);
	wl_display_roundtrip(backend.remote_display);

	struct wl_surface *child_surface = wl_compositor_create_surface(backend.compositor);
	struct wl_subsurface *subsurface = wl_subcompositor_get_subsurface(backend.subcompositor,
		child_surface, backend.main_surface);

	/* This is where the magic happens */
	wl_subsurface_set_position(subsurface, 0, 0);
	frontend.wlr_output = wlr_wl_output_create_from_surface(backend.wlr_backend, child_surface);
	wlr_output_init_render(frontend.wlr_output, allocator, renderer);

	output_frame.notify = output_handle_frame;
	wl_signal_add(&frontend.wlr_output->events.frame, &output_frame);

	/* Sync output size to panel size */
	struct wlr_output_state output_state;
	wlr_output_state_init(&output_state);
	wlr_output_state_set_enabled(&output_state, true);
	wlr_output_state_set_custom_mode(&output_state, server.width, server.height, 0);
	wlr_output_commit_state(frontend.wlr_output, &output_state);
	wlr_output_state_finish(&output_state);

	init_frontend(&server, &frontend);

	/* Setup Wayland protocol xdg-shell for plugin windows */
	wl_list_init(&server.toplevels);
	xdg_shell_init(&server, server.frontend->local_display);

	const char *socket = wl_display_add_socket_auto(local_display);
	setenv("WAYLAND_DISPLAY", socket, true);
	fprintf(stderr, "info: carthusian running on WAYLAND_DISPLAY=%s\n", socket);

	spawn("./plugins/clock.py --color red");
	spawn("./plugins/clock.py --color blue");
	spawn("./plugins/clock.py --color green");

	render(&server);

	wl_display_run(local_display);

	backend_finish(&backend);
	return 0;
}
