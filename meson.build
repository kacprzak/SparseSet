project('SparseVector', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20'])

gtest = dependency('gtest_main')

sparse_set_test = executable('SparseSet_test', 'SparseSet_test.cpp', dependencies: gtest)

test('SparseSet', sparse_set_test)
