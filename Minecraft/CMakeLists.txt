#Require version of cmake when this was created
cmake_minimum_required(VERSION 3.28.3)

#Name project
project(Minecraft)

#Enforce C99 or newer standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

#Find the GLFW3 package on the system
find_package(glfw3 3.3 REQUIRED)

#Define the executable and include all .c files
add_executable(${PROJECT_NAME} main.c glad.c)

#Include the local header directory to find my .h files
target_include_directories(${PROJECT_NAME} PRIVATE include)

#Link libraries and handle OS-specific dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

if(WIN32)
    # Windows requires opengl32.lib
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
elseif(APPLE)
    # macOS requires Cocoa, IOKit, and the OpenGL Frameworks
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "-framework Cocoa"
        "-framework IOKit"
        "-framework OpenGL"
    )
else()
    # Linux requires standard system OpenGL libraries
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL m) #m is the math library
endif()
