# Development fallback: use official OBS source headers with an installed runtime.
set(OBS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/reference/obs-studio" CACHE PATH "OBS Studio source checkout")
set(OBS_RUNTIME_DIR "C:/Program Files/obs-studio/bin/64bit" CACHE PATH "Installed OBS runtime")
if(NOT EXISTS "${OBS_SOURCE_DIR}/libobs/obs-module.h" OR NOT EXISTS "${OBS_RUNTIME_DIR}/obs.dll")
  message(FATAL_ERROR "Provide libobs SDK via CMAKE_PREFIX_PATH, or OBS_SOURCE_DIR and OBS_RUNTIME_DIR. See README.md")
endif()
get_filename_component(msvc_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
find_program(DUMPBIN_EXECUTABLE dumpbin HINTS "${msvc_bin}" REQUIRED)
find_program(LIB_EXECUTABLE lib HINTS "${msvc_bin}" REQUIRED)
execute_process(COMMAND "${DUMPBIN_EXECUTABLE}" /nologo /exports "${OBS_RUNTIME_DIR}/obs.dll"
  OUTPUT_VARIABLE exports COMMAND_ERROR_IS_FATAL ANY)
string(REGEX MATCHALL "[\r\n] +[0-9]+ +[0-9A-F]+ +[0-9A-F]+ +[^\r\n ]+" entries "${exports}")
set(definition "LIBRARY obs.dll\nEXPORTS\n")
foreach(entry IN LISTS entries)
  string(REGEX REPLACE ".* +([^ ]+)$" "\\1" symbol "${entry}")
  string(APPEND definition "${symbol}\n")
endforeach()
file(WRITE "${CMAKE_BINARY_DIR}/obs.def" "${definition}")
if(NOT EXISTS "${CMAKE_BINARY_DIR}/libobs-import.lib")
  execute_process(COMMAND "${LIB_EXECUTABLE}" /nologo /machine:x64 "/def:${CMAKE_BINARY_DIR}/obs.def" "/out:${CMAKE_BINARY_DIR}/libobs-import.lib"
    COMMAND_ERROR_IS_FATAL ANY OUTPUT_QUIET)
endif()
add_library(OBS::libobs SHARED IMPORTED)
file(WRITE "${CMAKE_BINARY_DIR}/obsconfig.h" "#pragma once\n#define OBS_RELEASE_CANDIDATE 0\n#define OBS_BETA 0\n#define OBS_DATA_PATH \"data\"\n#define OBS_INSTALL_PREFIX \"\"\n")
set_target_properties(OBS::libobs PROPERTIES
  IMPORTED_IMPLIB "${CMAKE_BINARY_DIR}/libobs-import.lib"
  IMPORTED_LOCATION "${OBS_RUNTIME_DIR}/obs.dll"
  INTERFACE_INCLUDE_DIRECTORIES "${OBS_SOURCE_DIR}/libobs;${CMAKE_BINARY_DIR}")
