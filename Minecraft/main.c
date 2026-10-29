#include <stdio.h>
#include <stdbool.h>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"
#define v3_print(v) printf("vec3: { x: %.2f, y: %.2f, z: %.2f }\n", (v).x, (v).y, (v).z)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CHUNK_WIDTH  16
#define CHUNK_HEIGHT 16
#define CHUNK_DEPTH  16

// Holds a single vertex configuration matching our shader input layout
typedef struct {
    float x, y, z;    // Position
    float nx, ny, nz; // Normal
    float u, v;       // UV coordinates
} ChunkVertex;

// Geometry constants for the 6 faces of a unit cube centered at the origin
// Move this to global scope so all functions can see it
const float cubeFaceVertices[6][6][8] = 
{
    // Back face (nz = -1)
    {
        {-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f},
        { 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f},
        { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f},
        { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f},
        {-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f},
        {-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f}
    },
    // Front face (nz = 1)
    {
        {-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f},
        { 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f},
        { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f},
        { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f},
        {-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f},
        {-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f}
    },
    // Left face (nx = -1)
    {
        {-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f},
        {-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f},
        {-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f},
        {-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f},
        {-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f},
        {-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f}
    },
    // Right face (nx = 1)
    {
        { 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f},
        { 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f},
        { 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f},
        { 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f},
        { 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f},
        { 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f}
    },
    // Bottom face (ny = -1)
    {
        {-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f},
        { 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f},
        { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f},
        { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f},
        {-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f},
        {-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f}
    },
    // Top face (ny = 1)
    {
        {-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f},
        { 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f},
        { 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f},
        { 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f},
        {-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f},
        {-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f}
    }
};

// Shader Strings
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 inPosition;\n"
    "layout (location = 1) in vec3 inNormal;\n"
    "layout (location = 2) in vec2 inTexCoords;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec2 TexCoords;\n"
    "uniform mat4 transformMatrix;\n"
    "uniform mat4 viewMatrix;\n"
    "uniform mat4 projectionMatrix;\n"
    "void main()\n"
    "{\n"
    "   FragPos = vec3(transformMatrix * vec4(inPosition, 1.0));\n"
    "   Normal = mat3(transpose(inverse(transformMatrix))) * inNormal;\n"
    "   TexCoords = inTexCoords;\n"
    "   gl_Position = projectionMatrix * viewMatrix * vec4(FragPos, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec2 TexCoords;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightColor;\n"
    "uniform sampler2D texture_diffuse;\n"
    "void main()\n"
    "{\n"
    "   vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;\n"
    "   float ambientStrength = 0.2;\n"
    "   vec3 ambient = ambientStrength * lightColor;\n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 lightDir = normalize(lightPos - FragPos);\n"
    "   float diff = max(dot(norm, lightDir), 0.0);\n"
    "   vec3 diffuse = diff * lightColor;\n"
    "   float specularStrength = 0.5;\n"
    "   vec3 viewDir = normalize(viewPos - FragPos);\n"
    "   vec3 reflectDir = reflect(-lightDir, norm);\n"
    "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "   vec3 specular = specularStrength * spec * lightColor;\n"
    "   vec3 result = (ambient + diffuse + specular) * textureColor;\n"
    "   FragColor = vec4(result, 1.0);\n"
    "}\n\0";

/* Declare Prototypes Prior to Main */
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
vec3_t processInput(GLFWwindow *window, vec3_t viewDir, vec3_t upVector, float moveSpeed, float deltaTime);
bool isTransparent(int x, int y, int z, unsigned char chunk[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH]);
int buildChunkMesh(unsigned char chunk[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH], ChunkVertex* vertexBuffer);

// Camera state tracking
vec3_t camPosition = { 8.0f, 8.0f, 20.0f }; // Adjusted initial placement to view the whole chunk
vec3_t viewDir     = { 0.0f, -0.3f, -1.0f }; 
vec3_t upVector    = { 0.0f, 1.0f, 0.0f };

float yaw   = -90.0f;
float pitch =   -15.0f;

bool firstMouse = true;
float lastX = 960.0f; 
float lastY = 540.0f; 

int main(void) 
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!glfwInit()) 
    {
        printf("bruh, GLFW and OpenGL did NOT work."); 
        return -1;
    }
    
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Minecraft 3D Camera", primaryMonitor, NULL); 
    if (!window)
    {
        printf("bruh, the game window did NOT open.");    
        return -1;
    }

    glfwMakeContextCurrent(window); 
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        printf("gladLoadGLLoader failed bruh");
        return -1;
    }
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
    glfwSetCursorPosCallback(window, mouse_callback);            
    glViewport(0,0,mode->width,mode->height); 
    glEnable(GL_DEPTH_TEST); 
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); 
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
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

    // --- Allocate space and generate the chunk landscape datagrid first ---
    unsigned char chunk[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH] = { 0 };
    for (int x = 0; x < CHUNK_WIDTH; x++)
    {
        for (int z = 0; z < CHUNK_DEPTH; z++)
        {
            for (int y = 0; y < 4; y++)
            {
                chunk[x][y][z] = 1; 
            }
        }
    }

    // Allocate temporary vertex heap configuration memory
    ChunkVertex* tempMeshBuffer = malloc(147456 * sizeof(ChunkVertex));
    int totalMeshVertices = buildChunkMesh(chunk, tempMeshBuffer);
    
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO); 
    glGenBuffers(1, &VBO);  
                            
    glBindVertexArray(VAO); 
    glBindBuffer(GL_ARRAY_BUFFER, VBO); 
    glBufferData(GL_ARRAY_BUFFER, totalMeshVertices * sizeof(ChunkVertex), tempMeshBuffer, GL_STATIC_DRAW); 
    free(tempMeshBuffer);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)0); 
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)(3 * sizeof(float))); 
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 

    // Pipeline Matrices and Lighting Setup
    float moveSpeed = 15.0f;   
    mat4_t transformMatrix = m4_identity();

    GLint transformMatrixLocation = glGetUniformLocation(shaderProgram, "transformMatrix");
    GLint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
    GLint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");

    GLint lightPosLoc   = glGetUniformLocation(shaderProgram, "lightPos");
    GLint viewPosLoc    = glGetUniformLocation(shaderProgram, "viewPos");
    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");

    vec3_t lightPosition = vec3(8.0f, 20.0f, 8.0f); // Placed above the center of our chunk

    float deltaTime = 0.0f; 
    float lastFrame = 0.0f; 

    // Generate Texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load("../dirt.png", &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        printf("Failed to load texture file dirt.png\n");
    }
    stbi_image_free(data); 

    while (!glfwWindowShouldClose(window)) 
    {
        float currentFrame = (float)glfwGetTime(); 
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        vec3_t frameMovement = processInput(window, viewDir, upVector, moveSpeed, deltaTime); 
        camPosition = v3_add(camPosition, frameMovement); 
                                                         
        vec3_t camTarget = v3_add(camPosition, viewDir); 
        
        mat4_t viewMatrix = m4_look_at(camPosition, camTarget, upVector);   
        mat4_t projectionMatrix = m4_perspective(45.0f, (float)mode->width / (float)mode->height, 0.1f, 100.0f); 

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        glUniform3f(lightPosLoc, lightPosition.x, lightPosition.y, lightPosition.z);
        glUniform3f(viewPosLoc, camPosition.x, camPosition.y, camPosition.z);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); 
        
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix.m[0][0]);
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix.m[0][0]);
        
        mat4_t globalTransform = m4_identity();
        glUniformMatrix4fv(transformMatrixLocation, 1, GL_FALSE, &globalTransform.m[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        
        // Render the entire optimized structure in one single call
        glDrawArrays(GL_TRIANGLES, 0, totalMeshVertices);

        glfwSwapBuffers(window); 
        glfwPollEvents(); 
    } 

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate(); 
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

vec3_t processInput(GLFWwindow *window, vec3_t viewDir, vec3_t upVector, float moveSpeed, float deltaTime)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
        
    vec3_t movement = vec3(0.0f, 0.0f, 0.0f);
    vec3_t forward = viewDir; 
    vec3_t right   = v3_norm(v3_cross(forward, upVector)); 

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement = v3_add(movement, forward);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement = v3_sub(movement, forward);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement = v3_sub(movement, right);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement = v3_add(movement, right);

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

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; 
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    float yawRad   = yaw * (M_PI / 180.0f);
    float pitchRad = pitch * (M_PI / 180.0f);

    vec3_t direction;
    direction.x = cosf(yawRad) * cosf(pitchRad);
    direction.y = sinf(pitchRad);
    direction.z = sinf(yawRad) * cosf(pitchRad);

    viewDir = v3_norm(direction); 
}

bool isTransparent(int x, int y, int z, unsigned char chunk[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH]) {
    if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH)
        return true;
    return chunk[x][y][z] == 0;
}

int buildChunkMesh(unsigned char chunk[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH], ChunkVertex* vertexBuffer) {
    int vertexCount = 0;

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_DEPTH; z++) {
                if (chunk[x][y][z] == 0) continue; 

                int dx[] = { 0,  0, -1,  1,  0,  0};
                int dy[] = { 0,  0,  0,  0, -1,  1};
                int dz[] = {-1,  1,  0,  0,  0,  0};

                for (int face = 0; face < 6; face++) {
                    if (isTransparent(x + dx[face], y + dy[face], z + dz[face], chunk)) {
                        for (int v = 0; v < 6; v++) {
                            ChunkVertex currentVert;
                            currentVert.x  = cubeFaceVertices[face][v][0] + (float)x;
                            currentVert.y  = cubeFaceVertices[face][v][1] + (float)y;
                            currentVert.z  = cubeFaceVertices[face][v][2] + (float)z;
                            currentVert.nx = cubeFaceVertices[face][v][3];
                            currentVert.ny = cubeFaceVertices[face][v][4];
                            currentVert.nz = cubeFaceVertices[face][v][5];
                            currentVert.u  = cubeFaceVertices[face][v][6];
                            currentVert.v  = cubeFaceVertices[face][v][7];

                            vertexBuffer[vertexCount++] = currentVert;
                        }
                    }
                }
            }
        }
    }
    return vertexCount;
}
