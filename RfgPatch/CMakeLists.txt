
cmake_minimum_required (VERSION 3.8)
project (RfgPatch VERSION 1.0.0
                  DESCRIPTION "RfgPatch"
                  LANGUAGES CXX)

# Get list of source files in project src folders recursively
# Places in SOURCES variable
file(GLOB_RECURSE SOURCES 
    "${CMAKE_SOURCE_DIR}/RfgPatch/*.cpp"
    "${CMAKE_SOURCE_DIR}/RfgPatch/*.h"
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/Dependencies/BinaryTools/*.h
)

# Add source to this project's executable.
add_executable (RfgPatch ${SOURCES})

# Add include directories
target_include_directories(RfgPatch SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/RfgPatch/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/Common/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/Dependencies/BinaryTools/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/RfgTools++/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/RfgTools++/RfgTools++/
)

# Add statically linked libs
target_link_libraries(RfgPatch PRIVATE Common)
target_link_libraries(RfgPatch PRIVATE RfgTools++)