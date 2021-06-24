project('lusush',
        'c',
        default_options: 'c_std=c99',
        version: '0.1.0',
        license: 'MIT')

inc = include_directories('include')

src = ['src/errors.c',
       'src/exec.c',
       'src/init.c',
       'src/input.c',
       'src/lusush.c',
       'src/prompt.c',
       'src/signals.c',
       'src/strings.c',
       'src/builtins/alias.c',
       'src/builtins/builtins.c',
       'src/builtins/history.c',
       'src/parser/node.c',
       'src/parser/parser.c',
       'src/scanner/scanner.c',
       'src/symtable/symtable.c',
       'src/vars.c']

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
