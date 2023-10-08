subdir('pixops')

# Loaders
#  - sources: additional source files
#  - c_args: additional C compiler arguments
#  - enabled: condition for building

loaders = {
  'png': {
    'sources': [ 'io-png.c' ],
    'enabled': enabled_loaders.contains('png'),
  },
  'bmp': {
    'sources': [ 'io-bmp.c' ],
    'enabled': not native_windows_loaders,
  },
  'gif': {
    'sources': [ 'io-gif.c', 'io-gif-animation.c', 'lzw.c' ],
    'enabled': not native_windows_loaders,
  },
  'ico': {
    'sources': [ 'io-ico.c' ],
    'enabled': not native_windows_loaders,
  },
  'ani': {
    'sources': [ 'io-ani.c', 'io-ani-animation.c' ],
    'enabled': true,
  },
  'jpeg': {
    'sources': [ 'io-jpeg.c' ],
    'enabled': enabled_loaders.contains('jpeg'),
  },
  'pnm': {
    'sources': [ 'io-pnm.c' ],
    'enabled': true,
  },
  'tiff': {
    'sources': [ 'io-tiff.c' ],
    'enabled': enabled_loaders.contains('tiff'),
  },
  'xpm': {
    'sources': [ 'io-xpm.c' ],
    'enabled': true,
  },
  'xbm': {
    'sources': [ 'io-xbm.c' ],
    'enabled': true,
  },
  'tga': {
    'sources': [ 'io-tga.c', 'gdk-pixbuf-buffer-queue.c' ],
    'enabled': true,
  },
  'icns': {
    'sources': [ 'io-icns.c' ],
    'enabled': true,
  },
  'qtif': {
    'sources': [ 'io-qtif.c' ],
    'enabled': true,
  },
}

gdk_pixbuf_inc = include_directories('.')

gdk_pixbuf_cflags = [
  '-DG_LOG_STRUCTURED=1',
  '-DG_LOG_DOMAIN="GdkPixbuf"',
  '-DGDK_PIXBUF_COMPILATION',
  '-DGDK_PIXBUF_PREFIX="@0@"'.format(gdk_pixbuf_prefix),
  '-DGDK_PIXBUF_LOCALEDIR="@0@"'.format(gdk_pixbuf_localedir),
  '-DGDK_PIXBUF_LIBDIR="@0@"'.format(gdk_pixbuf_libdir),
  '-DGDK_PIXBUF_BINARY_VERSION="@0@"'.format(gdk_pixbuf_binary_version),
  '-DGDK_PIXBUF_ENABLE_BACKEND',
  '-DPIXBUF_LIBDIR="@0@"'.format(gdk_pixbuf_loaderdir),
  '-DBUILT_MODULES_DIR="@0@"'.format(meson.current_build_dir()),
]

gdk_pixbuf_api_path = join_paths(gdk_pixbuf_api_name, 'gdk-pixbuf')

gdkpixbuf_features_conf = configuration_data()
gdkpixbuf_features_conf.set('GDK_PIXBUF_MAJOR', gdk_pixbuf_version_major)
gdkpixbuf_features_conf.set('GDK_PIXBUF_MINOR', gdk_pixbuf_version_minor)
gdkpixbuf_features_conf.set('GDK_PIXBUF_MICRO', gdk_pixbuf_version_micro)
gdkpixbuf_features_conf.set('GDK_PIXBUF_VERSION', meson.project_version())
gdkpixbuf_features_conf.set('GDK_PIXBUF_API_VERSION', gdk_pixbuf_api_version)
gdkpixbuf_features_conf.set('LT_CURRENT_MINUS_AGE', current_minus_age)

gdk_pixbuf_features_h = configure_file(input: 'gdk-pixbuf-features.h.in',
                                       output: 'gdk-pixbuf-features.h',
                                       configuration: gdkpixbuf_features_conf,
                                       install_dir: join_paths(gdk_pixbuf_includedir, gdk_pixbuf_api_path))

gdkpixbuf_headers = [
  'gdk-pixbuf.h',
  'gdk-pixbuf-animation.h',
  'gdk-pixbuf-autocleanups.h',
  'gdk-pixbuf-core.h',
  'gdk-pixbuf-io.h',
  'gdk-pixbuf-loader.h',
  'gdk-pixbuf-macros.h',
  'gdk-pixbuf-simple-anim.h',
  'gdk-pixbuf-transform.h',
]

install_headers(gdkpixbuf_headers + [ 'gdk-pixdata.h' ], subdir: gdk_pixbuf_api_path)

gdkpixbuf_sources = [
  'gdk-pixbuf.c',
  'gdk-pixbuf-animation.c',
  'gdk-pixbuf-data.c',
  'gdk-pixbuf-io.c',
  'gdk-pixbuf-loader.c',
  'gdk-pixbuf-scale.c',
  'gdk-pixbuf-simple-anim.c',
  'gdk-pixbuf-scaled-anim.c',
  'gdk-pixbuf-util.c',
]

gdkpixdata_sources = [
  'gdk-pixdata.c',
]

gdkpixbuf_marshals = gnome.genmarshal('gdk-pixbuf-marshal',
                                      sources: 'gdk-pixbuf-marshal.list',
                                      prefix: '_gdk_pixbuf_marshal',
                                      install_header: true,
                                      install_dir: join_paths(gdk_pixbuf_includedir, gdk_pixbuf_api_path))

gdkpixbuf_enums = gnome.mkenums('gdk-pixbuf-enum-types',
                                sources: gdkpixbuf_headers,
                                c_template: 'gdk-pixbuf-enum-types.c.template',
                                h_template: 'gdk-pixbuf-enum-types.h.template',
                                install_header: true,
                                install_dir: join_paths(gdk_pixbuf_includedir, gdk_pixbuf_api_path))
gdkpixbuf_enum_h = gdkpixbuf_enums[1]

if host_system == 'windows'
  gdk_pixbuf_win_rc = configure_file(
    input: 'gdk_pixbuf.rc.in',
    output: 'gdk_pixbuf.rc',
    configuration: gdkpixbuf_features_conf,
  )
  gdk_pixbuf_win_res = import('windows').compile_resources(gdk_pixbuf_win_rc)
  gdkpixbuf_sources += gdk_pixbuf_win_res
endif

# Check if we need to build loaders as built-in functionality
included_loaders_cflags = []
included_loaders_deps = []

foreach name, loader: loaders
  sources = loader.get('sources', [])
  cond = loader.get('enabled', true)
  cflags = loader.get('c_args', [])

  if cond and (builtin_loaders.contains(name) or builtin_all_loaders)
    cflag_for_included_loader = ['-DINCLUDE_@0@'.format(name)]
    included_loaders_cflags += cflag_for_included_loader

    mod = static_library('staticpixbufloader-@0@'.format(name),
                         sources,
                         gdkpixbuf_enum_h,
                         dependencies: loaders_deps + gdk_pixbuf_deps,
                         include_directories: [ root_inc, gdk_pixbuf_inc ],
                         c_args: common_cflags + gdk_pixbuf_cflags + cflags + cflag_for_included_loader)

    included_loaders_deps += declare_dependency(link_with: mod)
  endif
endforeach

# List of formats supported by the native Windows components-based loader(s)
windows_native_loader_formats = [ 'bmp', 'emf', 'gif', 'ico', 'jpeg', 'tiff', 'wmf' ]
windows_base_loader_sources = [ 'io-gdip-utils.c', 'io-gdip-animation.c' ]

# Build the loaders using native Windows components as static modules, if requested
if native_windows_loaders
  if builtin_loaders.contains('windows') or builtin_all_loaders
    cflag_for_included_loader = ['-DINCLUDE_gdiplus']
    included_loaders_cflags += cflag_for_included_loader
    win_loader_sources = windows_base_loader_sources
    foreach loader: windows_native_loader_formats
      win_loader_sources += 'io-gdip-@0@.c'.format(loader)
    endforeach

    mod = static_library('staticpixbufloader-gdiplus',
                         win_loader_sources,
                         gdkpixbuf_enum_h,
                         dependencies: loaders_deps + gdk_pixbuf_deps,
                         include_directories: [ root_inc, gdk_pixbuf_inc ],
                         c_args: common_cflags + gdk_pixbuf_cflags + cflag_for_included_loader)

    included_loaders_deps += declare_dependency(link_with: mod)
  endif
endif

# The main gdk-pixbuf shared library
gdkpixbuf = library('gdk_pixbuf-2.0',
                    sources: [
                      gdkpixbuf_sources,
                      pixops_sources,
                      gdkpixbuf_enums,
                      gdkpixbuf_marshals,
                      gdkpixdata_sources,
                    ],
                    soversion: soversion,
                    version: libversion,
		    darwin_versions: darwin_versions,
                    c_args: common_cflags + gdk_pixbuf_cflags + included_loaders_cflags,
                    link_args: common_ldflags,
                    include_directories: [
                      root_inc,
                      include_directories('pixops'),
                    ],
                    dependencies: [
                      gdk_pixbuf_deps,
                      included_loaders_deps,
                    ],
                    install: true)

gir = find_program('g-ir-scanner', required : get_option('introspection'))
build_gir = gir.found() and (not meson.is_cross_build() or get_option('introspection').enabled())

if build_gir
  gir_args = [
    '--quiet',
    '-DGDK_PIXBUF_COMPILATION',
    '-DGDK_PIXBUF_ENABLE_BACKEND',
  ]

  gdkpixbuf_gir = gnome.generate_gir(
    gdkpixbuf,
    sources: [
      gdkpixbuf_sources,
      gdkpixbuf_headers,
      gdkpixbuf_enums,
      gdkpixdata_sources,
      gdk_pixbuf_features_h,
    ],
    namespace: 'GdkPixbuf',
    nsversion: gdk_pixbuf_api_version,
    identifier_prefix: 'Gdk',
    symbol_prefix: 'gdk',
    export_packages: 'gdk-pixbuf-2.0',
    includes: [ 'GModule-2.0', 'Gio-2.0', ],
    header: 'gdk-pixbuf/gdk-pixbuf.h',
    install: true,
    extra_args: gir_args,
  )

  gdkpixdata_gir = gnome.generate_gir(
    gdkpixbuf,
    sources: gdkpixdata_sources + ['gdk-pixdata.h'],
    namespace: 'GdkPixdata',
    nsversion: gdk_pixbuf_api_version,
    identifier_prefix: 'Gdk',
    symbol_prefix: 'gdk',
    export_packages: 'gdk-pixbuf-2.0',
    includes: [ gdkpixbuf_gir[0] ],
    header: 'gdk-pixbuf/gdk-pixdata.h',
    install: true,
    extra_args: gir_args,
  )

  built_girs = [ gdkpixbuf_gir, gdkpixdata_gir ]
else
  built_girs = []
endif

gdkpixbuf_dep = declare_dependency(
  link_with: gdkpixbuf,
  include_directories: root_inc,
  dependencies: gdk_pixbuf_deps,
  sources: [ gdkpixbuf_enum_h, built_girs ],
)

# Now check if we are building loaders as installed shared modules
# We do this here because shared modules depend on libgdk-pixbuf
dynamic_loaders = []
dynamic_loaders_dep = []

foreach name, loader: loaders
  sources = loader.get('sources', [])
  cond = loader.get('enabled', true)
  cflags = loader.get('c_args', [])
  name_suffix = []

  # FIXME: workaround for https://gitlab.gnome.org/GNOME/glib/issues/1413
  if host_system == 'darwin'
    name_suffix = 'so'
  endif

  if cond and not (builtin_loaders.contains(name) or builtin_all_loaders)
    mod = shared_module('pixbufloader-@0@'.format(name),
                        sources,
                        gdkpixbuf_enum_h,
                        dependencies: loaders_deps + gdk_pixbuf_deps + [ gdkpixbuf_dep ],
                        include_directories: [ root_inc, gdk_pixbuf_inc ],
                        c_args: common_cflags + gdk_pixbuf_cflags + cflags,
                        name_suffix: name_suffix,
                        install: true,
                        install_dir: gdk_pixbuf_loaderdir)

    # We need the path to build loaders.cache for tests
    dynamic_loaders += mod.full_path()
    dynamic_loaders_dep += mod
  endif
endforeach

# Build the loaders using native Windows components as dynamic modules, if requested
if native_windows_loaders
  if not (builtin_loaders.contains('windows') or builtin_all_loaders)
    foreach loader: windows_native_loader_formats
      loader_sources = windows_base_loader_sources + [ 'io-gdip-@0@.c'.format(loader) ]

      mod = shared_module('pixbufloader-gdip-@0@'.format(loader),
                          loader_sources,
                          gdkpixbuf_enum_h,
                          dependencies: loaders_deps + gdk_pixbuf_deps + [ gdkpixbuf_dep ],
                          include_directories: [ root_inc, gdk_pixbuf_inc ],
                          c_args: common_cflags + gdk_pixbuf_cflags + cflags,
                          install: true,
                          install_dir: gdk_pixbuf_loaderdir)
      dynamic_loaders += mod.full_path()
      dynamic_loaders_dep = mod
    endforeach
  endif
endif

gdkpixbuf_bin = [
  [ 'gdk-pixbuf-csource' ],
  [ 'gdk-pixbuf-pixdata' ],
  [ 'gdk-pixbuf-query-loaders', [ 'queryloaders.c' ] ],
]

foreach bin: gdkpixbuf_bin
  bin_name = bin[0]
  bin_source = bin.get(1, bin_name + '.c')

  bin = executable(bin_name, bin_source,
                   gdkpixbuf_enum_h,
                   dependencies: gdk_pixbuf_deps + [ gdkpixbuf_dep ],
                   include_directories: [ root_inc, gdk_pixbuf_inc ],
                   c_args: common_cflags + gdk_pixbuf_cflags,
                   install: true)
  meson.override_find_program(bin_name, bin)

  # Used in tests
  set_variable(bin_name.underscorify(), bin)
endforeach

if not meson.is_cross_build()
  # The 'loaders.cache' used for testing, so we don't accidentally
  # load the installed cache; we always build it by default
  loaders_cache = custom_target('loaders.cache',
                                output: 'loaders.cache',
                                capture: true,
                                command: [
                                  gdk_pixbuf_query_loaders,
                                  dynamic_loaders,
                                ],
                                depends: dynamic_loaders_dep,
                                build_by_default: true)
  loaders_dep = declare_dependency(sources: [ loaders_cache ])
else
  loaders_cache = []
  loaders_dep = declare_dependency()
endif

pkgconfig = import('pkgconfig')
pkgconfig.generate(
  gdkpixbuf,
  name: 'GdkPixbuf',
  description: 'Image loading and scaling',
  variables: [
    'bindir=${prefix}/@0@'.format(get_option('bindir')),
    'gdk_pixbuf_binary_version=@0@'.format(gdk_pixbuf_binary_version),
    'gdk_pixbuf_binarydir=${libdir}/@0@/@1@'.format(gdk_pixbuf_api_name, gdk_pixbuf_binary_version),
    'gdk_pixbuf_moduledir=${gdk_pixbuf_binarydir}/loaders',
    'gdk_pixbuf_cache_file=${gdk_pixbuf_binarydir}/loaders.cache',
    'gdk_pixbuf_csource=${bindir}/gdk-pixbuf-csource',
    'gdk_pixbuf_pixdata=${bindir}/gdk-pixbuf-pixdata',
    'gdk_pixbuf_query_loaders=${bindir}/gdk-pixbuf-query-loaders',
  ],
  requires: 'gobject-2.0',
  subdirs: gdk_pixbuf_api_name,
  filebase: gdk_pixbuf_api_name,
)
