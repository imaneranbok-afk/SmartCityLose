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

# Détection automatique des fichiers sources
file(GLOB_RECURSE SOURCES 
    "${CMAKE_SOURCE_DIR}/TrafficCore/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/TrafficCore/demos/*.cpp"
)

# Affiche la liste des fichiers sources trouvés
message(STATUS "Fichiers sources détectés :")
foreach(source_file ${SOURCES})
    message(STATUS "- ${source_file}")
endforeach()

# Créer l'exécutable
add_executable(${PROJECT_NAME} ${SOURCES})

# Lier les bibliothèques
target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIB})

# Options de compilation pour Windows
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
    target_link_libraries(${PROJECT_NAME} PRIVATE gdi32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
    target_link_libraries(${PROJECT_NAME} PRIVATE shell32)
endif()

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
    "${CMAKE_SOURCE_DIR}/TrafficCore/resources/*.wav"
    "${CMAKE_SOURCE_DIR}/TrafficCore/resources/*.mp3"
    "${CMAKE_SOURCE_DIR}/TrafficCore/resources/*.ogg"
    "${CMAKE_SOURCE_DIR}/TrafficCore/resources/*.png"
    "${CMAKE_SOURCE_DIR}/TrafficCore/resources/*.jpg"
)

foreach(resource_file ${RESOURCE_FILES})
    file(COPY ${resource_file} DESTINATION ${CMAKE_BINARY_DIR}/resources)
    message(STATUS "Ressource copiée : ${resource_file}")
endforeach()

# Configurer la commande pour copier les ressources après la compilation
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/TrafficCore/resources
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    COMMENT "Copie des ressources dans le répertoire de sortie"
)
