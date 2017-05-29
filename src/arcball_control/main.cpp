#include <cmath>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>

#define GLFW_INCLUDE_GLU
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// �f�B���N�g���̐ݒ�t�@�C��
#include "common.h"

static int WIN_WIDTH   = 500;                       // �E�B���h�E�̕�
static int WIN_HEIGHT  = 500;                       // �E�B���h�E�̍���
static const char *WIN_TITLE = "OpenGL Course";     // �E�B���h�E�̃^�C�g��

static const double PI = 4.0 * std::atan(1.0);

// �V�F�[�_�t�@�C��
static std::string VERT_SHADER_FILE = std::string(SHADER_DIRECTORY) + "render.vert";
static std::string FRAG_SHADER_FILE = std::string(SHADER_DIRECTORY) + "render.frag";

// ���_�I�u�W�F�N�g
struct Vertex {
    Vertex(const glm::vec3 &position_, const glm::vec3 &color_)
        : position(position_)
        , color(color_) {
    }

    glm::vec3 position;
    glm::vec3 color;
};

static const glm::vec3 positions[8] = {
    glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3( 1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f,  1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f,  1.0f),
    glm::vec3( 1.0f,  1.0f, -1.0f),
    glm::vec3(-1.0f,  1.0f,  1.0f),
    glm::vec3( 1.0f, -1.0f,  1.0f),
    glm::vec3( 1.0f,  1.0f,  1.0f)
};

static const glm::vec3 colors[6] = {
    glm::vec3(1.0f, 0.0f, 0.0f),  // ��
    glm::vec3(0.0f, 1.0f, 0.0f),  // ��
    glm::vec3(0.0f, 0.0f, 1.0f),  // ��
    glm::vec3(1.0f, 1.0f, 0.0f),  // �C�G���[
    glm::vec3(0.0f, 1.0f, 1.0f),  // �V�A��
    glm::vec3(1.0f, 0.0f, 1.0f),  // �}�[���^
};

static const unsigned int faces[12][3] = {
    { 1, 6, 7 }, { 1, 7, 4 },
    { 2, 5, 7 }, { 2, 7, 4 },
    { 3, 5, 7 }, { 3, 7, 6 },
    { 0, 1, 4 }, { 0, 4, 2 },
    { 0, 1, 6 }, { 0, 6, 3 },
    { 0, 2, 5 }, { 0, 5, 3 }
};

// �o�b�t�@���Q�Ƃ���ԍ�
GLuint vaoId;
GLuint vertexBufferId;
GLuint indexBufferId;

// �V�F�[�_���Q�Ƃ���ԍ�
GLuint programId;

// �����̂̉�]�p�x
static float theta = 0.0f;

// Arcball�R���g���[���̂��߂̕ϐ�
bool isDragging = false;

enum ArcballMode {
    ARCBALL_MODE_NONE = 0x00,
    ARCBALL_MODE_TRANSLATE = 0x01,
    ARCBALL_MODE_ROTATE = 0x02,
    ARCBALL_MODE_SCALE = 0x04
};

int arcballMode = ARCBALL_MODE_NONE;
glm::mat4 modelMat;
glm::mat4 viewMat;
glm::mat4 projMat;
double scroll = 0.0;
glm::ivec2 oldPos;
glm::ivec2 newPos;
glm::vec3 translate(0.0f, 0.0f, 0.0f);
glm::mat4 lookMat;
glm::mat4 rotMat;


// VAO�̏�����
void initVAO() {
    // Vertex�z��̍쐬
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    int idx = 0;
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 3; j++) {
            Vertex v(positions[faces[i * 2 + 0][j]], colors[i]);
            vertices.push_back(v);
            indices.push_back(idx++);
        }

        for (int j = 0; j < 3; j++) {
            Vertex v(positions[faces[i * 2 + 1][j]], colors[i]);
            vertices.push_back(v);
            indices.push_back(idx++);
        }
    }

    // VAO�̍쐬
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    // ���_�o�b�t�@�̍쐬
    glGenBuffers(1, &vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // ���_�o�b�t�@�̗L����
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

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
        fprintf(stderr, "Failed to load a shader: %s\n", VERT_SHADER_FILE.c_str());
        exit(1);
    }

    // �t�@�C�������ׂēǂ�ŕϐ��Ɋi�[ (����)
    reader.seekg(0, std::ios::end);             // �t�@�C���ǂݎ��ʒu���I�[�Ɉړ� 
    codeSize = reader.tellg();                  // ���݂̉ӏ�(=�I�[)�̈ʒu���t�@�C���T�C�Y
    code.resize(codeSize);                      // �R�[�h���i�[����ϐ��̑傫����ݒ�
    reader.seekg(0);                            // �t�@�C���̓ǂݎ��ʒu��擪�Ɉړ�
    reader.read(&code[0], codeSize);            // �擪����t�@�C���T�C�Y����ǂ�ŃR�[�h�̕ϐ��Ɋi�[

    // �t�@�C�������
    reader.close();

    // �R�[�h�̃R���p�C��
    const char *codeChars = code.c_str();
    glShaderSource(shaderId, 1, &codeChars, NULL);
    glCompileShader(shaderId);

    // �R���p�C���̐��ۂ𔻒肷��
    GLint compileStatus;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        // �R���p�C�������s������G���[���b�Z�[�W�ƃ\�[�X�R�[�h��\�����ďI��
        fprintf(stderr, "Failed to compile a shader!\n");

        // �G���[���b�Z�[�W�̒������擾����
        GLint logLength;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            // �G���[���b�Z�[�W���擾����
            GLsizei length;
            std::string errMsg;
            errMsg.resize(logLength);
            glGetShaderInfoLog(shaderId, logLength, &length, &errMsg[0]);

            // �G���[���b�Z�[�W�ƃ\�[�X�R�[�h�̏o��
            fprintf(stderr, "[ ERROR ] %s\n", errMsg.c_str());
            fprintf(stderr, "%s\n", code.c_str());
        }
        exit(1);
    }

    return shaderId;
}

GLuint buildShaderProgram(const std::string &vShaderFile, const std::string &fShaderFile) {
    // �V�F�[�_�̍쐬
    GLuint vertShaderId = compileShader(vShaderFile, GL_VERTEX_SHADER);
    GLuint fragShaderId = compileShader(fShaderFile, GL_FRAGMENT_SHADER);
    
    // �V�F�[�_�v���O�����̃����N
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertShaderId);
    glAttachShader(programId, fragShaderId);
    glLinkProgram(programId);
    
    // �����N�̐��ۂ𔻒肷��
    GLint linkState;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkState);
    if (linkState == GL_FALSE) {
        // �����N�Ɏ��s������G���[���b�Z�[�W��\�����ďI��
        fprintf(stderr, "Failed to link shaders!\n");

        // �G���[���b�Z�[�W�̒������擾����
        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            // �G���[���b�Z�[�W���擾����
            GLsizei length;
            std::string errMsg;
            errMsg.resize(logLength);
            glGetProgramInfoLog(programId, logLength, &length, &errMsg[0]);

            // �G���[���b�Z�[�W���o�͂���
            fprintf(stderr, "[ ERROR ] %s\n", errMsg.c_str());
        }
        exit(1);
    }
    
    // �V�F�[�_�𖳌����������ID��Ԃ�
    glUseProgram(0);
    return programId;
}

// �V�F�[�_�̏�����
void initShaders() {
    programId = buildShaderProgram(VERT_SHADER_FILE, FRAG_SHADER_FILE);
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

    // �J�����̏�����
    projMat = glm::perspective(45.0f,
                               (float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 1000.0f);

    lookMat = glm::lookAt(glm::vec3(3.0f, 4.0f, 5.0f),   // ���_�̈ʒu
                          glm::vec3(0.0f, 0.0f, 0.0f),   // ���Ă����
                          glm::vec3(0.0f, 1.0f, 0.0f));  // ���E�̏����
    viewMat = lookMat;
}

// OpenGL�̕`��֐�
void paintGL() {
    // �w�i�F�̕`��
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // �V�F�[�_�̗L����
    glUseProgram(programId);
        
    // VAO�̗L����
    glBindVertexArray(vaoId);

    // 1�ڂ̗����̂�`��
    glm::mat4 mvpMat = projMat * viewMat * modelMat;

    // Uniform�ϐ��̓]��
    GLuint uid;
    uid = glGetUniformLocation(programId, "u_mvpMat");
    glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(mvpMat));
        
    glDrawElements(GL_TRIANGLES, 48, GL_UNSIGNED_INT, 0);

    // VAO�̖�����
    glBindVertexArray(0);

    // �V�F�[�_�̖�����
    glUseProgram(0);
}

void resizeGL(GLFWwindow *window, int width, int height) {
    // ���[�U�Ǘ��̃E�B���h�E�T�C�Y��ύX
    WIN_WIDTH = width;
    WIN_HEIGHT = height;
    
    // GLFW�Ǘ��̃E�B���h�E�T�C�Y��ύX
    glfwSetWindowSize(window, WIN_WIDTH, WIN_HEIGHT);
    
    // ���ۂ̃E�B���h�E�T�C�Y (�s�N�Z����) ���擾
    int renderBufferWidth, renderBufferHeight;
    glfwGetFramebufferSize(window, &renderBufferWidth, &renderBufferHeight);
    
    // �r���[�|�[�g�ϊ��̍X�V
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);

    // �ϊ��s��̏�����
    projMat = glm::perspective(45.0f,
                               (float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 1000.0f);
}

void mouseEvent(GLFWwindow *window, int button, int action, int mods) {
    // �N���b�N�����{�^���ŏ�����؂�ւ���
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        arcballMode = ARCBALL_MODE_ROTATE;
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        arcballMode = ARCBALL_MODE_SCALE;    
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        arcballMode = ARCBALL_MODE_TRANSLATE;
    }
     
    // �N���b�N���ꂽ�ʒu���擾
    double px, py;
    glfwGetCursorPos(window, &px, &py);

    if (action == GLFW_PRESS) {
        if (!isDragging) {
            isDragging = true;
            oldPos = glm::ivec2(px, py);
            newPos = glm::ivec2(px, py);
        }
    } else {        
        isDragging = false;
        oldPos = glm::ivec2(0, 0);
        newPos = glm::ivec2(0, 0);
        arcballMode = ARCBALL_MODE_NONE;
    }
}

glm::vec3 getVector(double x, double y) {
    glm::vec3 pt( 2.0 * x / WIN_WIDTH  - 1.0,
                 -2.0 * y / WIN_HEIGHT + 1.0, 0.0);

    const double xySquared = pt.x * pt.x + pt.y * pt.y;
    if (xySquared <= 1.0) {
        pt.z = std::sqrt(1.0 - xySquared);
    } else {
        pt = glm::normalize(pt);
    }

    return pt;
}

void updateRotate() {
    static const double Pi = 4.0 * std::atan(1.0);

    const glm::vec3 u = getVector(newPos.x, newPos.y);
    const glm::vec3 v = getVector(oldPos.x, oldPos.y);

    const double angle = std::acos(std::min(1.0f, glm::dot(u, v)));
    
    const glm::vec3 rotAxis = glm::cross(v, u);
    const glm::mat4 camera2objMat = glm::inverse(rotMat);

    const glm::vec3 objSpaceRotAxis = glm::vec3(camera2objMat * glm::vec4(rotAxis, 1.0f));
    
    glm::mat4 temp;
    temp = glm::rotate(temp, (float)(4.0 * angle), objSpaceRotAxis);
    rotMat = rotMat * temp;
}

void updateTranslate() {
    const glm::vec4 u(1.0f, 0.0f, 0.0f, 0.0f);
    const glm::vec4 v(0.0f, 1.0f, 0.0f, 0.0f);
    const glm::mat4 camera2objMat = glm::inverse(lookMat);

    const glm::vec3 objspaceU = glm::normalize(glm::vec3(camera2objMat * u));
    const glm::vec3 objspaceV = glm::normalize(glm::vec3(camera2objMat * v));

    const double dx = 10.0 * (newPos.x - oldPos.x) / WIN_WIDTH;
    const double dy = 10.0 * (newPos.y - oldPos.y) / WIN_HEIGHT;

    translate += (objspaceU * (float)dx - objspaceV * (float)dy);
}

void updateMouse() {
    switch (arcballMode) {
    case ARCBALL_MODE_ROTATE:
        updateRotate();
        break;

    case ARCBALL_MODE_TRANSLATE:
        updateTranslate();
        break;

    case ARCBALL_MODE_SCALE:
        break;
    }

    const float scaleVal = 1.0 - scroll * 0.1;
    modelMat = rotMat;
    modelMat = glm::scale(modelMat, glm::vec3(scaleVal, scaleVal, scaleVal));
    viewMat = lookMat;
    viewMat = glm::translate(viewMat, translate);
}

void mouseMoveEvent(GLFWwindow *window, double xpos, double ypos) {
    if (isDragging) {
        newPos = glm::ivec2(xpos, ypos);
        updateMouse();
        oldPos = glm::ivec2(xpos, ypos);
    }
}

void wheelEvent(GLFWwindow *window, double xpos, double ypos) {
    scroll += ypos;
    updateMouse();
}

int main(int argc, char **argv) {
    // OpenGL������������
    if (glfwInit() == GL_FALSE) {
        fprintf(stderr, "Initialization failed!\n");
        return 1;
    }

    // OpenGL�̃o�[�W�����ݒ� (Mac�̏ꍇ�ɂ͕K���K�v)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    // �}�E�X�̃C�x���g����������֐���o�^
    glfwSetMouseButtonCallback(window, mouseEvent);
    glfwSetCursorPosCallback(window, mouseMoveEvent);
    glfwSetScrollCallback(window, wheelEvent);

    // GLEW������������ (glfwMakeContextCurrent�̌�łȂ��Ƃ����Ȃ�)
    glewExperimental = true;
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

        // �`��p�o�b�t�@�̐؂�ւ�
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
