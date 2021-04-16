include(common)

add_library(DXC INTERFACE)
target_include_directories(DXC INTERFACE BEFORE "inc/")

set(DXC_BIN
    "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/dxcompiler.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/dxil.dll"
)

copyTargetCommand("${DXC_BIN}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} copied_dxc_bin)
add_dependencies(DXC copied_dxc_bin)
