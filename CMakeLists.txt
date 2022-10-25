# ���������� ��������� ������ Cmake
cmake_minimum_required(VERSION 3.0)


# ----  ����� ��������� -----
# �������� ������� � �������� ��������� �����
project(drs_server LANGUAGES C)
# ���� ������ ��������� ������� ������ � ���������� ����������� � ����������
include(sources/dap-sdk/cmake/ReadMKFile.cmake)
# ����� ���������
set(CMAKE_ENABLE_EXPORTS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)
set(CMAKE_C_STANDARD 11)

SET( CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET( CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET( CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
add_definitions("-DDAP_VERSION=\"${VERSION_MAJOR}-${VERSION_MINOR}.${VERSION_PATCH}\"")

# ������ � ������������ ����������� � ������������ ��������� �������� ������ ����������
# � ��� �� ����������� ������ ��� ������ � ��� �������
include(sources/dap-sdk/cmake/OS_Detection.cmake)
# ������������ ������� DAP SDK ������� ��� �����
set(DAPSDK_MODULES "core crypto io app-cli")
# ����������� DAP SDK
add_subdirectory(sources/dap-sdk)

# ���������� ���������� DRS
add_subdirectory(sources/drs)

# ---- ��������������� ����������� ������ ������� ---

# ������
set (SERVER_SOURCES
    sources/main_server.c
    sources/sig_unix_handler.c
    sources/eth_srv.c
    )
set (SERVER_HEADERS
    sources/eth_srv.h
    sources/sig_unix_handler.h
    )
# ��������� � �������
set (CLI_SOURCES
    sources/main_cli.c
    )

# �����
set (TESTS_SOURCES
    sources/main_server.c
    )
set (TESTS_HEADERS
    sources/main.c
    )


#-- ���������� ����������� ���� � ���������� � ����
add_executable(${PROJECT_NAME} ${SERVER_SOURCES} ${SERVER_HEADERS})
target_link_libraries(${PROJECT_NAME} dap_core dap_crypto dap_io dap_cli_server drs)

add_executable(${PROJECT_NAME}_cli ${CLI_SOURCES})
target_link_libraries(${PROJECT_NAME}_cli dap_core dap_app_cli)

#--- ��������� �������� ��� �����������
set(CMAKE_INSTALL_PREFIX /opt/drs_server)
set(BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
set(CONFIGDIR ${CMAKE_INSTALL_PREFIX}/etc)
set(SBINDIR ${CMAKE_INSTALL_PREFIX}/sbin)
set(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(RLIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(SHAREDIR ${CMAKE_INSTALL_PREFIX}/share)

INSTALL(DIRECTORY dist/ DESTINATION ${SHAREDIR} FILES_MATCHING PATTERN "*" PATTERN "*")
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(TARGETS ${PROJECT_NAME}_cli DESTINATION ${BINDIR})