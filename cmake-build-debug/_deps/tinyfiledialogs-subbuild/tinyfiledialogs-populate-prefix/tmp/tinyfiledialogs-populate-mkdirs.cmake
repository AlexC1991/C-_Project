# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-src"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-build"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-subbuild/tinyfiledialogs-populate-prefix"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-subbuild/tinyfiledialogs-populate-prefix/tmp"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-subbuild/tinyfiledialogs-populate-prefix/src/tinyfiledialogs-populate-stamp"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-subbuild/tinyfiledialogs-populate-prefix/src"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-subbuild/tinyfiledialogs-populate-prefix/src/tinyfiledialogs-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-subbuild/tinyfiledialogs-populate-prefix/src/tinyfiledialogs-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/tinyfiledialogs-subbuild/tinyfiledialogs-populate-prefix/src/tinyfiledialogs-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
