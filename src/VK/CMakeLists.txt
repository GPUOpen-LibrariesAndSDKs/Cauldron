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
    PostProc/BlurPS.cpp
    PostProc/BlurPS.h
    PostProc/DownSamplePS.cpp
    PostProc/DownSamplePS.h
    PostProc/PostProcPS.cpp
    PostProc/PostProcPS.h
    PostProc/PostProcCS.cpp
    PostProc/PostProcCS.h
    PostProc/SkyDome.cpp
    PostProc/SkyDome.h
    PostProc/SkyDomeProc.cpp
    PostProc/SkyDomeProc.h
    PostProc/ColorConversionPS.cpp
    PostProc/ColorConversionPS.h
    PostProc/Tonemapping.cpp
    PostProc/Tonemapping.h
    PostProc/TonemappingCS.cpp
    PostProc/TonemappingCS.h
    PostProc/TAA.cpp
    PostProc/TAA.h
    PostProc/MagnifierPS.h
    PostProc/MagnifierPS.cpp
)

set(Base_src
    base/CommandListRing.cpp
    base/CommandListRing.h
    base/Device.cpp
    base/Device.h
    base/DeviceProperties.cpp
    base/DeviceProperties.h
    base/DynamicBufferRing.cpp
    base/DynamicBufferRing.h
    base/GBuffer.cpp
    base/GBuffer.h
    base/GPUTimestamps.cpp
    base/GPUTimestamps.h
	base/FrameworkWindows.cpp
	base/FrameworkWindows.h
    base/FreeSyncHDR.cpp
    base/FreeSyncHDR.h
    base/Helper.cpp
    base/Helper.h
    base/Imgui.cpp
    base/Imgui.h
    base/Instance.cpp
    base/Instance.h
    base/InstanceProperties.cpp
    base/InstanceProperties.h
    base/ResourceViewHeaps.cpp
    base/ResourceViewHeaps.h
    base/ShaderCompilerHelper.cpp
    base/ShaderCompilerHelper.h
    base/StaticBufferPool.cpp
    base/StaticBufferPool.h
    base/SwapChain.cpp
    base/SwapChain.h
    base/Texture.cpp
    base/Texture.h
    base/UploadHeap.cpp
    base/UploadHeap.h
)

set(Extensions_src
    base/ExtFreeSyncHDR.cpp
    base/ExtFreeSyncHDR.h
    base/ExtFp16.cpp
    base/ExtFp16.h
    base/ExtRayTracing.cpp
	base/ExtRayTracing.h
    base/ExtValidation.cpp
    base/ExtValidation.h
	base/ExtVRS.cpp
	base/ExtVRS.h
    base/ExtDebugUtils.cpp
    base/ExtDebugUtils.h
)

set(Widgets_src
    widgets/Axis.cpp
    widgets/Axis.h
    widgets/CheckerBoardFloor.cpp
    widgets/CheckerboardFloor.h
    widgets/Wireframe.cpp
    widgets/Wireframe.h
    widgets/WireframeBox.h
    widgets/WireframeSphere.h
)

set(Shaders_PostProc_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blend.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blur.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/DownSamplePS.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tonemappers.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/LPMTonemapperHelper.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tonemapping.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TAASharpenerCS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TAA.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tonemappingCS.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ColorConversionPS.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/MagnifierPS.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/transferFunction.h
)

set(Shaders_GLTF_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTF_VS2PS_IO.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPBRLighting.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFDepthPass-frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFDepthPass-vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFMotionVectorsPass-frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFMotionVectorsPass-vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDome.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDome.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDomeProc.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDomeProc.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/functions.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/PBRTextures.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadowFiltering.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/PixelParams.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/skinning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFVertexFactory.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/perFrameStruct.h
)

add_library (Cauldron_VK STATIC ${GLTF_src} ${PostProc_src} ${Base_src} ${Widgets_src} ${Extensions_src})

target_link_libraries (Cauldron_VK Cauldron_Common Vulkan::Vulkan)
target_include_directories (Cauldron_VK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

copyTargetCommand("${Shaders_GLTF_src}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibVK copied_vk_shaders_gltf_src)
copyTargetCommand("${Shaders_PostProc_src}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibVK copied_vk_shaders_postproc_src)
add_dependencies(Cauldron_VK copied_vk_shaders_gltf_src copied_vk_shaders_postproc_src)


source_group("GLTF"             FILES ${GLTF_src})
source_group("PostProcess"      FILES ${PostProc_src})
source_group("Base"             FILES ${Base_src})
source_group("Widgets"          FILES ${Widgets_src})
source_group("Extensions"       FILES ${Extensions_src})

set_source_files_properties(${Shaders_GLTF_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
set_source_files_properties(${Shaders_PostProc_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
