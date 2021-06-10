cmake_minimum_required(VERSION 3.13)
project(UtilLib VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# ------------------ #
#       Options      #
# ------------------ #

option(INTEGRATE_GLM "Integrate utilities for glm library" ON)


# ------------------ #
#       Library      #
# ------------------ #

add_library(TorchUtilLib)

add_subdirectory(src/data)
add_subdirectory(src/functional)
add_subdirectory(src/async)
add_subdirectory(test)

target_include_directories(TorchUtilLib
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/include/nc
)

target_compile_options(TorchUtilLib PRIVATE -Wall -Wextra -Wpedantic)
target_compile_features(TorchUtilLib PUBLIC cxx_std_20)
target_compile_definitions(TorchUtilLib PUBLIC
    NC_USE_GLM=${INTEGRATE_GLM}
)


# ------------------ #
#       Install      #
# ------------------ #

include(GNUInstallDirs)
install(TARGETS TorchUtilLib
    EXPORT mycpputils-export
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY
    include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/TorchUtilLib)
install(FILES
    ${CMAKE_CURRENT_LIST_DIR}/cmake/TorchUtilLibConfig.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)


install(EXPORT mycpputils-export
    FILE
        TorchUtilLibTargets.cmake
    NAMESPACE
        TorchUtilLib::
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/TorchUtilLib
)
