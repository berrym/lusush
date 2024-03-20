project('lusush',
        'c',
        default_options: ['c_std=c99',
                          'optimization=3',
                          'warning_level=2'
                         ],
        version: '0.1.2',
        license: 'MIT')

inc = include_directories('include', 'include/third_party')

src = ['src/errors.c',
       'src/exec.c',
       'src/ht.c',
       'src/init.c',
       'src/input.c',
       'src/lusush.c',
       'src/prompt.c',
       'src/signals.c',
       'src/strings.c',
       'src/builtins/alias.c',
       'src/builtins/builtins.c',
       'src/builtins/history.c',
       'src/node.c',
       'src/parser.c',
       'src/pattern.c',
       'src/scanner.c',
       'src/shunt.c',
       'src/symtable.c',
       'src/vars.c',
       'src/wordexp.c',
       'src/third_party/linenoise.c',
      ]

if host_machine.cpu_family() == 'x86'
  add_project_arguments('-DX86_32_BIT_CPU', language: 'c')
elif host_machine.cpu_family() == 'x86_64'
  add_project_arguments('-DX86_64_BIT_CPU', language: 'c')
endif

add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')
add_project_arguments('-D_XOPEN_SOURCE=700', language: 'c')
add_project_arguments('-D_XOPEN_SOURCE_EXTENDED', language: 'c')

lusush_exe = executable('lusush',
                        src,
                        include_directories: inc)
