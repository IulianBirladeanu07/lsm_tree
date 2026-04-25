add_library(lsm_wal
    wal.cpp
)
target_include_directories(lsm_wal PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(lsm_wal lsm_util)