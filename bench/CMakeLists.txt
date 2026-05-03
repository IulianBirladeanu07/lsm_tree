find_package(SQLite3 REQUIRED)

add_executable(lsm_bench main.cpp)

target_include_directories(lsm_bench PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(lsm_bench
    lsm_core
    SQLite3::SQLite3
)

target_compile_options(lsm_bench PRIVATE -O2 -msse4.2)