project (Framework_Common)
if(MSVC)
  add_compile_options(/MP)
else()
add_compile_options(-M)
endif()
set(base_src
    "base/Sequence.cpp"
    "base/Sequence.h"
    "base/Benchmark.cpp"
    "base/Benchmark.h"
    "base/ImGuiHelper.cpp"
    "base/ImGuiHelper.h"
)

set(shader_compiler_src
    "base/ShaderCompiler.cpp"
    "base/ShaderCompiler.h"
    "base/ShaderCompilerCache.cpp"
    "base/ShaderCompilerCache.h"
    "base/DXCHelper.cpp"
    "base/DXCHelper.h"
)

set(GLTF_src
    "GLTF/GltfStructures.h"
    "GLTF/GltfCommon.cpp"
    "GLTF/GltfCommon.h"
    "GLTF/GltfPbrMaterial.cpp"
    "GLTF/GltfPbrMaterial.h"
    "GLTF/GltfHelpers.cpp"
    "GLTF/GltfHelpers.h"
)

set(Misc_src
    "Misc/Cache.cpp"
	"Misc/Cache.h"
	"Misc/Camera.cpp"
	"Misc/Camera.h"
	"Misc/DDSLoader.cpp"
	"Misc/DDSLoader.h"
	"Misc/Error.cpp"
	"Misc/Error.h"
	"Misc/ImgLoader.cpp"
	"Misc/ImgLoader.h"
	"Misc/Misc.cpp"
	"Misc/Misc.h"
	"Misc/Ring.h"
	"Misc/ThreadPool.cpp"
	"Misc/threadpool.h"

	"Misc/WirePrimitive.h"
	"Misc/WirePrimitives.cpp"
	"Misc/WirePrimitives.h"
)

add_library (Cauldron_Common STATIC ${base_src} ${GLTF_src} ${Misc_src} ${shader_compiler_src} ${media_src})
target_link_libraries (Cauldron_Common PUBLIC NJSON DXC Shcore)
target_include_directories (Cauldron_Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(media_src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../media/brdfLut.dds
)

copyTargetCommand("${media_src}" ${CMAKE_HOME_DIRECTORY}/bin copied_common_media_src)
add_dependencies (Cauldron_Common copied_common_media_src)

source_group("Base"         FILES ${base_src})
source_group("GLTF"         FILES ${GLTF_src})
source_group("Misc"         FILES ${Misc_src})
source_group("SC"           FILES ${shader_compiler_src})
