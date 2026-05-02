add_library(lsm_levels
    level_manager.cpp
)
target_include_directories(lsm_levels PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(lsm_levels lsm_sstable)
