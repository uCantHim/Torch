get_filename_component(NicosCppUtils_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

#include(CMakeFindDependencyMacro)

if (NOT TARGET NicosCppUtils::NicosCppUtils)
    include("${NicosCppUtils_CMAKE_DIR}/NicosCppUtilsTargets.cmake")
endif ()
