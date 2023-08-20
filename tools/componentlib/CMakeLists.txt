option(COMPONENTLIB_BUILD_TESTS "Built tests for componentlib" ON)

add_library(component INTERFACE
    include/componentlib/ComponentBase.h
    include/componentlib/ComponentID.h
    include/componentlib/ComponentStorage.h
    include/componentlib/IndirectTableImpl.h
    include/componentlib/IndirectTableImplIterators.h
    include/componentlib/StableTableImpl.h
    include/componentlib/StableTableImplIterators.h
    include/componentlib/Table.h
    include/componentlib/TableBase.h
    include/componentlib/TableIterators.h
    include/componentlib/TableJoinIterator.h
    include/componentlib/TableUtils.h
)

target_include_directories(component INTERFACE include/)
target_link_libraries(component INTERFACE torch_util)
set_target_properties(component PROPERTIES LINKER_LANGUAGE CXX)

if (${COMPONENTLIB_BUILD_TESTS})
    add_subdirectory(test)
endif ()
