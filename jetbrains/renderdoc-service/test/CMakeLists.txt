add_executable(RenderDocServiceTests
        renderdoc_service_tests.cpp
)

file(COPY ../../src/test/resources/samples/ DESTINATION ${CMAKE_BINARY_DIR}/bin/samples)
target_link_libraries(RenderDocServiceTests PUBLIC RenderDocService)
