project('lusush',
	'c',
	default_options: 'c_std=c99',
        version: '0.1.0',
        license: 'BSD2CLAUSE')

inc = include_directories('include')

src = ['src/alias.c',
       'src/builtins.c',
       'src/cmdlist.c',
       'src/errors.c',
       'src/exec.c',
       'src/init.c',
       'src/lusush.c',
       'src/opts.c',
       'src/parse.c']

editlinedep = dependency('libedit', required: false)
readlinedep = dependency('readline',  version: '>=5.2', required: false)
linedep = []

if editlinedep.found()
  src += 'src/input-editline.c'
  src += 'src/history-editline.c'
  src += 'src/prompt-editline.c'
  linedep += editlinedep
elif readlinedep.found()
  src += 'src/input-readline.c'
  src += 'src/history-readline.c'
  src += 'src/prompt-readline.c'
  linedep += readlinedep
else
  src += 'src/input.c'
  src += 'src/history.c'
  src += 'src/prompt.c'
endif

lusush_exe = executable('lusush',
			src,
		        dependencies: linedep,
			include_directories: inc)
