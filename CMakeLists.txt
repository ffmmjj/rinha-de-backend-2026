cmake_minimum_required(VERSION 3.22)
project(rinha_de_backend C)

set(CMAKE_C_STANDARD 23)

find_package(PkgConfig REQUIRED)

# h2o.h transitively includes openssl/ssl.h.
# We force H2O_USE_LIBUV=0 in the source files, so libuv is not needed.
set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/h2o/lib/pkgconfig:/opt/homebrew/opt/openssl@3/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
pkg_check_modules(H2O REQUIRED libh2o-evloop)
pkg_check_modules(SSL REQUIRED openssl)

add_executable(rinha_de_backend src/main.c src/server.c src/routes.c src/transaction.c src/parse.c src/fraud.c src/dataset.c src/features.c)

target_include_directories(rinha_de_backend PRIVATE src ${H2O_INCLUDE_DIRS} ${SSL_INCLUDE_DIRS})
target_link_directories(rinha_de_backend PRIVATE ${H2O_LIBRARY_DIRS} ${SSL_LIBRARY_DIRS})
target_link_libraries(rinha_de_backend PRIVATE ${H2O_LIBRARIES} ${SSL_LIBRARIES})

if(APPLE)
    target_link_libraries(rinha_de_backend PRIVATE m)
endif()
