cmake_minimum_required(VERSION 3.22)
project(rinha_de_backend C)

set(CMAKE_C_STANDARD 23)

find_package(PkgConfig REQUIRED)

# ── MHD (libmicrohttpd) build ──
pkg_check_modules(MHD REQUIRED libmicrohttpd)

add_executable(rinha_de_backend src/main.c src/server.c src/routes.c src/transaction.c src/parse.c src/fraud.c src/dataset.c src/features.c)

target_include_directories(rinha_de_backend PRIVATE src ${MHD_INCLUDE_DIRS})
target_link_directories(rinha_de_backend PRIVATE ${MHD_LIBRARY_DIRS})
target_link_libraries(rinha_de_backend PRIVATE ${MHD_LIBRARIES})

# ── h2o build ──
# h2o.h transitively includes openssl/ssl.h and libuv headers.
set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/h2o/lib/pkgconfig:/opt/homebrew/opt/openssl@3/lib/pkgconfig:/opt/homebrew/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
pkg_check_modules(H2O REQUIRED libh2o-evloop)
pkg_check_modules(SSL REQUIRED openssl)
pkg_check_modules(UV REQUIRED libuv)

add_executable(rinha_de_backend_h2o src/main_h2o.c src/server_h2o.c src/routes_h2o.c src/transaction.c src/parse.c src/fraud.c src/dataset.c src/features.c)

target_include_directories(rinha_de_backend_h2o PRIVATE src ${H2O_INCLUDE_DIRS} ${SSL_INCLUDE_DIRS} ${UV_INCLUDE_DIRS})
target_link_directories(rinha_de_backend_h2o PRIVATE ${H2O_LIBRARY_DIRS} ${SSL_LIBRARY_DIRS} ${UV_LIBRARY_DIRS})
target_link_libraries(rinha_de_backend_h2o PRIVATE ${H2O_LIBRARIES} ${SSL_LIBRARIES} ${UV_LIBRARIES})

if(APPLE)
    target_link_libraries(rinha_de_backend_h2o PRIVATE m)
endif()
