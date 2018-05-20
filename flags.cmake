set (CUTTLEBONE_SRC_DIR "${CMAKE_SOURCE_DIR}/../../cuttlebone")

add_subdirectory(${CUTTLEBONE_SRC_DIR} build)

list(APPEND app_link_libs
    cuttlebone)

list(APPEND app_include_dirs
    ${CUTTLEBONE_SRC_DIR})
