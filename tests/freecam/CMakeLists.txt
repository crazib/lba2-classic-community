add_executable(test_freecam_math
    test_freecam_math.cpp
    ${CMAKE_SOURCE_DIR}/SOURCES/EDITOR/FREECAM_MATH.CPP
)

target_include_directories(test_freecam_math PRIVATE
    ${CMAKE_SOURCE_DIR}/SOURCES
    ${CMAKE_SOURCE_DIR}/LIB386/H
)

if(NOT MSVC)
    target_link_libraries(test_freecam_math PRIVATE m)
endif()

add_test(NAME test_freecam_math COMMAND test_freecam_math)
register_host_test(test_freecam_math)
