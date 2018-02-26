set(NGL_ANDROID_ENV $ENV{NGL_ANDROID_ENV}/${ANDROID_ABI})
set(LIB_PATH ${NGL_ANDROID_ENV}/lib)
set(HEADER_PATH ${NGL_ANDROID_ENV}/include)

cmake_minimum_required(VERSION 3.4.1)
add_library(nmd_native SHARED nmd.c)

include_directories(${HEADER_PATH})

add_library(nmd STATIC IMPORTED)
set_target_properties(nmd PROPERTIES IMPORTED_LOCATION ${LIB_PATH}/libnopemd.a)
add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${LIB_PATH}/libavfilter.so)
add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${LIB_PATH}/libswresample.so)
add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${LIB_PATH}/libswscale.so)
add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${LIB_PATH}/libavformat.so)
add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${LIB_PATH}/libavcodec.so)
add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${LIB_PATH}/libavutil.so)
add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION ${LIB_PATH}/libavdevice.so)

target_link_libraries(
        nmd_native
        log
        android
        EGL
        GLESv2
        OpenSLES
        m
        z
        nmd
        avfilter
        swresample
        swscale
        avformat
        avcodec
        avutil
        avdevice
)
