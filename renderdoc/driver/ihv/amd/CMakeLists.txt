set(sources
    amd_counters.cpp
    amd_counters.h
    amd_isa.cpp
    amd_isa.h
    amd_isa_devices.cpp
    amd_isa_devices.h
    amd_rgp.cpp
    amd_rgp.h
    official/RGP/DevDriverAPI.h)

if (WIN32)
    list(APPEND sources
         ags_wrapper.cpp
         amd_hooks.cpp
         amd_isa_win32.cpp
         ags_wrapper.h
         official/ags/amd_ags.h
         official/DXExt/AmdDxExt.h
         official/DXExt/AmdDxExtApi.h
         official/DXExt/AmdDxExtIface.h
         official/DXExt/AmdExtD3D.h
         official/DXExt/AmdExtD3DCommandListMarkerApi.h
         official/GPUPerfAPI/Include/gpu_perf_api.h
         official/GPUPerfAPI/Include/gpu_perf_api_counters.h
         official/GPUPerfAPI/Include/gpu_perf_api_functions.h
         official/GPUPerfAPI/Include/gpu_perf_api_function_types.h
         official/GPUPerfAPI/Include/gpu_perf_api_interface_loader.h
         official/GPUPerfAPI/Include/gpu_perf_api_stub.h
         official/GPUPerfAPI/Include/gpu_perf_api_types.h
         official/GPUPerfAPI/Include/gpu_perf_api_vk.h
         official/RGA/Common/AmdDxGsaCompile.h
         official/RGA/Common/AsicReg/devices.h
         official/RGA/elf/elf32.h
         official/RGA/elf/elf_common.h
    )
else()
    list(APPEND sources amd_isa_posix.cpp)
endif()

set(include_dirs ${RDOC_INCLUDES})

add_library(rdoc_amd OBJECT ${sources})
target_compile_definitions(rdoc_amd ${RDOC_DEFINITIONS})
target_include_directories(rdoc_amd ${include_dirs})
