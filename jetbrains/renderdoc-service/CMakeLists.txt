add_subdirectory(src)
add_subdirectory(test)

target_include_directories(RenderDocService INTERFACE ${CMAKE_CURRENT_LIST_DIR}/api)
