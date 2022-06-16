include(common)

set(GLTF_src
    GLTF/GltfBBoxPass.cpp
    GLTF/GltfBBoxPass.h
    GLTF/GltfDepthPass.cpp
    GLTF/GltfDepthPass.h
    GLTF/GltfHelpers.cpp
    GLTF/GltfHelpers.h
    GLTF/GltfPbrPass.cpp
    GLTF/GltfPbrPass.h
    GLTF/GltfMotionVectorsPass.cpp
    GLTF/GltfMotionVectorsPass.h
    GLTF/GLTFTexturesAndBuffers.cpp
    GLTF/GLTFTexturesAndBuffers.h
)

set(PostProc_src
    PostProc/Bloom.cpp
    PostProc/Bloom.h
    PostProc/BakeSkinning.cpp
    PostProc/BakeSkinning.h
    PostProc/BlurPS.cpp
    PostProc/BlurPS.h
    PostProc/Debug.cpp
    PostProc/Debug.h
    PostProc/DownSamplePS.cpp
    PostProc/DownSamplePS.h
    PostProc/PostProcCS.cpp
    PostProc/PostProcCS.h
    PostProc/PostProcPS.cpp
    PostProc/PostProcPS.h
    PostProc/SkyDome.cpp
    PostProc/SkyDome.h
    PostProc/SkyDomeProc.cpp
    PostProc/SkyDomeProc.h
    PostProc/TAA.cpp
    PostProc/TAA.h
    PostProc/ColorConversionPS.h
    PostProc/ColorConversionPS.cpp
    PostProc/Tonemapping.cpp
    PostProc/Tonemapping.h
    PostProc/TonemappingCS.cpp
    PostProc/TonemappingCS.h
    PostProc/ShadowResolvePass.cpp
    PostProc/ShadowResolvePass.h
    PostProc/MagnifierPS.h
    PostProc/MagnifierPS.cpp
)

set(Base_src
    base/Buffer.cpp
    base/Buffer.h
    base/CommandListRing.cpp
    base/CommandListRing.h
    base/Device.cpp
    base/Device.h
    base/DynamicBufferRing.cpp
    base/DynamicBufferRing.h
    base/Fence.cpp
    base/Fence.h
	base/FrameworkWindows.cpp
	base/FrameworkWindows.h
    base/FreeSyncHDR.cpp
    base/FreeSyncHDR.h
    base/GBuffer.cpp
    base/GBuffer.h
    base/GPUTimestamps.cpp
    base/GPUTimestamps.h
    base/Helper.cpp
    base/Helper.h
    base/Imgui.cpp
    base/Imgui.h
    base/ResourceViewHeaps.cpp
    base/ResourceViewHeaps.h
    base/ShaderCompilerHelper.cpp
    base/ShaderCompilerHelper.h
    base/StaticBufferPool.cpp
    base/StaticBufferPool.h
    base/StaticConstantBufferPool.cpp
    base/StaticConstantBufferPool.h
    base/SaveTexture.cpp
    base/SaveTexture.h
    base/SwapChain.cpp
    base/SwapChain.h
    base/Texture.cpp
    base/Texture.h
    base/UploadHeapSimple.cpp
    base/UploadHeapSimple.h
    base/UploadHeap.cpp
    base/UploadHeap.h
    base/UserMarkers.h
    base/UserMarkers.cpp
)

set(Widgets_src
    widgets/Wireframe.cpp
    widgets/Wireframe.h
    widgets/WireframeBox.h
    widgets/WireframeSphere.h
)

set(Shaders_PostProc_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blend.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blur.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Debug.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/DownSamplePS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Tonemappers.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/LPMTonemapperHelper.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Tonemapping.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TonemappingCS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ColorConversionPS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TAA.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TAASharpenerCS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/motionBlur.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/MagnifierPS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/transferFunction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/FidelityFX/include/gpu/ffx_core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/FidelityFX/include/gpu/ffx_common_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/FidelityFX/include/gpu/ffx_core_cpu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/FidelityFX/include/gpu/ffx_core_gpu_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/FidelityFX/include/gpu/ffx_core_gpu_common_half.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/FidelityFX/include/gpu/ffx_core_portability.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/FidelityFX/include/gpu/ffx_core_hlsl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/FidelityFX/include/gpu/ffx_lpm.h
)

set(Shaders_GLTF_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-PS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-VS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-IO.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPBRLighting.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFDepthPass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFNormals.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ShadowResolve.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFMotionVectorsPass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDome.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDomeProc.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/functions.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/PBRTextures.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadowFiltering.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/PBRPixelParams.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/BakeSkinning.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Skinning.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFVertexFactory.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/perFrameStruct.h
)

add_library (Cauldron_DX12 STATIC ${GLTF_src} ${PostProc_src} ${Base_src} ${Widgets_src} )

target_link_libraries (Cauldron_DX12 Cauldron_Common D3D12X winpixeventruntimelib DX12AgilitySDK)
target_include_directories (Cauldron_DX12 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

copyTargetCommand("${Shaders_GLTF_src}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibDX copied_dx12_shaders_gltf_src)
copyTargetCommand("${Shaders_PostProc_src}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibDX copied_dx12_shaders_postproc_src)
add_dependencies(Cauldron_DX12 copied_dx12_shaders_gltf_src copied_dx12_shaders_postproc_src)


source_group("GLTF"             FILES ${GLTF_src})
source_group("PostProcess"      FILES ${PostProc_src})
source_group("Base"             FILES ${Base_src})
source_group("Widgets"          FILES ${Widgets_src})

set_source_files_properties(${Shaders_GLTF_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
set_source_files_properties(${Shaders_PostProc_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
