include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(test_bloom_filter test_bloom_filter.cpp)
target_include_directories(test_bloom_filter PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_bloom_filter lsm_util GTest::gtest_main)
gtest_discover_tests(test_bloom_filter)

add_executable(test_block_builder test_block_builder.cpp)
target_include_directories(test_block_builder PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_block_builder lsm_sstable GTest::gtest_main)
gtest_discover_tests(test_block_builder)

add_executable(test_block test_block.cpp)
target_include_directories(test_block PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_block lsm_sstable GTest::gtest_main)
gtest_discover_tests(test_block)

add_executable(test_sstable test_sstable.cpp)
target_include_directories(test_sstable PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_sstable lsm_sstable GTest::gtest_main)
gtest_discover_tests(test_sstable)

add_executable(test_wal test_wal.cpp)
target_include_directories(test_wal PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_wal lsm_wal GTest::gtest_main)
gtest_discover_tests(test_wal)

add_executable(test_concurrent_skip_list test_concurrent_skip_list.cpp)
target_include_directories(test_concurrent_skip_list PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_concurrent_skip_list lsm_memtable GTest::gtest_main)
gtest_discover_tests(test_concurrent_skip_list)

add_executable(test_memtable test_memtable.cpp)
target_include_directories(test_memtable PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_memtable lsm_memtable GTest::gtest_main)
gtest_discover_tests(test_memtable)

add_executable(test_block_cache test_block_cache.cpp)
target_include_directories(test_block_cache PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_block_cache lsm_sstable GTest::gtest_main)
gtest_discover_tests(test_block_cache)

add_executable(test_level_manager test_level_manager.cpp)
target_include_directories(test_level_manager PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_level_manager lsm_levels GTest::gtest_main)
gtest_discover_tests(test_level_manager)

add_executable(test_merging_iterator test_merging_iterator.cpp)
target_include_directories(test_merging_iterator PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_merging_iterator lsm_iterator GTest::gtest_main)
gtest_discover_tests(test_merging_iterator)

add_executable(test_thread_pool test_thread_pool.cpp)
target_include_directories(test_thread_pool PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_thread_pool lsm_compaction GTest::gtest_main)
gtest_discover_tests(test_thread_pool)

add_executable(test_sstable_iterator test_sstable_iterator.cpp)
target_include_directories(test_sstable_iterator PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_sstable_iterator lsm_iterator GTest::gtest_main)
gtest_discover_tests(test_sstable_iterator)

add_executable(test_block_iterator test_block_iterator.cpp)
target_include_directories(test_block_iterator PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_block_iterator lsm_iterator GTest::gtest_main)
gtest_discover_tests(test_block_iterator)

add_executable(test_compaction_strategy test_compaction_strategy.cpp)
target_include_directories(test_compaction_strategy PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_compaction_strategy lsm_compaction lsm_levels GTest::gtest_main)
gtest_discover_tests(test_compaction_strategy)

add_executable(test_lsm_tree test_lsm_tree.cpp)
target_include_directories(test_lsm_tree PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_lsm_tree lsm_core GTest::gtest_main)
gtest_discover_tests(test_lsm_tree)
