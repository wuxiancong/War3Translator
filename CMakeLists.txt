cmake_minimum_required(VERSION 3.16)

project(War3Translator VERSION 0.1 LANGUAGES CXX)
if(MSVC)
    add_compile_options(/utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
add_definitions(-D_WIN32_WINNT=0x0601 -DWINVER=0x0601)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Quick Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick Widgets Network)

set(PROJECT_SOURCES
    main.cpp
    qml.qrc

    SingleApplication.h
    SingleApplication.cpp

    workers/DebugWorker.h
    workers/DebugWorker.cpp

    monitors/GameMonitor.h
    monitors/GameMonitor.cpp

    helpers/FileHelper.h
    helpers/FileHelper.cpp
    helpers/DebugHelper.h
    helpers/DebugHelper.cpp
    helpers/ProcessHelper.h
    helpers/ProcessHelper.cpp
    helpers/HardwareHelper.h
    helpers/HardwareHelper.cpp

    managers/IpcManager.h
    managers/IpcManager.cpp
    managers/HookManager.h
    managers/HookManager.cpp
    managers/ImageManager.h
    managers/ImageManager.cpp
    managers/NetworkManager.h
    managers/NetworkManager.cpp
    managers/SettingsManager.h
    managers/SettingsManager.cpp
    managers/TranslateManager.h
    managers/TranslateManager.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(War3Translator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(War3Translator
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(War3Translator
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Widgets
    wbemuuid
    ole32
    oleaut32
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.War3Translator)
endif()

set_target_properties(War3Translator PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}TRANSLATOR_API
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS War3Translator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(War3Translator)
    qt_finalize_executable(War3Translator)
endif()

# --- War3Translator ---
add_library(Translator SHARED
    logger/war3log.h
    logger/war3log.cpp
    logger/war3loga.h
    logger/war3loga.cpp
    shared/SharedMemory.h
    translator/Translator.h
    translator/Translator.cpp
)
target_link_libraries(Translator PRIVATE psapi version)
set_target_properties(Translator PROPERTIES OUTPUT_NAME "translator" PREFIX "")
target_compile_definitions(Translator PRIVATE TRANSLATOR_EXPORTS)
set(TRANSLATOR_FUNC_LIST
    "isWar3TranslatorInitialized"
    "uninstallAllHooks"
    "installAllHooks"
    "getHookStatus"
    "initialize"
)

string(REPLACE ";" "\n\t" TRANSLATOR_EXPORTS "${TRANSLATOR_FUNC_LIST}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/translator/translator.def.in
    ${CMAKE_CURRENT_BINARY_DIR}/translator/translator.def
    @ONLY
)

if(MSVC)
    set_property(TARGET Translator PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    target_compile_options(Translator PRIVATE
        $<$<CONFIG:Release>:/O1 /GL /Ob2>
        $<$<CONFIG:Debug>:/Od /RTC1>
        "/EHa"
    )

target_link_options(Translator PRIVATE
    $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    "/DEF:${CMAKE_CURRENT_BINARY_DIR}/translator/Translator.def"
)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_options(Translator PRIVATE
        "-Wl,--output-def=${CMAKE_CURRENT_BINARY_DIR}/translator/Translator.def"
        "-Wl,--enable-stdcall-fixup"
        "-Wl,--kill-at"
    )
endif()

# 专门处理 OpenSSL DLL，确保它们在 .exe 旁边
set(OPENSSL_DLLS
    "${CMAKE_CURRENT_SOURCE_DIR}/openssl/libcrypto-1_1.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/openssl/libssl-1_1.dll"
)

# 确保在 War3Translator 编译成功后执行复制
foreach(DLL_PATH ${OPENSSL_DLLS})
    add_custom_command(TARGET War3Translator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL_PATH}"
        "$<TARGET_FILE_DIR:War3Translator>"
        COMMENT "正在复制 OpenSSL 依赖库到输出目录..."
    )
endforeach()