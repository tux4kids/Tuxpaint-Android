ffi_c_sources = [
  'prep_cif.c',
  'types.c',
  'raw_api.c',
  'java_raw_api.c',
  'closures.c',
]

ffi_asm_sources = []

if get_option('ffi-debug')
  ffi_c_sources += ['debug.c']
endif

# Translate source files to source paths
foreach c : c_sources
  ffi_c_sources += arch_subdir + '/' + c
endforeach
foreach asm : asm_sources
  ffi_asm_sources += arch_subdir + '/' + asm
endforeach

if is_msvc_like
  # GCC and Clang accept assembly files directly, but MSVC's cl.exe does not.
  # You need to manually pass the files through the pre-processor first and
  # then through the assembler, and then link the objects into the target.
  # FIXME: Add native support in Meson for this.
  cl = find_program('cl')
  if host_cpu_family.startswith('x86')
    if host_cpu_family == 'x86'
      ml = find_program('ml')
    elif host_cpu_family == 'x86_64'
      ml = find_program('ml64')
    else
      error('Unknown cpu_family: ' + host_cpu_family)
    endif
    assembler_args = [ml, '/Fo', '@OUTPUT@', '/c', '@INPUT@']
    if get_option('buildtype').startswith('debug')
      assembler_args += ['/Zi', '/Zd']
    endif
  elif host_cpu_family == 'aarch64'
    assembler_args = [find_program('armasm64'), '-o', '@OUTPUT@', '@INPUT@']
    if get_option('buildtype').startswith('debug')
      assembler_args += ['-g']
    endif
  else
    error('Unsupported MSVC target: ' + host_cpu_family)
  endif
  ffi_asm_objs = []
  foreach asm_source : ffi_asm_sources
    incflags = ['/I' + join_paths(meson.current_source_dir(), '..'),
                '/I' + join_paths(meson.current_build_dir(), '..'),
                '/I' + join_paths(meson.current_source_dir(), '..', 'include'),
                '/I' + join_paths(meson.current_build_dir(), '..', 'include')]
    preproc_name = asm_source.underscorify() + '.i'
    obj_name = asm_source.underscorify() + '.obj'
    preproc = custom_target(preproc_name,
        input : asm_source,
        output : preproc_name,
        command : [cl, '/nologo', '/EP', '/P', '/Fi@OUTPUT@',
                   '/DTARGET=' + TARGET, '/DFFI_BUILDING',
                   '@INPUT@'] + incflags)
    ffi_asm_objs += custom_target(obj_name,
        input : preproc,
        output : obj_name,
        command : assembler_args)
  endforeach
  ffi_asm_sources = ffi_asm_objs
endif

ffi_link_args = []

if get_option('ffi-build-versioned') and cc.has_argument('-x assembler-with-cpp')
  ffi_map_file = 'libffi.map'
  ffi_symbol_map = configure_file(
      output: ffi_map_file,
      command : [meson.get_compiler('c'),
                 get_option('c_args'),
                 '-I' + join_paths(meson.current_build_dir(), '..'),
                 '-I' + join_paths(meson.current_build_dir(), '..','include'),
                 '-D' + TARGET,
                 '-E',
                 '-x',
                 'assembler-with-cpp',
                 join_paths(meson.current_source_dir(),'../libffi.map.in'),
                 '-o',
                 '@OUTPUT@'
                 ]
    )
  version_link_arg = '-Wl,--version-script=@0@'.format(join_paths(meson.current_build_dir(), ffi_map_file))
  if cc.has_link_argument([version_link_arg])
    ffi_link_args += version_link_arg
  endif
endif

ffi_lib = library('ffi', ffi_c_sources, ffi_asm_sources,
  c_args : '-DTARGET=' + TARGET,
  include_directories : ffiinc,
  link_args: ffi_link_args,
  # Taken from the libtool-version file
  # current - age . age . revision
  version : '7.1.0',
  # current - age
  soversion : '7',
  # current + 1
  darwin_versions : '9',
  install : true)

pkgconf = import('pkgconfig')
pkgconf.generate(ffi_lib,
  description : 'Library supporting Foreign Function Interfaces',
  filebase : 'libffi')

c_args = []
if get_option('default_library') == 'static'
  c_args += ['-DFFI_STATIC_BUILD']
endif

ffi_dep = declare_dependency(link_with : ffi_lib,
  compile_args: c_args,
  include_directories : ffiinc)
