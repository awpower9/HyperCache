cmake_minimum_required(VERSION 3.10)
project(mini-redis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Server Executable ----
add_executable(server
    server.cpp
    hashtable.cpp
    avl.cpp
    zset.cpp
    thread_pool.cpp
)

# ---- Client Executable ----
add_executable(client
    client.cpp
)

# ---- Platform-specific linking ----
if(WIN32)
    # Link Winsock2 library on Windows
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
endif()

# ---- Compiler warnings ----
if(MSVC)
    target_compile_definitions(server PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(client PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(server PRIVATE /W3)
    target_compile_options(client PRIVATE /W3)
else()
    target_compile_options(server PRIVATE -Wall -Wextra)
    target_compile_options(client PRIVATE -Wall -Wextra)
endif()
