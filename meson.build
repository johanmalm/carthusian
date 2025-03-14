project(
  'carthusian',
  'c',
  license: 'GPL2-only',
  default_options: ['c_std=c11', 'warning_level=2'],
)

add_project_arguments(
  [
    '-DWLR_USE_UNSTABLE',
    '-D_POSIX_C_SOURCE=200809L',
    '-Wno-unused-parameter',
    '-Wno-unused-const-variable',
    '-Wno-unused-variable',
    '-Wundef',
  ],
  language: 'c',
)

wlroots = dependency('wlroots-0.18', version: ['>=0.18.1', '<0.19.0'])
wayland_server = dependency('wayland-server')
wayland_client = dependency('wayland-client')
wayland_cursor = dependency('wayland-cursor')
wayland_egl = dependency('wayland-egl', required: false, disabler: true)
egl = dependency('egl', version: '>= 1.5', required: false, disabler: true)
glesv2 = dependency('glesv2', required: false, disabler: true)

deps = [
    wlroots,
    wayland_server,
    wayland_client,
    wayland_cursor,
    wayland_egl,
    egl,
    glesv2
]

subdir('protocol')
subdir('src')

executable(
  meson.project_name(),
  carthusian_src,
  proto_src,
  dependencies: deps,
  include_directories: include_directories('include'),
)
