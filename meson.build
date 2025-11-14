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
  input: 'src/version.h.in',
  output: 'version.h',
  configuration: {'version': meson.project_version()},
)

# Release
libbmath_deps = [dependency('iconv')]
libbmath_inc = include_directories('src')
libbmath = shared_library(
  'bmath',
  'src/lexer.c',
  'src/parser.c',
  'src/print.c',
  'src/symbol.c',
  'src/functions.c',
  include_directories: libbmath_inc,
  dependencies: libbmath_deps,
  install: true,
  soversion: '2',
  version: '2.0.0',
)
libbmath_dep = declare_dependency(link_with: libbmath, include_directories: libbmath_inc)

libini_dep = dependency('inih')

# Test
unity_dep = dependency('unity', static: true, required: false)
if unity_dep.found()
  parser_test = executable(
    'bmath_parser_test',
    'test/parser.c',
    install: false,
    dependencies: [unity_dep, libbmath_dep],
  )

  functions_test = executable(
    'bmath_functions_test',
    'test/functions.c',
    install: false,
    dependencies: [unity_dep, libbmath_dep],
  )

  conversions_test = executable(
    'bmath_conversions_test',
    'test/conversions.c',
    install: false,
    dependencies: [unity_dep, libbmath_dep],
  )

  config_test = executable(
    'bmath_config_test',
    'src/config.c',
    'test/config.c',
    install: false,
    dependencies: [unity_dep, libini_dep],
  )

  test('parser', parser_test, args: [], verbose: true)
  test('functions', functions_test, args: [], verbose: true)
  test('conversions', conversions_test, args: [], verbose: true)
  test('config', config_test, args: [], verbose: true)
endif

# todo: figure out argp dep for non-gnu platforms
bmath_deps = [
  dependency('readline'),
  libini_dep,
  libbmath_dep,
]
executable(
  'bmath',
  'src/config.c',
  'src/execute.c',
  'src/bmath.c',
  dependencies: bmath_deps,
  install: true,
  pie: true,
)

install_man('man/bmath.1')
install_man('man/bmath-config.5')
install_data('etc/bmath/config.conf', install_dir: '/etc/bmath')
install_data('usr/share/mime/packages/x-bmath.xml', install_dir: '/usr/share/mime/packages')
meson.add_install_script('update-mime-database', '/usr/share/mime')

install_headers(
  'libbmath/src/lexer.h',
  'libbmath/src/parser.h',
  'libbmath/src/print.h',
  'libbmath/src/symbol.h',
  'libbmath/src/token.h',
  subdir: 'bmath',
)
