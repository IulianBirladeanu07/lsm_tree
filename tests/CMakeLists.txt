add_executable(test_bloom_filter test_bloom_filter.cpp)
target_link_libraries(test_bloom_filter lsm_util)

add_executable(test_block_builder test_block_builder.cpp)
target_link_libraries(test_block_builder lsm_sstable)

add_executable(test_block test_block.cpp)
target_link_libraries(test_block lsm_sstable)

add_executable(test_sstable test_sstable.cpp)
target_link_libraries(test_sstable lsm_sstable)

add_executable(test_wal test_wal.cpp)
target_link_libraries(test_wal lsm_wal)

add_executable(test_concurrent_skip_list test_concurrent_skip_list.cpp)
target_link_libraries(test_concurrent_skip_list lsm_memtable)

add_executable(test_memtable test_memtable.cpp)
target_link_libraries(test_memtable lsm_memtable)