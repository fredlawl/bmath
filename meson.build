# for devel: meson install --tags devel
# for deployment: meson install --tags runtime,man
# for test: meson test
project(
  'bmath',
  'c',
  version: '1.0.6',
  default_options: [
    'c_std=c11'])


add_project_arguments([
  '-Wshadow',
  '-Wvla',
  '-Wmissing-field-initializers',
  '-fno-omit-frame-pointer'], language: ['c'])

add_project_link_arguments('-Wl,--build-id', language: ['c'])

# default build type
if get_option('buildtype') == 'debug'
  add_global_arguments('-DDEBUG', language : 'c')
endif

if get_option('buildtype') == 'release'
  add_project_arguments(['-fstack-protector'], language: ['c'])
endif

configure_file(
    input: 'src/version.h.in',
    output: 'version.h',
    configuration: {
      'version': meson.project_version()})

# Release
libbmath_deps = [dependency('iconv')]
libbmath = shared_library(
  'bmath',
  'src/parser.c',
  'src/print.c',
  dependencies: libbmath_deps,
  install: true,
  soversion: '0',
  version: '0.0.2')

# Test
criterion_dep = dependency('criterion', required: false)
if criterion_dep.found()
  test_exe = executable(
    'bmath_test',
    'test/parser.c',
    install: false,
    dependencies: [criterion_dep],
    link_with: libbmath)

  test('everything', test_exe, args: ['--verbose'], verbose: true)
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
  pie: true)

install_man('man/bmath.1')
install_headers('src/print.h', 'src/parser.h', subdir: 'bmath')

