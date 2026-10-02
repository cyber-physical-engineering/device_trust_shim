cmake_minimum_required(VERSION 3.15)
project(DeviceTrustShim VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only library. Link to dts::DeviceTrustShim from your own target.
add_library(DeviceTrustShim INTERFACE)
add_library(dts::DeviceTrustShim ALIAS DeviceTrustShim)
target_include_directories(DeviceTrustShim INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(DeviceTrustShim INTERFACE cxx_std_17)

# Build the examples and tests only when this is the top-level project,
# not when another project pulls DTS in with add_subdirectory().
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(DTS_TOP_LEVEL ON)
else()
    set(DTS_TOP_LEVEL OFF)
endif()
option(DTS_BUILD_EXAMPLES "Build the DTS examples" ${DTS_TOP_LEVEL})
option(DTS_BUILD_TESTS "Build the DTS unit tests" ${DTS_TOP_LEVEL})

if(DTS_BUILD_EXAMPLES)
    foreach(example
            radiology_device_example
            infusion_pump_example
            dicom_adapter_example
            clinical_trial_adapter_example
            industrial_adapter_example
            building_automation_example)
        add_executable(${example} examples/${example}.cpp)
        target_link_libraries(${example} PRIVATE dts::DeviceTrustShim)
    endforeach()
endif()

if(DTS_BUILD_TESTS)
    enable_testing()
    add_executable(test_audit_chain tests/test_audit_chain.cpp)
    target_link_libraries(test_audit_chain PRIVATE dts::DeviceTrustShim)
    add_test(NAME AuditChainTests COMMAND test_audit_chain)
endif()

# Install the headers and a CMake package, so find_package(DeviceTrustShim)
# gives other projects the dts::DeviceTrustShim target.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS DeviceTrustShim EXPORT DeviceTrustShimTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/DeviceTrustShim)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/DeviceTrustShimConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeviceTrustShimConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DeviceTrustShimConfig.cmake"
    INSTALL_DESTINATION ${ConfigPackageLocation})

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DeviceTrustShimConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DeviceTrustShimConfigVersion.cmake"
    DESTINATION ${ConfigPackageLocation})

install(EXPORT DeviceTrustShimTargets
    FILE DeviceTrustShimTargets.cmake
    NAMESPACE dts::
    DESTINATION ${ConfigPackageLocation})
