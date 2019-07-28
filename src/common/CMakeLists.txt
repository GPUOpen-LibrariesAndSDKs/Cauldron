project (Framework_Common)

set(base_src
    "base/ImGuiHelper.cpp"
    "base/ImGuiHelper.h"
    "base/ShaderCompiler.cpp"
    "base/ShaderCompiler.h"
)

set(GLTF_src
    "GLTF/GltfStructures.h"
    "GLTF/GltfCommon.cpp"
    "GLTF/GltfCommon.h"
    "GLTF/GltfPbrMaterial.cpp"
    "GLTF/GltfPbrMaterial.h"
    "GLTF/glTFHelpers.cpp"
    "GLTF/glTFHelpers.h"
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

if(WIN32)
	set(Misc_src_platform
		"Misc/FrameworkWindows.cpp"
		"Misc/FrameworkWindows.h"
		"Misc/WICLoader.cpp"
		"Misc/WICLoader.h"
	)
endif()


add_library (${PROJECT_NAME} STATIC ${base_src} ${GLTF_src} ${Misc_src} ${Misc_src_platform})
target_link_libraries (${PROJECT_NAME} NJSON DxgiFormat DirectXMath)
target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

source_group("base"         FILES ${base_src})
source_group("GLTF"         FILES ${GLTF_src})
source_group("Misc"         FILES ${Misc_src})
