
cmake_minimum_required(VERSION 2.8)
project(snort CXX C)

set(SNORT_VERSION_MAJOR 2)
set(SNORT_VERSION_MINOR 10)
set(SNORT_VERSION_BUILD  044)

# minimum requirement is std11
list (APPEND CMAKE_CFLAGS "${CFLAGS} -std=c++11 ")
list (APPEND CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# ensure cmake will look in the cmake directory for configuration files
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")


include (cmake/macros.cmake)  
include (cmake/createCommandLineOptions.txt) 
include (cmake/setConfigureOptions.txt) 
include (cmake/includePredefinedPackages.txt)
include (cmake/sanityChecks.txt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(etc)
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(lua)
add_subdirectory(doc)


# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
