project('lusush',
        'c',
        default_options: 'c_std=c99',
        version: '0.1.0',
        license: 'MIT')

inc = include_directories('include')

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
       'src/scanner.c',
       'src/symtable.c',
       'src/vars.c']

if host_machine.cpu_family() == 'x86'
  add_project_arguments('-DX86_32_BIT_CPU', language: 'c')
elif host_machine.cpu_family() == 'x86_64'
  add_project_arguments('-DX86_64_BIT_CPU', language: 'c')
endif

editlinedep = dependency('libedit', required: false)
readlinedep = dependency('readline',  version: '>=5.2', required : false)
linedep = []

if editlinedep.found()
  add_project_arguments('-DUSING_EDITLINE -DUSING_EDITLINE_HISTORY', language : 'c')
  linedep += editlinedep
elif readlinedep.found()
  add_project_arguments('-DUSING_READLINE -DUSING_READLINE_HISTORY', language : 'c')
  linedep += readlinedep
endif

lusush_exe = executable('lusush',
                        src,
                        dependencies: linedep,
                        include_directories: inc)
