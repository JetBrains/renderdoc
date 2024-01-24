set(sources
    dxil_bytecode.cpp
    dxil_bytecode_editor.cpp
    dxil_common.cpp
    dxil_debuginfo.cpp
    dxil_disassemble.cpp
    dxil_reflect.cpp
    llvm_decoder.cpp
    llvm_encoder.cpp
    dxil_bytecode.h
    dxil_bytecode_editor.h
    dxil_common.h
    dxil_debuginfo.h
    llvm_bitreader.h
    llvm_bitwriter.h
    llvm_common.h
    llvm_decoder.h
    llvm_encoder.h
)

set(definitions ${RDOC_DEFINITIONS})

add_library(rdoc_dxil OBJECT ${sources})
target_compile_definitions(rdoc_dxil ${definitions})
target_include_directories(rdoc_dxil ${RDOC_INCLUDES})
