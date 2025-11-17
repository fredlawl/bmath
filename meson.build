# for devel: meson install --tags devel
# for deployment: meson install --tags runtime,man
# for test: meson test
project('bmath', 'c', version: '4.0.0', default_options: ['c_std=gnu11'])
pkg = import('pkgconfig')

add_project_arguments(
  [
    '-Wshadow',
    '-Wvla',
    '-Wmissing-field-initializers',
    '-fno-omit-frame-pointer',
    '-fstack-protector-strong',
    '-fstack-clash-protection',
  ],
  language: ['c'],
)

add_project_link_arguments('-Wl,--build-id', language: ['c'])

# default build type
if get_option('buildtype') == 'debug'
  add_global_arguments('-DDEBUG', language: 'c')
endif

if get_option('buildtype') == 'release'
  add_project_arguments(['-pie', '-fpie', '-D_FORTIFY_SOURCE=2'], language: ['c'])
  add_project_link_arguments(['-Wl,-z,relro,-z,now', '-Wl,-z,noexecstack'], language: ['c'])
endif

configure_file(
  input: 'bmath/version.h.in',
  output: 'version.h',
  configuration: {'version': meson.project_version()},
)

# Release
libbmath_deps = [dependency('iconv')]
libbmath_inc = include_directories('libbmath/src')
libbmath = shared_library(
  'bmath',
  'libbmath/src/lexer.c',
  'libbmath/src/parser.c',
  'libbmath/src/print.c',
  'libbmath/src/symbol.c',
  'libbmath/src/functions.c',
  include_directories: libbmath_inc,
  dependencies: libbmath_deps,
  install: true,
  soversion: '2',
  version: '2.0.0',
)
libbmath_dep = declare_dependency(link_with: libbmath, include_directories: libbmath_inc)

go_make = find_program('make')
go_server_src_dir = meson.project_source_root() / 'bmath-server'
go_build_bin_dir = meson.current_build_dir()
go_cgo_cflags = '-I' + (meson.project_source_root() / 'libbmath/src')
go_cgo_ldflags = '-L' + meson.current_build_dir() + ' -Wl,-rpath,' + meson.current_build_dir()

compile_bmath_ls = custom_target(
  'bmath-ls',
  output: 'bmath-ls',
  command: [
    go_make,
    '-C', go_server_src_dir,
    (go_build_bin_dir / 'bmath-ls'),
    'TARGET=' + (go_build_bin_dir / 'bmath-ls'),
    'CGO_CFLAGS=' + go_cgo_cflags,
    'CGO_LDFLAGS=' + go_cgo_ldflags,
    'GOCACHE=' + (go_build_bin_dir / '.go-cache'),
  ],
  depends: [libbmath],
  build_by_default: true,
  build_always_stale: true,
  capture: false,
  install: true,
  install_dir: get_option('bindir'),
)

custom_target(
  'bmath-format',
  output: 'bmath-format',
  command: [
    go_make,
    '-C', go_server_src_dir,
    (go_build_bin_dir / 'bmath-format'),
    'FMT_TARGET=' + (go_build_bin_dir / 'bmath-format'),
    'CGO_CFLAGS=' + go_cgo_cflags,
    'CGO_LDFLAGS=' + go_cgo_ldflags,
    'GOCACHE=' + (go_build_bin_dir / '.go-cache'),
  ],
  depends: [libbmath],
  build_by_default: true,
  build_always_stale: true,
  capture: false,
  install: true,
  install_dir: get_option('bindir'),
)

test(
  'bmath-server',
  go_make,
  args: [
    '-C', go_server_src_dir,
    'test',
    'CGO_ENABLED=1',
    'CGO_CFLAGS=' + go_cgo_cflags,
    'CGO_LDFLAGS=' + go_cgo_ldflags,
  ],
  depends: [libbmath],
  verbose: true,
)

libini_dep = dependency('inih')

# Test
unity_dep = dependency('unity', static: true, required: false)
if unity_dep.found()
  lexer_test = executable(
    'libbmath_test_lexer',
    'libbmath/test/lexer.c',
    install: false,
    include_directories: libbmath_inc,
    dependencies: [unity_dep, libbmath_dep],
  )

  parser_test = executable(
    'libbmath_test_parser',
    'libbmath/test/parser.c',
    install: false,
    include_directories: libbmath_inc,
    dependencies: [unity_dep, libbmath_dep],
  )

  functions_test = executable(
    'libbmath_test_functions',
    'libbmath/test/functions.c',
    install: false,
    include_directories: libbmath_inc,
    dependencies: [unity_dep, libbmath_dep],
  )

  conversions_test = executable(
    'libbmath_test_conversions',
    'libbmath/test/conversions.c',
    install: false,
    include_directories: libbmath_inc,
    dependencies: [unity_dep, libbmath_dep],
  )

  config_test = executable(
    'bmath_test_config',
    'bmath/src/config.c',
    'bmath/test/config.c',
    install: false,
    include_directories: libbmath_inc,
    dependencies: [unity_dep, libini_dep],
  )

  test('parser', parser_test, args: [], verbose: true)
  test('functions', functions_test, args: [], verbose: true)
  test('conversions', conversions_test, args: [], verbose: true)
  test('config', config_test, args: [], verbose: true)
  test('lexer', lexer_test, args: [], verbose: true)
endif

# todo: figure out argp dep for non-gnu platforms
bmath_deps = [
  dependency('readline'),
  libini_dep,
  libbmath_dep,
]
bmath = executable(
  'bmath',
  'bmath/src/config.c',
  'bmath/src/execute.c',
  'bmath/src/cmd.c',
  dependencies: bmath_deps,
  include_directories: libbmath_inc,
  install: true,
  pie: true,
)

custom_target(
  'cscope',
  output: ['cscope.out', 'cscope.files'],
  command: [
    './scripts/cscope.sh',
    '@CURRENT_SOURCE_DIR@',
  ],
  depends: [bmath],
  build_by_default: true,
  build_always_stale: true,
  capture: false,
  install: false,
)

install_man('man/bmath.1')
install_man('man/bmath-config.5')
install_data('etc/bmath/config.conf', install_dir: '/etc/bmath')
install_data(
  'usr/share/mime/packages/x-bmath.xml',
  install_dir: '/usr/share/mime/packages',
)
meson.add_install_script('update-mime-database', '/usr/share/mime')

install_headers(
  'libbmath/src/lexer.h',
  'libbmath/src/parser.h',
  'libbmath/src/print.h',
  'libbmath/src/symbol.h',
  'libbmath/src/token.h',
  'libbmath/src/type.h',
  subdir: 'bmath',
)
