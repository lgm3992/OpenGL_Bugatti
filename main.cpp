#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ===== Vertex Shader =====
const char* vertexShaderSource = R"(
#version 330 core
precision mediump float;

uniform mat4 worldMat, viewMat, projMat;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 v_normal;
out vec2 v_texCoord;

void main() {
    gl_Position = projMat * viewMat * worldMat * vec4(position, 1.0);
    v_normal = mat3(transpose(inverse(worldMat))) * normal;
    v_texCoord = texCoord;
}
)";

// ===== Fragment Shader =====
const char* fragmentShaderSource = R"(
#version 330 core
precision mediump float;

in vec3 v_normal;
in vec2 v_texCoord;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 norm = normalize(v_normal);

    // 다양한 방향의 조명
    vec3 keyLight = normalize(vec3(0.5, 1.0, 0.3));
    vec3 fillLight = normalize(vec3(-0.3, 0.5, -0.2));
    vec3 rimLight = normalize(vec3(0.0, 0.3, -1.0));

    // 조명 계산
    float keyDiff = max(dot(norm, keyLight), 0.0);
    float fillDiff = max(dot(norm, fillLight), 0.0);
    float rimDiff = max(dot(norm, rimLight), 0.0);

    // 3가지 뚜렷한 색상 정의
    vec3 primaryBlue = vec3(0.05, 0.25, 0.65);    // 진한 Bugatti 블루 (메인 바디)
    vec3 blackColor = vec3(0.08, 0.08, 0.12);     // 검정 (하부/타이어/그릴)
    vec3 silverColor = vec3(0.75, 0.78, 0.82);    // 밝은 실버 (루프/악센트)

    // 법선 방향에 따라 색상 영역 결정
    float normalY = norm.y;

    vec3 objectColor;

    // 아래를 향하는 면 (하부, 타이어, 섀시) - 검정
    if (normalY < -0.2) {
        objectColor = blackColor;
    }
    // 위를 강하게 향하는 면 (루프, 후드) - 실버
    else if (normalY > 0.7) {
        objectColor = silverColor;
    }
    // 중간 영역 (사이드 패널, 도어) - 블루
    else {
        // 부드러운 전환
        if (normalY > 0.4) {
            float t = smoothstep(0.4, 0.7, normalY);
            objectColor = mix(primaryBlue, silverColor, t);
        } else if (normalY < 0.0) {
            float t = smoothstep(-0.2, 0.0, normalY);
            objectColor = mix(blackColor, primaryBlue, t);
        } else {
            objectColor = primaryBlue;
        }
    }

    // 조명 적용
    vec3 ambient = 0.25 * objectColor;
    vec3 diffuse = keyDiff * 0.6 * objectColor;
    vec3 fill = fillDiff * 0.2 * objectColor * 0.5;
    vec3 rim = rimDiff * 0.3 * vec3(0.6, 0.7, 0.9);

    // 스페큘러 하이라이트 (실버 영역에 강조)
    vec3 viewDir = vec3(0.0, 0.0, 1.0);
    vec3 reflectDir = reflect(-keyLight, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    // 실버 영역에 더 강한 반사
    float specularStrength = normalY > 0.5 ? 0.8 : 0.3;
    vec3 specular = spec * silverColor * specularStrength;

    vec3 finalColor = ambient + diffuse + fill + rim + specular;
    fragColor = vec4(finalColor, 1.0);
}
)";

// ===== Camera Class =====
class Camera {
public:
    glm::vec3 position;
    float yaw;
    float pitch;
    float sensitivity;
    float fov;
    float minFov;
    float maxFov;

    Camera() : position(0.0f, 25.0f, 80.0f), yaw(-90.0f), pitch(-15.0f),
               sensitivity(0.1f), fov(45.0f), minFov(10.0f), maxFov(90.0f) {}

    glm::mat4 GetViewMatrix() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        glm::vec3 cameraFront = glm::normalize(front);
        return glm::lookAt(position, position + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    void ProcessMouseScroll(float yoffset) {
        fov -= yoffset * 2.0f;
        if (fov < minFov) fov = minFov;
        if (fov > maxFov) fov = maxFov;
    }

    float GetFov() const {
        return fov;
    }
};

// Global camera instance
Camera camera;

// Mouse state
float lastX = 400.0f, lastY = 300.0f;
bool firstMouse = true;

// Mouse callback
void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    float xposf = static_cast<float>(xpos);
    float yposf = static_cast<float>(ypos);

    if (firstMouse) {
        lastX = xposf;
        lastY = yposf;
        firstMouse = false;
    }

    float xoffset = xposf - lastX;
    float yoffset = lastY - yposf; // reversed since y-coordinates range from bottom to top
    lastX = xposf;
    lastY = yposf;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Scroll callback
void scroll_callback(GLFWwindow*, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// ===== Vertex/ObjData =====
struct Vertex {
    glm::vec3 pos;
    glm::vec3 nor;
    glm::vec2 tex;
};

struct ObjData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

// ===== OBJ Loader (v/vt/vn, v//vn, v 지원) =====
bool loadOBJ(const std::string& filename, ObjData& objData) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << filename << std::endl;
        return false;
    }

    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec3> temp_normals;
    std::vector<glm::vec2> temp_texcoords;

    // Vertex cache to reuse vertices: key = "pos/tex/norm", value = index
    std::map<std::string, unsigned int> vertexCache;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 p;
            iss >> p.x >> p.y >> p.z;
            temp_positions.push_back(p);
        }
        else if (prefix == "vn") {
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        else if (prefix == "vt") {
            glm::vec2 t;
            iss >> t.x >> t.y;
            temp_texcoords.push_back(t);
        }
        else if (prefix == "f") {
            std::string v1, v2, v3, v4;
            iss >> v1 >> v2 >> v3;

            // Check if there's a 4th vertex (quad face)
            bool isQuad = false;
            if (iss >> v4) {
                isQuad = true;
            }

            auto parseVertex = [&](const std::string& s) -> unsigned int {
                // Check cache first
                auto it = vertexCache.find(s);
                if (it != vertexCache.end()) {
                    return it->second;
                }

                unsigned int pi = 0, ti = 0, ni = 0;
                bool has_tex = false, has_norm = false;

                size_t p1 = s.find('/');
                if (p1 == std::string::npos) {
                    // Format: v
                    pi = std::stoul(s) - 1;
                } else {
                    pi = std::stoul(s.substr(0, p1)) - 1;
                    size_t p2 = s.find('/', p1 + 1);
                    if (p2 == std::string::npos) {
                        // Format: v/vt
                        ti = std::stoul(s.substr(p1 + 1)) - 1;
                        has_tex = true;
                    } else {
                        // Format: v/vt/vn or v//vn
                        if (p2 > p1 + 1) {
                            ti = std::stoul(s.substr(p1 + 1, p2 - p1 - 1)) - 1;
                            has_tex = true;
                        }
                        if (p2 + 1 < s.size()) {
                            ni = std::stoul(s.substr(p2 + 1)) - 1;
                            has_norm = true;
                        }
                    }
                }

                Vertex v{};
                if (pi < temp_positions.size()) {
                    v.pos = temp_positions[pi];
                } else {
                    std::cerr << "Invalid position index: " << pi << std::endl;
                    v.pos = glm::vec3(0.0f);
                }

                if (has_norm && ni < temp_normals.size())
                    v.nor = temp_normals[ni];
                else
                    v.nor = glm::vec3(0.0f, 1.0f, 0.0f);

                if (has_tex && ti < temp_texcoords.size())
                    v.tex = temp_texcoords[ti];
                else
                    v.tex = glm::vec2(0.0f, 0.0f);

                objData.vertices.push_back(v);
                unsigned int idx = (unsigned int)(objData.vertices.size() - 1);
                vertexCache[s] = idx;
                return idx;
            };

            unsigned int idx1 = parseVertex(v1);
            unsigned int idx2 = parseVertex(v2);
            unsigned int idx3 = parseVertex(v3);

            // First triangle
            objData.indices.push_back(idx1);
            objData.indices.push_back(idx2);
            objData.indices.push_back(idx3);

            // If quad, create second triangle
            if (isQuad) {
                unsigned int idx4 = parseVertex(v4);
                objData.indices.push_back(idx1);
                objData.indices.push_back(idx3);
                objData.indices.push_back(idx4);
            }
        }
    }

    file.close();

    std::cout << "Loaded OBJ file: " << filename << "\n";
    std::cout << "  Vertices : " << objData.vertices.size() << "\n";
    std::cout << "  Indices  : " << objData.indices.size()  << "\n";
    std::cout << "  Triangles: " << objData.indices.size() / 3 << "\n";
    std::cout << "  Cache entries: " << vertexCache.size() << " (should match vertices)\n";

    float cacheHitRate = 0.0f;
    if (objData.indices.size() > 0) {
        cacheHitRate = 100.0f * (1.0f - (float)objData.vertices.size() / (float)objData.indices.size());
        std::cout << "  Cache hit rate: " << cacheHitRate << "%\n";
    }

    return !objData.vertices.empty() && !objData.indices.empty();
}

// ===== Texture Loader =====
unsigned int loadTexture(const std::string& filename) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "Loaded texture: " << filename << " (" << width << "x" << height << ", " << nrChannels << " channels)\n";
    } else {
        std::cout << "Failed to load texture: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// ===== Shader / Program =====
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cout << "Shader compilation failed:\n" << log << std::endl;
    }
    return shader;
}

unsigned int createProgram(unsigned int vs, unsigned int fs) {
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cout << "Program linking failed:\n" << log << std::endl;
    }
    return prog;
}

int main(int argc, char* argv[]) {
    std::string objFilePath = "models/bugatti.obj";
    if (argc > 1) objFilePath = argv[1];

    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(800, 600, "OpenGL Bugatti OBJ", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Setup mouse input
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    // 백페이스 컬링 비활성화 (기본값이지만 명시적으로)
    glDisable(GL_CULL_FACE);

    unsigned int vs = compileShader(GL_VERTEX_SHADER,   vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int program = createProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glUseProgram(program);

    ObjData objData;
    if (!loadOBJ(objFilePath, objData)) {
        std::cout << "OBJ load failed. Check models/bugatti.obj\n";
        glfwTerminate();
        return -1;
    }

    // Texture not used - rendering with solid color

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(objData.vertices.size() * sizeof(Vertex)),
                 objData.vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(objData.indices.size() * sizeof(unsigned int)),
                 objData.indices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, nor));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, tex));

    int worldMatLoc = glGetUniformLocation(program, "worldMat");
    int viewMatLoc  = glGetUniformLocation(program, "viewMat");
    int projMatLoc  = glGetUniformLocation(program, "projMat");

    glm::mat4 worldMatrix, viewMatrix, projMatrix;

    std::cout << "\n=== Controls ===\n";
    std::cout << "Mouse: Look around\n";
    std::cout << "Mouse Wheel: Zoom in/out\n";
    std::cout << "W: Toggle wireframe mode\n";
    std::cout << "ESC: exit\n";

    bool wireframeMode = false;
    bool wKeyPressed = false;

    while (!glfwWindowShouldClose(window)) {
        // Time for animation
        float currentFrame = static_cast<float>(glfwGetTime());

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Toggle wireframe mode
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && !wKeyPressed) {
            wKeyPressed = true;
            wireframeMode = !wireframeMode;
            if (wireframeMode) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                std::cout << "Wireframe mode: ON\n";
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                std::cout << "Wireframe mode: OFF\n";
            }
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) {
            wKeyPressed = false;
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(VAO);

        // --- World: 자동 회전 ---
        // Bounding box: X[-62.97, 44.90], Y[-4.50, 38.36], Z[-59.38, 40.48]
        // Center: (-9.04, 16.93, -9.45)
        worldMatrix = glm::mat4(1.0f);
        // GLM은 나중에 곱한 변환이 먼저 적용됨
        worldMatrix = glm::scale(worldMatrix, glm::vec3(1.2f, 1.2f, 1.2f)); // 3. 약간 키우기
        worldMatrix = glm::rotate(worldMatrix, currentFrame * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 2. 회전
        worldMatrix = glm::translate(worldMatrix, glm::vec3(9.04f, -16.93f, 9.45f)); // 1. 모델 중심을 원점으로

        // --- View: 마우스로 제어되는 카메라 ---
        viewMatrix = camera.GetViewMatrix();

        // --- Projection ---
        projMatrix = glm::perspective(glm::radians(camera.GetFov()),
                                      800.0f / 600.0f,
                                      0.1f, 1000.0f);

        glUniformMatrix4fv(worldMatLoc, 1, GL_FALSE, glm::value_ptr(worldMatrix));
        glUniformMatrix4fv(viewMatLoc,  1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projMatLoc,  1, GL_FALSE, glm::value_ptr(projMatrix));

        glDrawElements(GL_TRIANGLES,
                       (GLsizei)objData.indices.size(),
                       GL_UNSIGNED_INT,
                       0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}