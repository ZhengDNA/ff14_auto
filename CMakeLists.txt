cmake_minimum_required(VERSION 3.19)
project(ff14_auto)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SourceFiles source/app.rc)

add_executable(
        ff14_auto
        MACOSX_BUNDLE
        main.cpp
        ${SourceFiles}
        ${HeaderFiles}
        ${ResourceFiles}
        ${UISrcs}
)
