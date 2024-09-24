add_library(RenderDocService STATIC
        RenderDocService.cpp
        RenderDocService.h
        RenderDocReplay.cpp
        RenderDocReplay.h
        util/RenderDocConverterUtils.h
        util/RenderDocConverterUtils.cpp
        RenderDocDebugSession.cpp
        RenderDocDebugSession.h
        util/StringUtils.cpp
        util/StringUtils.h
        util/ArrayUtils.h
        RenderDocEvent.cpp
        RenderDocEvent.h
        RenderDocFile.cpp
        RenderDocFile.h
)

if (ENABLE_PCH_HEADERS)
    include(${PROJECT_SOURCE_DIR}/PrecompiledHeader.cmake)
    add_precompiled_header(RenderDocService pch.h SOURCE_CXX pch.cpp FORCEINCLUDE)
endif ()

target_include_directories(RenderDocService SYSTEM PRIVATE ../../../renderdoc)

target_link_libraries(RenderDocService PRIVATE renderdoc PUBLIC RenderDocRoot)
