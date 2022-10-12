set(COMPONENTLIB_BUILD_TESTS ${TORCH_BUILD_TEST})

add_library(component INTERFACE
    include/componentlib/TableBase.h
    include/componentlib/Table.h
    include/componentlib/Table.inl
    include/componentlib/TableInternal.inl
    include/componentlib/TableIterators.h
    include/componentlib/TableIterators.inl
    include/componentlib/TableJoinIterator.h
    include/componentlib/TableUtils.h
    include/componentlib/ComponentStorage.h
)
target_include_directories(component INTERFACE include/)
target_link_libraries(component INTERFACE torch_util)
set_target_properties(component PROPERTIES LINKER_LANGUAGE CXX)

if (${COMPONENTLIB_BUILD_TESTS})
    add_subdirectory(test)
endif ()
