project('SparseVector', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3'])

gtest = dependency('gtest_main')

exe = executable('SparseVector', 'main.cpp', dependencies: [gtest],
  install : true)

test('basic', exe)
