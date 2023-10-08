# Resources contain PNG and BMP files, so we need these two loaders
# enabled in order to build them
# FIXME: On Windows glib-compile-resources will fail to execute
# gdk-pixbuf-pixdata from build directory because it needs all DLL locations in
# $PATH. Ideally we should use gnome.compile_resources() and let Meson deal with
# this problem: See https://github.com/mesonbuild/meson/issues/8266.
if enabled_loaders.contains('png') and host_system != 'windows'
  # Resources; we cannot use gnome.compile_resources() here, because we need to
  # override the environment in order to use the utilities we just built instead
  # of the system ones
  resources_c = custom_target('resources.c',
    input: 'resources.gresource.xml',
    output: 'resources.c',
    command: [
      gen_resources,
      '--glib-compile-resources=@0@'.format(glib_compile_resources.full_path()),
      '--pixdata=@0@'.format(gdk_pixbuf_pixdata.full_path()),
      '--loaders=@0@'.format(loaders_cache.full_path()),
      '--sourcedir=@0@'.format(meson.current_source_dir()),
      '--source',
      '@INPUT@',
      '@OUTPUT@',
    ],
    depends: [
      gdk_pixbuf_pixdata,
      loaders_cache,
    ],
  )

  resources_h = custom_target('resources.h',
    input: 'resources.gresource.xml',
    output: 'resources.h',
    command: [
      gen_resources,
      '--glib-compile-resources=@0@'.format(glib_compile_resources.full_path()),
      '--pixdata=@0@'.format(gdk_pixbuf_pixdata.full_path()),
      '--loaders=@0@'.format(loaders_cache.full_path()),
      '--sourcedir=@0@'.format(meson.current_source_dir()),
      '--header',
      '@INPUT@',
      '@OUTPUT@',
    ],
    depends: [
      gdk_pixbuf_pixdata,
      loaders_cache,
    ],
  )
  no_resources = false
else
  resources_c = []
  resources_h = []
  no_resources = true
endif

# {
#   'test name': {
#     'suites': [ test suites ], (optional)
#     'needs_resources': bool, (optional)
#     'skip': bool, (optional)
#   }
# }
#
# test suites:
#  - conform: Behavior conformance test
#  - security: CVEs and the like
#  - slow: Needs special timeout value
#  - format: Per-format test
#  - io: Loading/saving
#  - ops: Pixel operations
installed_tests = {
  'pixbuf-construction': { 'suites': ['conform'], },
  'animation': {
    'suites': ['format'],
  },
  'cve-2015-4491': {
    'suites': ['security'],
    'needs_resources': true,
    'skip': no_resources,
  },
  'pixbuf-fail': { 'suites': ['conform', 'slow'], },
  'pixbuf-icon-serialize': { 'suites': ['conform'], },
  'pixbuf-randomly-modified': { 'suites': ['slow'], },
  'pixbuf-threads': { 'suites': ['io'], },
  'pixbuf-gif': {
    'suites': ['io'],
  },
  'pixbuf-icc': {
    'suites': ['io'],
  },
  'pixbuf-jpeg': {
    'suites': ['format'],
    'skip': not enabled_loaders.contains('jpeg'),
  },
  'pixbuf-dpi': {
    'suites': ['io'],
  },
  'pixbuf-pixdata': {
    'suites': ['format'],
    'needs_resources': true,
    'skip': no_resources,
  },
  'pixbuf-stream': { 'suites': ['io'], },
  'pixbuf-reftest': {
    'suites': ['conform'],
  },
  'pixbuf-resource': {
    'suites': ['io'],
    'needs_resources': true,
    'skip': no_resources,
  },
  'pixbuf-scale': { 'suites': ['ops'], },
  'pixbuf-scale-two-step': { 'suites': ['ops'], },
  'pixbuf-short-gif-write': {
    'suites': ['format'],
  },
  'pixbuf-gif-circular-table': {
    'suites': ['format'],
  },
  'pixbuf-save': { 'suites': ['io'] },
  'pixbuf-readonly-to-mutable': { 'suites': ['conform'], },
  'pixbuf-composite': {
    'suites': ['ops'],
    'skip': not enabled_loaders.contains('png'),
  },
  'pixbuf-area-updated': { 'suites': ['slow'], },
}

test_data = [
  'test-image.png',
  'test-animation.gif',
  '1_partyanimsm2.gif',
  'test-animation.ani',
  'icc-profile.jpeg',
  'icc-profile.png',
  'dpi.jpeg',
  'dpi.png',
  'dpi.tiff',
  'premature-end.png',
  'premature-end.jpg',
  'bug143608-comment.jpg',
  'bug725582-testrotate.jpg',
  'bug725582-testrotate.png',
  'bug753605-atsize.jpg',
  'cve-2015-4491.bmp',
  'large.png',
  'large.jpg',
  'bug775218.jpg',
  'test-image.pixdata',
  'test-image-rle.pixdata',
  'bug775693.pixdata',
  'bug775229.pixdata',
  'aero.gif',
  'circular-table.gif',
  'issue70.jpg',
  'issue205.jpg',
]

installed_test_bindir = join_paths(gdk_pixbuf_libexecdir, 'installed-tests', meson.project_name())
installed_test_datadir = join_paths(gdk_pixbuf_datadir, 'installed-tests', meson.project_name())

if get_option('installed_tests')
  install_data(test_data, install_dir: installed_test_bindir)
  install_subdir('test-images', install_dir: installed_test_bindir)
endif

test_deps = gdk_pixbuf_deps + [ gdkpixbuf_dep, ]
test_args = [ '-k' ]
test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('GDK_PIXBUF_MODULE_FILE', loaders_cache.full_path())

foreach test_name, test_data: installed_tests
  test_sources = [ test_name + '.c', 'test-common.c' ]
  test_suites = test_data.get('suites', [])
  needs_resources = test_data.get('needs_resources', false)
  if needs_resources
    test_sources += [ resources_c, resources_h ]
  endif
  skip_if_true = test_data.get('skip', false)
  if skip_if_true
    continue
  endif

  custom_target(test_name + '.test',
    output: test_name + '.test',
    command: [
      gen_installed_test,
      '--testbindir="@0@"'.format(installed_test_bindir),
      '--testbin=@0@'.format(test_name),
      '@OUTPUT@',
    ],
    install: get_option('installed_tests'),
    install_dir: installed_test_datadir,
  )

  test_bin = executable(test_name, test_sources,
    dependencies: test_deps,
    include_directories: [ root_inc, gdk_pixbuf_inc, ],
    c_args: common_cflags,
    install: get_option('installed_tests'),
    install_dir: installed_test_bindir,
  )

  # Two particularly slow tests
  if test_suites.contains('slow')
    timeout = 300
  else
    timeout = 30
  endif

  test(test_name, test_bin,
    suite: test_suites,
    args: test_args,
    env: test_env,
    timeout: timeout,
  )
endforeach

executable('pixbuf-read',
  'pixbuf-read.c',
  dependencies: test_deps,
  include_directories: [
    root_inc,
    include_directories('../gdk-pixbuf')
  ],
  c_args: common_cflags,
)
