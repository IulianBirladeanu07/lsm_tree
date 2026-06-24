include(FetchContent)

FetchContent_Declare(
    sqlite3
    URL      https://sqlite.org/2024/sqlite-amalgamation-3460100.zip
    URL_HASH SHA256=77823cb110929c2bcb0f5d48e4833b5c59a8a6e40cdea3936b99e199dbbe5784
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite3)

add_library(sqlite3_bundled STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3_bundled PUBLIC ${sqlite3_SOURCE_DIR})
target_compile_options(sqlite3_bundled PRIVATE -DSQLITE_THREADSAFE=1)

add_executable(lsm_bench main.cpp)
target_include_directories(lsm_bench PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(lsm_bench lsm_core sqlite3_bundled)
target_compile_options(lsm_bench PRIVATE -O2 -msse4.2)

add_executable(demo demo.cpp)
target_include_directories(demo PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(demo lsm_core)
target_compile_options(demo PRIVATE -O2 -msse4.2)

add_executable(demo_recovery demo_recovery.cpp)
target_include_directories(demo_recovery PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(demo_recovery lsm_core)
target_compile_options(demo_recovery PRIVATE -O2 -msse4.2)