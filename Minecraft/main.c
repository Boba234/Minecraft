#include <stdio.h>
#include <stdbool.h>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

//Basic shader for triangle
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
//Basic fragment shader for triangle
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

/*Declare all helper functions prior to the main function*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

int main(void) 
{
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //Set OpenGL to version 3.3 and the CORE profile 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!glfwInit()) //Initialize GLFW and OpenGL
        {
            printf("bruh, GLFW and OpenGL did NOT work."); //OpenGL failed to load
            return -1;
        }

    GLFWwindow* window = glfwCreateWindow(640, 480, "Minecraft", NULL, NULL); //Create the game window

    if (!window)
        {
            printf("bruh, the game window did NOT open.");    // Window or OpenGL context creation failed
            return -1;
        }

    glfwMakeContextCurrent(window); //Set OpenGL context to the game window
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) //Use glad to get function pointers
        {
            printf("gladLoadGLLoader failed bruh");// Handle initialization failure here (e.g., print error and exit)
            return -1;
        }
    
    glViewport(0,0,640,480); //Convert window size to -1 to +1 for OpenGL rendering
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //Set callback function for when we resize the window
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
             0.5f,  0.5f, 0.0f,  // top right
            0.5f, -0.5f, 0.0f,  // bottom right
            -0.5f, -0.5f, 0.0f,  // bottom left
            -0.5f,  0.5f, 0.0f   // top left 
        };

    unsigned int indices[] =  {  // note that we start from 0!
            0, 1, 3,   // first triangle
            1, 2, 3    // second triangle
        };
    
    unsigned int VBO, VAO, EBO; //Declare IDs for VAO, VBO, and EBO
                               
    glGenVertexArrays(1, &VAO); //Generate vertex array VAO
    glGenBuffers(1, &VBO);  //Generate vertex buffer VBO
    glGenBuffers(1, &EBO);  //Generate vertex buffer EBO
                            
    glBindVertexArray(VAO); //Bind VAO to use it
    glBindBuffer(GL_ARRAY_BUFFER, VBO); //Bind VBO to use it
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Populate VBO with data from vertices array
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); //Bind EBO to use it
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); //Populate EBO with data from indices array

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); //Setup attribute pointer 0 (Also stores the VBO associated with this VAO behind the scenes)
    glEnableVertexAttribArray(0); //Activate pointer 0 inside our VAO

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); //Sets currently bound buffer to null
                                      //DO NOT unbind the EBO before unbinding VAO. The EBO is stored in the VAO

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0); //Sets bound VAO to null  

    //Camera setup
    vec3_t camPosition = vec3(0.0f, 2.0f, 0.0f);
    vec3_t viewDir = vec3(0.0f, 0.0f, -1.0f);
    vec3_t upVector = vec3(0.0f, 1.0f, 0.0f);
    
    mat4_t identityMatrix = m4_identity();
    m4_print(identityMatrix);

    while (!glfwWindowShouldClose(window)) //This is the main game loop
        {
            processInput(window); //Call function to get user inputs

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Teal background
            glClear(GL_COLOR_BUFFER_BIT);
            
            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glfwSwapBuffers(window); //Display what was drawn

            glfwPollEvents(); //Poll for keyboard and mouse events
        }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate(); //Terminate glfw and close program
    return 0;
}


/*Function callback for resizing the window and ensuring the glViewport also updates --------------------------------------------------------------------*/

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

/*Function for handling user inputs ---------------------------------------------------------------------------------------------------------------------*/

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        printf("Pressed W Key"); 
}

