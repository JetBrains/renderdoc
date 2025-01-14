add_executable(RenderDocHost
        main.cpp
        PeerBase.h
        Server.cpp
        Server.h
        Server.cpp
        Server.h
)

if (ENABLE_PCH_HEADERS)
    include(${PROJECT_SOURCE_DIR}/PrecompiledHeader.cmake)
    add_precompiled_header(RenderDocHost pch.h SOURCE_CXX pch.cpp FORCEINCLUDE)
endif ()

if (APPLE)
    set_target_properties(RenderDocHost PROPERTIES
            INSTALL_RPATH "@executable_path"
            BUILD_WITH_INSTALL_RPATH FALSE
            MACOSX_RPATH TRUE
    )
elseif (UNIX)
    set_target_properties(RenderDocHost PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH FALSE
    )
endif()

target_link_libraries(RenderDocHost PRIVATE RenderDocService RenderDocRoot)
