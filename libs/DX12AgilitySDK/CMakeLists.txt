include(common)

add_library(DX12AgilitySDK INTERFACE)
target_include_directories(DX12AgilitySDK INTERFACE BEFORE "inc/")

set(DX12AgilitySDK_BIN
    "${CMAKE_CURRENT_SOURCE_DIR}/1.600.10/bin/x64/D3D12Core.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/1.600.10/bin/x64/d3d12SDKLayers.dll"
)

copyTargetCommand("${DX12AgilitySDK_BIN}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12/ copied_dx12_agility_adk_bin)
add_dependencies(DX12AgilitySDK copied_dx12_agility_adk_bin)
