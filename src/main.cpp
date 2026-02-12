#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

// --- Paramètres Globaux ---
const unsigned int PARTICLE_COUNT = 1000000;
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int GRID_RES = 128;       // Grille 3D 128x128x128 = 2M cellules. Pour la VRAM, on fait gaffe. 64³ c'est mieux si lourd. Essayons 64 ou 96.
// 3D Texture of 128^3 floats (RGB) ~ 8MB * 3 = 24MB. C'est OK.
// Mais le compute shader de remplissage peut être lourd.
const int GRID_RES_3D = 64; 
const float WORLD_SIZE = 3000.0f; // Taille du monde simulé pour la grille

// --- Post-Processing Bloom ---
GLuint hdrFBO;
GLuint hdrColorBuffer;
GLuint pingpongFBO[2];
GLuint pingpongColorbuffers[2];
GLuint quadVAO = 0;
GLuint quadVBO;
GLuint blurProgram;
GLuint finalProgram;

// Bloom settings
bool enableBloom = true;
float exposure = 0.8f;
float bloomIntensity = 0.8f;
int scrWidth = WINDOW_WIDTH;
int scrHeight = WINDOW_HEIGHT;

// État de l'application
bool isPaused = false;
float timeSpeed = 0.01f;
float zoom = 0.5f;
glm::vec2 cameraPos(0.0f, 0.0f);
float blackHoleMass = 0.0f;     // Initialisé à 0 pour "Sans Soleil"
float selfGravityStrength = 2000.0f; // Force par défaut pour effondrement
float frictionStrength = 4.0f;       // Friction pour aider l'agglomération
float initialRotation = 0.2f;      // Rotation faible pour effondrement chaotique
float dispersion = 1200.0f;
float galaxyThickness = 50.0f; // Epaisseur initiale

// --- Camera 3D / FPS Mode ---
bool fpsMode = false;
glm::vec3 cam3Pos(0.0f, 0.0f, 1500.0f);
glm::vec3 cam3Front(0.0f, 0.0f, -1.0f);
glm::vec3 cam3Up(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

// --- Buffers GPU ---
// Nous utilisons vec4 pour position (x,y,z,w) et vitesse (vx,vy,vz,w) pour alignement facile
GLuint posVBO[2]; 
GLuint velVBO[2]; 
GLuint VAO[2];    // Vertex Array Objects pour lier ces buffers

// Transform Feedback Object
GLuint transformFeedback[2];

// Grid / Density Map constants
GLuint densityFBO;
GLuint densityTex;
GLuint densityProgram;

// Indices pour le ping-pong
unsigned int currIdx = 0;
unsigned int nextIdx = 1;

// --- Shaders ---
GLuint physicsProgram;
GLuint renderProgram;

// --- Initialisation des Données ---
// Passage en vec4 pour la 3D (x,y,z, padding)
void InitParticlesCPU(std::vector<glm::vec4>& positions, std::vector<glm::vec4>& velocities) {
    positions.resize(PARTICLE_COUNT);
    velocities.resize(PARTICLE_COUNT);
    
    std::srand(static_cast<unsigned int>(std::time(0)));

    for (unsigned int i = 0; i < PARTICLE_COUNT; i++) {
        // Disque d'accrétion initial
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        // Distribution
        float r = dispersion * sqrt((rand() % 10000) / 10000.0f); 

        // Position 3D : X, Y sur le disque, Z pour l'épaisseur (Gaussienne approx)
        float z = ((rand() % 2000) / 1000.0f - 1.0f) * galaxyThickness * (1.0f - r/dispersion); // Plus fin au bord ? ou inverse ? Disons uniforme pour l'instant
        
        positions[i] = glm::vec4(cos(angle) * r, sin(angle) * r, z * 2.0f, 1.0f);

        // Vitesse : Rotation perpendiculaire pour l'orbite
        float dist = r + 1.0f;
        
        float orbitalSpeed = 0.0f;
        if(blackHoleMass > 1.0f) {
           orbitalSpeed = sqrt(blackHoleMass / dist) * initialRotation;
        } else {
           orbitalSpeed = r * 0.005f * initialRotation; 
        }

        glm::vec2 tangent = {-sin(angle), cos(angle)};
        velocities[i] = glm::vec4(tangent.x * orbitalSpeed, tangent.y * orbitalSpeed, 0.0f, 0.0f);
        
        // Bruit 3D
        velocities[i].x += ((rand()%100)/100.0f - 0.5f) * 2.0f; 
        velocities[i].y += ((rand()%100)/100.0f - 0.5f) * 2.0f;
        velocities[i].z += ((rand()%100)/100.0f - 0.5f) * 0.5f; // Petite vitesse verticale
    }
}

// --- Shader Sources ---

// 0. DENSITY MAP SHADERS (Projeté sur 3D)
// Nous devons utiliser un Geometry Shader pour sélectionner la couche (Layered Rendering) de la texture 3D
// OU utiliser un Compute Shader (plus standard pour la 3D grid).
// OpenGL 3.3 n'a pas de Compute Shader. OpenGL 4.3 oui.
// Mac supporte OpenGL 4.1 max. Pas de Compute Shader (facilement). 
// Solution Mac/GL3.3 : Utiliser le blending dans une Texture 3D via un Geometry Shader qui choisit gl_Layer.

const char* densityVS = R"(
#version 330 core
layout (location = 0) in vec4 aPos; 
layout (location = 1) in vec4 aVel; 

out vec4 vVel; // Pass to GS

void main() {
    // On passe juste le point
    gl_Position = aPos;
    vVel = aVel;
}
)";

const char* densityGS = R"(
#version 330 core
layout (points) in;
layout (points, max_vertices = 1) out;

in vec4 vVel[];
out vec4 gVel; // Pass to FS

uniform mat4 projection; // Ortho 3D ? Non, juste mapping coords
uniform float worldSize;
uniform int gridRes;

void main() {
    vec3 pos = gl_in[0].gl_Position.xyz;
    
    // Map world pos to grid coords [0, 1]
    vec3 uvw = (pos / worldSize) + 0.5;
    
    // Check bounds
    if(uvw.x >= 0.0 && uvw.x <= 1.0 && 
       uvw.y >= 0.0 && uvw.y <= 1.0 && 
       uvw.z >= 0.0 && uvw.z <= 1.0) 
    {
        // Select Layer for 3D Texture rendering
        // En OpenGL pour rendre dans une texture 3D, on utilise gl_Layer
        // Il faut attacher la texture 3D au complet au FBO
        gl_Layer = int(uvw.z * float(gridRes));
        
        // Coords 2D dans la slice
        gl_Position = vec4(uvw.x * 2.0 - 1.0, uvw.y * 2.0 - 1.0, 0.0, 1.0);
        gl_PointSize = 1.0; ; // 1 pixel = 1 cellule
        
        gVel = vVel[0];
        EmitVertex();
        EndPrimitive();
    }
}
)";

const char* densityFS = R"(
#version 330 core
in vec4 gVel;
out vec4 FragColor;

void main() {
    // R = Masse, G = Momentum X, B = Momentum Y, A = Momentum Z
    // On n'utilise pas alpha blending classique mais ADD blending
    float mass = 1.0;
    FragColor = vec4(mass, gVel.xyz * mass);
}
)";

// 1. PHYSICS VERTEX SHADER (Calculs GPU 3D)
const char* physicsVS = R"(
#version 330 core
layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inVel;

out vec4 outPos;
out vec4 outVel;

uniform float dt;
uniform float blackHoleMass;
uniform sampler3D gridTex; // 3D Texture
uniform float worldSize;
uniform float selfGravityStrength;
uniform float frictionStrength;
uniform float gridRes; 

// Gradient 3D (Sobel ou Central Differences)
vec3 GetGravityGradient(vec3 uvw) {
    float texel = 1.0 / gridRes;
    
    // Pour x
    float L = texture(gridTex, uvw + vec3(-texel, 0, 0)).r;
    float R = texture(gridTex, uvw + vec3( texel, 0, 0)).r;
    
    // Pour y
    float D = texture(gridTex, uvw + vec3(0, -texel, 0)).r;
    float U = texture(gridTex, uvw + vec3(0,  texel, 0)).r;
    
    // Pour z
    float B = texture(gridTex, uvw + vec3(0, 0, -texel)).r;
    float F = texture(gridTex, uvw + vec3(0, 0,  texel)).r;
    
    return vec3(R - L, U - D, F - B);
}

void main() {
    vec3 pos = inPos.xyz;
    vec3 vel = inVel.xyz;
    
    // -- PHYSIQUE --
    
    // 1. Gravité Centrale (Trou noir 3D)
    vec3 diff = vec3(0.0) - pos;
    float distSq = dot(diff, diff) + 10.0;
    float dist = sqrt(distSq);
    vec3 force = (diff / dist) * (blackHoleMass / distSq);
    
    // 2. Self-Gravity & Collisions (via Grid 3D)
    vec3 uvw = (pos / worldSize) + 0.5;
    
    if(uvw.x > 0.0 && uvw.x < 1.0 && uvw.y > 0.0 && uvw.y < 1.0 && uvw.z > 0.0 && uvw.z < 1.0) {
        
        // --- A. Gravité Locale 3D ---
        vec3 grad = GetGravityGradient(uvw);
        force += grad * selfGravityStrength; 

        // --- B. Friction / Collision (3D) ---
        vec4 cell = texture(gridTex, uvw);
        float localMass = cell.r;
        
        if(localMass > 1.0) {
            vec3 avgVel = cell.gba / localMass;
            vec3 relVel = avgVel - vel;
            // Friction isotrope 3D
            force += relVel * frictionStrength * log(localMass);
        }
    }
    
    // Intégration
    vel += force * dt;
    pos += vel * dt;
    
    outPos = vec4(pos, 1.0);
    outVel = vec4(vel, 0.0);
}
)";

// 2. RENDER SHADERS (Affichage 3D)
const char* renderVS = R"(
#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aVel;

out vec4 vColor;

uniform mat4 projection;
uniform mat4 view;

vec4 GetColorFromSpeed(float speed) {
    float t = clamp(speed / 10.0, 0.0, 1.0);
    vec4 cold = vec4(0.0, 0.5, 1.0, 1.0);
    vec4 hot  = vec4(1.0, 0.2, 0.1, 1.0);
    return mix(cold, hot, t); 
}

void main() {
    // Calcul de la position vue caméra
    vec4 viewPos = view * vec4(aPos.xyz, 1.0);
    gl_Position = projection * viewPos;
    
    // --- GESTION DE LA PROFONDEUR VISUELLE ---
    
    // 1. Taille selon la distance physique à la caméra
    // En ortho (2D), cela permet de voir les particules du "fond" plus petites que celles du "dessus"
    float distToCam = length(viewPos.xyz);
    gl_PointSize = 2000.0 / distToCam; 
    
    if(gl_PointSize > 6.0) gl_PointSize = 6.0;
    if(gl_PointSize < 1.0) gl_PointSize = 1.0;
    
    // 2. Couleur
    vColor = GetColorFromSpeed(length(aVel.xyz));
    
    // 3. Atténuation "Atmosphérique" selon la hauteur Z (pour le volume galactique)
    // Les particules très loin du plan central (Z=0) sont moins opaques
    float heightFromPlane = abs(aPos.z);
    float alphaFade = 1.0 - smoothstep(10.0, 100.0, heightFromPlane); 
    vColor.a *= (0.3 + 0.7 * alphaFade); // Garde au moins 30% d'opacité
}
)";

const char* renderFS = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    if(dot(coord,coord) > 0.25) discard;
    
    float d = length(coord);
    float alpha = 1.0 - smoothstep(0.3, 0.5, d);
    
    FragColor = vec4(vColor.rgb * 1.5, vColor.a * alpha); 
}
)";

// 3. POST-PROCESS SHADERS
const char* quadVS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}
)";

const char* blurFS = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {             
    vec2 tex_offset = 1.0 / textureSize(image, 0); 
    vec3 result = texture(image, TexCoords).rgb * weight[0]; 
    if(horizontal) {
        for(int i = 1; i < 5; ++i) {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else {
        for(int i = 1; i < 5; ++i) {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
)";

const char* finalFS = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure;
uniform float bloomIntensity;

void main() {             
    const float gamma = 2.2;
    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 result = hdrColor;
    
    if(bloom) {
        vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
        result += bloomColor * bloomIntensity; 
    }

    // Tone mapping
    result = vec3(1.0) - exp(-result * exposure);
    // Gamma correction
    result = pow(result, vec3(1.0 / gamma));
    
    FragColor = vec4(result, 1.0);
}
)";

// --- Grid Visualization Shaders ---
const char* gridVS = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* gridFS = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 gridColor;

void main() {
    FragColor = gridColor;
}
)";

// --- Compilation Shader Helper ---
GLuint CreateShader(const char* src, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    // Check errors (simplifié)
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "SHADER ERROR:\n" << infoLog << std::endl;
    }
    return shader;
}

void InitGPU() {
    // 1. Setup Buffers CPU
    std::vector<glm::vec4> initialPos;
    std::vector<glm::vec4> initialVel;
    InitParticlesCPU(initialPos, initialVel);

    // 2. Setup VAO/VBOs
    glGenVertexArrays(2, VAO);
    glGenBuffers(2, posVBO);
    glGenBuffers(2, velVBO);
    glGenTransformFeedbacks(2, transformFeedback);

    for (int i = 0; i < 2; i++) {
        glBindVertexArray(VAO[i]);

        // Position Buffer (vec4)
        glBindBuffer(GL_ARRAY_BUFFER, posVBO[i]);
        glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(glm::vec4), initialPos.data(), GL_DYNAMIC_COPY);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL); // size 4
        glEnableVertexAttribArray(0);

        // Velocity Buffer (vec4)
        glBindBuffer(GL_ARRAY_BUFFER, velVBO[i]);
        glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(glm::vec4), initialVel.data(), GL_DYNAMIC_COPY);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL); // size 4
        glEnableVertexAttribArray(1);

        // Setup Transform Feedback pour ce set
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformFeedback[i]);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posVBO[i]);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velVBO[i]);
        
        glBindVertexArray(0);
    }
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    // 3. Compile Physics Shader (TF)
    GLuint vs = CreateShader(physicsVS, GL_VERTEX_SHADER);
    physicsProgram = glCreateProgram();
    glAttachShader(physicsProgram, vs);

    // Indiquer ce qu'on veut capturer AVANT le linking
    const char* varyings[] = { "outPos", "outVel" };
    glTransformFeedbackVaryings(physicsProgram, 2, varyings, GL_SEPARATE_ATTRIBS);
    
    glLinkProgram(physicsProgram);
    // Check Link Errors...
    glDeleteShader(vs);

    // 4. Compile Render Shader
    GLuint rVS = CreateShader(renderVS, GL_VERTEX_SHADER);
    GLuint rFS = CreateShader(renderFS, GL_FRAGMENT_SHADER);
    renderProgram = glCreateProgram();
    glAttachShader(renderProgram, rVS);
    glAttachShader(renderProgram, rFS);
    glLinkProgram(renderProgram);
    glDeleteShader(rVS);
    glDeleteShader(rFS);

    // 5. Compile Grid Shader
    GLuint gVS = CreateShader(gridVS, GL_VERTEX_SHADER);
    GLuint gFS = CreateShader(gridFS, GL_FRAGMENT_SHADER);
    GLuint gridProgram = glCreateProgram();
    glAttachShader(gridProgram, gVS);
    glAttachShader(gridProgram, gFS);
    glLinkProgram(gridProgram);
    glDeleteShader(gVS);
    glDeleteShader(gFS);
}

void InitDensityMap() {
    // 1. Framebuffer
    glGenFramebuffers(1, &densityFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, densityFBO);
    
    // 2. Texture 3D (Float R,G,B,A pour Masse, MomX, MomY, MomZ)
    // GL_RGB32F -> GL_RGBA32F car on a besoin de 4 composants
    glGenTextures(1, &densityTex);
    glBindTexture(GL_TEXTURE_3D, densityTex);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, GRID_RES_3D, GRID_RES_3D, GRID_RES_3D, 0, GL_RGBA, GL_FLOAT, NULL);
    
    // Pas de mipmaps pour l'instant en 3D (couteux et complexe à générer sans compute)
    // On reste sur du Nearest/Linear local pour la gravité short-range
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Attacher la texture 3D au FBO pour Layered Rendering
    // On attache toute la texture, le Geometry Shader choisira la layer (Z)
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, densityTex, 0);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Density FBO Error! Status: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
        
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3. Shader
    GLuint vs = CreateShader(densityVS, GL_VERTEX_SHADER);
    GLuint gs = CreateShader(densityGS, GL_GEOMETRY_SHADER);
    GLuint fs = CreateShader(densityFS, GL_FRAGMENT_SHADER);
    densityProgram = glCreateProgram();
    glAttachShader(densityProgram, vs);
    glAttachShader(densityProgram, gs);
    glAttachShader(densityProgram, fs);
    glLinkProgram(densityProgram);
    // check errors...
    GLint success;
    glGetProgramiv(densityProgram, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(densityProgram, 512, NULL, infoLog);
        std::cerr << "DENSITY LINK ERROR:\n" << infoLog << std::endl;
    }
}

void InitPostProcessing(int width, int height) {
    scrWidth = width;
    scrHeight = height;

    // 1. HDR Buffer
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    
    glGenTextures(1, &hdrColorBuffer);
    glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
         std::cout << "HDR Framebuffer not complete!" << std::endl;
         
    // 2. Ping Pong buffers
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3. Shaders
    GLuint vs = CreateShader(quadVS, GL_VERTEX_SHADER);
    GLuint fs = CreateShader(blurFS, GL_FRAGMENT_SHADER);
    blurProgram = glCreateProgram();
    glAttachShader(blurProgram, vs);
    glAttachShader(blurProgram, fs);
    glLinkProgram(blurProgram);

    GLuint fs2 = CreateShader(finalFS, GL_FRAGMENT_SHADER);
    finalProgram = glCreateProgram();
    glAttachShader(finalProgram, vs);
    glAttachShader(finalProgram, fs2);
    glLinkProgram(finalProgram);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteShader(fs2);
}

void RenderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void ResetSimulation() {
    std::vector<glm::vec4> initialPos; // vec4
    std::vector<glm::vec4> initialVel;
    InitParticlesCPU(initialPos, initialVel);
    
    // Re-upload aux deux buffers pour être sûr
    for(int i=0; i<2; i++) {
        glBindBuffer(GL_ARRAY_BUFFER, posVBO[i]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, PARTICLE_COUNT * sizeof(glm::vec4), initialPos.data());
        glBindBuffer(GL_ARRAY_BUFFER, velVBO[i]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, PARTICLE_COUNT * sizeof(glm::vec4), initialVel.data());
    }
}

// --- Grid Visualization ---
GLuint gridProgram;
GLuint gridVBO, gridVAO;
bool showGrid = false;
int gridLinesCount = 0;

void InitGrid() {
    // Generate grid lines
    std::vector<glm::vec3> lines;
    float step = WORLD_SIZE / (float)GRID_RES_3D;
    float halfSize = WORLD_SIZE / 2.0f;

    // We'll draw a simplified grid, maybe every N cells to avoid clutter
    // Drawing every 4th cell line
    int stride = 4; 
    
    // Lines along X
    for (int y = 0; y <= GRID_RES_3D; y += stride) {
        for (int z = 0; z <= GRID_RES_3D; z += stride) {
             float yPos = (float)y / GRID_RES_3D * WORLD_SIZE - halfSize;
             float zPos = (float)z / GRID_RES_3D * WORLD_SIZE - halfSize;
             lines.push_back(glm::vec3(-halfSize, yPos, zPos));
             lines.push_back(glm::vec3(halfSize, yPos, zPos));
        }
    }
    // Lines along Y
    for (int x = 0; x <= GRID_RES_3D; x += stride) {
        for (int z = 0; z <= GRID_RES_3D; z += stride) {
             float xPos = (float)x / GRID_RES_3D * WORLD_SIZE - halfSize;
             float zPos = (float)z / GRID_RES_3D * WORLD_SIZE - halfSize;
             lines.push_back(glm::vec3(xPos, -halfSize, zPos));
             lines.push_back(glm::vec3(xPos, halfSize, zPos));
        }
    }
    // Lines along Z
    for (int x = 0; x <= GRID_RES_3D; x += stride) {
        for (int y = 0; y <= GRID_RES_3D; y += stride) {
             float xPos = (float)x / GRID_RES_3D * WORLD_SIZE - halfSize;
             float yPos = (float)y / GRID_RES_3D * WORLD_SIZE - halfSize;
             lines.push_back(glm::vec3(xPos, yPos, -halfSize));
             lines.push_back(glm::vec3(xPos, yPos, halfSize));
        }
    }

    // Outer box (Always draw borders)
    // ... (Optional, already covered if loops go to GRID_RES_3D)

    gridLinesCount = lines.size();

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);

    // Compile Shader
    GLuint vs = CreateShader(gridVS, GL_VERTEX_SHADER);
    GLuint fs = CreateShader(gridFS, GL_FRAGMENT_SHADER);
    gridProgram = glCreateProgram();
    glAttachShader(gridProgram, vs);
    glAttachShader(gridProgram, fs);
    glLinkProgram(gridProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

// --- MAIN ---
int main() {
    if (!glfwInit()) return -1;

    // OpenGL 3.3 suffit pour Transform Feedback de base, mais 4.1 est mieux sur Mac
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Galaxy GPU Sim", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Désactiver V-Sync pour voir les max FPS

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed GLAD" << std::endl;
        return -1;
    }
    
    // Enable Point Sprite / Size control
    glEnable(GL_PROGRAM_POINT_SIZE); 
    // Sur Mac/OpenGL 3+, GL_POINT_SPRITE est implicite ou géré par glEnable(GL_PROGRAM_POINT_SIZE)
    
    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    InitGPU();
    Init
    InitPostProcessing(WINDOW_WIDTH, WINDOW_HEIGHT);
    InitGrid();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        static double lastTime = glfwGetTime();
        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // Limite dt pour stabilité physique
        if (dt > 0.1f) dt = 0.1f;

        // UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Gestion Input Mode (FPS vs Normal)
        if (fpsMode) {
             glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
             glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (fpsMode) {
            // --- FPS INPUT HANDLING ---
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                fpsMode = false;
                firstMouse = true; // Reset mouse look next time
            }

            // Mouse Look
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            if (firstMouse) {
                lastX = (float)xpos;
                lastY = (float)ypos;
                firstMouse = false;
            }

            float xoffset = (float)xpos - lastX;
            float yoffset = lastY - (float)ypos; // Reversed since y-coordinates go from bottom to top
            lastX = (float)xpos;
            lastY = (float)ypos;

            float sensitivity = 0.1f; 
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            yaw   += xoffset;
            pitch += yoffset;

            // Constrain pitch
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;

            glm::vec3 direction;
            direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            direction.y = sin(glm::radians(pitch));
            direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cam3Front = glm::normalize(direction);

            // WASD Movement
            float cameraSpeed = 1000.0f * dt; 
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraSpeed *= 3.0f; // Boost

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                cam3Pos += cameraSpeed * cam3Front;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                cam3Pos -= cameraSpeed * cam3Front;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                cam3Pos -= glm::normalize(glm::cross(cam3Front, cam3Up)) * cameraSpeed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                cam3Pos += glm::normalize(glm::cross(cam3Front, cam3Up)) * cameraSpeed;

        } else {
            // --- 2D / UI Controls ---
            ImGui::Begin("GPU Controls");
            ImGui::Text("Particules: %u", PARTICLE_COUNT);
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            
            if (ImGui::Button("Reset / Regen")) ResetSimulation();
            if (ImGui::Button("ENTER FPS MODE (3D Fly)")) {
                fpsMode = true;
                firstMouse = true;
            }
            ImGui::TextColored(ImVec4(1,1,0,1), "Press ESC to exit FPS mode");

            ImGui::Separator();
            ImGui::SliderFloat("Rotation Init", &initialRotation, 0.0f, 5.0f);
            ImGui::SliderFloat("Dispersion", &dispersion, 100.0f, 1000.0f);
            ImGui::SliderFloat("Epaisseur Galaxie", &galaxyThickness, 0.0f, 300.0f); // Nouveau controle
            ImGui::SliderFloat("Self-Gravity", &selfGravityStrength, 0.0f, 5000.0f);
            ImGui::SliderFloat("Friction (Sticky)", &frictionStrength, 0.0f, 10.0f);
            ImGui::Separator();
            ImGui::Checkbox("Bloom", &enableBloom);
            ImGui::SliderFloat("Bloom Intensity", &bloomIntensity, 0.0f, 2.0f);
            ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f);
            ImGui::Checkbox("Show Calculation Grid", &showGrid); // Add Checkbox
            ImGui::Separator();
            ImGui::SliderFloat("Masse Trou Noir", &blackHoleMass, 0.0f, 100000.0f);
            ImGui::SliderFloat("Time Speed", &timeSpeed, 0.01f, 5.0f);
            ImGui::SliderFloat("Zoom", &zoom, 0.01f, 5.0f);
            ImGui::End();

            // --- Camera Controls (Mouse Pan & Zoom + Keyboard) ---
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureMouse) {
                // Pan avec Clic Gauche maintenu
                // Correction: Sensibilité inversement proportionnelle au zoom (comme le clavier) pour un drag naturel
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                    // Facteur d'ajustement arbitraire pour que le "drag" suive bien la souris (dépend de la résolution/viewSize)
                    // Si viewSize = 1000/zoom, alors 1 pixel écran correspond environ à (1000/zoom)/ScreenHeight en monde
                    // On utilise 2.0f comme facteur empirique ici basique
                    float dragSpeed = 2.0f / zoom; 
                    cameraPos.x -= delta.x * dragSpeed;
                    cameraPos.y += delta.y * dragSpeed; 
                    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                }
                
                // Zoom Molette
                if (io.MouseWheel != 0.0f) {
                    zoom -= io.MouseWheel * zoom * 0.1f;
                    if (zoom < 0.01f) zoom = 0.01f;
                    if (zoom > 10.0f) zoom = 10.0f;
                }
            }
            
            // Clavier (Flèches / WASD)
            // Correction: Vitesse inversement proportionnelle au zoom pour être plus rapide loin et plus précis près
            float moveSpeed = 1000.0f * dt / zoom; 

            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
                cameraPos.x += moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) 
                cameraPos.x -= moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
                cameraPos.y += moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
                cameraPos.y -= moveSpeed;
        }

            
        // --- STEP 1: PHYSICS UPDATE (Transform Feedback) ---
        if (!isPaused) {
            // -- STEP 1.A: Compute Density Map --
            glBindFramebuffer(GL_FRAMEBUFFER, densityFBO);
            // Viewport doit couvrir x,y de la texture 3D
            glViewport(0, 0, GRID_RES_3D, GRID_RES_3D);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            // Additive blending
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE); 
            
            glUseProgram(densityProgram);
            glUniform1f(glGetUniformLocation(densityProgram, "worldSize"), WORLD_SIZE);
            glUniform1i(glGetUniformLocation(densityProgram, "gridRes"), GRID_RES_3D);
            
            glBindVertexArray(VAO[currIdx]);
            glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);
            
            glDisable(GL_BLEND);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // --- GENERATE MIPMAPS ---
            // Pas de mipmaps 3D auto en OpenGL 3.3 facilement
            // glGenerateMipmap marche pour TEXTURE_3D en OpenGL 4.0+
            // Testons:
            // glBindTexture(GL_TEXTURE_3D, densityTex);
            // glGenerateMipmap(GL_TEXTURE_3D);
            
            
            // -- STEP 1.B: Physics Update with TF --
            glUseProgram(physicsProgram);
            
            glUniform1f(glGetUniformLocation(physicsProgram, "dt"), dt * timeSpeed);
            glUniform1f(glGetUniformLocation(physicsProgram, "blackHoleMass"), blackHoleMass);
            glUniform2f(glGetUniformLocation(physicsProgram, "centerPos"), 0.0f, 0.0f);
            glUniform1f(glGetUniformLocation(physicsProgram, "worldSize"), WORLD_SIZE);
            glUniform1f(glGetUniformLocation(physicsProgram, "selfGravityStrength"), selfGravityStrength);
            glUniform1f(glGetUniformLocation(physicsProgram, "frictionStrength"), frictionStrength);
            glUniform1f(glGetUniformLocation(physicsProgram, "gridRes"), (float)GRID_RES_3D);
            
            // Bind Texture Grid 3D
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, densityTex);
            glUniform1i(glGetUniformLocation(physicsProgram, "gridTex"), 0);

            // On désactive le rendu graphique, on veut juste écrire dans les buffers
            glEnable(GL_RASTERIZER_DISCARD);

            // Bind Source VAO (Current)
            glBindVertexArray(VAO[currIdx]);

            // Bind Destination TF (Next)
            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformFeedback[nextIdx]);

            // Start TF
            glBeginTransformFeedback(GL_POINTS);
                glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);
            glEndTransformFeedback();

            // Cleanup & Swap
            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
            glDisable(GL_RASTERIZER_DISCARD);
            
            // Swap indices ping-pong
            std::swap(currIdx, nextIdx);
        }

        // --- STEP 2: RENDER (TO HDR FBO) ---
        // On s'assure que la taille est OK (resize dynamique possible)
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        if(w != scrWidth || h != scrHeight) {
             InitPostProcessing(w, h); // Reconstruction brutale si resize
        }

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.02f, 1.0f); // Noir spatial
        glClear(GL_COLOR_BUFFER_BIT);

        // Enable Additive Blending pour effet "Galaxie lumineuse"
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glUseProgram(renderProgram);
        
        // Matrices
        glm::mat4 proj;
        glm::mat4 view;

        if (fpsMode) {
             // 3D Perspective
             proj = glm::perspective(glm::radians(60.0f), (float)w / (float)h, 1.0f, 10000.0f); 
             view = glm::lookAt(cam3Pos, cam3Pos + cam3Front, cam3Up);
        } else {
             // 2D Ortho (Original)
             // Correction: le zoom inverse (plus c'est grand, plus on voit large)
             // Pour l'UI, un gros chiffre = gros zoom (vision de près), donc petit champ
             float viewSize = 1000.0f / zoom; 
             float halfW = viewSize * (float)w / (float)h;
             float halfH = viewSize;
             
             // En mode 2D, on regarde d'en haut (Top-Down) mais on projette tout
             // Ortho est naturellement profond infini mais sans perspective
             proj = glm::ortho(
                -halfW + cameraPos.x, 
                 halfW + cameraPos.x, 
                -halfH + cameraPos.y, 
                 halfH + cameraPos.y,
                 -5000.0f, 5000.0f // Plane de coupe large pour voir les particules en Z
            );
            // On tourne la caméra pour qu'elle regarde Z=0 bien à plat
            // Par défaut ortho projette Z sur l'écran si on ne change rien
            view = glm::lookAt(
                glm::vec3(cameraPos.x, cameraPos.y, 1000.0f), // Eye
                glm::vec3(cameraPos.x, cameraPos.y, 0.0f),    // Center
                glm::vec3(0.0f, 1.0f, 0.0f)                   // Up
            );
        }

        glUniformMatrix4fv(glGetUniformLocation(renderProgram, "projection"), 1, GL_FALSE, &proj[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(renderProgram, "view"), 1, GL_FALSE, &view[0][0]);

        // Dessiner le buffer "Current" (qui vient d'être mis à jour)
        glBindVertexArray(VAO[currIdx]); 
        glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);
        
        glDisable(GL_BLEND);

        // --- STEP 3: POST-PROCESSING (BLOOM) ---
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // On ne rend pas tout de suite à l'écran, on va faire du Ping-Pong Blur
        
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10; // 10 passes de flou
        
        glUseProgram(blurProgram);
        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]); 
            glUniform1i(glGetUniformLocation(blurProgram, "horizontal"), horizontal);
            
            // Premier passage : on prend l'image HDR, sinon les buffers ping-pong précédents
            glBindTexture(GL_TEXTURE_2D, first_iteration ? hdrColorBuffer : pingpongColorbuffers[!horizontal]); 
            
            RenderQuad();
            
            horizontal = !horizontal;
            if (first_iteration) first_iteration = false;
        }
        
        // Final Combine
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, w, h); // On s'assure de l'écran principal
        glClear(GL_COLOR_BUFFER_BIT); // Clear de sûreté

        glUseProgram(finalProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer); // La scène
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]); // Le blur
        
        glUniform1i(glGetUniformLocation(finalProgram, "scene"), 0);
        glUniform1i(glGetUniformLocation(finalProgram, "bloomBlur"), 1);
        glUniform1i(glGetUniformLocation(finalProgram, "bloom"), enableBloom);
        glUniform1f(glGetUniformLocation(finalProgram, "exposure"), exposure);
        glUniform1f(glGetUniformLocation(finalProgram, "bloomIntensity"), bloomIntensity);
        
        RenderQuad();

        // Render UI (Par dessus tout le post-process)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(2, VAO);
    glDeleteBuffers(2, posVBO);
    glDeleteBuffers(2, velVBO);
    // ... clean imGui etc
    return 0;
}
