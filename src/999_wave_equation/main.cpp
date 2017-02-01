#include <cstdio>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define GLFW_INCLUDE_GLU  // GLU���C�u�������g�p����̂ɕK�v
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "wave_equation.h"

static int WIN_WIDTH   = 500;                       // �E�B���h�E�̕�
static int WIN_HEIGHT  = 500;                       // �E�B���h�E�̍���
static const char *WIN_TITLE = "OpenGL Course";     // �E�B���h�E�̃^�C�g��

static const char *TEX_FILE = "hue.png";

static const char *VERT_SHADER_FILE = "glsl.vert";
static const char *FRAG_SHADER_FILE = "glsl.frag";

// VAO�֘A�̕ϐ�
GLuint vaoId;
GLuint vboId;
GLuint iboId;

// �V�F�[�_���Q�Ƃ���ԍ�
GLuint vertShaderId;
GLuint fragShaderId;
GLuint programId;

// �e�N�X�`��
GLuint textureId;

// �g���������̌v�Z�Ɏg���p�����[�^
WaveEquation waveEqn;
static const int xCells = 100;
static const int yCells = 100;
static const double speed = 0.5;
static const double dx = 0.05;
static const double dt = 0.05;

// ���_�̃f�[�^
std::vector<glm::vec3> positions;

// OpenGL�̏������֐�
void initializeGL() {
    // �w�i�F�̐ݒ� (��)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // �[�x�e�X�g�̗L����
    glEnable(GL_DEPTH_TEST);

    // �g���������V�~�����[�V�����̏�����
    waveEqn.setParams(xCells, yCells, speed, dx, dt);

    // ���_�f�[�^�̏�����
    for (int y = 0; y < yCells; y++) {
        for (int x = 0; x < xCells; x++) {
            double vx = (x - xCells / 2) * dx;
            double vy = (y - yCells / 2) * dx;
            positions.push_back(glm::vec3(vx, vy, 0.0));

            waveEqn.set(x, y, 2.0 * exp(-5.0 * (vx * vx + vy * vy)));
        }
    }

    waveEqn.start();

    // VAO�̗p��
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    glGenBuffers(1, &vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * positions.size(),
                 &positions[0], GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

    std::vector<unsigned int> indices;
    for (int y = 0; y < yCells - 1; y++) {
        for (int x = 0; x < xCells - 1; x++) {
            const int i0 = y * xCells + x;
            const int i1 = y * xCells + (x + 1);
            const int i2 = (y + 1) * xCells + x;
            const int i3 = (y + 1) * xCells + (x + 1);

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i0);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }

    glGenBuffers(1, &iboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(unsigned int) * indices.size(),
                 &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    // �e�N�X�`���̗p��
    int texWidth, texHeight, channels;
    unsigned char *bytes = stbi_load(TEX_FILE, &texWidth, &texHeight, &channels, STBI_rgb_alpha);
    if (!bytes) {
        fprintf(stderr, "Failed to load image file: %s\n", TEX_FILE);
        exit(1);
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_1D, textureId);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, texWidth,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_1D, 0);

    stbi_image_free(bytes);

    // �V�F�[�_�̗p��
    vertShaderId = glCreateShader(GL_VERTEX_SHADER);
    fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // �t�@�C���̓ǂݍ���
    std::ifstream vertShaderFile(VERT_SHADER_FILE, std::ios::in);
    if (!vertShaderFile.is_open()) {
        fprintf(stderr, "Failed to load vertex shader: %s\n", VERT_SHADER_FILE);
        exit(1);
    }

    std::string line;

    std::string vertShaderBuffer;
    while (!vertShaderFile.eof()) {
        std::getline(vertShaderFile, line);
        vertShaderBuffer += line + "\r\n";
    }
    const char *vertShaderCode = vertShaderBuffer.c_str();
    
    std::ifstream fragShaderFile(FRAG_SHADER_FILE, std::ios::in);
    if (!fragShaderFile.is_open()) {
        fprintf(stderr, "Failed to load fragment shader: %s\n", FRAG_SHADER_FILE);
        exit(1);
    }

    std::string fragShaderBuffer;
    while (!fragShaderFile.eof()) {
        std::getline(fragShaderFile, line);
        fragShaderBuffer += line + "\n";
    }
    const char *fragShaderCode = fragShaderBuffer.c_str();

    // �V�F�[�_�̃R���p�C��
    GLint compileStatus;
    glShaderSource(vertShaderId, 1, &vertShaderCode, NULL);
    glCompileShader(vertShaderId);
    glGetShaderiv(vertShaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        fprintf(stderr, "Failed to compile vertex shader!\n");
        
        GLint logLength;
        glGetShaderiv(vertShaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            char *errmsg = new char[logLength + 1];
            glGetShaderInfoLog(vertShaderId, logLength, &length, errmsg);

            std::cerr << errmsg << std::endl;
            fprintf(stderr, "%s", vertShaderCode);
        }
    }

    glShaderSource(fragShaderId, 1, &fragShaderCode, NULL);
    glCompileShader(fragShaderId);
    glGetShaderiv(fragShaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        fprintf(stderr, "Failed to compile fragment shader!\n");

        GLint logLength;
        glGetShaderiv(fragShaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            char *errmsg = new char[logLength + 1];
            glGetShaderInfoLog(fragShaderId, logLength, &length, errmsg);

            std::cerr << errmsg << std::endl;
            fprintf(stderr, "%s", vertShaderCode);
        }
    }

    // �V�F�[�_�v���O�����̗p��
    programId = glCreateProgram();
    glAttachShader(programId, vertShaderId);
    glAttachShader(programId, fragShaderId);

    GLint linkState;
    glLinkProgram(programId);
    glGetProgramiv(programId, GL_LINK_STATUS, &linkState);
    if (linkState == GL_FALSE) {
        fprintf(stderr, "Failed to compile shader!\n");

        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            char *errmsg = new char[logLength];
            glGetProgramInfoLog(programId, logLength, &length, errmsg);

            std::cerr << errmsg << std::endl;
            delete[] errmsg;
        }

        exit(1);
    }
}

// OpenGL�̕`��֐�
void paintGL() {
    // �w�i�F�Ɛ[�x�l�̃N���A
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    // ���W�̕ϊ�
    glm::mat4 projMat = glm::perspective(45.0f,
        (float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 1000.0f);

    glm::mat4 lookAt = glm::lookAt(glm::vec3(5.0f, 5.0f, 3.0f),   // ���_�̈ʒu
                                   glm::vec3(0.0f, 0.0f, 0.0f),   // ���Ă����
                                   glm::vec3(0.0f, 0.0f, 1.0f));  // ���E�̏����

    glm::mat4 mvpMat = projMat * lookAt;

    // VAO�̗L����
    glBindVertexArray(vaoId);

    // �V�F�[�_�̗L����
    glUseProgram(programId);

    // �e�N�X�`���̗L����
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, textureId);

    // Uniform�ϐ��̓]��
    GLuint mvpMatLocId = glGetUniformLocation(programId, "u_mvpMat");
    glUniformMatrix4fv(mvpMatLocId, 1, GL_FALSE, glm::value_ptr(mvpMat));

    GLuint texLocId = glGetUniformLocation(programId, "u_texture");
    glUniform1i(texLocId, 0);

    // �O�p�`�̕`��
    glDrawElements(GL_TRIANGLES, 3 * (yCells - 1) * (xCells - 1) * 2, GL_UNSIGNED_INT, 0);

    // VAO�̖�����
    glBindVertexArray(0);

    // �V�F�[�_�̖�����
    glUseProgram(0);

    // �e�N�X�`���̖�����
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, 0);
}

void resizeGL(GLFWwindow *window, int width, int height) {
    WIN_WIDTH = width;
    WIN_HEIGHT = height;
    glfwSetWindowSize(window, WIN_WIDTH, WIN_HEIGHT);
}

// �A�j���[�V�����̂��߂̃A�b�v�f�[�g
void update() {
    // �g���f�[�^�̍X�V
    waveEqn.step();

    for (int y = 0; y < yCells; y++) {
        for (int x = 0; x < xCells; x++) {
            positions[y * xCells + x].z = waveEqn.get(x, y);
        }
    }

    glBindVertexArray(vaoId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * positions.size(), &positions[0]);
    glBindVertexArray(0);
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
        update();

        // �`��p�o�b�t�@�̐؂�ւ�
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
