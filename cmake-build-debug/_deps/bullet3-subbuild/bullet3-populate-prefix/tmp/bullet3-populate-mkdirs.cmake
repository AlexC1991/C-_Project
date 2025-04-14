# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-src"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-build"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-subbuild/bullet3-populate-prefix"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-subbuild/bullet3-populate-prefix/tmp"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-subbuild/bullet3-populate-prefix/src/bullet3-populate-stamp"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-subbuild/bullet3-populate-prefix/src"
  "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-subbuild/bullet3-populate-prefix/src/bullet3-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-subbuild/bullet3-populate-prefix/src/bullet3-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/batty/Desktop/C++_Project/cmake-build-debug/_deps/bullet3-subbuild/bullet3-populate-prefix/src/bullet3-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
