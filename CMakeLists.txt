cmake_minimum_required(VERSION 3.20)
project(TotalA C CXX)

set(MSVC5_INCLUDE "${CMAKE_SOURCE_DIR}/toolchain/msvc5-sp3/INCLUDE")
if(NOT EXISTS "${MSVC5_INCLUDE}/WINDOWS.H")
  message(FATAL_ERROR "MSVC 5's headers are missing: ${MSVC5_INCLUDE} (see CONTRIBUTING.md)")
endif()
get_filename_component(MSVC5_INCLUDE "${MSVC5_INCLUDE}" REALPATH)

set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")
set(CMAKE_C_FLAGS_RELEASE "/O2")
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2")
set(CMAKE_BUILD_TYPE Release)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/modern_headers.py" "${MSVC5_INCLUDE}" "${CMAKE_BINARY_DIR}"
  RESULT_VARIABLE HEADERS_RESULT)
if(NOT HEADERS_RESULT EQUAL 0)
  message(FATAL_ERROR "tools/modern_headers.py failed")
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
find_program(UV uv REQUIRED)
execute_process(COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/sources.py" --link-order
  OUTPUT_VARIABLE LINK_ORDER OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE LINK_ORDER_RESULT)
if(NOT LINK_ORDER_RESULT EQUAL 0)
  message(FATAL_ERROR "tools/sources.py --link-order failed")
endif()
string(REPLACE "\n" ";" SOURCES "${LINK_ORDER}")
option(BT_DATA_ORDER "Lay the game's data out in 3.1c's order" OFF)
option(BITEXACT_ROUTE "Route the game logic's CRT calls to lib/bitexact" ON)
if(BITEXACT_ROUTE)
  list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/util/fixed_trig.cpp")
endif()

set(BITEXACT_DIR "${CMAKE_SOURCE_DIR}/lib/bitexact")
set(BITEXACT_SHIM_DIR "${CMAKE_SOURCE_DIR}/cmake/bitexact")
set(BITEXACT_TABLES "${CMAKE_BINARY_DIR}/bitexact/bx_tables.c")
add_custom_command(
  OUTPUT "${BITEXACT_TABLES}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/bitexact"
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/bitexact_tables.py" "${BITEXACT_TABLES}"
  DEPENDS "${CMAKE_SOURCE_DIR}/tools/bitexact_tables.py"
  VERBATIM)
add_library(bitexact STATIC
  "${BITEXACT_DIR}/trig.c" "${BITEXACT_DIR}/math.c" "${BITEXACT_DIR}/strtof.c" "${BITEXACT_DIR}/rand.c" "${BITEXACT_DIR}/qsort.c"
  "${BITEXACT_TABLES}")
if(BITEXACT_ROUTE)
  target_sources(bitexact PRIVATE "${BITEXACT_SHIM_DIR}/trig.c" "${BITEXACT_SHIM_DIR}/sscanf.c")
endif()
target_include_directories(bitexact PRIVATE "${BITEXACT_DIR}")
target_include_directories(bitexact SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(bitexact PRIVATE
  -fms-compatibility -fms-extensions -fms-compatibility-version=11.00 -msse2 -mfpmath=sse /clang:-ffp-contract=off /GS-
  -fno-strict-aliasing -fwrapv -Wno-nonportable-include-path -Wno-microsoft -Wno-comment
  -vfsoverlay "${CMAKE_BINARY_DIR}/overlay.json")

add_library(wasmloader STATIC lib/wasmloader/wasmloader.c lib/wasmloader/json.c lib/wasmloader/format.c)
target_include_directories(wasmloader PUBLIC "${CMAKE_SOURCE_DIR}/lib/wasmloader")
target_include_directories(wasmloader SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/third_party/wasmtime/include")
target_include_directories(wasmloader SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(wasmloader PRIVATE
  -fms-compatibility -fms-extensions -fms-compatibility-version=11.00 -mno-sse /GS-
  -fno-strict-aliasing -fwrapv -Wno-nonportable-include-path -Wno-microsoft
  -vfsoverlay "${CMAKE_BINARY_DIR}/overlay.json")

option(BT_COVERAGE "Build the game with clang source-based coverage" OFF)
option(BT_ACCESS_LOG "Build the game with access-log callbacks for test mode (use build/access)" OFF)
set(COMPILER_RT_DIR "${CMAKE_SOURCE_DIR}/build/compiler-rt" CACHE PATH "compiler-rt checkout for the coverage runtime")

add_library(game OBJECT ${SOURCES})
add_library(data OBJECT link/data.cpp)
set(GAME_COMPILE_OPTIONS
  -fms-compatibility -fms-extensions -fms-compatibility-version=11.00
  -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -Xclang -fno-assume-sane-operator-new -mno-sse
  /Gz /GR- /GS- /Gw -Wno-nonportable-include-path -Wno-c++11-narrowing -Wno-writable-strings -Wno-microsoft -Wno-c++20-extensions
  -vfsoverlay "${CMAKE_BINARY_DIR}/overlay.json")
target_include_directories(game SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/wasmloader" "${CMAKE_SOURCE_DIR}/third_party/wasmtime/include")
foreach(target game data)
  target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/include")
  target_include_directories(${target} SYSTEM PRIVATE "${MSVC5_INCLUDE}")
  target_compile_options(${target} PRIVATE ${GAME_COMPILE_OPTIONS})
endforeach()

if(BT_ACCESS_LOG)
  set(ACCESS_LOG_OPTIONS
    -fsanitize=address -fsanitize-address-use-after-return=never
    "SHELL:/clang:-mllvm /clang:-asan-instrumentation-with-call-threshold=0"
    "SHELL:/clang:-mllvm /clang:-asan-globals=0"
    "SHELL:/clang:-mllvm /clang:-asan-stack=0"
    "SHELL:/clang:-mllvm /clang:-asan-opt=0")
  foreach(target game data)
    target_compile_options(${target} PRIVATE ${ACCESS_LOG_OPTIONS})
  endforeach()
  target_compile_definitions(game PRIVATE BT_ACCESS_LOG=1)
  add_library(accesslog OBJECT lib/accesslog/access_log.cpp)
  target_include_directories(accesslog SYSTEM PRIVATE "${MSVC5_INCLUDE}")
  target_compile_options(accesslog PRIVATE ${GAME_COMPILE_OPTIONS})
  set(ACCESS_LOG_OBJECTS "$<TARGET_OBJECTS:accesslog>")
  set(ACCESS_LOG_TARGETS accesslog)
  set(ACCESS_LOG_ALIAS_FLAGS --access-log)
  set(ACCESS_LOG_TABLES COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/accesslog.py" tables --build-dir "${CMAKE_BINARY_DIR}")
  set(ACCESS_LOG_TABLE_FILES BYPRODUCTS "${CMAKE_BINARY_DIR}/accesslog.attr" "${CMAKE_BINARY_DIR}/accesslog.items")
  set(ACCESS_LOG_TABLE_DEPS "${CMAKE_SOURCE_DIR}/tools/accesslog.py" "${CMAKE_SOURCE_DIR}/data/subsystems.csv" "${CMAKE_SOURCE_DIR}/data/state_classes.csv")
  set(ACCESS_LOG_LINK_FLAGS /nodefaultlib:clang_rt.asan.lib /nodefaultlib:clang_rt.asan_cxx.lib
      /nodefaultlib:clang_rt.asan_dynamic.lib /nodefaultlib:clang_rt.asan_dynamic_runtime_thunk.lib)
else()
  set(ACCESS_LOG_OBJECTS "")
  set(ACCESS_LOG_TARGETS "")
  set(ACCESS_LOG_ALIAS_FLAGS "")
  set(ACCESS_LOG_TABLES "")
  set(ACCESS_LOG_TABLE_FILES "")
  set(ACCESS_LOG_TABLE_DEPS "")
  set(ACCESS_LOG_LINK_FLAGS "")
endif()

if(BT_COVERAGE)
  set(PROFILE_DIR "${COMPILER_RT_DIR}/compiler-rt/lib/profile")
  if(NOT EXISTS "${PROFILE_DIR}/InstrProfiling.c")
    message(FATAL_ERROR "The coverage runtime sources are missing: ${PROFILE_DIR}\nRun tools/coverage_runtime.sh, or set COMPILER_RT_DIR")
  endif()
  set(PROFILE_SOURCES "")
  foreach(name InstrProfiling InstrProfilingBuffer InstrProfilingFile InstrProfilingInternal InstrProfilingMerge InstrProfilingMergeFile
               InstrProfilingNameVar InstrProfilingPlatformWindows InstrProfilingUtil InstrProfilingValue InstrProfilingVersionVar
               InstrProfilingWriter WindowsMMap)
    list(APPEND PROFILE_SOURCES "${PROFILE_DIR}/${name}.c")
  endforeach()
  list(APPEND PROFILE_SOURCES "${PROFILE_DIR}/InstrProfilingRuntime.cpp")
  add_library(clang_rt.profile STATIC ${PROFILE_SOURCES})
  target_include_directories(clang_rt.profile PRIVATE "${CMAKE_SOURCE_DIR}/cmake/coverage" "${COMPILER_RT_DIR}/compiler-rt/include" "${PROFILE_DIR}")
  target_include_directories(clang_rt.profile SYSTEM PRIVATE "${MSVC5_INCLUDE}")
  target_compile_options(clang_rt.profile PRIVATE
    -fms-compatibility -fms-extensions -fms-compatibility-version=11.00 -fno-strict-aliasing -fwrapv -mno-sse
    /GR- /GS- /Gw -w -vfsoverlay "${CMAKE_BINARY_DIR}/overlay.json" "/FI${CMAKE_SOURCE_DIR}/cmake/coverage/prelude.h")
  set_source_files_properties("${PROFILE_DIR}/InstrProfilingRuntime.cpp" PROPERTIES COMPILE_OPTIONS "/GX-")
  foreach(target game data)
    target_compile_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping -fprofile-update=atomic)
  endforeach()
  target_compile_definitions(game PRIVATE BT_COVERAGE=1)
endif()

foreach(source IN LISTS SOURCES)
  file(STRINGS "${source}" flags_line REGEX "^//[ \t]*FLAGS:" LIMIT_COUNT 1)
  file(STRINGS "${source}" try_line REGEX "^[ \t]*try[ \t]*[{]" LIMIT_COUNT 1)
  if(flags_line MATCHES "/GX")
    set_source_files_properties("${source}" PROPERTIES COMPILE_OPTIONS "/EHsc")
  elseif(try_line)
    set_source_files_properties("${source}" PROPERTIES COMPILE_OPTIONS "/EHa")
  endif()
endforeach()

set(CRT_CALLS_CSV "${CMAKE_SOURCE_DIR}/data/crt_calls.csv")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CRT_CALLS_CSV}")
file(STRINGS "${CRT_CALLS_CSV}" CRT_CALL_ROWS)
set(ROUTED_SOURCES "")
foreach(row IN LISTS CRT_CALL_ROWS)
  if(row MATCHES "^0x[0-9a-f]+,[^,]+,(fsqrt|rand|srand|_hypot|_CIacos|sscanf|atof),[a-z]+,([^,]+),game,")
    list(APPEND ROUTED_SOURCES "${CMAKE_SOURCE_DIR}/${CMAKE_MATCH_2}")
  endif()
endforeach()
list(REMOVE_DUPLICATES ROUTED_SOURCES)
if(NOT BITEXACT_ROUTE)
  set(ROUTED_SOURCES "")
endif()
foreach(source IN LISTS ROUTED_SOURCES)
  if(NOT source IN_LIST SOURCES)
    message(FATAL_ERROR "data/crt_calls.csv names ${source}, which is not a source of the build")
  endif()
  set_property(SOURCE "${source}" APPEND PROPERTY COMPILE_OPTIONS "/FI${BITEXACT_SHIM_DIR}/route.h")
endforeach()

set(MSVC5_LIB "${CMAKE_SOURCE_DIR}/toolchain/msvc5-sp3/LIB")
find_program(DLLTOOL llvm-dlltool REQUIRED)
find_program(LLD_LINK lld-link REQUIRED)
find_program(LLVM_RC llvm-rc REQUIRED)

set(IMPORT_LIBS "")
foreach(dll smackw32 dplayx ddraw dsound)
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/${dll}.lib"
    COMMAND "${DLLTOOL}" -m i386 -k -d "${CMAKE_SOURCE_DIR}/link/${dll}.def" -l "${CMAKE_BINARY_DIR}/${dll}.lib"
    DEPENDS "${CMAKE_SOURCE_DIR}/link/${dll}.def")
  list(APPEND IMPORT_LIBS "${CMAKE_BINARY_DIR}/${dll}.lib")
endforeach()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
file(GLOB MSVC5_LIBS "${MSVC5_LIB}/*.LIB")
foreach(lib IN LISTS MSVC5_LIBS)
  get_filename_component(stem "${lib}" NAME_WE)
  string(TOLOWER "${stem}" lower)
  foreach(name "${stem}.lib" "${lower}.lib")
    file(CREATE_LINK "${lib}" "${CMAKE_BINARY_DIR}/lib/${name}" SYMBOLIC)
  endforeach()
endforeach()

set(SYSTEM_LIBS "$<TARGET_FILE:bitexact>")
if(BT_COVERAGE)
  list(APPEND SYSTEM_LIBS "$<TARGET_FILE:clang_rt.profile>")
  set(PROFILE_TARGETS clang_rt.profile)
else()
  set(PROFILE_TARGETS "")
endif()
foreach(lib LIBCMT LIBCPMT OLDNAMES DPLAY)
  list(APPEND SYSTEM_LIBS "${MSVC5_LIB}/${lib}.LIB")
endforeach()
foreach(lib KERNEL32 USER32 GDI32 ADVAPI32 SHELL32 WINMM)
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/${lib}.def" "${CMAKE_BINARY_DIR}/${lib}.lib"
    COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/aliases.py"
            --import-def "${MSVC5_LIB}/${lib}.LIB" --out "${CMAKE_BINARY_DIR}/${lib}.def"
    COMMAND "${DLLTOOL}" -m i386 -k -d "${CMAKE_BINARY_DIR}/${lib}.def" -l "${CMAKE_BINARY_DIR}/${lib}.lib"
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/aliases.py")
  list(APPEND IMPORT_LIBS "${CMAKE_BINARY_DIR}/${lib}.lib")
endforeach()

set(ZLIB_DIR "${CMAKE_SOURCE_DIR}/toolchain/thirdparty/zlib-1.0.4")
set(ZLIB_SOURCES "")
foreach(name adler32 compress crc32 deflate gzio infblock infcodes inffast inflate inftrees infutil trees uncompr zutil)
  if(EXISTS "${ZLIB_DIR}/${name}.c")
    list(APPEND ZLIB_SOURCES "${ZLIB_DIR}/${name}.c")
  endif()
endforeach()
set(ZLIB_OBJECTS "")
set(ZLIB_TARGETS "")
if(ZLIB_SOURCES)
  add_library(zlib OBJECT ${ZLIB_SOURCES})
  target_include_directories(zlib SYSTEM PRIVATE "${MSVC5_INCLUDE}")
  target_compile_options(zlib PRIVATE
    -fms-compatibility -fms-extensions -fms-compatibility-version=11.00 -fno-strict-aliasing -fwrapv -mno-sse
    /Gz /Zp1 /Gy /GS- -w -vfsoverlay "${CMAKE_BINARY_DIR}/overlay.json")
  set(ZLIB_OBJECTS "$<TARGET_OBJECTS:zlib>")
  set(ZLIB_TARGETS zlib)
endif()

option(BT_COMPILE_ONLY "Configure for building the game and bitexact targets only (no orig/TotalA.exe)" OFF)
if(BT_COMPILE_ONLY)
  set(ART_DIR "")
else()
  execute_process(COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/resources.py" --print-art --art-out "${CMAKE_BINARY_DIR}/art"
    OUTPUT_VARIABLE ART_DIR OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE ART_RESULT)
  if(NOT ART_RESULT EQUAL 0)
    message(FATAL_ERROR "tools/resources.py --print-art failed")
  endif()
endif()
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/TotalA.res"
  COMMAND "${LLVM_RC}" /C 1252 /I "${ART_DIR}" /FO "${CMAKE_BINARY_DIR}/TotalA.res" "${CMAKE_SOURCE_DIR}/src/res/TotalA.rc"
  DEPENDS "${CMAKE_SOURCE_DIR}/src/res/TotalA.rc")

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/objects.rsp" CONTENT "$<JOIN:$<TARGET_OBJECTS:game>;$<TARGET_OBJECTS:data>,\n>\n")
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/extra_data.cpp"
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/aliases.py"
          --extra-data --objects "@${CMAKE_BINARY_DIR}/objects.rsp" --out "${CMAKE_BINARY_DIR}/extra_data.cpp"
  DEPENDS game data $<TARGET_OBJECTS:game> $<TARGET_OBJECTS:data> "${CMAKE_SOURCE_DIR}/tools/aliases.py"
          "${CMAKE_SOURCE_DIR}/data/globals.csv")
add_library(extra OBJECT "${CMAKE_BINARY_DIR}/extra_data.cpp")
target_compile_options(extra PRIVATE ${GAME_COMPILE_OPTIONS})
target_include_directories(extra SYSTEM PRIVATE "${MSVC5_INCLUDE}")

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/alias_objects.rsp"
  CONTENT "$<JOIN:$<TARGET_OBJECTS:game>;$<TARGET_OBJECTS:data>;$<TARGET_OBJECTS:extra>,\n>\n")
if(BT_DATA_ORDER)
  set(DATA_ORDER_FLAG "")
else()
  set(DATA_ORDER_FLAG --no-data-order)
endif()
if(BT_COVERAGE)
  set(LIVE_COUNTERS_OUTPUT "${CMAKE_BINARY_DIR}/live_counters.txt")
  set(LIVE_COUNTERS_FLAG --live-counters "${CMAKE_BINARY_DIR}/live_counters.txt")
else()
  set(LIVE_COUNTERS_OUTPUT "")
  set(LIVE_COUNTERS_FLAG "")
endif()
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/aliases.obj" "${CMAKE_BINARY_DIR}/objs/objects.rsp" ${LIVE_COUNTERS_OUTPUT}
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/aliases.py"
          --objects "@${CMAKE_BINARY_DIR}/alias_objects.rsp" --libs ${SYSTEM_LIBS} ${IMPORT_LIBS} ${ZLIB_OBJECTS} ${ACCESS_LOG_OBJECTS}
          --winmain 0x49e830 ${DATA_ORDER_FLAG} ${ACCESS_LOG_ALIAS_FLAGS} --patch-dir "${CMAKE_BINARY_DIR}/objs" ${LIVE_COUNTERS_FLAG} --out "${CMAKE_BINARY_DIR}/aliases.obj"
  DEPENDS game data extra bitexact ${PROFILE_TARGETS} ${ZLIB_TARGETS} ${ACCESS_LOG_TARGETS} ${ACCESS_LOG_OBJECTS} $<TARGET_OBJECTS:game> $<TARGET_OBJECTS:data> $<TARGET_OBJECTS:extra> ${IMPORT_LIBS}
          "${CMAKE_SOURCE_DIR}/tools/aliases.py"
  VERBATIM COMMAND_EXPAND_LISTS)

add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/TotalA.exe"
  COMMAND "${LLD_LINK}" /nologo /machine:x86 /subsystem:windows,4.0 /base:0x400000 /fixed /dynamicbase:no /largeaddressaware:no /errorlimit:0 /entry:WinMainCRTStartup /include:_WinMainCRTStartup /out:${CMAKE_BINARY_DIR}/TotalA.exe
          /libpath:${CMAKE_BINARY_DIR}/lib /opt:noref,noicf /incremental:no /brepro /safeseh:no /force:multiple ${ACCESS_LOG_LINK_FLAGS} /map:${CMAKE_BINARY_DIR}/TotalA.map
          "@${CMAKE_BINARY_DIR}/objs/objects.rsp" "${CMAKE_BINARY_DIR}/aliases.obj" "${CMAKE_BINARY_DIR}/TotalA.res"
          ${ZLIB_OBJECTS} ${ACCESS_LOG_OBJECTS} ${IMPORT_LIBS} ${SYSTEM_LIBS} $<TARGET_FILE:wasmloader>
  ${ACCESS_LOG_TABLES}
  ${ACCESS_LOG_TABLE_FILES}
  DEPENDS game data extra wasmloader ${PROFILE_TARGETS} ${ZLIB_TARGETS} ${ACCESS_LOG_TARGETS} ${ACCESS_LOG_OBJECTS} $<TARGET_OBJECTS:game> $<TARGET_OBJECTS:data> $<TARGET_OBJECTS:extra> "${CMAKE_BINARY_DIR}/aliases.obj" "${CMAKE_BINARY_DIR}/TotalA.res" ${IMPORT_LIBS}
  ${ACCESS_LOG_TABLE_DEPS}
  VERBATIM COMMAND_EXPAND_LISTS)
add_custom_target(TotalA ALL DEPENDS "${CMAKE_BINARY_DIR}/TotalA.exe")

add_library(block_test_objects OBJECT src/state/block.cpp src/host/win32/memory.cpp src/host/win32/threads.cpp tests/block_test.cpp)
target_include_directories(block_test_objects SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(block_test_objects PRIVATE ${GAME_COMPILE_OPTIONS})
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/block_test.exe"
  COMMAND "${LLD_LINK}" /nologo /machine:x86 /subsystem:console /out:${CMAKE_BINARY_DIR}/block_test.exe /libpath:${CMAKE_BINARY_DIR}/lib /incremental:no /safeseh:no
          $<TARGET_OBJECTS:block_test_objects> "${CMAKE_BINARY_DIR}/KERNEL32.lib" "${MSVC5_LIB}/LIBCMT.LIB" "${MSVC5_LIB}/OLDNAMES.LIB"
  DEPENDS block_test_objects $<TARGET_OBJECTS:block_test_objects> "${CMAKE_BINARY_DIR}/KERNEL32.lib"
  VERBATIM COMMAND_EXPAND_LISTS)
add_custom_target(block_test DEPENDS "${CMAKE_BINARY_DIR}/block_test.exe")

add_library(replay_test_objects OBJECT src/harness/replay_text.cpp tests/replay_test.cpp)
target_include_directories(replay_test_objects SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(replay_test_objects PRIVATE ${GAME_COMPILE_OPTIONS})
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/replay_test.exe"
  COMMAND "${LLD_LINK}" /nologo /machine:x86 /subsystem:console /out:${CMAKE_BINARY_DIR}/replay_test.exe /libpath:${CMAKE_BINARY_DIR}/lib /incremental:no /safeseh:no
          $<TARGET_OBJECTS:replay_test_objects> "${CMAKE_BINARY_DIR}/KERNEL32.lib" "${MSVC5_LIB}/LIBCMT.LIB" "${MSVC5_LIB}/OLDNAMES.LIB"
  DEPENDS replay_test_objects $<TARGET_OBJECTS:replay_test_objects> "${CMAKE_BINARY_DIR}/KERNEL32.lib"
  VERBATIM COMMAND_EXPAND_LISTS)
add_custom_target(replay_test DEPENDS "${CMAKE_BINARY_DIR}/replay_test.exe")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/replay_subsystems.json")
file(READ "${CMAKE_SOURCE_DIR}/tests/replay_subsystems.json" REPLAY_SUBSYSTEMS_JSON)
set(REPLAY_SUBSYSTEMS)
set(REPLAY_WASM_INJECT_COMMANDS)
string(JSON replay_count LENGTH "${REPLAY_SUBSYSTEMS_JSON}")
math(EXPR replay_last "${replay_count} - 1")
foreach(index RANGE ${replay_last})
  string(JSON subsystem MEMBER "${REPLAY_SUBSYSTEMS_JSON}" ${index})
  list(APPEND REPLAY_SUBSYSTEMS ${subsystem})
  string(JSON REPLAY_INJECT_${subsystem} GET "${REPLAY_SUBSYSTEMS_JSON}" ${subsystem} inject define)
  string(JSON wasm_variant ERROR_VARIABLE wasm_missing GET "${REPLAY_SUBSYSTEMS_JSON}" ${subsystem} wasm inject_variant)
  if(NOT wasm_missing)
    string(JSON wasm_define GET "${REPLAY_SUBSYSTEMS_JSON}" ${subsystem} wasm inject_define)
    list(APPEND REPLAY_WASM_INJECT_COMMANDS COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/wasm_modules.py" build-variant --define ${wasm_define} --name ${wasm_variant}
         COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/build/wasm-modules/${wasm_variant}.wasm" "${CMAKE_BINARY_DIR}/${wasm_variant}.wasm")
  endif()
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/data/subsystems.csv")
set(REPLAY_MODULE_SOURCES)
set(REPLAY_INJECT_DEFINITIONS)
foreach(subsystem IN LISTS REPLAY_SUBSYSTEMS)
  file(STRINGS "${CMAKE_SOURCE_DIR}/data/subsystems.csv" module_rows REGEX "^[^,]*,[^,]*,${subsystem},")
  foreach(row IN LISTS module_rows)
    string(REGEX REPLACE "^[^,]*,([^,]*),.*$" "\\1" module_source "${row}")
    list(APPEND REPLAY_MODULE_SOURCES "${module_source}")
  endforeach()
  list(APPEND REPLAY_INJECT_DEFINITIONS ${REPLAY_INJECT_${subsystem}})
endforeach()
list(REMOVE_DUPLICATES REPLAY_MODULE_SOURCES)
foreach(variant native inject)
  set(module_target replay_${variant}_modules)
  add_library(${module_target} OBJECT ${REPLAY_MODULE_SOURCES})
  target_include_directories(${module_target} PRIVATE "${CMAKE_SOURCE_DIR}/include")
  target_include_directories(${module_target} SYSTEM PRIVATE "${MSVC5_INCLUDE}" "${CMAKE_SOURCE_DIR}/lib/wasmloader" "${CMAKE_SOURCE_DIR}/third_party/wasmtime/include")
  target_compile_options(${module_target} PRIVATE ${GAME_COMPILE_OPTIONS} "/FI${BITEXACT_SHIM_DIR}/route.h")
  if(variant STREQUAL "inject")
    target_compile_definitions(${module_target} PRIVATE ${REPLAY_INJECT_DEFINITIONS})
  endif()
  set(REPLAY_${variant}_OBJECTS "$<TARGET_OBJECTS:${module_target}>")
  set(REPLAY_${variant}_TARGETS ${module_target})
endforeach()
add_library(replay_native_test_objects OBJECT src/state/block.cpp src/host/win32/memory.cpp src/host/win32/threads.cpp src/host/win32/library.cpp src/host/win32/time.cpp src/wasm/wind_meteors_glue.cpp src/harness/replay_text.cpp tests/replay_objects_fixtures.cpp tests/replay_vtables_fixtures.cpp src/data/vtables.cpp tests/replay_wasm.cpp tests/replay_native_test.cpp)
target_include_directories(replay_native_test_objects PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(replay_native_test_objects SYSTEM PRIVATE "${MSVC5_INCLUDE}" "${CMAKE_SOURCE_DIR}/lib/wasmloader" "${CMAKE_SOURCE_DIR}/third_party/wasmtime/include")
target_compile_options(replay_native_test_objects PRIVATE ${GAME_COMPILE_OPTIONS})
foreach(variant native inject)
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/replay_${variant}_test.exe"
    COMMAND "${LLD_LINK}" /nologo /machine:x86 /subsystem:console /out:${CMAKE_BINARY_DIR}/replay_${variant}_test.exe /libpath:${CMAKE_BINARY_DIR}/lib /incremental:no /safeseh:no /opt:ref /force:unresolved
            $<TARGET_OBJECTS:replay_native_test_objects> ${REPLAY_${variant}_OBJECTS} $<TARGET_FILE:bitexact> $<TARGET_FILE:wasmloader> "${CMAKE_BINARY_DIR}/KERNEL32.lib" "${MSVC5_LIB}/LIBCMT.LIB" "${MSVC5_LIB}/OLDNAMES.LIB"
    DEPENDS replay_native_test_objects ${REPLAY_${variant}_TARGETS} bitexact wasmloader $<TARGET_OBJECTS:replay_native_test_objects> ${REPLAY_${variant}_OBJECTS} "${CMAKE_BINARY_DIR}/KERNEL32.lib"
    VERBATIM COMMAND_EXPAND_LISTS)
endforeach()
add_custom_target(replay_native_test DEPENDS "${CMAKE_BINARY_DIR}/replay_native_test.exe" "${CMAKE_BINARY_DIR}/replay_inject_test.exe")
add_library(replay_fixture_recorder_objects OBJECT tests/replay_fixture_recorder.cpp)
target_include_directories(replay_fixture_recorder_objects PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(replay_fixture_recorder_objects SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(replay_fixture_recorder_objects PRIVATE ${GAME_COMPILE_OPTIONS})
add_dependencies(replay_native_test replay_fixture_recorder_objects)

add_library(purecall_test_objects OBJECT src/harness/purecall.cpp tests/purecall_test.cpp)
target_include_directories(purecall_test_objects SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(purecall_test_objects PRIVATE ${GAME_COMPILE_OPTIONS})
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/purecall_test.exe"
  COMMAND "${LLD_LINK}" /nologo /machine:x86 /subsystem:console /out:${CMAKE_BINARY_DIR}/purecall_test.exe /libpath:${CMAKE_BINARY_DIR}/lib /incremental:no /safeseh:no /map:${CMAKE_BINARY_DIR}/purecall_test.map
          $<TARGET_OBJECTS:purecall_test_objects> "${CMAKE_BINARY_DIR}/KERNEL32.lib" "${MSVC5_LIB}/LIBCMT.LIB" "${MSVC5_LIB}/OLDNAMES.LIB"
  DEPENDS purecall_test_objects $<TARGET_OBJECTS:purecall_test_objects> "${CMAKE_BINARY_DIR}/KERNEL32.lib"
  VERBATIM COMMAND_EXPAND_LISTS)
add_custom_target(purecall_test DEPENDS "${CMAKE_BINARY_DIR}/purecall_test.exe")

add_library(ref_test_objects OBJECT src/state/block.cpp src/host/win32/memory.cpp src/host/win32/threads.cpp tests/ref_test.cpp)
target_include_directories(ref_test_objects SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(ref_test_objects PRIVATE ${GAME_COMPILE_OPTIONS})
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/ref_test.exe"
  COMMAND "${LLD_LINK}" /nologo /machine:x86 /subsystem:console /out:${CMAKE_BINARY_DIR}/ref_test.exe /libpath:${CMAKE_BINARY_DIR}/lib /incremental:no /safeseh:no
          $<TARGET_OBJECTS:ref_test_objects> "${CMAKE_BINARY_DIR}/KERNEL32.lib" "${MSVC5_LIB}/LIBCMT.LIB" "${MSVC5_LIB}/OLDNAMES.LIB"
  DEPENDS ref_test_objects $<TARGET_OBJECTS:ref_test_objects> "${CMAKE_BINARY_DIR}/KERNEL32.lib"
  VERBATIM COMMAND_EXPAND_LISTS)
add_custom_target(ref_test DEPENDS "${CMAKE_BINARY_DIR}/ref_test.exe")

add_custom_target(wasm_modules
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/wasm_modules.py" build
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  USES_TERMINAL)

option(BT_WASM_HOST_DEBUG "Build wasmtime_host.dll unstripped with panic=unwind (build/wasm_host_debug)" OFF)
if(BT_WASM_HOST_DEBUG)
  set(WASM_HOST_ARGS --debug)
  set(WASM_HOST_DIR "${CMAKE_SOURCE_DIR}/build/wasm_host_debug")
else()
  set(WASM_HOST_ARGS)
  set(WASM_HOST_DIR "${CMAKE_SOURCE_DIR}/build/wasm_host")
endif()

add_custom_target(wasm_runtime
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/wasm_host.py" build ${WASM_HOST_ARGS}
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/wasm_modules.py" build
  COMMAND "${CMAKE_COMMAND}" -E copy "${WASM_HOST_DIR}/wasmtime_host.dll" "${CMAKE_BINARY_DIR}/wasmtime_host.dll"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/build/wasm-modules/wind_meteors.wasm" "${CMAKE_BINARY_DIR}/wind_meteors.wasm"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/wasm/modules.json" "${CMAKE_BINARY_DIR}/modules.json"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  USES_TERMINAL)

add_custom_target(wasm_runtime_inject
  ${REPLAY_WASM_INJECT_COMMANDS}
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  USES_TERMINAL)
