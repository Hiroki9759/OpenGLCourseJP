#include <cmath>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define GLFW_INCLUDE_GLU
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// �f�B���N�g���̐ݒ�t�@�C��
#include "common.h"

static int WIN_WIDTH   = 500;                       // �E�B���h�E�̕�
static int WIN_HEIGHT  = 500;                       // �E�B���h�E�̍���
static const char *WIN_TITLE = "OpenGL Course";     // �E�B���h�E�̃^�C�g��

static const double PI = 4.0 * std::atan(1.0);

// �V�F�[�_�t�@�C��
static const std::string VERT_SHADER_FILE = std::string(SHADER_DIRECTORY) + "render.vert";
static const std::string FRAG_SHADER_FILE = std::string(SHADER_DIRECTORY) + "render.frag";

// ���f���̃t�@�C��
static const std::string BUNNY_FILE = std::string(DATA_DIRECTORY) + "bunny.obj";

// ���_�ԍ��z��̑傫��
static size_t indexBufferSize = 0;

// ���_�I�u�W�F�N�g
struct Vertex {
    Vertex(const glm::vec3 &position_, const glm::vec3 &normal_)
        : position(position_)
        , normal(normal_) {
    }

    glm::vec3 position;
    glm::vec3 normal;
};

// �o�b�t�@���Q�Ƃ���ԍ�
GLuint vaoId;
GLuint vertexBufferId;
GLuint indexBufferId;

// �V�F�[�_���Q�Ƃ���ԍ�
GLuint vertShaderId;
GLuint fragShaderId;
GLuint programId;

// �����̂̉�]�p�x
static float theta = 0.0f;

// VAO�̏�����
void initVAO() {
    // ���f���̃��[�h
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, BUNNY_FILE.c_str());
    if (!err.empty()) {
        std::cerr << "[WARNING] " << err << std::endl;
    }

    if (!success) {
        std::cerr << "Failed to load OBJ file: " << BUNNY_FILE << std::endl;
        exit(1);
    }

    // Vertex�z��̍쐬
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    for (int s = 0; s < shapes.size(); s++) {
        const tinyobj::mesh_t &mesh = shapes[s].mesh;
        for (int i = 0; i < mesh.indices.size(); i++) {
            const tinyobj::index_t &index = mesh.indices[i];
            
            glm::vec3 position, normal;

            if (index.vertex_index >= 0) {
                position = glm::vec3(attrib.vertices[index.vertex_index * 3 + 0],
                                     attrib.vertices[index.vertex_index * 3 + 1],
                                     attrib.vertices[index.vertex_index * 3 + 2]);
            }

            if (index.normal_index >= 0) {
                normal = glm::vec3(attrib.normals[index.normal_index * 3 + 0],
                                   attrib.normals[index.normal_index * 3 + 1],
                                   attrib.normals[index.normal_index * 3 + 2]);
            }
            
            const unsigned int vertexIndex = vertices.size();
            vertices.push_back(Vertex(position, normal));
            indices.push_back(vertexIndex);
        }
    }
    indexBufferSize = indices.size();

    // VAO�̍쐬
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    // ���_�o�b�t�@�̍쐬
    glGenBuffers(1, &vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // ���_�o�b�t�@�̗L����
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // ���_�ԍ��o�b�t�@�̍쐬
    glGenBuffers(1, &indexBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(),
                 indices.data(), GL_STATIC_DRAW);

    // VAO��OFF�ɂ��Ă���
    glBindVertexArray(0);
}

GLuint compileShader(const std::string &filename, GLuint type) {
    // �V�F�[�_�̍쐬
    GLuint shaderId = glCreateShader(type);
    
    // �t�@�C���̓ǂݍ���
    std::ifstream reader;
    size_t codeSize;
    std::string code;

    // �t�@�C�����J��
    reader.open(filename.c_str(), std::ios::in);
    if (!reader.is_open()) {
        // �t�@�C�����J���Ȃ�������G���[���o���ďI��
        fprintf(stderr, "Failed to load vertex shader: %s\n", VERT_SHADER_FILE);
        exit(1);
    }

    // �t�@�C�������ׂēǂ�ŕϐ��Ɋi�[ (����)
    reader.seekg(0, std::ios::end);             // �t�@�C���ǂݎ��ʒu���I�[�Ɉړ� 
    codeSize = reader.tellg();                  // ���݂̉ӏ�(=�I�[)�̈ʒu���t�@�C���T�C�Y
    code.resize(codeSize);                      // �R�[�h���i�[����ϐ��̑傫����ݒ�
    reader.seekg(0);                            // �t�@�C���̓ǂݎ��ʒu��퓬�Ɉړ�
    reader.read(&code[0], codeSize);            // �擪����t�@�C���T�C�Y����ǂ�ŃR�[�h�̕ϐ��Ɋi�[

    // �t�@�C�������
    reader.close();

    // �R�[�h�̃R���p�C��
    GLint compileStatus;
    const char *codeChars = code.c_str();
    glShaderSource(shaderId, 1, &codeChars, NULL);
    glCompileShader(shaderId);
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus == GL_FALSE) {
        // �R���p�C�������s������G���[���b�Z�[�W�ƃ\�[�X�R�[�h��\�����ďI��
        fprintf(stderr, "Failed to compile vertex shader!\n");

        GLint logLength;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            std::string errMsg;
            errMsg.resize(logLength);
            glGetShaderInfoLog(shaderId, logLength, &length, &errMsg[0]);

            fprintf(stderr, "[ ERROR] %s\n", errMsg.c_str());
            fprintf(stderr, "%s\n", code.c_str());
        }
        exit(1);
    }

    return shaderId;
}

// �V�F�[�_�̏�����
void initShaders() {
    // �V�F�[�_�̍쐬
    vertShaderId = compileShader(VERT_SHADER_FILE, GL_VERTEX_SHADER);
    fragShaderId = compileShader(FRAG_SHADER_FILE, GL_FRAGMENT_SHADER);

    // �V�F�[�_�v���O�����̃����N
    programId = glCreateProgram();
    glAttachShader(programId, vertShaderId);
    glAttachShader(programId, fragShaderId);

    GLint linkState;
    glLinkProgram(programId);
    glGetProgramiv(programId, GL_LINK_STATUS, &linkState);
    if (linkState == GL_FALSE) {
        // �����N�Ɏ��s������G���[���b�Z�[�W��\�����ďI��
        fprintf(stderr, "Failed to link shaders!\n");

        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            std::string errMsg;
            errMsg.resize(logLength);
            glGetProgramInfoLog(programId, logLength, &length, &errMsg[0]);

            fprintf(stderr, "[ ERROR ] %s\n", errMsg.c_str());
        }
        exit(1);
    }

    // �V�F�[�_�̖��������Ă���
    glUseProgram(0);
}

// OpenGL�̏������֐�
void initializeGL() {
    // �[�x�e�X�g�̗L����
    glEnable(GL_DEPTH_TEST);

    // �w�i�F�̐ݒ� (��)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // VAO�̏�����
    initVAO();

    // �V�F�[�_�̗p��
    initShaders();
}

// OpenGL�̕`��֐�
void paintGL() {
    // �w�i�F�̕`��
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // �r���[�|�[�g�ϊ��̎w��
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    // ���W�̕ϊ�
    glm::mat4 projMat = glm::perspective(45.0f,
        (float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 1000.0f);

    glm::mat4 viewMat = glm::lookAt(glm::vec3(3.0f, 4.0f, 5.0f),   // ���_�̈ʒu
                                    glm::vec3(0.0f, 0.0f, 0.0f),   // ���Ă����
                                    glm::vec3(0.0f, 1.0f, 0.0f));  // ���E�̏����

    glm::mat4 modelMat = glm::rotate(theta, glm::vec3(0.0f, 1.0f, 0.0f)); 

    glm::mat4 mvMat = viewMat * modelMat;
    glm::mat4 mvpMat = projMat * viewMat * modelMat;

    // VAO�̗L����
    glBindVertexArray(vaoId);

    // �V�F�[�_�̗L����
    glUseProgram(programId);

    // Uniform�ϐ��̓]��
    GLuint uid;
    uid = glGetUniformLocation(programId, "u_mvpMat");
    glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(mvpMat));

    // �O�p�`�̕`��
    glDrawElements(GL_TRIANGLES, indexBufferSize, GL_UNSIGNED_INT, 0);

    // VAO�̖�����
    glBindVertexArray(0);

    // �V�F�[�_�̖�����
    glUseProgram(0);
}

void resizeGL(GLFWwindow *window, int width, int height) {
    WIN_WIDTH = width;
    WIN_HEIGHT = height;
    glfwSetWindowSize(window, WIN_WIDTH, WIN_HEIGHT);
}

// �A�j���[�V�����̂��߂̃A�b�v�f�[�g
void animate() {
    theta += 2.0f * PI / 360.0f;  // 10����1��]
}

int main(int argc, char **argv) {
    // OpenGL������������
    if (glfwInit() == GL_FALSE) {
        fprintf(stderr, "Initialization failed!\n");
        return 1;
    }

    // Window�̍쐬
    GLFWwindow *window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE,
                                          NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Window creation failed!");
        glfwTerminate();
        return 1;
    }

    // OpenGL�̕`��Ώۂ�Window��ǉ�
    glfwMakeContextCurrent(window);

    // GLEW������������ (glfwMakeContextCurrent�̌�łȂ��Ƃ����Ȃ�)
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed!\n");
        return 1;
    }

    // �E�B���h�E�̃��T�C�Y�������֐��̓o�^
    glfwSetWindowSizeCallback(window, resizeGL);

    // OpenGL��������
    initializeGL();

    // ���C�����[�v
    while (glfwWindowShouldClose(window) == GL_FALSE) {
        // �`��
        paintGL();

        // �A�j���[�V����
        animate();

        // �`��p�o�b�t�@�̐؂�ւ�
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
