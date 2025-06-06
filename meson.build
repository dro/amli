# AMLI project definition, enable sanitizers by default (for examples/tests).
project(
    'amli',
    'c',
    default_options: [
        'c_std=c11'
    ] 
)

# Full library source file listing.
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
    'src/aml_platform.c',
    'src/aml_resource.c'
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
test('feature test 1', runtest, args : [join_paths(meson.source_root(), 'tests/feature_test_1/DSDT.aml')])
test('board test 1', runtest, args : [join_paths(meson.source_root(), 'tests/board_test_1/DSDT.aml')])
test('board test 2', runtest, args : [join_paths(meson.source_root(), 'tests/board_test_2/DSDT.aml')])
# test('fuzzer crash 1', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-0ba3f0b526f857edb57f80ac0f23fe9b945b0886')])
# test('fuzzer crash 2', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-0f491d86d3e1bec527ac6af613dee3ad46d10b03')])
# test('fuzzer crash 3', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-1b68fe6ef658788c8c8b54db30bc872890e1b2fd')])
# test('fuzzer crash 4', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-1f135e460cec126bfaacfcf0dd050dc101886182')])
# test('fuzzer crash 5', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-204172ee151eff0e8f63aa45813c0d354c7ef37c')])
# test('fuzzer crash 6', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-2964797c99d4210da135ab2b750d3b70b0d3d4e1')])
# test('fuzzer crash 7', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-2ced2c468f6d9cabc75a9f943401cb2272324941')])
# test('fuzzer crash 8', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-381e289d30b398510a7596fc645ef72526f48ea6')])
# test('fuzzer crash 9', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-704dd848546e5b5c3a841e02b4e24d31c85c56ed')])
# test('fuzzer crash 10', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-a21eb80009f850215ecba19048dc4c45c70f03f8')])
# test('fuzzer crash 11', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-c3a1d370314c063a255d9ff554905edf75ccefd2')])
# test('fuzzer crash 12', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-d12a8eda04d34118e0733274a62aed45508ff5e8')])
# test('fuzzer crash 13', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-d9c9cd22d01b20505b59be5ecabf8a8331864443')])
# test('fuzzer crash 14', runtest, args : [join_paths(meson.source_root(), 'tests/fuzzer/crash-f1748aa28504f30af2d722934eb2fd2ce348e61b')])