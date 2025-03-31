# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/external)
# execute_process(COMMAND git checkout "030c995" WORKING_DIRECTORY ${EXTERNAL_DIR}/esp-idf)
# execute_process(COMMAND git submodule update --init --recursive WORKING_DIRECTORY ${EXTERNAL_DIR})

# set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/components/tinyusb/hw/bsp/espressif/components/tinyusb_src)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# idf_build_set_property(COMPILE_DEFINITIONS "MAX3421_DEVICE=1" APPEND)
project("BlueRetro$ENV{BR_HW}$ENV{BR_SYS}")
