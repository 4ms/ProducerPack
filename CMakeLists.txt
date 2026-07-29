cmake_minimum_required(VERSION 3.19)

if(NOT "${METAMODULE_SDK_DIR}" STREQUAL "")
    message("METAMODULE_SDK_DIR set by CMake variable ${METAMODULE_SDK_DIR}")
elseif(DEFINED ENV{METAMODULE_SDK_DIR})
    set(METAMODULE_SDK_DIR "$ENV{METAMODULE_SDK_DIR}")
    message("METAMODULE_SDK_DIR set by environment variable ${METAMODULE_SDK_DIR}")
else()
    set(METAMODULE_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../metamodule-plugin-sdk")
    message("METAMODULE_SDK_DIR set to default: ${METAMODULE_SDK_DIR}")
endif()

include(${METAMODULE_SDK_DIR}/plugin.cmake)

project(ProducerPack VERSION 0.2 DESCRIPTION "4ms-ProducerPack for MetaModule" LANGUAGES C CXX ASM)

add_library(ProducerPack STATIC)

# Add sources from the Rack source repo
set(SOURCE_DIR .)

target_sources(
    ProducerPack
    PRIVATE ${SOURCE_DIR}/src/DJFilter.cpp
            ${SOURCE_DIR}/src/Boost.cpp
            ${SOURCE_DIR}/src/Spatializer.cpp
            ${SOURCE_DIR}/src/StereoWidth.cpp
            ${SOURCE_DIR}/src/StereoCrossfader.cpp
            ${SOURCE_DIR}/src/SeventiesEQ.cpp
            ${SOURCE_DIR}/src/SeventiesComp.cpp
            ${SOURCE_DIR}/src/Decay.cpp
            ${SOURCE_DIR}/src/AuxSends.cpp
            ${SOURCE_DIR}/src/Bitcrusher.cpp
            ${SOURCE_DIR}/src/DrumBus.cpp
            ${SOURCE_DIR}/src/TapeDelay.cpp
            ${SOURCE_DIR}/src/plugin.cpp
            ${SOURCE_DIR}/src/plugin.hpp
            ${SOURCE_DIR}/src/helpers/math_lut.cpp
)

# Add includes and compile options for source repo
target_include_directories(ProducerPack PRIVATE ${SOURCE_DIR}/src)

if("${INSTALL_DIR}" STREQUAL "")
    set(INSTALL_DIR ${CMAKE_CURRENT_LIST_DIR}/metamodule-plugins)
endif()

# cmake-format: off
create_plugin(
    SOURCE_LIB      ProducerPack
    PLUGIN_NAME     4ms-ProducerPack
    PLUGIN_JSON     ${CMAKE_CURRENT_LIST_DIR}/plugin.json
    SOURCE_ASSETS   ${CMAKE_CURRENT_LIST_DIR}/assets
    DESTINATION     ${INSTALL_DIR}
)
# cmake-format: on
