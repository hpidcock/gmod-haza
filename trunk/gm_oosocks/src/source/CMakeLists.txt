include_directories (${gm_oosocks_SOURCE_DIR}/include) 


add_library(gm_oosocks SHARED gm_oosocks.cpp Socket.cpp)

target_link_libraries(gm_oosocks Ws2_32)

