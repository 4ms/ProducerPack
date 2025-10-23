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

project(4ms-ProducerPack VERSION 0.1 DESCRIPTION "4ms-ProducerPack for MetaModule" LANGUAGES C CXX ASM)

add_library(4ms-ProducerPack STATIC)

# Add sources from the Rack source repo
set(SOURCE_DIR .)

target_sources(4ms-ProducerPack PRIVATE
    ${SOURCE_DIR}/src/DJFilter.cpp                    # Note: this file contains init(rack::Plugin*
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
    ${SOURCE_DIR}/src/plugin.cpp        
    ${SOURCE_DIR}/src/plugin.hpp        
    ) 

# Add includes and compile options for source repo
target_include_directories(4ms-ProducerPack PRIVATE ${SOURCE_DIR}/src)

# Call this to link and create the plugin file
# cmake-format: off
create_plugin(
    SOURCE_LIB      4ms-ProducerPack
    PLUGIN_NAME     4ms-ProducerPack
    PLUGIN_JSON     ${CMAKE_CURRENT_LIST_DIR}/plugin.json
    SOURCE_ASSETS   ${CMAKE_CURRENT_LIST_DIR}/assets
    DESTINATION     ${CMAKE_CURRENT_LIST_DIR}/metamodule-plugins
)
# cmake-format: on
