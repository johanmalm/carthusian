#include "panel.h"

static struct wl_callback *frame_callback;

static void
frame_handle_done(void *data, struct wl_callback *callback, uint32_t t)
{
	struct server *server = data;
	wl_callback_destroy(callback);
	frame_callback = NULL;
	render(server);
}

static const struct wl_callback_listener frame_listener = {
	.done = frame_handle_done,
};

/* See wlroots/render/egl.c for smarter implementation */
void
render(struct server *server)
{
	struct backend *backend = server->backend;
	eglMakeCurrent(backend->egl.display, backend->egl.surface,
		backend->egl.surface, backend->egl.context);

	/* Set eglSwapInterval to zero to avoid eglSwapBuffers() blocking */
	eglSwapInterval(backend->egl.display, 0);

	glViewport(0, 0, server->width, server->height);

	frame_callback = wl_surface_frame(server->backend->main_surface);
	wl_callback_add_listener(frame_callback, &frame_listener, server);
	eglSwapBuffers(backend->egl.display, backend->egl.surface);
	wl_display_flush(backend->remote_display);
}
