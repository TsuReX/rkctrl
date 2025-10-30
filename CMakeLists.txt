## Указание минимальной необходимой версии cmake для сборки проекта

cmake_minimum_required(VERSION 3.5)


set(PROJ_NAME rkctrl)

set(MAIN_SOURCES "${MAIN_SOURCES}" "rkctrl.c")
set(INCLUDE "${INCLUDE}" ".")

## Предотвращение проверки работоспособности компилятора
## (необходимо для избежания проблемы при кросс-сборке)
set(CMAKE_C_COMPILER_WORKS 1)

## Указание используемых в проекте языков
project(${PROJ_NAME} C)

message("-- MAIN_SOURCES ${MAIN_SOURCES}")
set(CMAKE_EXECUTABLE_SUFFIX .elf )

include_directories(${INCLUDE})

message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")

add_executable(${PROJ_NAME} ${MAIN_SOURCES})

#target_link_libraries(${PROJ_NAME} ${LINKER_LIBS} ${LINKER_FLAGS} ${LIBS})
