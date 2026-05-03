add_library(lsm_core
    lsm_tree.cpp
)
target_include_directories(lsm_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(lsm_core
    lsm_wal
    lsm_memtable
    lsm_sstable
    lsm_levels
    lsm_iterator
    lsm_compaction
)