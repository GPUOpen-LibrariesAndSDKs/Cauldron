project (DXC)

add_library(${PROJECT_NAME} SHARED IMPORTED GLOBAL)

set_property(TARGET ${PROJECT_NAME} PROPERTY IMPORTED_IMPLIB dxcompiler.lib)

set( SC_DEST_DIR ${CMAKE_HOME_DIRECTORY}/bin)

file(MAKE_DIRECTORY ${SC_DEST_DIR})

set(DXC_dlls bin/dxil.dll bin/dxcompiler.dll)
file(COPY ${DXC_dlls} DESTINATION ${SC_DEST_DIR})
