add_library(DirectXMath INTERFACE)
target_include_directories(DirectXMath INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/DirectXMath/Inc")

if(NOT WIN32)
    target_include_directories(DirectXMath INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/sal")
endif()
