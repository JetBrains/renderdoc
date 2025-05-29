if(APPLE)
    add_subdirectory(macos)
    list(TRANSFORM test_sources PREPEND "macos/")
elseif(UNIX)
    add_subdirectory(linux)
    list(TRANSFORM test_sources PREPEND "linux/")
elseif(WIN32)
    add_subdirectory(windows)
    list(TRANSFORM test_sources PREPEND "windows/")
else()
    message(FATAL_ERROR "Unsupported operating system for RenderDocServiceTests.")
endif()

add_executable(RenderDocServiceTests
        ${test_sources}
        renderdoc_service_test_utils.cpp
        renderdoc_service_test_utils.h
        renderdoc_service_tests.h
        renderdoc_service_tests.cpp
)

file(COPY ../../src/test/resources/samples/ DESTINATION ${CMAKE_BINARY_DIR}/bin/samples)
target_link_libraries(RenderDocServiceTests PUBLIC RenderDocService)
