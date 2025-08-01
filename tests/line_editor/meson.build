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
  'test_lle_031_completion_display.c',
  'test_lle_032_undo_stack_structure.c',
  'test_lle_033_undo_operation_recording.c',
  'test_lle_034_undo_redo_execution.c',
  'test_lle_035_syntax_highlighting_framework.c',
  'test_lle_036_basic_shell_syntax.c',
  'test_lle_037_syntax_display_integration.c',
  'test_phase_2b3_syntax_integration.c',
  'test_phase_2b4_history_integration.c',
  'test_phase_2b5_keybinding_integration.c',
  'test_phase_2c_performance_optimization.c',
  'test_phase_2d_final_integration.c',
  'test_lle_038_core_line_editor_api.c',
  'test_lle_039_line_editor_implementation.c',
  'test_lle_040_input_event_loop.c',
  'test_lle_042_theme_system_integration.c',
  'test_lle_043_configuration_integration.c',
  'test_lle_046_comprehensive_integration.c',
  'test_display_cursor_apis.c',
  'test_lle_phase3_search.c',
  'test_multiline_architecture_rewrite.c',
  'test_backspace_enhancement.c',
  'test_backspace_logic.c',
  'test_boundary_crossing_fix.c',
  'test_display_stabilization.c',
  'test_history_navigation_fix.c'
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

# Phase 2C Performance benchmark
benchmark_phase_2c_exe = executable('benchmark_phase_2c',
                                   'benchmark_phase_2c.c',
                                   dependencies: [lle_dep],
                                   include_directories: inc,
                                   c_args: ['-O2'],
                                   install: false)

# Phase 2D Final Integration benchmark
benchmark_phase_2d_exe = executable('benchmark_phase_2d',
                                   'benchmark_phase_2d.c',
                                   dependencies: [lle_dep],
                                   include_directories: inc,
                                   c_args: ['-O2'],
                                   install: false)
