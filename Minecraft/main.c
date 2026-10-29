#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"
#define v3_print(v) printf("vec3: { x: %.2f, y: %.2f, z: %.2f }\n", (v).x, (v).y, (v).z)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* --- World & Chunk Configuration Constants --- */

#define CHUNK_WIDTH  16
#define CHUNK_HEIGHT 16
#define CHUNK_DEPTH  16

// Dynamic View Pipeline (A radius of 4 chunks around the player = 9x9 chunk ring grid)

#define RENDER_DISTANCE 4
#define MAX_ACTIVE_CHUNKS ((RENDER_DISTANCE * 2 + 1) * (RENDER_DISTANCE * 2 + 1))
#define MAX_CHUNK_VERTICES (CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH * 36) //Calc the max number of verticies for memory allocation (36 is 6 facecs * 6 vertices per face)

/* --- Data Structures --- */

// Holds a single vertex configuration matching our shader input layout
typedef struct {
    float x, y, z;    // Position
    float nx, ny, nz; // Normal
    float u, v;       // UV coordinates
} ChunkVertex;

// Structure to hold individual chunk states and GPU resources
typedef struct {
    int chunkX, chunkZ; // Absolute world grid index coordinates
    unsigned char voxels[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH]; //each voxel gets 1 byte of memory 
    unsigned int VAO, VBO;
    int totalVertices;
    bool isLoaded;
} Chunk;

/* --- Static Data & Shaders --- */

// Reference permutation table for Ken Perlin's Improved Noise Algorithm
static const int p[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233, 7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,
    20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,
    230,220,105,92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,5,202,
    38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,
    2,44,154,163, 70,221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185,
    112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,114,
    67,29,24,72,243,141,128,195,78,212,151,160,137,91,90,15,131,13,201,95,96,53,194,233, 7,225,140,36,
    103,30,69,142,8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,
    32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,
    158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,
    216,80,73,209,76,132,187,208, 89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,
    64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,
    42,223,183,170,213,119,248,152,2,44,154,163, 70,221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,
    108,110,79,113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,
    150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,212
};

// Cube face templates attributes matrix (Position, Normal, UV coordinates)
const float cubeFaceVertices[6][6][8] = { //6 faces, 6 verticies per square (2 trianlges), 8 attributes 
    // Back face (nz = -1)
    {{-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f}, { 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f}, { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f}, { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f}, {-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f}, {-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f}},
    // Front face (nz = 1)
    {{-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f}, { 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f}, { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f}, { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f}, {-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f}, {-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f}},
    // Left face (nx = -1)
    {{-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f}, {-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f}, {-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f}, {-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f}, {-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f}, {-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f}},
    // Right face (nx = 1)
    {{ 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f}, { 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f}, { 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f}, { 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f}, { 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f}, { 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f}},
    // Bottom face (ny = -1)
    {{-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f}, { 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f}, { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f}, { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f}, {-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f}, {-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f}},
    // Top face (ny = 1)
    {{-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f}, { 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f}, { 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f}, { 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f}, {-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f}, {-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f}}
};

// GLSL Shader Sources
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 inPosition;\n"
    "layout (location = 1) in vec3 inNormal;\n"
    "layout (location = 2) in vec2 inTexCoords;\n" //UV Texture coords
    "out vec3 FragPos;\n" //Send out variables to fragement shader
    "out vec3 Normal;\n"
    "out vec2 TexCoords;\n"
    "uniform mat4 transformMatrix;\n" //variables for proper vertex positioning
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
    "in vec3 FragPos;\n" //recieve in variables from vertex shader
    "in vec3 Normal;\n"
    "in vec2 TexCoords;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightColor;\n"
    "uniform sampler2D texture_diffuse;\n"
    "void main()\n"
    "{\n"
    "   vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;\n" //Apply lighting effects
    "   float ambientStrength = 0.3;\n"
    "   vec3 ambient = ambientStrength * lightColor;\n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 lightDir = normalize(lightPos - FragPos);\n"
    "   float diff = max(dot(norm, lightDir), 0.0);\n"
    "   vec3 diffuse = diff * lightColor;\n"
    "   float specularStrength = 0.2;\n"
    "   vec3 viewDir = normalize(viewPos - FragPos);\n"
    "   vec3 reflectDir = reflect(-lightDir, norm);\n"
    "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);\n"
    "   vec3 specular = specularStrength * spec * lightColor;\n"
    "   vec3 result = (ambient + diffuse + specular) * textureColor;\n"
    "   FragColor = vec4(result, 1.0);\n"
    "}\n\0";

/* --- Function Declarations --- */
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
vec3_t processInput(GLFWwindow *window, vec3_t viewDir, vec3_t upVector, float moveSpeed, float deltaTime);
bool isTransparent(int x, int y, int z, unsigned char voxels[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH]);
int buildChunkMesh(Chunk* chunk, ChunkVertex* vertexBuffer);
float perlin2D(float x, float y);
void generateChunkData(Chunk* chunk, int cx, int cz);
void uploadChunkMesh(Chunk* chunk, ChunkVertex* tempMeshBuffer);
void updateInfiniteWorld(Chunk* activeChunks, int playerChunkX, int playerChunkZ, ChunkVertex* tempMeshBuffer);
unsigned int compileShader(unsigned int type, const char* source);

/* --- Camera Tracking Globals --- */
vec3_t camPosition = { 8.0f, 15.0f, 8.0f }; 
vec3_t viewDir     = { 0.0f, -0.3f, -1.0f }; 
vec3_t upVector    = { 0.0f, 1.0f, 0.0f }; //Global up vector
float yaw = -90.0f, pitch = -15.0f;
bool firstMouse = true;
float lastX = 960.0f, lastY = 540.0f; //Center of screen

// --- Perlin Noise Mathematics -----------------------------------------------------------------------------------------------------------------

static inline float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
static inline float lerp(float t, float a, float b) { return a + t * (b - a); }
static inline float grad2D(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float perlin2D(float x, float y) {
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;
    x -= floorf(x);   y -= floorf(y);
    float u = fade(x); float v = fade(y);
    int A = p[X] + Y, B = p[X + 1] + Y;
    return lerp(v, lerp(u, grad2D(p[A], x, y), grad2D(p[B], x - 1, y)),
                   lerp(u, grad2D(p[A + 1], x, y - 1), grad2D(p[B + 1], x - 1, y - 1)));
}

// Procedurally fills voxel layout mapping seamless global indices offsets using Perlin noise ------------------------------------------------------

void generateChunkData(Chunk* chunk, int cx, int cz) { 
    chunk->chunkX = cx; //Dereference chunk, assign its x,z, and number of verticies
    chunk->chunkZ = cz;
    chunk->totalVertices = 0;

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            float worldX = (float)(cx * CHUNK_WIDTH + x); //get each x and z coordinate in world space 
            float worldZ = (float)(cz * CHUNK_DEPTH + z);
            
            float noiseVal = perlin2D(worldX * 0.03f, worldZ * 0.03f);
            float normalizedNoise = (noiseVal + 1.0f) * 0.5f; //get perlin noise values for that world space xz
            
            int targetHeight = 3 + (int)(normalizedNoise * 10.0f);
            if (targetHeight >= CHUNK_HEIGHT) targetHeight = CHUNK_HEIGHT - 1;

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                chunk->voxels[x][y][z] = (y < targetHeight) ? 1 : 0; //if the y<target height, assign the block a 1 (dirt)
            }
        }
    }
}

// Compiles and loads chunk geometry directly to the VRAM on the GPU ---------------------------------------------------------------

void uploadChunkMesh(Chunk* chunk, ChunkVertex* tempMeshBuffer) {
    if (chunk->totalVertices > 0) //if chunk has been previously loaded, free up the memory we used for that 
    {
        glDeleteVertexArrays(1, &chunk->VAO);
        glDeleteBuffers(1, &chunk->VBO);
    }
    
    int totalVertices = buildChunkMesh(chunk, tempMeshBuffer); //get total number of vertices we want to draw / build the mesh
    chunk->totalVertices = totalVertices; //assign the number of vertices we created to the chunk

    if (totalVertices == 0) return; //if empty, exit

    glGenVertexArrays(1, &chunk->VAO); //create VAO and VBO
    glGenBuffers(1, &chunk->VBO);
    
    glBindVertexArray(chunk->VAO); //bind VAO and VBO so we can use them
    glBindBuffer(GL_ARRAY_BUFFER, chunk->VBO);
    glBufferData(GL_ARRAY_BUFFER, totalVertices * sizeof(ChunkVertex), tempMeshBuffer, GL_STATIC_DRAW); //Send data to buffer

    // Vertex Attributes Configuration
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)0); //xyz
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)(3 * sizeof(float))); //normal coordinates
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)(6 * sizeof(float))); //UV coordinates
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0); //Clear currently bound VAO for safety
}

// Dynamic Chunk Manager: Recycles out-of-bounds resources and streams new structures ----------------------------------------------------------------

void updateInfiniteWorld(Chunk* activeChunks, int playerChunkX, int playerChunkZ, ChunkVertex* tempMeshBuffer)
{
    for (int xOffset = -RENDER_DISTANCE; xOffset <= RENDER_DISTANCE; xOffset++) {
        for (int zOffset = -RENDER_DISTANCE; zOffset <= RENDER_DISTANCE; zOffset++) { //Loop through chunks around the player based on render distance
            int targetChunkX = playerChunkX + xOffset;
            int targetChunkZ = playerChunkZ + zOffset;
                            //modulo the x coord by 9                     add 9 to ensure positive   if val was already positive, its now too big, so modulo again. Then multiply by row and add the same math for the z coord to find the index of the chunk in a 1D array
            int slotIndex = ((targetChunkX % (RENDER_DISTANCE * 2 + 1) + (RENDER_DISTANCE * 2 + 1)) % (RENDER_DISTANCE * 2 + 1)) * (RENDER_DISTANCE * 2 + 1) +
                            ((targetChunkZ % (RENDER_DISTANCE * 2 + 1) + (RENDER_DISTANCE * 2 + 1)) % (RENDER_DISTANCE * 2 + 1));

            Chunk* currentChunk = &activeChunks[slotIndex];//grab address of the target chunk in the active chunks array

            if (!currentChunk->isLoaded || currentChunk->chunkX != targetChunkX || currentChunk->chunkZ != targetChunkZ) //If the chunk is not loaded, or if a chunk that left the render distance is in this slot, clear the vertex buffer data, then generate the new chunk / send it to the gpu
            { 
                if (currentChunk->isLoaded && currentChunk->totalVertices > 0) 
                {
                    glDeleteVertexArrays(1, &currentChunk->VAO);
                    glDeleteBuffers(1, &currentChunk->VBO);
                }
                generateChunkData(currentChunk, targetChunkX, targetChunkZ);
                uploadChunkMesh(currentChunk, tempMeshBuffer);
                currentChunk->isLoaded = true;
            }
        }
    }
}

//Start of Main function --------------------------------------------------------------------------------------------------

int main(void) 
{
    // Initialize GLFW and Context Setup
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Minecraft", primaryMonitor, NULL); 
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window); 
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //lock cursor
    glfwSetCursorPosCallback(window, mouse_callback);            
    glViewport(0, 0, mode->width, mode->height); 
    glEnable(GL_DEPTH_TEST); 
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); 
    
    // Shader Compilation & Linking
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);  
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader); 
    glDeleteShader(fragmentShader);

    // Allocate Chunk System and Temporary Mesh Buffers
    Chunk* activeChunks = calloc(MAX_ACTIVE_CHUNKS, sizeof(Chunk)); //set aside cleaned memory for chunks in view
    ChunkVertex* tempMeshBuffer = malloc(MAX_CHUNK_VERTICES * sizeof(ChunkVertex)); //set aside memory for the vertex buffer
    if (!activeChunks || !tempMeshBuffer) {
        fprintf(stderr, "Memory allocation failed for chunks or mesh buffer\n");
        return -1;
    }

    int lastPlayerChunkX = (int)floorf(camPosition.x / CHUNK_WIDTH);
    int lastPlayerChunkZ = (int)floorf(camPosition.z / CHUNK_DEPTH);
    updateInfiniteWorld(activeChunks, lastPlayerChunkX, lastPlayerChunkZ, tempMeshBuffer);//get player coordinates then update the world

    float moveSpeed = 15.0f; //player moveSpeed

    GLint transformMatrixLocation = glGetUniformLocation(shaderProgram, "transformMatrix"); //Get locations of shader uniform variables to send to GPU later
    GLint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
    GLint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");

    float deltaTime = 0.0f, lastFrame = 0.0f; //setup deltaTime

    // Texture Loading
    unsigned int texture;
    glGenTextures(1, &texture); 
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //Create and vind a texture variable. then setup some parameters

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load("../dirt.png", &width, &height, &nrChannels, 0); //load texture with stb package
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); //determine if its RGB or an RGBA texture, and send texture from cpu to gpu
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        fprintf(stderr, "Failed to load texture at ../dirt.png\n");
    }
    stbi_image_free(data); //free the memory we used

    // Main Render Loop ---------------------------------------------------------------------
    while (!glfwWindowShouldClose(window)) 
    {
        float currentFrame = (float)glfwGetTime(); 
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame; //calc deltaTime 
        
        vec3_t frameMovement = processInput(window, viewDir, upVector, moveSpeed, deltaTime); 
        camPosition = v3_add(camPosition, frameMovement); 
        vec3_t camTarget = v3_add(camPosition, viewDir); //Process inputs and update player position / look
        
        int currentPlayerChunkX = (int)floorf(camPosition.x / CHUNK_WIDTH); //get the chunk we are in
        int currentPlayerChunkZ = (int)floorf(camPosition.z / CHUNK_DEPTH);

        if (currentPlayerChunkX != lastPlayerChunkX || currentPlayerChunkZ != lastPlayerChunkZ) //if we changed chunks, update the world 
        {
            updateInfiniteWorld(activeChunks, currentPlayerChunkX, currentPlayerChunkZ, tempMeshBuffer);
            lastPlayerChunkX = currentPlayerChunkX;
            lastPlayerChunkZ = currentPlayerChunkZ;
        }

        mat4_t viewMatrix = m4_look_at(camPosition, camTarget, upVector);   
        mat4_t projectionMatrix = m4_perspective(45.0f, (float)mode->width / (float)mode->height, 0.1f, 400.0f); //create view and projection matrix, 45 is the FOV, .1 is near plane, 400 is far plane

        vec3_t lightPosition = vec3(camPosition.x + 30.0f, 40.0f, camPosition.z + 30.0f); //setup light position

        glClearColor(0.4f, 0.6f, 0.9f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //draw background then clear drawing buffers
        
        glUseProgram(shaderProgram);
        glUniform3f(lightPosLoc, lightPosition.x, lightPosition.y, lightPosition.z); //send light position to shader program
        glUniform3f(viewPosLoc, camPosition.x, camPosition.y, camPosition.z);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); 
        
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, (float*)&viewMatrix.m); //send view and projection matrix to shader program
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, (float*)&projectionMatrix.m);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture); //bind texture to program

        for (int i = 0; i < MAX_ACTIVE_CHUNKS; i++) //for all active chunks
        {
            Chunk* currentChunk = &activeChunks[i];
            if (!currentChunk->isLoaded || currentChunk->totalVertices == 0) continue;

            int distX = abs(currentChunk->chunkX - currentPlayerChunkX);
            int distZ = abs(currentChunk->chunkZ - currentPlayerChunkZ);
            
            if (distX > RENDER_DISTANCE || distZ > RENDER_DISTANCE) continue; //if it is in the render distance, and isn't currently loaded

            mat4_t chunkTransform = m4_translation(vec3((float)(currentChunk->chunkX * CHUNK_WIDTH), 0.0f, (float)(currentChunk->chunkZ * CHUNK_DEPTH)));
            glUniformMatrix4fv(transformMatrixLocation, 1, GL_FALSE, (float*)&chunkTransform.m); //send transform to shader program

            glBindVertexArray(currentChunk->VAO); //bind vao and draw chunk
            glDrawArrays(GL_TRIANGLES, 0, currentChunk->totalVertices);
        }

        glfwSwapBuffers(window); //Swap the draw pixels to the screen
        glfwPollEvents(); //Poll for more input events
    } 

    //Game loop end ------------------------------------------------------------------------------------------------------------------

    // Cleanup Resources
    for (int i = 0; i < MAX_ACTIVE_CHUNKS; i++) 
    {
        if (activeChunks[i].isLoaded && activeChunks[i].totalVertices > 0) 
        {
            glDeleteVertexArrays(1, &activeChunks[i].VAO);
            glDeleteBuffers(1, &activeChunks[i].VBO);
        }
    }

    free(activeChunks);
    free(tempMeshBuffer);
    glfwTerminate(); 
    return 0;
}

/* --- Utility & Callback Implementations --- */

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

//-------------------------------------------------------------------------------------------------------------------------

vec3_t processInput(GLFWwindow *window, vec3_t viewDir, vec3_t upVector, float moveSpeed, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) //if press esc, close game
        glfwSetWindowShouldClose(window, true);
        
    vec3_t movement = vec3(0.0f, 0.0f, 0.0f);
    vec3_t forward = viewDir; 
    vec3_t right   = v3_norm(v3_cross(forward, upVector)); //calc local right from local forward and global up

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement = v3_add(movement, forward); //move based on WASD
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement = v3_sub(movement, forward);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement = v3_sub(movement, right);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement = v3_add(movement, right);
    
    // Prevent division by zero / NaN when no movement keys are pressed
    float lenSq = movement.x * movement.x + movement.y * movement.y + movement.z * movement.z;
    if (lenSq == 0.0f)
    {
        return vec3(0.0f, 0.0f, 0.0f);
    }

    return v3_muls(v3_norm(movement), moveSpeed * deltaTime);    //return the normalied movement vector * movespeed * deltaTime
}

//--------------------------------------------------------------------------------------------------------------------------

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) 
{
    float xpos = (float)xposIn, ypos = (float)yposIn;

    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos; lastY = ypos;
    
    float sensitivity = 0.1f; //mouse sensitivity
    xoffset *= sensitivity; 
    yoffset *= sensitivity;
    
    yaw += xoffset; 
    pitch += yoffset;
    
    if (pitch > 89.0f)  pitch = 89.0f; //clamp pitch so you can only look up / down correctly
    if (pitch < -89.0f) pitch = -89.0f;
    
    float yawRad = yaw * (M_PI / 180.0f);
    float pitchRad = pitch * (M_PI / 180.0f);
    
    vec3_t direction;
    direction.x = cosf(yawRad) * cosf(pitchRad);
    direction.y = sinf(pitchRad);
    direction.z = sinf(yawRad) * cosf(pitchRad);
    viewDir = v3_norm(direction); //do linear algebra to determine how we turn the player based on rotational input
}

//--------------------------------------------------------------------------------------------------------------------------

bool isTransparent(int x, int y, int z, unsigned char voxels[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH]) {
    if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH) //Check if neighbor is outside the current chunk
        return true;
    return voxels[x][y][z] == 0; //Check if neighbor is air (air = 0)
}

// Function for building chunk mesh ----------------------------------------------------------------------------------------

int buildChunkMesh(Chunk* chunk, ChunkVertex* vertexBuffer) {
    int vertexCount = 0;
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_DEPTH; z++) {
                if (chunk->voxels[x][y][z] == 0) continue; //If voxel is 0 (Air), skip the rest of the loop and check the next block 
                
                int dx[] = { 0,  0, -1,  1,  0,  0}; //Define cardinal directions of blocks
                int dy[] = { 0,  0,  0,  0, -1,  1}; //back,front,left,right,down,up
                int dz[] = {-1,  1,  0,  0,  0,  0};
                
                for (int face = 0; face < 6; face++) {
                    if (isTransparent(x + dx[face], y + dy[face], z + dz[face], chunk->voxels)) //If a neighboring face is transparent, add the relevent face we need to draw to the mesh
                    {
                        for (int v = 0; v < 6; v++) //For every vertex in a face, update its attributes, then add that vertex to the vertexbuffer
                        {
                            ChunkVertex currentVert;
                            currentVert.x  = cubeFaceVertices[face][v][0] + (float)x; //Add the position within the chunk to the vertex xyz
                            currentVert.y  = cubeFaceVertices[face][v][1] + (float)y;
                            currentVert.z  = cubeFaceVertices[face][v][2] + (float)z;
                            currentVert.nx = cubeFaceVertices[face][v][3]; //Grab the normal coordinates from cubeFaceVertices
                            currentVert.ny = cubeFaceVertices[face][v][4];
                            currentVert.nz = cubeFaceVertices[face][v][5];
                            currentVert.u  = cubeFaceVertices[face][v][6]; //Grab the texture UV coordinates from cubeFaceVertices
                            currentVert.v  = cubeFaceVertices[face][v][7];
                            vertexBuffer[vertexCount++] = currentVert; //Add the vertex we want to draw to the buffer
                        }
                    }
                }
            }
        }
    }
    return vertexCount;
}


// ----------------------------------------------------------------------------------------------------------------

unsigned int compileShader(unsigned int type, const char* source) 
{
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id); //compile the passed shader code for the gpu
    
    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog); //catch if we fail to compile for some error
    }
    return id;
}
