add_library(DxgiFormat INTERFACE)

if(NOT WIN32)
    target_include_directories(DxgiFormat INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/dxgiformat")
endif()
