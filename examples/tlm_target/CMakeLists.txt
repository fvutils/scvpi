cmake_minimum_required(VERSION 3.15)
project(scvpi_tlm_target_example LANGUAGES CXX)

set(SCVPI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
find_package(SystemCLanguage REQUIRED)
find_package(
    Python3
    COMPONENTS Interpreter Development
    QUIET)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(SC_INCLUDE_DYNAMIC_PROCESSES)

add_executable(tlm_target_native_test src/native_test.cpp ${SCVPI_DIR}/src/scvpi_tlm_target.cpp)
target_include_directories(tlm_target_native_test PRIVATE ${SCVPI_DIR}/src)
target_link_libraries(tlm_target_native_test PRIVATE SystemC::systemc)
enable_testing()
add_test(NAME tlm_target_native COMMAND tlm_target_native_test)

if(Python3_Development_FOUND)
    add_executable(
        tlm_target_cocotb
        src/cocotb_main.cpp ${SCVPI_DIR}/src/scvpi.cpp ${SCVPI_DIR}/src/scvpi_tlm_bridge.cpp
        ${SCVPI_DIR}/src/scvpi_tlm_target.cpp)
    target_include_directories(tlm_target_cocotb PRIVATE ${SCVPI_DIR}/src ${Python3_INCLUDE_DIRS})
    target_link_libraries(tlm_target_cocotb PRIVATE SystemC::systemc ${CMAKE_DL_LIBS})
    set_target_properties(tlm_target_cocotb PROPERTIES ENABLE_EXPORTS ON)
    # Keep C++ checks active even when the example is built in Release mode.
    target_compile_options(tlm_target_cocotb PRIVATE -UNDEBUG)

    add_library(scvpi_tlm_target_ext SHARED ${SCVPI_DIR}/src/scvpi_tlm_target_ext.cpp)
    target_include_directories(
        scvpi_tlm_target_ext PRIVATE ${SCVPI_DIR}/src ${Python3_INCLUDE_DIRS}
                                     ${SystemC_INCLUDE_DIRS})
    target_link_libraries(scvpi_tlm_target_ext PRIVATE Python3::Python)
    set_target_properties(scvpi_tlm_target_ext PROPERTIES POSITION_INDEPENDENT_CODE ON)

    message(STATUS "Cocotb example target enabled; install cocotb and use the example Makefile")
else()
    message(STATUS "Python development files not found; building native target test only")
endif()
