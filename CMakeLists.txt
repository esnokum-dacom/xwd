cmake_minimum_required(VERSION 3.15)
project(xwd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb xcb-xinerama xcb-shm)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale)

set(DAEMON_SRC
    src/config.cpp
    src/image.cpp
    src/ipc_server.cpp
    src/main.cpp
    src/monitors.cpp
    src/root_atoms.cpp
    src/stb_image_impl.cpp
    src/wallpaper.cpp
    src/wallpaper_v.cpp
    src/xcb_pxf.cpp
    src/shm/xcb_shm_check.cpp
    src/shm/shm_seg.cpp
)

set(CTL_SRC
    src/xwdctl.cpp
)

add_executable(xwd ${DAEMON_SRC})
target_include_directories(xwd PRIVATE external-dependencies)
target_link_libraries(xwd PRIVATE 
    PkgConfig::XCB 
    PkgConfig::FFMPEG
    Threads::Threads
)

add_executable(xwdctl ${CTL_SRC})
target_include_directories(xwdctl PRIVATE external-dependencies)

install(TARGETS xwd xwdctl
    RUNTIME DESTINATION bin
)

configure_file(
    ${CMAKE_SOURCE_DIR}/services/systemd/xwd.service.in
    ${CMAKE_BINARY_DIR}/xwd.service
    @ONLY
)
install(FILES
    ${CMAKE_BINARY_DIR}/xwd.service
    DESTINATION lib/systemd/user
)

configure_file(
    ${CMAKE_SOURCE_DIR}/services/runit/run.in
    ${CMAKE_BINARY_DIR}/run
    @ONLY
)
install(PROGRAMS
    ${CMAKE_BINARY_DIR}/run
    DESTINATION etc/sv/xwd
)
