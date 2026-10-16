# Matchball Tennis - Kern-Tests ohne Unreal
#
# Baut alle Dateien aus Source/MatchballTennis/*/Kern/ zusammen mit den Tests aus
# Source/MatchballTennisTests/Kern/ zu einem eigenstaendigen Testprogramm.
# Laeuft unter Windows (Visual Studio / MSVC) und Linux (GCC/Clang), z. B. in der Cloud-Sitzung.
#
#   cmake -S Unreal/Tools/KernTests -B Unreal/Tools/KernTests/build
#   cmake --build Unreal/Tools/KernTests/build --config Release
#   ctest --test-dir Unreal/Tools/KernTests/build -C Release --output-on-failure

cmake_minimum_required(VERSION 3.16)
project(MatchballKernTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(MB_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../Source" ABSOLUTE)
get_filename_component(MB_DATEN "${CMAKE_CURRENT_SOURCE_DIR}/../../Data" ABSOLUTE)

file(GLOB MB_KERN_QUELLEN CONFIGURE_DEPENDS "${MB_SOURCE}/MatchballTennis/*/Kern/*.cpp")
file(GLOB MB_TEST_QUELLEN CONFIGURE_DEPENDS "${MB_SOURCE}/MatchballTennisTests/Kern/*.cpp")

add_executable(MatchballKernTests ${MB_KERN_QUELLEN} ${MB_TEST_QUELLEN} "${CMAKE_CURRENT_SOURCE_DIR}/KernTestMain.cpp")
target_include_directories(MatchballKernTests PRIVATE "${MB_SOURCE}/MatchballTennis" "${MB_SOURCE}/MatchballTennisTests")
target_compile_definitions(MatchballKernTests PRIVATE MB_KERN_EIGENSTAENDIG=1 MB_DATEN_PFAD="${MB_DATEN}")

if(MSVC)
  # Wie Unreal: hohe Warnstufe, UTF-8-Quellen, keine Ausnahmen noetig
  target_compile_options(MatchballKernTests PRIVATE /W4 /WX /utf-8 /permissive-)
else()
  target_compile_options(MatchballKernTests PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion -Werror)
endif()

enable_testing()
add_test(NAME Kern COMMAND MatchballKernTests)
