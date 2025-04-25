cmake_minimum_required(VERSION 3.28.0)

project(chadtp C)

set(SOURCES "http.c")

add_executable(${PROJECT_NAME} ${SOURCES})
