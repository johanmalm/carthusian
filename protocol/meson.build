wayland_scanner = dependency('wayland-scanner', native: true, version: '>=1.23.1')
wayland_scanner = find_program(
  wayland_scanner.get_variable('wayland_scanner'),
  native: true,
)

wayland_protocols = dependency('wayland-protocols', version: '>=1.14')
wp_dir = wayland_protocols.get_variable('pkgdatadir')

wayland_scanner_code = generator(
	wayland_scanner,
	output: '@BASENAME@-protocol.c',
	arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_server = generator(
	wayland_scanner,
	output: '@BASENAME@-protocol.h',
	arguments: ['server-header', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
	wayland_scanner,
	output: '@BASENAME@-client-protocol.h',
	arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

client_protocols = [
	wp_dir / 'stable/xdg-shell/xdg-shell.xml',
	'wlr-layer-shell-unstable-v1.xml',
]

proto_src = []
foreach xml : client_protocols
	proto_src += wayland_scanner_code.process(xml)
	proto_src += wayland_scanner_client.process(xml)
	proto_src += wayland_scanner_server.process(xml)
endforeach
