cmake_minimum_required(VERSION 3.15)

# Nom du projet
project(SmartCity VERSION 1.0 LANGUAGES CXX)

# Configuration de la version C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options de compilation
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)  # Pour les DLL sous Windows

# Trouver les packages nécessaires
# Custom Raylib detection
set(RAYLIB_PATH "C:/raylib")
if(EXISTS "${RAYLIB_PATH}/include/raylib.h")
    message(STATUS "Using custom Raylib path: ${RAYLIB_PATH}")
    include_directories("${RAYLIB_PATH}/include")
    link_directories("${RAYLIB_PATH}/lib")
    set(RAYLIB_LIB raylib)
else()
    find_package(raylib CONFIG REQUIRED)
    set(RAYLIB_LIB raylib)
endif()

# Inclure les répertoires d'en-têtes
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/TrafficCore/src
    ${CMAKE_SOURCE_DIR}/TrafficCore/include
)

# Détection des fichiers sources
file(GLOB_RECURSE CORE_SOURCES "${CMAKE_SOURCE_DIR}/TrafficCore/src/*.cpp")
# Exclude Spawner.cpp since the functionality has been merged into TrafficManager
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*Spawner\\.cpp$")
# Ensure PathFinder.cpp is included even if the glob was cached earlier
list(APPEND CORE_SOURCES "${CMAKE_SOURCE_DIR}/TrafficCore/src/PathFinder.cpp")
list(APPEND CORE_SOURCES "${CMAKE_SOURCE_DIR}/TrafficCore/src/MapLoader.cpp")
file(GLOB DEMO_SOURCES "${CMAKE_SOURCE_DIR}/TrafficCore/demos/*.cpp")
file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/TrafficCore/tests/*.cpp")

# Option to enable deterministic spawner module
option(ENABLE_SPAWNER "Enable deterministic vehicle Spawner" ON)
if(ENABLE_SPAWNER)
    message(STATUS "Spawner module: ENABLED")
else()
    message(STATUS "Spawner module: DISABLED")
endif()

# Affiche la liste des fichiers sources trouvés
message(STATUS "Fichiers sources détectés :")
foreach(source_file ${CORE_SOURCES})
    message(STATUS "- CORE: ${source_file}")
endforeach()
foreach(source_file ${DEMO_SOURCES})
    message(STATUS "- DEMO: ${source_file}")
endforeach()

# Créer l'exécutable principal (Demo)
# Exclure le fichier de simulation spécifique des sources globales pour éviter les conflits de main()
list(FILTER DEMO_SOURCES EXCLUDE REGEX "boukhalf_simulation.cpp")

# Créer l'exécutable principal (SmartCity) basé sur boukhalf_simulation uniquement
add_executable(${PROJECT_NAME} ${CORE_SOURCES} "${CMAKE_SOURCE_DIR}/TrafficCore/demos/boukhalf_simulation.cpp")

# Propagate features based on options
if(ENABLE_SPAWNER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_SPAWNER=1)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIB})

# Options de compilation pour Windows
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm gdi32 opengl32 shell32)
endif()

# --- AJOUT DES TESTS ---
foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${CORE_SOURCES} ${test_file})
    target_link_libraries(${test_name} PRIVATE ${RAYLIB_LIB})
    
    if(WIN32)
        target_compile_definitions(${test_name} PRIVATE _WIN32_WINNT=0x0A00)
        target_link_libraries(${test_name} PRIVATE winmm gdi32 opengl32 shell32)
    endif()
endforeach()

# Configuration de l'installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)


# Créer un répertoire pour les ressources
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)

# Copier les ressources (sons, images, etc.)
file(GLOB_RECURSE RESOURCE_FILES 
    "${CMAKE_SOURCE_DIR}/TrafficCore/assets/*.wav"
    "${CMAKE_SOURCE_DIR}/TrafficCore/assets/*.mp3"
    "${CMAKE_SOURCE_DIR}/TrafficCore/assets/*.ogg"
    "${CMAKE_SOURCE_DIR}/TrafficCore/assets/*.png"
    "${CMAKE_SOURCE_DIR}/TrafficCore/assets/*.jpg"
    "${CMAKE_SOURCE_DIR}/TrafficCore/assets/*.glb"
)

foreach(resource_file ${RESOURCE_FILES})
    file(COPY ${resource_file} DESTINATION ${CMAKE_BINARY_DIR}/resources)
    message(STATUS "Ressource copiée : ${resource_file}")
endforeach()

# Configurer la commande pour copier les ressources après la compilation
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/TrafficCore/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copie des ressources dans le répertoire de sortie"
)
