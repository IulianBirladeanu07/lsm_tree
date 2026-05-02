add_library(lsm_iterator
    merging_iterator.cpp
)

target_include_directories(lsm_iterator PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(lsm_iterator lsm_sstable)