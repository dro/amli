project('amli', 'c')
src = files(
    'src/aml_arena.c',
    'src/aml_base.c',
    'src/aml_buffer_field.c',
    'src/aml_compare.c',
    'src/aml_conv.c',
    'src/aml_data.c',
    'src/aml_debug.c',
    'src/aml_decoder.c',
    'src/aml_eval.c',
    'src/aml_eval_expression.c',
    'src/aml_eval_named.c',
    'src/aml_eval_namespace.c',
    'src/aml_eval_reference.c',
    'src/aml_eval_statement.c',
    'src/aml_field.c',
    'src/aml_hash.c',
    'src/aml_heap.c',
    'src/aml_host.c',
    'src/aml_method.c',
    'src/aml_mutex.c',
    'src/aml_namespace.c',
    'src/aml_object.c',
    'src/aml_operation_region.c',
    'src/aml_osi.c',
    'src/aml_pci.c',
    'src/aml_state.c',
    'src/aml_state_snapshot.c',
    'src/aml_string_conv.c',
    'src/aml_platform.c'
)
inc = include_directories('src')

# Build runtest example application if we aren't being used as a subproject.
runtest_src = (src + files('examples/runtest/runtest_main.c', 'examples/runtest/runtest_host.c'))
runtest_inc = [inc, include_directories('examples/runtest')]
runtest = executable(
    'runtest',
    runtest_src,
    include_directories: runtest_inc,
    build_by_default: not meson.is_subproject()
)


# Define all test cases.
test('feature test', runtest, args : ['tests/feature_test_1/DSDT.aml'])