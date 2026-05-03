add_library(lsm_compaction
    thread_pool.cpp
    leveled_compaction.cpp
    tiered_compaction.cpp
    compaction_scheduler.cpp
)

target_include_directories(lsm_compaction PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(lsm_compaction lsm_levels lsm_iterator)