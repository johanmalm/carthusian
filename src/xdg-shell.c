#include "panel.h"

#define XDG_SHELL_VERSION (3)

#define MARGIN_HORIZONTAL (3)
#define MARGIN_VERTICAL (3)
#define PADDING (3)

static void
arrange_toplevels(struct server *server)
{
	struct toplevel *toplevel;
	int y = MARGIN_HORIZONTAL;
	int x = MARGIN_VERTICAL;
	wl_list_for_each(toplevel, &server->toplevels, link) {
		wlr_scene_node_set_position(&toplevel->scene_tree->node, x, y);
		x += toplevel->pending.width + PADDING;
	}
}

static void
handle_xdg_toplevel_map(struct wl_listener *listener, void *data)
{
	struct toplevel *toplevel = wl_container_of(listener, toplevel, map);
	wl_list_insert(toplevel->server->toplevels.prev, &toplevel->link);
	wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, true);
	arrange_toplevels(toplevel->server);
}

static void
handle_xdg_toplevel_unmap(struct wl_listener *listener, void *data)
{
	struct toplevel *toplevel = wl_container_of(listener, toplevel, unmap);
	wl_list_remove(&toplevel->link);
	arrange_toplevels(toplevel->server);
}

static void
handle_xdg_toplevel_commit(struct wl_listener *listener, void *data)
{
	struct toplevel *toplevel = wl_container_of(listener, toplevel, commit);
	if (toplevel->xdg_toplevel->base->initial_commit) {
		wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
		return;
	}
	wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &toplevel->pending);

	// TODO: do some updated-required logic here
	arrange_toplevels(toplevel->server);
}

static void
handle_xdg_toplevel_destroy(struct wl_listener *listener, void *data)
{
	struct toplevel *toplevel = wl_container_of(listener, toplevel, destroy);
	wl_list_remove(&toplevel->map.link);
	wl_list_remove(&toplevel->unmap.link);
	wl_list_remove(&toplevel->commit.link);
	wl_list_remove(&toplevel->destroy.link);
	free(toplevel);
}

static void
handle_new_xdg_toplevel(struct wl_listener *listener, void *data)
{
	struct server *server = wl_container_of(listener, server, new_xdg_toplevel);
	struct wlr_xdg_toplevel *xdg_toplevel = data;

	struct toplevel *toplevel = calloc(1, sizeof(*toplevel));
	toplevel->server = server;
	toplevel->xdg_toplevel = xdg_toplevel;
	toplevel->scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree,
		xdg_toplevel->base);
	toplevel->scene_tree->node.data = toplevel;
	xdg_toplevel->base->data = toplevel->scene_tree;

	toplevel->map.notify = handle_xdg_toplevel_map;
	wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);
	toplevel->unmap.notify = handle_xdg_toplevel_unmap;
	wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);
	toplevel->commit.notify = handle_xdg_toplevel_commit;
	wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->commit);
	toplevel->destroy.notify = handle_xdg_toplevel_destroy;
	wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);
}
void
xdg_shell_init(struct server *server, struct wl_display *local_display)
{
	server->xdg_shell = wlr_xdg_shell_create(local_display, XDG_SHELL_VERSION);
	if (!server->xdg_shell) {
		fprintf(stderr, "fatal: unable to create the XDG shell interface\n");
		exit(EXIT_FAILURE);
	}

	server->new_xdg_toplevel.notify = handle_new_xdg_toplevel;
	wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_xdg_toplevel);
}

