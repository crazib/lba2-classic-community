add_executable(test_editor_terrain_patterns
    test_editor_terrain_patterns.cpp
    ${CMAKE_SOURCE_DIR}/SOURCES/3DEXT/EDITOR_TERRAIN_PATTERNS.CPP)

target_include_directories(test_editor_terrain_patterns PRIVATE
    ${CMAKE_SOURCE_DIR}/SOURCES
    ${CMAKE_SOURCE_DIR}/SOURCES/3DEXT
    ${CMAKE_SOURCE_DIR}/LIB386/H)

add_test(NAME test_editor_terrain_patterns COMMAND test_editor_terrain_patterns)
register_host_test(test_editor_terrain_patterns)
