cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(War3Translator VERSION 0.1 LANGUAGES CXX)

add_compile_definitions(_WIN32_WINNT=0x0601 WINVER=0x0601)
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Quick Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick Widgets Network)

# 主程序和注入 DLL 共用的源文件清单
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

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
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
    Qt${QT_VERSION_MAJOR}::Network
    wbemuuid
    ole32
    oleaut32
)

if(QT_VERSION VERSION_LESS 6.1.0)
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
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(War3Translator)
    qt_finalize_executable(War3Translator)
endif()

# 注入游戏进程的功能 DLL
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
set_property(TARGET Translator PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 主程序会在运行时加载该 DLL，因此构建主程序时也必须先生成它
add_dependencies(War3Translator Translator)
install(TARGETS Translator
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR})

set(TRANSLATOR_FUNC_LIST
    "getOriginalGameChatInputLogicInGame"
    "isWar3TranslatorInitialized"
    "uninstallAllHooks"
    "installAllHooks"
    "getHookStatus"
    "initialize"
)

string(REPLACE ";" "\n\t" TRANSLATOR_EXPORTS "${TRANSLATOR_FUNC_LIST}")
set(TRANSLATOR_DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/translator.def")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/translator/translator.def.in
    ${TRANSLATOR_DEF_FILE}
    @ONLY
)

if(MSVC)
    target_compile_options(Translator PRIVATE
        $<$<CONFIG:Release>:/O1>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/Ob2>
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/RTC1>
        "/EHa"
    )

    target_link_options(Translator PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        "/DEF:${TRANSLATOR_DEF_FILE}"
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_sources(Translator PRIVATE "${TRANSLATOR_DEF_FILE}")
    target_link_options(Translator PRIVATE
        "-Wl,--enable-stdcall-fixup"
        "-Wl,--kill-at"
    )
endif()

# 将运行所需的 OpenSSL DLL 复制到主程序目录
set(OPENSSL_DLLS
    "${CMAKE_CURRENT_SOURCE_DIR}/openssl/libcrypto-1_1.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/openssl/libssl-1_1.dll"
)

foreach(DLL_PATH ${OPENSSL_DLLS})
    add_custom_command(TARGET War3Translator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL_PATH}"
        "$<TARGET_FILE_DIR:War3Translator>"
        COMMENT "正在复制 OpenSSL 运行库..."
    )
endforeach()
install(FILES ${OPENSSL_DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR})

# MinGW 程序需要随包提供 GCC 运行库，避免依赖开发环境的 PATH
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(MINGW_RUNTIME_NAMES
        libgcc_s_dw2-1.dll
        libgcc_s_seh-1.dll
        libgcc_s_sjlj-1.dll
        libstdc++-6.dll
        libwinpthread-1.dll
    )

    foreach(RUNTIME_NAME IN LISTS MINGW_RUNTIME_NAMES)
        find_file(MINGW_RUNTIME_DLL
            NAMES "${RUNTIME_NAME}"
            PATHS "${MINGW_BIN_DIR}"
            NO_DEFAULT_PATH
        )
        if(MINGW_RUNTIME_DLL)
            foreach(TARGET_NAME War3Translator Translator)
                add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINGW_RUNTIME_DLL}"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>"
                    COMMENT "正在复制 MinGW 运行库 ${RUNTIME_NAME}..."
                )
            endforeach()
            install(FILES "${MINGW_RUNTIME_DLL}" DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
        unset(MINGW_RUNTIME_DLL CACHE)
        unset(MINGW_RUNTIME_DLL)
    endforeach()
endif()

# 部署 Qt 运行库、平台插件和 QML 依赖，避免启动时依赖开发环境
if(WIN32)
    if(TARGET Qt${QT_VERSION_MAJOR}::qmake)
        get_target_property(_qt_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    elseif(DEFINED QT_QMAKE_EXECUTABLE)
        set(_qt_qmake_executable "${QT_QMAKE_EXECUTABLE}")
    endif()

    if(_qt_qmake_executable)
        get_filename_component(_qt_bin_dir "${_qt_qmake_executable}" DIRECTORY)
    endif()
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt.exe windeployqt
        HINTS "${_qt_bin_dir}"
    )
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "未找到 windeployqt，请检查当前 Qt 工具链安装。")
    endif()

    set(_qt_deploy_options)
    set(_qt_deploy_path "$ENV{PATH}")
    set(_qt_install_deploy_path "${_qt_bin_dir}")
    if(MSVC)
        list(APPEND _qt_deploy_options --no-compiler-runtime)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        get_filename_component(_mingw_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(_qt_deploy_path "${_qt_bin_dir};${_mingw_bin_dir};${_qt_deploy_path}")
        set(_qt_install_deploy_path "${_qt_bin_dir};${_mingw_bin_dir}")
    endif()

    add_custom_command(TARGET War3Translator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${_qt_deploy_path}"
            "${WINDEPLOYQT_EXECUTABLE}"
            ${_qt_deploy_options}
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}"
            "$<TARGET_FILE:War3Translator>"
        COMMENT "正在部署 Qt 运行库和插件..."
        VERBATIM
    )

    install(CODE "
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E env
                \"PATH=${_qt_install_deploy_path}\"
                \"${WINDEPLOYQT_EXECUTABLE}\"
                ${_qt_deploy_options}
                --qmldir \"${CMAKE_CURRENT_SOURCE_DIR}\"
                \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/War3Translator.exe\"
            RESULT_VARIABLE _qt_deploy_result
        )
        if(NOT _qt_deploy_result EQUAL 0)
            message(FATAL_ERROR \"windeployqt 安装部署失败: \${_qt_deploy_result}\")
        endif()
    ")
endif()

