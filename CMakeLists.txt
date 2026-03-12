cmake_minimum_required(VERSION 3.32)
project(ark_fbs_python LANGUAGES CXX)

option(ARK_FBS_VENDOR_FLATBUFFERS "Build with vendored flatbuffers source" ON)
set(ARK_FBS_FLATBUFFERS_SOURCE_DIR "" CACHE PATH
    "Path to flatbuffers source dir (alternative to third_party/flatbuffers)")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake >= 3.30 excludes free-threaded Python (gil_disabled) from FindPython by
# default. scikit-build-core fails to pass this flag in Windows cross-compile
# scenarios (e.g. AMD64 host building ARM64 cp314t wheels), so the library is
# looked up as python314.lib instead of python314t.lib. Detect via SKBUILD_SOABI
# (set by scikit-build-core before CMake runs) and opt in explicitly.
if(DEFINED SKBUILD_SOABI)
  string(REGEX MATCH "[0-9]+t[-.]" _is_free_threaded "${SKBUILD_SOABI}")
  if(_is_free_threaded)
    set(Python_FIND_ABI "ANY" "ANY" "ANY" "ON")
  endif()
  unset(_is_free_threaded)
endif()

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

if(ARK_FBS_VENDOR_FLATBUFFERS)
  # Expect the user's fork of flatbuffers as a submodule/subdir.
  # Layout:
  #   third_party/flatbuffers/ (contains include/, src/, CMakeLists.txt, ...)
  set(_fb_src "")
  if(ARK_FBS_FLATBUFFERS_SOURCE_DIR)
    set(_fb_src "${ARK_FBS_FLATBUFFERS_SOURCE_DIR}")
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

  # Relax flatbuffers' strict warnings and ensure PIC for linking into our shared module.
  # Use an interface library so our flags are applied after ProjectConfig (last wins).
  add_library(flatbuffers_warning_relax INTERFACE)
  target_compile_options(flatbuffers_warning_relax INTERFACE
    # Required when linking flatbuffers static lib into _ark_fbs .so
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fPIC>
  )
  if(TARGET flatbuffers)
    target_link_libraries(flatbuffers PRIVATE flatbuffers_warning_relax)
  elseif(TARGET flatbuffers::flatbuffers)
    target_link_libraries(flatbuffers::flatbuffers PRIVATE flatbuffers_warning_relax)
  endif()

  # Provide mandatory nlohmann/json header for our fork's idl_gen_text.cpp.
  # We expect a single-header drop at: third_party/nlohmann_json/nlohmann/json.hpp
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/nlohmann/json.hpp")
    if(TARGET flatbuffers)
      target_include_directories(flatbuffers PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json"
      )
    elseif(TARGET flatbuffers::flatbuffers)
      target_include_directories(flatbuffers::flatbuffers PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json"
      )
    endif()
  else()
    message(FATAL_ERROR
      "Missing nlohmann/json headers.\n"
      "Expected: third_party/nlohmann_json/nlohmann/json.hpp\n"
    )
  endif()
else()
  find_package(Flatbuffers REQUIRED)
endif()

pybind11_add_module(_ark_fbs MODULE
  cpp/flatbuffers_idl.cpp
)

if(TARGET flatbuffers)
  target_link_libraries(_ark_fbs PRIVATE flatbuffers)
elseif(TARGET flatbuffers::flatbuffers)
  target_link_libraries(_ark_fbs PRIVATE flatbuffers::flatbuffers)
endif()

install(TARGETS _ark_fbs
  LIBRARY DESTINATION ark_fbs
  RUNTIME DESTINATION ark_fbs
  ARCHIVE DESTINATION ark_fbs
)

