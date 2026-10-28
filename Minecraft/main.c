#include <stdio.h>
#include <stdbool.h>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"
#define v3_print(v) printf("vec3: { x: %.2f, y: %.2f, z: %.2f }\n", (v).x, (v).y, (v).z)

//Basic shader for triangle
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 inPosition;\n"
    "layout (location = 1) in vec3 inNormal;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "uniform mat4 transformMatrix;\n"
    "uniform mat4 viewMatrix;\n"
    "uniform mat4 projectionMatrix;\n"
    "void main()\n"
    "{\n"
    "   FragPos = vec3(transformMatrix * vec4(inPosition, 1.0));\n"
    "   Normal = mat3(transpose(inverse(transformMatrix))) * inNormal;\n"
    "   gl_Position = projectionMatrix * viewMatrix * vec4(FragPos, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 objectColor;\n"
    "void main()\n"
    "{\n"
    "   // 1. Ambient Lighting\n"
    "   float ambientStrength = 0.2;\n"
    "   vec3 ambient = ambientStrength * lightColor;\n"
    "   \n"
    "   // 2. Diffuse Shading\n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 lightDir = normalize(lightPos - FragPos);\n"
    "   float diff = max(dot(norm, lightDir), 0.0);\n"
    "   vec3 diffuse = diff * lightColor;\n"
    "   \n"
    "   // 3. Specular Shading\n"
    "   float specularStrength = 0.5;\n"
    "   vec3 viewDir = normalize(viewPos - FragPos);\n"
    "   vec3 reflectDir = reflect(-lightDir, norm);\n"
    "   // 32 is the shininess exponent (higher number = smaller, tighter shine spot)\n"
    "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "   vec3 specular = specularStrength * spec * lightColor;\n"
    "   \n"
    "   // 4. Combine Result for PHONG Lighting\n"
    "   vec3 result = (ambient + diffuse + specular) * objectColor;\n"
    "   FragColor = vec4(result, 1.0);\n"
    "}\n\0";
/*Declare all helper functions prior to the main function*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
vec3_t processInput(GLFWwindow *window, vec3_t viewDir, vec3_t upVector, float moveSpeed, float deltaTime);

// Camera state tracking
vec3_t camPosition = { 0.0f, 0.0f, 3.0f };
vec3_t viewDir     = { 0.0f, 0.0f, -1.0f }; // Automatically calculated by mouse
vec3_t upVector    = { 0.0f, 1.0f, 0.0f };

// Mouse orientation angles (Initialized looking straight down negative Z)
float yaw   = -90.0f;
float pitch =   0.0f;

// Handle first-frame jump spikes
bool firstMouse = true;
float lastX = 960.0f; // Half of window width (640 / 2)
float lastY = 540.0f; // Half of window height (480 / 2)

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
    
    //Setup fullscreen
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Minecraft 3D Camera", primaryMonitor, NULL); 
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
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Lock mouse to screen center
    glfwSetCursorPosCallback(window, mouse_callback);            // Register mouse tracking function
    glViewport(0,0,mode->width,mode->height); //Convert window size to -1 to +1 for OpenGL rendering
    glEnable(GL_DEPTH_TEST); //Enabke deoth testing so objects are correctly drawn in 3D
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
        // Positions          // Normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    unsigned int VBO, VAO; //Declare IDs for VAO, VBO, and EBO
                               
    glGenVertexArrays(1, &VAO); //Generate vertex array VAO
    glGenBuffers(1, &VBO);  //Generate vertex buffer VBO
                            
    glBindVertexArray(VAO); //Bind VAO to use it
    glBindBuffer(GL_ARRAY_BUFFER, VBO); //Bind VBO to use it
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Populate VBO with data from vertices array

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); //Setup attribute pointer 0 (Also stores the VBO associated with this VAO behind the scenes)
    glEnableVertexAttribArray(0); //Activate pointer 0 inside our VAO
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); //Setup normal attribute at attriute 1, which starts after 3 floats
    glEnableVertexAttribArray(1);
    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); //Sets currently bound buffer to null
                                      //DO NOT unbind the EBO before unbinding VAO. The EBO is stored in the VAO

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0); //Sets bound VAO to null  

    //3D Environment Setup
    vec3_t camTarget = vec3(0.0f, 0.0f, 0.0f);   //Camera view vector. To look into the world, that is the -z direction
    float  moveSpeed = 10.0f;   //Move speed vector
    mat4_t transformMatrix = m4_translation(vec3(0.0f,0.0f,0.0f));

    //Setup addresses of matricies we want to pass to Shader
    GLint transformMatrixLocation = glGetUniformLocation(shaderProgram, "transformMatrix");
    GLint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
    GLint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");

    GLint lightPosLoc   = glGetUniformLocation(shaderProgram, "lightPos");
    GLint viewPosLoc    = glGetUniformLocation(shaderProgram, "viewPos");
    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLint objColorLoc   = glGetUniformLocation(shaderProgram, "objectColor");

    // Position of the sun/lamp in the sky
    vec3_t lightPosition = vec3(1.2f, 3.0f, 2.0f);

    vec3_t cubePositions[] = {
        vec3( 0.0f,  0.0f,  0.0f),  // Center cube
        vec3( 2.0f,  0.5f, -2.0f),  // Right and slightly back
        vec3(-2.0f, -0.5f, -4.0f)   // Left and further back
    };

    float deltaTime = 0.0f; // Time spent between current frame and last frame
    float lastFrame = 0.0f; // Time stamp of the previous frame

    
    while (!glfwWindowShouldClose(window)) //This is the main game loop
        {
            float currentFrame = (float)glfwGetTime(); 
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            
            vec3_t forward = viewDir; //Set forward as our current looking direction
            
            vec3_t frameMovement = processInput(window, viewDir, upVector, moveSpeed, deltaTime); //Process user inputs. Local Forward and Global Up are given to process strafing movement

            camPosition = v3_add(camPosition, frameMovement); //Add processed input to our camera
                                                             
            vec3_t camTarget = v3_add(camPosition, viewDir); //Update viewing target        
            
            mat4_t viewMatrix = m4_look_at(camPosition, camTarget, upVector);   //Create viewing matrix
            mat4_t projectionMatrix = m4_perspective(45.0f, (float)mode->width / (float)mode->height, 0.1f, 100.0f); //create projection matrix, FOV 45, aspect ratio 640/480, 0.1f is near plane, 100.0f is far clip plane (view distance)

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Teal background
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            glUseProgram(shaderProgram);
            
            // Upload lighting data
            glUniform3f(lightPosLoc, lightPosition.x, lightPosition.y, lightPosition.z);
            glUniform3f(viewPosLoc, camPosition.x, camPosition.y, camPosition.z);
            glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // Pure white light
            glUniform3f(objColorLoc, 1.0f, 0.5f, 0.2f);   // Original orange cube color
            
            //Pass matricies to Shader program
            glUniformMatrix4fv(transformMatrixLocation, 1, GL_FALSE, &transformMatrix.m[0][0]);
            glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix.m[0][0]);
            glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix.m[0][0]);
            
            glBindVertexArray(VAO);
            
            for (int i = 0; i < 3; i++)
            {
                // Generate a new spatial translation matrix for this iteration
                transformMatrix = m4_translation(cubePositions[i]);
        
                // Pass the address of the first float inside the 3D math array struct safely
                glUniformMatrix4fv(transformMatrixLocation, 1, GL_FALSE, &transformMatrix.m[0][0]);
        
                // Draw the 36 continuous vertices directly without EBO indices
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            

            glfwSwapBuffers(window); //Display what was drawn

            glfwPollEvents(); //Poll for keyboard and mouse events
        }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate(); //Terminate glfw and close program
    return 0;
}


/*Function callback for resizing the window and ensuring the glViewport also updates --------------------------------------------------------------------*/

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

/*Function for handling user inputs ---------------------------------------------------------------------------------------------------------------------*/

vec3_t processInput(GLFWwindow *window, vec3_t viewDir, vec3_t upVector, float moveSpeed, float deltaTime)
{
    // Close window on Escape
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
        
    vec3_t movement = vec3(0.0f, 0.0f, 0.0f);
    
    // Calculate direction vectors relative to current look angle
    vec3_t forward = viewDir; 
    vec3_t right   = v3_norm(v3_cross(forward, upVector)); 

    // Accumulate directional frames based on key states
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement = v3_add(movement, forward);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement = v3_sub(movement, forward);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement = v3_sub(movement, right);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement = v3_add(movement, right);

    // Normalize movement so running diagonally doesn't make you move faster!
    // Scale the final direction by the speed factor
    return v3_muls(v3_norm(movement), moveSpeed * deltaTime);    
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // Calculate how far the mouse drifted since the last frame
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; // Adjust to slow down or speed up mouse look
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    // Constrain pitch so the player's neck doesn't snap completely backward
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Convert Yaw and Pitch degrees into radians
    float yawRad   = yaw * (M_PI / 180.0f);
    float pitchRad = pitch * (M_PI / 180.0f);

    // Calculate actual 3D direction vector components using trigonometry
    vec3_t direction;
    direction.x = cosf(yawRad) * cosf(pitchRad);
    direction.y = sinf(pitchRad);
    direction.z = sinf(yawRad) * cosf(pitchRad);

    viewDir = v3_norm(direction); // Store normalized direction vector
}
