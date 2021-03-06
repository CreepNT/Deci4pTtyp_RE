cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{DOLCESDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{DOLCESDK}/share/dolce.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define DOLCESDK to point to your SDK path!")
  endif()
endif()

project(Deci4pTtyP)
include("${DOLCESDK}/share/dolce.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

link_directories(
  ${CMAKE_SOURCE_DIR}/extra/
)

add_executable(${PROJECT_NAME}
  main.c
  deci4p_tty.c
  func_tables.c
)

target_link_libraries(${PROJECT_NAME}
    SceProcEventForDriver_stub
    SceIofilemgrForDriver_stub
    SceThreadmgrForKernel_stub
    SceSysclibForDriver_stub
    SceSysmemForDriver_stub
    ScePmMgrForDriver_stub
    SceDebugForDriver_stub
    SceDipswForDriver_stub
    SceCpuForDriver_stub

    SceDebugForKernel_stub.a
    SceThreadmgrForDriver_stub.a
    SceDeci4pDfMgrForDebugger_stub_weak.a
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES LINK_FLAGS "-nostdlib"
  COMPILE_FLAGS "-D__VITA_KERNEL__"
)

dolce_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
  CONFIG config.yml
  UNSAFE
)