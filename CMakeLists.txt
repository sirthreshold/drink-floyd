cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# Project: Drink Floyd (Original Audio-Reactive Visualizer)
# Target: Windows STANDALONE Application ONLY (No DAW plugin, No VST3)
# ==============================================================================
project(DrinkFloyd
    VERSION 0.1.0
    DESCRIPTION "Drink Floyd - Standalone Audio-Reactive Music Visualizer"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Locate JUCE 8 (prefer local or sibling directory, fallback to FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# STANDALONE GUI APPLICATION TARGET ONLY
# Explicitly NO juce_add_plugin, NO VST3 target, NO AU target, NO AAX target
juce_add_gui_app(DrinkFloyd
    PRODUCT_NAME "Drink Floyd"
    COMPANY_NAME "Original"
    BUNDLE_ID "com.drinkfloyd.visualizer"
    VERSION 0.1.0
)

# Project Source Files
target_sources(DrinkFloyd PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/HeaderBar.h
    Source/HeaderBar.cpp
    Source/VisualCanvas.h
    Source/VisualCanvas.cpp
    Source/ParameterPanel.h
    Source/ParameterPanel.cpp
)

# Compile Definitions
target_compile_definitions(DrinkFloyd PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DrinkFloyd,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DrinkFloyd,JUCE_VERSION>"
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# Link Libraries
target_link_libraries(DrinkFloyd PRIVATE
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
