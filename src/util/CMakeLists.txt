add_library(lsm_util
    bloom_filter.cpp
    crc32.cpp
)
target_include_directories(lsm_util PUBLIC ${PROJECT_SOURCE_DIR}/include)