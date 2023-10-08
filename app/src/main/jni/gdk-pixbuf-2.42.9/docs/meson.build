gidocgen_dep = dependency('gi-docgen',
  version: '>= 2021.1',
  fallback: ['gi-docgen', 'dummy_dep'],
  required: get_option('gtk_doc'),
)


toml_conf = configuration_data()
toml_conf.set('VERSION', meson.project_version())
pixbuf_toml = configure_file(input: 'gdk-pixbuf.toml.in', output: 'gdk-pixbuf.toml', configuration: toml_conf)
pixdata_toml = configure_file(input: 'gdk-pixdata.toml.in', output: 'gdk-pixdata.toml', configuration: toml_conf)

gidocgen = find_program('gi-docgen', required: get_option('gtk_doc'))

docs_dir = gdk_pixbuf_datadir / 'doc'

expand_content_md_files = [
  'scaling-compositing.md',
]

build_docs = get_option('gtk_doc')
if get_option('docs')
  warning('The docs option is deprecated; use -Dgtk_doc=true')
  build_docs = true
endif

if build_docs
  custom_target('gdk-pixbuf-doc',
    input: [ pixbuf_toml, gdkpixbuf_gir[0] ],
    output: 'gdk-pixbuf',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../gdk-pixbuf'),
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    depend_files: [ expand_content_md_files ],
    build_by_default: true,
    install: true,
    install_dir: docs_dir,
  )

  custom_target('gdk-pixdata-doc',
    input: [ pixdata_toml, gdkpixdata_gir[0] ],
    output: 'gdk-pixdata',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../gdk-pixbuf'),
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    build_by_default: true,
    install: true,
    install_dir: docs_dir,
  )
endif

rst2man = find_program('rst2man', required: false)
if get_option('man') and not rst2man.found()
  error('No rst2man found, but man pages were explicitly enabled')
endif

if get_option('man') and rst2man.found()
  rst_files = [
    [ 'gdk-pixbuf-csource', ],
    [ 'gdk-pixbuf-query-loaders', ],
  ]

  rst2man_flags = [
    '--syntax-highlight=none',
  ]

  foreach rst: rst_files
    man_name = rst[0]
    man_section = rst.get(1, '1')

    custom_target('man-@0@'.format(man_name),
      input: '@0@.rst'.format(man_name),
      output: '@0@.@1@'.format(man_name, man_section),
      command: [
        rst2man,
        rst2man_flags,
        '@INPUT@',
      ],
      capture: true,
      install: true,
      install_dir: get_option('mandir') / 'man@0@'.format(man_section),
    )
  endforeach
endif
