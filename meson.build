project('lusush',
	'c',
	default_options : 'c_std=c99',
        version : '0.1.0',
        license : 'BSD2CLAUSE')

inc = include_directories('include')

src = ['src/alias.c',
       'src/builtins.c',
       'src/cmdlist.c',
       'src/errors.c',
       'src/exec.c',
       'src/init.c',
       'src/lusush.c',
       'src/opts.c',
       'src/parse.c',
       'src/prompt.c',
       'src/util.c']

editlinedep = dependency('libedit', required : false)
readlinedep = dependency('readline',  version : '>=5.2', required : false)

if editlinedep.found() or readlinedep.found()
  src += 'src/input-readline.c'
  src += 'src/history-readline.c'
else
  src += 'src/input.c'
  src += 'src/history.c'
endif

lusush_exe = executable('lusush',
			src,
		        dependencies : [editlinedep, readlinedep],
			include_directories : inc)
