include(common)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/D3D12Core.dll OR
   NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/d3d12SDKLayers.dll OR
   NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/d3dconfig.exe)
    message(STATUS "Downloading AgilitySDK")
    set(AGILITYSDK "D3D12-AgilitySDK-1.613.0")

    file(DOWNLOAD https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.613.0 ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}.zip
        STATUS DOWNLOAD_RESULT)
        
    list(GET DOWNLOAD_RESULT 0 DOWNLOAD_RESULT_CODE)
    if(NOT DOWNLOAD_RESULT_CODE EQUAL 0)
        message(FATAL_ERROR "Failed downloading AgilitySDK! Error: ${DOWNLOAD_RESULT}.")
    endif()

    file(ARCHIVE_EXTRACT
        INPUT ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}.zip
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK})
        
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}/build/native/bin/x64/D3D12Core.dll OR
       NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}/build/native/bin/x64/d3d12SDKLayers.dll OR
       NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}/build/native/bin/x64/d3dconfig.exe)
        message(FATAL_ERROR "Failed unpacking AgilitySDK!")
    endif()

    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}/build/native/bin/x64/D3D12Core.dll ${CMAKE_CURRENT_SOURCE_DIR}/D3D12Core.dll)
    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}/build/native/bin/x64/d3d12SDKLayers.dll ${CMAKE_CURRENT_SOURCE_DIR}/d3d12SDKLayers.dll)
    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}/build/native/bin/x64/d3dconfig.exe ${CMAKE_CURRENT_SOURCE_DIR}/d3dconfig.exe)

    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}/LICENSE.txt ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt)
    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}/LICENSE-CODE.txt ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE-CODE.txt)

    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK}.zip)
    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/_${AGILITYSDK})
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dxc.exe OR
   NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dxcompiler.dll OR
   NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dxil.dll)
    message(STATUS "Downloading DXC")
    set(DXCOMPILER "D3D12-DXC-1.8.2403.18")

    file(DOWNLOAD https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DXC/1.8.2403.18 ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}.zip
        STATUS DOWNLOAD_RESULT)
        
    list(GET DOWNLOAD_RESULT 0 DOWNLOAD_RESULT_CODE)
    if(NOT DOWNLOAD_RESULT_CODE EQUAL 0)
        message(FATAL_ERROR "Failed downloading DXC! Error: ${DOWNLOAD_RESULT}.")
    endif()

    file(ARCHIVE_EXTRACT
        INPUT ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}.zip
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER})
        
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/build/native/bin/x64/dxc.exe OR
       NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/build/native/bin/x64/dxcompiler.dll OR
       NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/build/native/bin/x64/dxil.dll)
        message(FATAL_ERROR "Failed unpacking DXC!")
    endif()

    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/build/native/bin/x64/dxc.exe ${CMAKE_CURRENT_SOURCE_DIR}/dxc.exe)
    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/build/native/bin/x64/dxcompiler.dll ${CMAKE_CURRENT_SOURCE_DIR}/dxcompiler.dll)
    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/build/native/bin/x64/dxil.dll ${CMAKE_CURRENT_SOURCE_DIR}/dxil.dll)

    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/LICENCE-MIT.txt ${CMAKE_CURRENT_SOURCE_DIR}/LICENCE-MIT.txt)
    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/LICENSE-LLVM.txt ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE-LLVM.txt)
    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}/LICENSE-MS.txt ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE-MS.txt)

    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER}.zip)
    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/_${DXCOMPILER})
endif()

copyTargetCommand("${CMAKE_CURRENT_SOURCE_DIR}/dxcompiler.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dxc" copied_DXCwg_dxcompiler)
copyTargetCommand("${CMAKE_CURRENT_SOURCE_DIR}/dxil.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dxc" copied_DXCwg_dxil)
copyTargetCommand("${CMAKE_CURRENT_SOURCE_DIR}/D3D12Core.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/d3d12" copied_D3D12wg_core)
copyTargetCommand("${CMAKE_CURRENT_SOURCE_DIR}/d3d12SDKLayers.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/d3d12" copied_D3D12wg_layers)
add_dependencies(DXC copied_DXCwg_dxcompiler)
add_dependencies(DXC copied_DXCwg_dxil)
add_dependencies(DXC copied_D3D12wg_core)
add_dependencies(DXC copied_D3D12wg_layers)
