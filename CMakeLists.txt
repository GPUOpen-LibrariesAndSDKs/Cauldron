#set a debug override for the project before we change project name for Cauldron
cmake_minimum_required(VERSION 3.6)

option (GFX_API_DX12 "Build Cauldron with DX12" ON)
option (GFX_API_VK "Build Cauldron with Vulkan" ON)

if(NOT DEFINED GFX_API)
    project (Cauldron)
else()
    project (Cauldron_${GFX_API})

    set_property(DIRECTORY ${CMAKE_PROJECT_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

    if(GFX_API STREQUAL DX12)
        set(GFX_API_DX12 ON)
        set(GFX_API_VK OFF)
    elseif(GFX_API STREQUAL VK)
        set(GFX_API_DX12 OFF)
        set(GFX_API_VK ON)
    else()
        message(STATUS "----------------------------------------------------------------------------------------")
        message(STATUS "")
        message(STATUS "** Almost there!!")
        message(STATUS "")
        message(STATUS " This framework supports DX12 and VULKAN, you need to invoke cmake in one of these ways:")
        message(STATUS "")
        message(STATUS " Examples:")
        message(STATUS "  Generate selected one:")
        message(STATUS "    cmake <project_root_dir> -DGFX_API=DX12")
        message(STATUS "    cmake <project_root_dir> -DGFX_API=VK")
        message(STATUS "  Generate with switches (Default is ON):")
        message(STATUS "    cmake <project_root_dir> [-DGFX_API_DX12=ON|OFF] [-DGFX_API_VK=ON|OFF]")
        message(STATUS "")
        message(STATUS "----------------------------------------------------------------------------------------")
        message(FATAL_ERROR "")
    endif()
endif()

# Check MSVC toolset version, Visual Studio 2019 required
if(MSVC_TOOLSET_VERSION VERSION_LESS 142)
    message(FATAL_ERROR "Cannot find MSVC toolset version 142 or greater. Please make sure Visual Studio 2019 or newer installed")
endif()

list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} )

# output exe to bin directory
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
# append 'd' to debug libs
set(CMAKE_DEBUG_POSTFIX d)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_HOME_DIRECTORY}/bin )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

add_compile_options(/MP)

# reference libs used by both backends
add_subdirectory(src/Common)
add_subdirectory(libs/json)
add_subdirectory(libs/ImGUI)
add_subdirectory(libs/stb)
add_subdirectory(libs/DXC)
add_subdirectory(libs/vectormath)

if(GFX_API_DX12)
    add_subdirectory(libs/d3d12x)
    add_subdirectory(libs/AGS)
    add_subdirectory(libs/WinPixEventRuntime)
    add_subdirectory(libs/DX12AgilitySDK)
    add_subdirectory(src/DX12)
endif()
if(GFX_API_VK)
    find_package(Vulkan REQUIRED)
    add_subdirectory(libs/VulkanMemoryAllocator)
    add_subdirectory(src/VK)
endif()

