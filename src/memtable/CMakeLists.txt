add_library(lsm_memtable
    memtable.cpp
)
target_include_directories(lsm_memtable PUBLIC ${PROJECT_SOURCE_DIR}/include)