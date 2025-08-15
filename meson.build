# for devel: meson install --tags devel
# for deployment: meson install --tags runtime,man
# for test: meson test
project('bmath', 'c', version: '3.0.0', default_options: ['c_std=c11'])

add_project_arguments(
  [
    '-Wshadow',
    '-Wvla',
    '-Wmissing-field-initializers',
    '-fno-omit-frame-pointer',
    '-D_FORTIFY_SOURCE=2',
    '-fstack-protector-strong',
    '-fstack-clash-protection',
    '-O2',
  ],
  language: ['c'],
)

add_project_link_arguments('-Wl,--build-id', language: ['c'])

# default build type
if get_option('buildtype') == 'debug'
  add_global_arguments('-DDEBUG', language: 'c')
endif

if get_option('buildtype') == 'release'
  add_project_arguments(['-pie', '-fpie'], language: ['c'])
  add_project_link_arguments(['-Wl,-z,relro,-z,now', '-Wl,-z,noexecstack'], language: ['c'])
endif

configure_file(
  input: 'src/version.h.in',
  output: 'version.h',
  configuration: {'version': meson.project_version()},
)

# Release
libbmath_deps = [dependency('iconv')]
libbmath = shared_library(
  'bmath',
  'src/parser.c',
  'src/print.c',
  'src/token.c',
  'src/functions.c',
  dependencies: libbmath_deps,
  install: true,
  soversion: '1',
  version: '1.0.0',
)

# Test
unity_dep = dependency('unity', static: true, required: false)
if unity_dep.found() 
  parser_test = executable(
    'bmath_parser_test',
    'test/parser.c',
    install: false,
    dependencies: [unity_dep],
    link_with: libbmath,
  )

  functions_test = executable(
    'bmath_functions_test',
    'test/functions.c',
    install: false,
    dependencies: [unity_dep],
    link_with: libbmath,
  )

  conversions_test = executable(
    'bmath_conversions_test',
    'test/conversions.c',
    install: false,
    dependencies: [unity_dep],
    link_with: libbmath,
  )

  test('parser', parser_test, args: [], verbose: true)
  test('functions', functions_test, args: [], verbose: true)
  test('conversions', conversions_test, args: [], verbose: true)
endif

# todo: figure out argp dep for non-gnu platforms
bmath_deps = [dependency('readline')]
executable(
  'bmath',
  'src/bmath.c',
  dependencies: bmath_deps,
  link_with: libbmath,
  install: true,
  install_rpath: join_paths(get_option('prefix'), get_option('libdir')),
  pie: true,
)

install_man('man/bmath.1')
install_headers('src/print.h', 'src/parser.h', subdir: 'bmath')
