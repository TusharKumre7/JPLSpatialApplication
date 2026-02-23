cmake_minimum_required(VERSION 3.28)

if (NOT DEFINED MINIAUDIOCPP_SRC_DIR)
  message(FATAL_ERROR "MINIAUDIOCPP_SRC_DIR not set")
endif()

set(MINIAUDIOCPP_ROOT "${MINIAUDIOCPP_SRC_DIR}")

# ---- MiniaudioCpp static library
file(GLOB_RECURSE MINIAUDIOCPP_SOURCES
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/src/**.h"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/src/**.cpp"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/include/**.h"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/miniaudio/miniaudio.h"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/dr_libs/**.h"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/c89atomic/**.h"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/choc/**.h"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/miniaudio/extras/stb_vorbis.c"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/miniaudio/miniuaudio.h"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/c89atomic.h"
)

# Exclude files that clash with JPLSpatial library
# if (TARGET JPLSpatial)
    # list(FILTER MINIAUDIOCPP_SOURCES EXCLUDE REGEX ".*Core.h$|.*ErrorReporting.h$|.*ErrorReporting.cpp$")
# endif()

add_library(MiniaudioCpp STATIC ${MINIAUDIOCPP_SOURCES})
add_library(JPL::MiniaudioCpp ALIAS MiniaudioCpp)

source_group(TREE "${MINIAUDIOCPP_ROOT}" FILES ${MINIAUDIOCPP_SOURCES})

# C++ version
target_compile_features(MiniaudioCpp PUBLIC cxx_std_20)

target_include_directories(MiniaudioCpp PRIVATE
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/include/MiniaudioCpp"
)


target_include_directories(MiniaudioCpp PUBLIC
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/include"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/c89atomic"
    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/src/MiniAudioInterface"

    "${MINIAUDIOCPP_ROOT}/MiniaudioCpp/vendor/choc"
)
