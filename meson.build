project('SparseVector', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20'])

gtest = dependency('gtest_main')

exe = executable('SparseSet_test', 'main.cpp', dependencies: [gtest],
  install : true)

test('basic', exe)
