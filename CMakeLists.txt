cmake_minimum_required(VERSION 3.22)
project(rinha_de_backend C)

set(CMAKE_C_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MHD REQUIRED libmicrohttpd)

add_executable(rinha_de_backend src/main.c src/server.c src/routes.c src/transaction.c src/parse.c src/fraud.c)

target_include_directories(rinha_de_backend PRIVATE src ${MHD_INCLUDE_DIRS})
target_link_directories(rinha_de_backend PRIVATE ${MHD_LIBRARY_DIRS})
target_link_libraries(rinha_de_backend PRIVATE ${MHD_LIBRARIES})
