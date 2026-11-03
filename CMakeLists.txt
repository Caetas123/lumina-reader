cmake_minimum_required(VERSION 3.24)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/project-version.txt"
)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/project-version.txt" LUMINA_READER_VERSION LIMIT_COUNT 1)
string(STRIP "${LUMINA_READER_VERSION}" LUMINA_READER_VERSION)
if(NOT LUMINA_READER_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "project-version.txt must contain one semantic version such as 0.2.0.")
endif()

project(LuminaReader VERSION "${LUMINA_READER_VERSION}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Network
    Qml
    Quick
    QuickControls2
    QuickDialogs2
    Sql
    Test
)

find_package(Qt6 6.5 QUIET COMPONENTS Pdf)
find_package(Qt6 6.5 QUIET COMPONENTS CorePrivate)
find_package(Qt6 6.5 QUIET COMPONENTS TextToSpeech)
find_path(LUMINA_LIBARCHIVE_INCLUDE_DIR NAMES archive.h)
find_library(LUMINA_LIBARCHIVE_LIBRARY NAMES archive libarchive)
find_path(LUMINA_DJVU_INCLUDE_DIR NAMES ddjvuapi.h PATH_SUFFIXES libdjvu)
find_library(LUMINA_DJVU_LIBRARY NAMES djvulibre ddjvu libddjvu)

qt_standard_project_setup(REQUIRES 6.5)

if(Qt6_VERSION VERSION_GREATER_EQUAL "6.8")
    qt_policy(SET QTP0004 NEW)
endif()

add_library(lumina_core STATIC
    src/application/AppController.cpp
    src/application/AppController.h
    src/application/BackupController.cpp
    src/application/BackupController.h
    src/application/LookupController.cpp
    src/application/LookupController.h
    src/application/ReaderController.cpp
    src/application/ReaderController.h
    src/application/SettingsController.cpp
    src/application/SettingsController.h
    src/application/SpeechController.cpp
    src/application/SpeechController.h
    src/domain/core/Error.h
    src/domain/lookup/LookupProvider.h
    src/domain/ocr/OcrProvider.h
    src/domain/reader/DocumentEngine.h
    src/infrastructure/database/Database.cpp
    src/infrastructure/database/Database.h
    src/infrastructure/archive/ArchiveReader.cpp
    src/infrastructure/archive/ArchiveReader.h
    src/infrastructure/library/LibraryStore.cpp
    src/infrastructure/library/LibraryStore.h
    src/infrastructure/logging/Logging.cpp
    src/infrastructure/logging/Logging.h
    src/infrastructure/ocr/TesseractOcrProvider.cpp
    src/infrastructure/ocr/TesseractOcrProvider.h
    src/infrastructure/reader/DocumentEngineFactory.cpp
    src/infrastructure/reader/DocumentEngineFactory.h
    src/infrastructure/reader/DjvuEngine.cpp
    src/infrastructure/reader/DjvuEngine.h
    src/infrastructure/text/TextDecoder.cpp
    src/infrastructure/text/TextDecoder.h
    src/pdf/PdfEngine.h
)

target_include_directories(lumina_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(lumina_core
    PUBLIC
        LUMINA_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(lumina_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Sql
)

if(TARGET Qt6::Pdf)
    target_compile_definitions(lumina_core PUBLIC LUMINA_HAS_QT_PDF=1)
    target_link_libraries(lumina_core PUBLIC Qt6::Pdf)
endif()

if(TARGET Qt6::CorePrivate)
    target_compile_definitions(lumina_core PUBLIC LUMINA_HAS_QT_ZIP=1)
    target_link_libraries(lumina_core PUBLIC Qt6::CorePrivate)
endif()

if(TARGET Qt6::TextToSpeech)
    target_compile_definitions(lumina_core PUBLIC LUMINA_HAS_QT_SPEECH=1)
    target_link_libraries(lumina_core PUBLIC Qt6::TextToSpeech)
endif()

if(LUMINA_LIBARCHIVE_INCLUDE_DIR AND LUMINA_LIBARCHIVE_LIBRARY)
    target_compile_definitions(lumina_core PUBLIC LUMINA_HAS_LIBARCHIVE=1)
    target_include_directories(lumina_core PUBLIC "${LUMINA_LIBARCHIVE_INCLUDE_DIR}")
    target_link_libraries(lumina_core PUBLIC "${LUMINA_LIBARCHIVE_LIBRARY}")
endif()

if(LUMINA_DJVU_INCLUDE_DIR AND LUMINA_DJVU_LIBRARY)
    target_compile_definitions(lumina_core PUBLIC LUMINA_HAS_DJVU=1)
    target_include_directories(lumina_core PUBLIC "${LUMINA_DJVU_INCLUDE_DIR}")
    target_link_libraries(lumina_core PUBLIC "${LUMINA_DJVU_LIBRARY}")
endif()

qt_add_executable(lumina_reader
    src/main.cpp
)

set_source_files_properties(
    src/ui/qml/Theme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(lumina_reader
    URI LuminaReader
    VERSION 1.0
    QML_FILES
        src/ui/qml/Main.qml
        src/ui/qml/Theme.qml
    RESOURCES
        assets/lumina-logo.png
)

if(WIN32)
    set(LUMINA_WINDOWS_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/lumina-reader.ico")
    configure_file(
        packaging/windows/lumina-reader.rc.in
        "${CMAKE_CURRENT_BINARY_DIR}/lumina-reader.rc"
        @ONLY
    )
    target_sources(lumina_reader PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/lumina-reader.rc")
endif()

target_link_libraries(lumina_reader
    PRIVATE
        lumina_core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickDialogs2
)

set_target_properties(lumina_reader PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Lumina Reader"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.caetas.luminareader"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    WIN32_EXECUTABLE TRUE
)

include(CTest)

if(BUILD_TESTING)
    qt_add_executable(lumina_smoke_tests
        tests/unit/smoke_test.cpp
    )

    target_link_libraries(lumina_smoke_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_smoke_tests COMMAND lumina_smoke_tests)

    qt_add_executable(lumina_database_tests
        tests/unit/database_test.cpp
    )

    target_link_libraries(lumina_database_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_database_tests COMMAND lumina_database_tests)

    qt_add_executable(lumina_lookup_tests
        tests/unit/lookup_test.cpp
    )

    target_link_libraries(lumina_lookup_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_lookup_tests COMMAND lumina_lookup_tests)

    qt_add_executable(lumina_library_tests
        tests/unit/library_test.cpp
    )

    target_link_libraries(lumina_library_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_library_tests COMMAND lumina_library_tests)

    qt_add_executable(lumina_reader_tests
        tests/unit/reader_test.cpp
    )

    target_link_libraries(lumina_reader_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_reader_tests COMMAND lumina_reader_tests)

    qt_add_executable(lumina_archive_tests
        tests/unit/archive_test.cpp
    )

    target_link_libraries(lumina_archive_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_archive_tests COMMAND lumina_archive_tests)

    qt_add_executable(lumina_settings_tests
        tests/unit/settings_test.cpp
    )

    target_link_libraries(lumina_settings_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_settings_tests COMMAND lumina_settings_tests)

    qt_add_executable(lumina_backup_tests
        tests/unit/backup_test.cpp
    )

    target_link_libraries(lumina_backup_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_backup_tests COMMAND lumina_backup_tests)

    qt_add_executable(lumina_ocr_tests
        tests/unit/ocr_test.cpp
    )

    target_link_libraries(lumina_ocr_tests
        PRIVATE
            lumina_core
            Qt6::Test
    )

    add_test(NAME lumina_ocr_tests COMMAND lumina_ocr_tests)
endif()

include(GNUInstallDirs)

install(TARGETS lumina_reader
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
