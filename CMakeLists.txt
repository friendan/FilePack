cmake_minimum_required(VERSION 3.16)
project(FilePack)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # 静态链接CRT
set(RESOURCE_FILES App.rc) # 添加资源文件

add_compile_options("/utf-8") # 设置工程代码文件是UTF8编码
add_compile_options("/FS") # 允许多个编译器实例同时写入PDB文件
add_definitions(-DUNICODE -D_UNICODE) # 设置UNICODE字符集
add_definitions(-DLIBARCHIVE_STATIC) # libarchive 静态库


include_directories(ezui/include/EzUI) # 包含ezui头文件目录
file(GLOB EZUI_SOURCES "ezui/sources/*.cpp") # 收集ezui源文件

# ======== zlib 静态库 ========
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(zlib/zlib-1.3.2)

# ======== libarchive 静态库 ========
if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()
add_subdirectory(libarchive/libarchive-3.8.7)

# 添加项目源文件
set(PROJECT_SOURCES
    main.cpp
    MainForm.h
    MainForm.cpp
    AppConst.hpp
    AppUtil.hpp
    AppUtil.cpp
    PathUtil.hpp
    PathUtil.cpp
    FileListView.hpp
)


add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES} ${EZUI_SOURCES} ${RESOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE
    archive_static
    zlibstatic
    user32
    gdi32
    comctl32
    ole32
    oleaut32
    uuid
    d2d1
    dwrite
    windowscodecs
    shell32
)

# 忽略特定链接器警告
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "/ignore:4286 /ignore:4217"
)

# 确保资源文件依赖关系
set_property(SOURCE App.rc PROPERTY OBJECT_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Main.htm
    ${CMAKE_CURRENT_SOURCE_DIR}/FileListView.htm
)
