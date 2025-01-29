if(APPLE)
    set(TEST_SOURCE renderdoc_service_tests_macos.cpp)
elseif(UNIX)
    set(TEST_SOURCE renderdoc_service_tests_linux.cpp)
elseif(WIN32)
    set(TEST_SOURCE renderdoc_service_tests_windows.cpp)
else()
    message(FATAL_ERROR "Unsupported operating system for RenderDocServiceTests.")
endif()

add_executable(RenderDocServiceTests
        ${TEST_SOURCE}
        renderdoc_service_test_utils.cpp
        renderdoc_service_test_utils.h
)

file(COPY ../../src/test/resources/samples/ DESTINATION ${CMAKE_BINARY_DIR}/bin/samples)
target_link_libraries(RenderDocServiceTests PUBLIC RenderDocService)
