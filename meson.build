project('lusush',
        'c',
        default_options: ['c_std=c99',
                          'optimization=3',
                          'warning_level=2'
                         ],
        version: '1.0.0',
        license: 'MIT')

inc = include_directories('include',
                          'include/libhashtable',
                          'include/linenoise')

src = ['src/builtins/alias.c',
       'src/builtins/builtins.c',
       'src/builtins/history.c',
       'src/arithmetic.c',
       'src/autocorrect.c',
       'src/completion.c',
       'src/config.c',
       'src/debug/debug_core.c',
       'src/debug/debug_trace.c',
       'src/debug/debug_breakpoints.c',
       'src/debug/debug_builtin.c',
       'src/debug/debug_profile.c',
       'src/debug/debug_analysis.c',
       'src/errors.c',
       'src/executor.c',
       'src/expand.c',
       'src/globals.c',
       'src/init.c',
       'src/input.c',
       'src/redirection.c',
       'src/libhashtable/ht.c',
       'src/libhashtable/ht_fnv1a.c',
       'src/libhashtable/ht_strdouble.c',
       'src/libhashtable/ht_strfloat.c',
       'src/libhashtable/ht_strint.c',
       'src/libhashtable/ht_strstr.c',
       'src/linenoise/encodings/utf8.c',
       'src/linenoise/linenoise.c',
       'src/lusush.c',
       'src/node.c',
       'src/opts.c',
       'src/parser.c',
       'src/posix_opts.c',
       'src/network.c',
       'src/prompt.c',
       'src/themes.c',
       'src/signals.c',
       'src/strings.c',
       'src/symtable.c',
       'src/tokenizer.c',
      ]

add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')
add_project_arguments('-D_XOPEN_SOURCE=700', language: 'c')
add_project_arguments('-D_XOPEN_SOURCE_EXTENDED', language: 'c')

executable('lusush',
           src,
           include_directories: inc)
