add_library(component STATIC
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
torch_default_compile_options(component)
target_include_directories(component PUBLIC include/)
target_link_libraries(component PUBLIC torch_util)
set_target_properties(component PROPERTIES LINKER_LANGUAGE CXX)
