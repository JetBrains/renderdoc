set(sources
    dxgi_common.cpp
    dxgi_hooks.cpp
    dxgi_stringise.cpp
    dxgi_wrapped.cpp
    ../dx/official/dxgi.h
    ../dx/official/dxgi1_2.h
    ../dx/official/dxgi1_3.h
    ../dx/official/dxgi1_4.h
    ../dx/official/dxgi1_5.h
    ../dx/official/dxgi1_6.h
    ../dx/official/dxgicommon.h
    ../dx/official/dxgidebug.h
    ../dx/official/dxgiformat.h
    ../dx/official/dxgitype.h
    dxgi_common.h
    dxgi_wrapped.h
)

set(definitions ${RDOC_DEFINITIONS})

add_library(rdoc_dxgi OBJECT ${sources})
target_compile_definitions(rdoc_dxgi ${definitions})
target_include_directories(rdoc_dxgi ${RDOC_INCLUDES})
