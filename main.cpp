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

uniform sampler2D textureSampler;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(textureSampler, v_texCoord);
}
)";

// ===== Camera Class =====
class Camera {
public:
    glm::vec3 position;
    float yaw;
    float pitch;
    float sensitivity;

    Camera() : position(0.0f, 0.0f, 5.0f), yaw(-90.0f), pitch(0.0f), sensitivity(0.1f) {}

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
            std::string v1, v2, v3;
            iss >> v1 >> v2 >> v3;

            auto parseVertex = [&](const std::string& s) {
                unsigned int pi = 0, ti = 0, ni = 0;
                size_t p1 = s.find('/');
                if (p1 == std::string::npos) {
                    pi = std::stoul(s) - 1;
                } else {
                    pi = std::stoul(s.substr(0, p1)) - 1;
                    size_t p2 = s.find('/', p1 + 1);
                    if (p2 == std::string::npos) {
                        ti = std::stoul(s.substr(p1 + 1)) - 1;
                    } else {
                        if (p2 > p1 + 1)
                            ti = std::stoul(s.substr(p1 + 1, p2 - p1 - 1)) - 1;
                        if (p2 + 1 < s.size())
                            ni = std::stoul(s.substr(p2 + 1)) - 1;
                    }
                }

                Vertex v{};
                v.pos = temp_positions[pi];
                if (!temp_normals.empty() && ni < temp_normals.size())
                    v.nor = temp_normals[ni];
                else
                    v.nor = glm::vec3(0.0f, 1.0f, 0.0f);

                if (!temp_texcoords.empty() && ti < temp_texcoords.size())
                    v.tex = temp_texcoords[ti];
                else
                    v.tex = glm::vec2(0.0f, 0.0f);

                objData.vertices.push_back(v);
                return (unsigned int)(objData.vertices.size() - 1);
            };

            objData.indices.push_back(parseVertex(v1));
            objData.indices.push_back(parseVertex(v2));
            objData.indices.push_back(parseVertex(v3));
        }
    }

    file.close();

    std::cout << "Loaded OBJ file: " << filename << "\n";
    std::cout << "  Vertices : " << objData.vertices.size() << "\n";
    std::cout << "  Indices  : " << objData.indices.size()  << "\n";
    std::cout << "  Triangles: " << objData.indices.size() / 3 << "\n";

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
    std::string objFilePath = "models/cat.obj";
    if (argc > 1) objFilePath = argv[1];

    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(800, 600, "OpenGL Cat OBJ", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Setup mouse input
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int vs = compileShader(GL_VERTEX_SHADER,   vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int program = createProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glUseProgram(program);

    ObjData objData;
    if (!loadOBJ(objFilePath, objData)) {
        std::cout << "OBJ load failed. Check models/cat.obj\n";
        glfwTerminate();
        return -1;
    }

    // Load texture
    unsigned int texture = loadTexture("textures/cat.jpg");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

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
    int textureLoc  = glGetUniformLocation(program, "textureSampler");

    // Bind texture to texture unit 0
    glUniform1i(textureLoc, 0);

    glm::mat4 worldMatrix, viewMatrix, projMatrix;

    std::cout << "\n=== Controls ===\n";
    std::cout << "Mouse: Look around\n";
    std::cout << "ESC: exit\n";

    while (!glfwWindowShouldClose(window)) {
        // Time for animation
        float currentFrame = static_cast<float>(glfwGetTime());

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(VAO);

        // --- World: 위치 낮추기 + 자동 회전 + 세우기 ---
        worldMatrix = glm::mat4(1.0f);
        worldMatrix = glm::translate(worldMatrix, glm::vec3(0.0f, -1.5f, 0.0f)); // 고양이를 아래로
        worldMatrix = glm::rotate(worldMatrix, currentFrame * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Y축 자동 회전
        worldMatrix = glm::rotate(worldMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // X축으로 세우기
        worldMatrix = glm::scale(worldMatrix, glm::vec3(0.05f));

        // --- View: 마우스로 제어되는 카메라 ---
        viewMatrix = camera.GetViewMatrix();

        // --- Projection ---
        projMatrix = glm::perspective(glm::radians(45.0f),
                                      800.0f / 600.0f,
                                      0.1f, 100.0f);

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
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}