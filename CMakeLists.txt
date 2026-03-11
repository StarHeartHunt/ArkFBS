cmake_minimum_required(VERSION 3.15)
project(flatbuffers_idl_python LANGUAGES CXX)

option(FLATBUFFERS_IDL_VENDOR_FLATBUFFERS "Build with vendored flatbuffers source" ON)
set(FLATBUFFERS_IDL_FLATBUFFERS_SOURCE_DIR "" CACHE PATH
    "Path to flatbuffers source dir (alternative to third_party/flatbuffers)")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 CONFIG REQUIRED)

if(FLATBUFFERS_IDL_VENDOR_FLATBUFFERS)
  # Expect the user's fork of flatbuffers as a submodule/subdir.
  # Layout:
  #   third_party/flatbuffers/ (contains include/, src/, CMakeLists.txt, ...)
  set(_fb_src "")
  if(FLATBUFFERS_IDL_FLATBUFFERS_SOURCE_DIR)
    set(_fb_src "${FLATBUFFERS_IDL_FLATBUFFERS_SOURCE_DIR}")
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/flatbuffers/CMakeLists.txt")
    set(_fb_src "${CMAKE_CURRENT_SOURCE_DIR}/third_party/flatbuffers")
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt")
    # Dev convenience when this package lives inside a flatbuffers checkout.
    set(_fb_src "${CMAKE_CURRENT_SOURCE_DIR}/..")
  endif()

  if(NOT _fb_src OR NOT EXISTS "${_fb_src}/CMakeLists.txt")
    message(FATAL_ERROR
      "Could not find flatbuffers sources.\n"
      "Set FLATBUFFERS_IDL_FLATBUFFERS_SOURCE_DIR or add third_party/flatbuffers submodule.")
  endif()

  set(FLATBUFFERS_BUILD_FLATLIB ON CACHE BOOL "" FORCE)
  set(FLATBUFFERS_BUILD_FLATC OFF CACHE BOOL "" FORCE)
  set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(FLATBUFFERS_BUILD_SHAREDLIB OFF CACHE BOOL "" FORCE)
  set(FLATBUFFERS_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory("${_fb_src}" flatbuffers-build)

  # Provide mandatory nlohmann/json header for our fork's idl_gen_text.cpp.
  # We expect it as a git submodule at third_party/nlohmann_json.
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include/nlohmann/json.hpp")
    if(TARGET flatbuffers)
      target_include_directories(flatbuffers PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include"
      )
    elseif(TARGET flatbuffers::flatbuffers)
      target_include_directories(flatbuffers::flatbuffers PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/single_include"
      )
    endif()
  else()
    message(FATAL_ERROR
      "Missing third_party/nlohmann_json submodule.\n"
      "Run: git submodule add https://github.com/nlohmann/json.git third_party/nlohmann_json\n"
      "and ensure submodules are checked out with --recursive.")
  endif()
else()
  find_package(Flatbuffers REQUIRED)
endif()

pybind11_add_module(_flatbuffers_idl MODULE
  cpp/flatbuffers_idl.cpp
)

if(TARGET flatbuffers)
  target_link_libraries(_flatbuffers_idl PRIVATE flatbuffers)
elseif(TARGET flatbuffers::flatbuffers)
  target_link_libraries(_flatbuffers_idl PRIVATE flatbuffers::flatbuffers)
endif()

install(TARGETS _flatbuffers_idl
  LIBRARY DESTINATION flatbuffers_idl
  RUNTIME DESTINATION flatbuffers_idl
  ARCHIVE DESTINATION flatbuffers_idl
)

