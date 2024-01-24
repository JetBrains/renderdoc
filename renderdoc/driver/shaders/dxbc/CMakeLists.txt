set(sources
    dxbc_bytecode.cpp
    dxbc_bytecode_editor.cpp
    dxbc_bytecode_vendorext.cpp
    dxbc_compile.cpp
    dxbc_debug.cpp
    dxbc_bytecode_ops.cpp
    dxbc_container.cpp
    dxbc_reflect.cpp
    dxbc_sdbg.cpp
    dxbc_spdb.cpp
    dxbc_stringise.cpp
    dxbc_bytecode.h
    dxbc_bytecode_editor.h
    dxbc_bytecode_ops.h
    dxbc_common.h
    dxbc_compile.h
    dxbc_debug.h
    dxbc_container.h
    dxbc_reflect.h
    dxbc_sdbg.h
    dxbc_spdb.h
    official/cvconst.h
    official/cvinfo.h
)

set(definitions ${RDOC_DEFINITIONS})

add_library(rdoc_dxbc OBJECT ${sources})
target_compile_definitions(rdoc_dxbc ${definitions})
target_include_directories(rdoc_dxbc ${RDOC_INCLUDES})

add_dependencies(rdoc_dxbc rdoc_dxil)

