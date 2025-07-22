# LLE Test Configuration

lle_test_sources = [
  'test_text_buffer.c',
  'test_cursor_math.c',
  'test_terminal_manager.c',
  'test_termcap_integration.c',
  'test_lle_010_integration.c',
  'test_lle_011_terminal_output.c',
  'test_lle_015_prompt_structure.c',
  'test_lle_016_prompt_parsing.c',
  'test_lle_017_prompt_rendering.c',
  'test_lle_018_multiline_input_display.c',
  'test_lle_019_theme_interface.c',
  'test_lle_020_basic_theme_application.c',
  'test_lle_021_key_input_handling.c',
  'test_lle_022_key_event_processing.c',
  'test_lle_023_basic_editing_commands.c',
  'test_lle_024_history_structure.c',
  'test_lle_025_history_management.c',
  'test_lle_026_history_navigation.c',
  'test_lle_027_utf8_text_handling.c',
  'test_lle_028_unicode_cursor_movement.c',
  'test_lle_hist_no_dups.c',
  'test_lle_029_completion_framework.c',
  'test_lle_030_basic_file_completion.c',
  'test_lle_031_completion_display.c'
]

# Individual component tests
foreach test_src : lle_test_sources
  test_name = test_src.split('.')[0]
  test_exe = executable(test_name,
                       test_src,
                       dependencies: [lle_dep],
                       include_directories: inc,
                       install: false)
  test(test_name, test_exe)
endforeach

# Performance benchmark (disabled until benchmark.c is implemented)
# benchmark_exe = executable('lle_benchmark',
#                           'benchmark.c',
#                           dependencies: [lle_dep],
#                           include_directories: inc,
#                           c_args: ['-O2'],
#                           install: false)
