project('libffi', 'c', version : '3.2.9999',
        meson_version : '>= 0.49.0',
        default_options : ['buildtype=debugoptimized',
                           'warning_level=1'])

cc = meson.get_compiler('c')

is_msvc_like = ['msvc', 'clang-cl'].contains(cc.get_id())

# For FFI_EXTERN symbol exporting
add_project_arguments('-DFFI_BUILDING', language : 'c')
if get_option('default_library') == 'static'
  add_project_arguments('-DFFI_STATIC_BUILD', language : 'c')
endif

ffi_conf = configuration_data()

# NOTE: host = "cross" or "target"
host_cpu_family = host_machine.cpu_family()
host_system = host_machine.system()
message('host cpu: ' + host_machine.cpu())
message('host cpu_family: ' + host_cpu_family)
message('host system: ' + host_system)

# IMPORTANT: Some of these use set(), others set10(), and others only set(, 1)
# conditionally. This is on purpose.
# Some C code uses #ifdef HAVE_XXX and some #if !HAVE_XXX. To make things worse,
# some symbols are also used inside .h.in headers that are configured and then
# #include-ed at build time and installed. Each symbol has been carefully
# checked. Please double-check before changing.
#
# Nothing checks for STACK_DIRECTION

if cc.symbols_have_underscore_prefix()
  ffi_conf.set('SYMBOL_UNDERSCORE', 1)
endif

# Assembly directive support
if cc.compiles('asm (".cfi_startproc\n.cfi_endproc");', name : 'ASM .cfi')
  ffi_conf.set('HAVE_AS_CFI_PSEUDO_OP', 1)
endif

if host_cpu_family == 'sparc'
  if cc.compiles('asm (".text; foo: nop; .data; .align 4; .byte 0; .uaword %r_disp32(foo); .text");', name : 'ASM SPARC UA PCREL')
    ffi_conf.set('HAVE_AS_SPARC_UA_PCREL', 1)
  endif
  if cc.compiles('asm (".register %g2, #scratch");', name : 'ASM .register')
    ffi_conf.set('HAVE_AS_REGISTER_PSEUDO_OP', 1)
  endif
endif

if host_cpu_family == 'x86' or host_cpu_family == 'x86_64'
  if cc.compiles('asm (".text; foo: nop; .data; .long foo-.; .text");', name : 'ASM x86 PCREL')
    ffi_conf.set('HAVE_AS_X86_PCREL', 1)
  endif
  if cc.compiles('asm (".ascii \\"string\\"");', name : 'ASM .ascii')
    ffi_conf.set('HAVE_AS_ASCII_PSEUDO_OP', 1)
  endif
  if cc.compiles('asm (".string \\"string\\"");', name : 'ASM .string')
    ffi_conf.set('HAVE_AS_STRING_PSEUDO_OP', 1)
  endif
endif

# If not defined, define it as unsigned int
size_t = cc.sizeof('size_t')
if size_t > 0
  ffi_conf.set('SIZEOF_SIZE_T', size_t)
else
  message('"size_t" is not defined, using fallback')
  ffi_conf.set('size_t', 'unsigned int')
endif

# Checking for long double is important
size_long_double = cc.sizeof('long double')
size_double = cc.sizeof('double')
ffi_conf.set('SIZEOF_LONG_DOUBLE', size_long_double)
ffi_conf.set('SIZEOF_DOUBLE', size_double)
ffi_conf.set('HAVE_LONG_DOUBLE', 0)
ffi_conf.set('HAVE_LONG_DOUBLE_VARIANT', 0)
if host_cpu_family == 'alpha'
  message('"long double" support is detected at compile-time')
  ffi_conf.set('HAVE_LONG_DOUBLE', 'defined(__LONG_DOUBLE_128__)')
elif host_cpu_family == 'mips'
  message('"long double" support is detected at compile-time')
  ffi_conf.set('HAVE_LONG_DOUBLE', 'defined(__mips64)')
else
  if size_long_double > 0
    if size_long_double > size_double
      message('sizeof "long double" is greater than "double"')
      ffi_conf.set('HAVE_LONG_DOUBLE', 1)
      if host_cpu_family == 'powerpc' and host_system != 'darwin'
        message('"long double" size can be different')
        ffi_conf.set('HAVE_LONG_DOUBLE_VARIANT', 1)
      endif
    endif
  endif
endif

# Exception handling frame
# FIXME: Actually check for this instead of hard-coding it
# Also, check if this is actually correct
if host_cpu_family == 'x86_64'
  message('.eh_frame is hard-coded to not be ro')
  ffi_conf.set('EH_FRAME_FLAGS', '"aw"')
else
  message('.eh_frame is hard-coded to ro')
  ffi_conf.set('HAVE_RO_EH_FRAME', 1)
  ffi_conf.set('EH_FRAME_FLAGS', '"a"')
endif

if ['arm', 'aarch64'].contains(host_cpu_family) and host_system == 'darwin'
  message('Cannot use PROT_EXEC on this target, using fallback')
  ffi_conf.set('FFI_EXEC_TRAMPOLINE_TABLE', 1)
else
  ffi_conf.set('FFI_EXEC_TRAMPOLINE_TABLE', 0)
  if ['android', 'darwin', 'openbsd', 'freebsd', 'solaris'].contains(host_system)
    message('Cannot use malloc on this target, using fallback')
    ffi_conf.set('FFI_MMAP_EXEC_WRIT', 1)
  endif
endif

if host_cpu_family == 'x86_64' and not is_msvc_like
  # FIXME: Actually check for this instead of hard-coding it
  message('Assembler supports .unwind section type')
  ffi_conf.set('HAVE_AS_X86_64_UNWIND_SECTION_TYPE', 1)
endif

# Check mmap()
# XXX: All these are unused
#if cc.has_function('mmap')
#  ffi_conf.set('HAVE_MMAP', 1)
#endif
#ffi_conf.set('HAVE_MMAP_FILE', 1) # Works everywhere
#ffi_conf.set('HAVE_MMAP_DEV_ZERO',
#  host_system != 'windows' and host_system != 'darwin')
#mmap_anon = '''#include <sys/types.h>
##include <sys/mman.h>
##include <unistd.h>
#
##ifndef MAP_ANONYMOUS
##define MAP_ANONYMOUS MAP_ANON
##endif
#
#int n = MAP_ANONYMOUS;
#'''
#ffi_conf.set('HAVE_MMAP_ANON',
#  cc.compiles(mmap_anon, name : 'mmap anonymous'))

# Misc functions
#ffi_conf.set('HAVE_ALLOCA', cc.has_function('alloca')) # XXX: unused
ffi_conf.set10('HAVE_MEMCPY', cc.has_function('memcpy'))
ffi_conf.set('HAVE_MKOSTEMP', cc.has_function('mkostemp'))

# Misc headers
ffi_conf.set10('HAVE_ALLOCA_H', cc.has_header('alloca.h'))
ffi_conf.set('HAVE_INTTYPES_H', cc.has_header('inttypes.h'))
ffi_conf.set('HAVE_STDINT_H', cc.has_header('stdint.h'))
# Checks in the configure file that aren't used
#ffi_conf.set10('HAVE_DLFCN_H', cc.has_header('dlfcn.h'))
#ffi_conf.set10('HAVE_MEMORY_H', cc.has_header('memory.h'))
#ffi_conf.set10('HAVE_STDLIB_H', cc.has_header('stdlib.h'))
#ffi_conf.set10('HAVE_STRING_H', cc.has_header('string.h'))
#ffi_conf.set10('HAVE_STRINGS_H', cc.has_header('strings.h'))
#ffi_conf.set10('HAVE_SYS_MMAN_H', cc.has_header('sys/mman.h'))
#ffi_conf.set10('HAVE_SYS_STAT_H', cc.has_header('sys/stat.h'))
#ffi_conf.set10('HAVE_SYS_TYPES_H', cc.has_header('sys/types.h'))
#ffi_conf.set10('HAVE_UNISTD_H', cc.has_header('unistd.h'))

# Misc defines
if cc.has_function_attribute('visibility')
  t = find_program('test-cc-supports-hidden-visibility.py')
  res = run_command(t, cc.cmd_array(), check: false)
  if res.returncode() == 0
    message('.hidden pseudo-op is available')
    ffi_conf.set('HAVE_HIDDEN_VISIBILITY_ATTRIBUTE', 1)
  else
    message('.hidden pseudo-op is NOT available: ' + res.stdout() + res.stderr())
  endif
endif

# User options
if get_option('ffi-debug')
  ffi_conf.set('FFI_DEBUG', 1)
endif
if not get_option('raw_api')
  ffi_conf.set('FFI_NO_RAW_API', 1)
endif
if not get_option('structs')
  ffi_conf.set('FFI_NO_STRUCTS', 1)
endif
if get_option('purify_safety')
  ffi_conf.set('USING_PURIFY', 1)
endif
if get_option('pax_emutramp')
  ffi_conf.set('FFI_MMAP_EXEC_EMUTRAMP_PAX', 1)
endif

# This if/else ladder is based on the configure.host file
TARGET = ''
if host_cpu_family.startswith('x86')
  arch_subdir = 'x86'
  if host_system == 'windows'
    if size_t == 4
      TARGET = 'X86_WIN32'
      if is_msvc_like
        c_sources = ['ffiold-msvc.c']
        asm_sources = ['win32_msvc.S']
      else
        c_sources = ['ffi.c']
        asm_sources = ['sysv.S']
      endif
    else
      TARGET = 'X86_WIN64'
      c_sources = ['ffiw64.c']
      if is_msvc_like
        asm_sources = ['win64_intel.S']
      else
        asm_sources = ['win64.S']
      endif
    endif
  elif ['darwin', 'ios', 'linux', 'android'].contains(host_system)
    if size_t == 4
      if ['darwin', 'ios'].contains(host_system)
        TARGET = 'X86_DARWIN'
      else
        TARGET = 'X86'
      # FIXME: TARGET_X32 support
      endif
      c_sources = ['ffi.c']
      asm_sources = ['sysv.S']
    else
      TARGET = 'X86_64'
      c_sources = ['ffi64.c', 'ffiw64.c']
      asm_sources = ['unix64.S', 'win64.S']
    endif
  endif
elif host_cpu_family == 'aarch64'
  arch_subdir = 'aarch64'
  TARGET = 'AARCH64'
  c_sources = ['ffi.c']
  if is_msvc_like
    asm_sources = ['win64_armasm.S']
  else
    asm_sources = ['sysv.S']
  endif
elif host_cpu_family == 'arm'
  arch_subdir = 'arm'
  TARGET = 'ARM'
  c_sources = ['ffi.c']
  asm_sources = ['sysv.S']
elif host_cpu_family == 'riscv64'
  arch_subdir = 'riscv'
  TARGET = 'RISCV'
  c_sources = ['ffi.c']
  asm_sources = ['sysv.S']
endif

if TARGET == ''
  error('Unsupported pair: system "@0@", cpu family "@1@"'.format(host_system, host_cpu_family))
endif

# Used in ffi.h.in to generate ffi-$arch.h
ffi_conf.set('TARGET', TARGET)
ffi_conf.set('VERSION', meson.project_version())

# Configure fficonfig.h (not installed)
configure_file(input : 'fficonfig.h.meson', output : 'fficonfig.h',
  configuration : ffi_conf)

msvcc = find_program('msvcc.sh')

ffiinc = [include_directories('.'), include_directories('include')]

# Configure and install headers
subdir('include')

# Configure ffi_conf some more and declare libffi.so
subdir('src')

# TODO: Install texinfo files
install_man([
  'man/ffi.3',
  'man/ffi_call.3',
  'man/ffi_prep_cif.3',
  'man/ffi_prep_cif_var.3'])
