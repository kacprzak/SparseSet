project('SparseVector', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20'])

gtest = dependency('gtest_main')
gbenchmark = dependency('benchmark')

test('SparseSet',  executable('SparseSet_test',  'SparseSet_test.cpp',  dependencies: gtest))
test('SparseMap',  executable('SparseMap_test',  'SparseMap_test.cpp',  dependencies: gtest))
test('SparseTree', executable('SparseTree_test', 'SparseTree_test.cpp', dependencies: gtest))

benchmark('SparseSet', executable('SparseSet_benchmark', 'SparseSet_benchmark.cpp', dependencies: gbenchmark))
benchmark('SparseTree', executable('SparseTree_benchmark', 'SparseTree_benchmark.cpp', dependencies: gbenchmark))
