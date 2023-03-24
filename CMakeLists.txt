# Минимально потребная версия Cmake
cmake_minimum_required(VERSION 3.10)


# ----  Общие настройки -----
# Название проекта и указание основного языка
project(drs_server LANGUAGES C)
# Этот инклуд реализует парсинг файлов и всасывание результатов в переменную
include(dap-sdk/cmake/ReadMKFile.cmake)
# Общие настройки
set(CMAKE_ENABLE_EXPORTS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)
set(CMAKE_C_STANDARD 11)

SET( CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET( CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET( CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
add_definitions("-DDAP_VERSION=\"${VERSION_MAJOR}-${VERSION_MINOR}.${VERSION_PATCH}\"")

# Инклуд с определением операционки и выставлением различных дефайнов общего назначения
# А так же выставление флагов для релиза и для отладки
include(dap-sdk/cmake/OS_Detection.cmake)
# Перечисление модулей DAP SDK которые нам нужны
set(DAPSDK_MODULES "core crypto io app-cli")
# Подключение DAP SDK
add_subdirectory(dap-sdk)

# Подключаем библиотеку DRS
add_subdirectory(libs/libdrs)

# ---- Непосредственно подключение файлов проекта ---

# Сервер
set (SERVER_SOURCES
     sources/main_server.c
    )
# Консолька к серверу
set (CLI_SOURCES
    sources/main_cli.c
    )

#-- Определяем исполняемый файл и библиотеки к нему
add_executable(drs_server ${SERVER_SOURCES} ${SERVER_HEADERS})
target_link_libraries(${PROJECT_NAME} dap_sdk_static libdrs_static)

add_executable(drs_client ${CLI_SOURCES})
target_link_libraries(drs_client dap_sdk_static dap_app_cli)

#--- Описываем действия при инсталляции
set(CMAKE_INSTALL_PREFIX /opt/drs_server)
set(BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
set(CONFIGDIR ${CMAKE_INSTALL_PREFIX}/etc)
set(SBINDIR ${CMAKE_INSTALL_PREFIX}/sbin)
set(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(RLIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(SHAREDIR ${CMAKE_INSTALL_PREFIX}/share)

INSTALL(DIRECTORY dist/ DESTINATION ${SHAREDIR} FILES_MATCHING PATTERN "*" PATTERN "*")
INSTALL(TARGETS drs_server DESTINATION ${BINDIR})
INSTALL(TARGETS drs_client DESTINATION ${BINDIR})
