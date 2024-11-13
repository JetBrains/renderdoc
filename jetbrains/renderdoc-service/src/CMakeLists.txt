add_library(RenderDocService STATIC
        RenderDocService.cpp
        RenderDocService.h
        RenderDocReplay.cpp
        RenderDocReplay.h
        RenderDocFile.cpp
        RenderDocFile.h
        RenderDocEvent.cpp
        RenderDocEvent.h
        RenderDocDrawCallDebugSession.cpp
        RenderDocDrawCallDebugSession.h
        RenderDocDebugSession.cpp
        RenderDocDebugSession.h
        util/RenderDocConverterUtils.h
        util/RenderDocConverterUtils.cpp
        util/StringUtils.cpp
        util/StringUtils.h
        util/ArrayUtils.h
        util/RenderDocActionHelpers.cpp
        util/RenderDocActionHelpers.h
        util/RenderDocLineBreakpointsMapper.cpp
        util/RenderDocLineBreakpointsMapper.h
        RenderDocTexturePreviewService.cpp
        RenderDocTexturePreviewService.h
)

if (ENABLE_PCH_HEADERS)
    include(${PROJECT_SOURCE_DIR}/PrecompiledHeader.cmake)
    add_precompiled_header(RenderDocService pch.h SOURCE_CXX pch.cpp FORCEINCLUDE)
endif ()

target_include_directories(RenderDocService SYSTEM PRIVATE ../../../renderdoc)

target_link_libraries(RenderDocService PRIVATE renderdoc PUBLIC RenderDocRoot)
