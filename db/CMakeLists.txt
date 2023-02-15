add_library(database database.cpp)

target_include_directories(database PUBLIC 
                ${CMAKE_SOURCE_DIR}/db_drivers/liblmdb)

target_link_libraries(database PUBLIC 
                lmdb)