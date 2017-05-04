#include <cstdio>
#include <cmath>

#define GLFW_INCLUDE_GLU  // GLU���C�u�������g�p����̂ɕK�v
#include <GLFW/glfw3.h>

static const int WIN_WIDTH   = 500;                 // �E�B���h�E�̕�
static const int WIN_HEIGHT  = 500;                 // �E�B���h�E�̍���
static const char *WIN_TITLE = "OpenGL Course";     // �E�B���h�E�̃^�C�g��

static const double PI = 4.0 * atan(1.0);           // �~�����̒�`

static float theta = 0.0f;

static const float positions[8][3] = {
    { -1.0f, -1.0f, -1.0f },
    {  1.0f, -1.0f, -1.0f },
    { -1.0f,  1.0f, -1.0f },
    { -1.0f, -1.0f,  1.0f },
    {  1.0f,  1.0f, -1.0f },
    { -1.0f,  1.0f,  1.0f },
    {  1.0f, -1.0f,  1.0f },
    {  1.0f,  1.0f,  1.0f }
};

static const float colors[6][4] = {
    { 1.0f, 0.0f, 0.0f, 0.9f },  // ��
    { 0.0f, 1.0f, 0.0f, 0.9f },  // ��
    { 0.0f, 0.0f, 1.0f, 0.9f },  // ��
    { 1.0f, 1.0f, 0.0f, 0.9f },  // �C�G���[
    { 0.0f, 1.0f, 1.0f, 0.9f },  // �V�A��
    { 1.0f, 0.0f, 1.0f, 0.9f },  // �}�[���^
};

static const unsigned int indices[12][3] = {
    { 1, 6, 7 }, { 1, 7, 4 },
    { 2, 5, 7 }, { 2, 7, 4 },
    { 3, 5, 7 }, { 3, 7, 6 },
    { 0, 1, 4 }, { 0, 4, 2 },
    { 0, 1, 6 }, { 0, 6, 3 },
    { 0, 2, 5 }, { 0, 5, 3 }
};

// OpenGL�̏������֐�
void initializeGL() {
    // �w�i�F�̐ݒ� (��)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // �[�x�e�X�g�̗L����
    glEnable(GL_DEPTH_TEST);
}

// OpenGL�̕`��֐�
void paintGL() {
    // �w�i�F�Ɛ[�x�l�̃N���A
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // �r���[�|�[�g�ϊ��̎w��
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    // ���W�̕ϊ�
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 1000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(3.0f, 4.0f, 5.0f,     // ���_�̈ʒu
              0.0f, 0.0f, 0.0f,     // ���Ă����
              0.0f, 1.0f, 0.0f);    // ���E�̏����

    glRotatef(theta, 0.0f, 1.0f, 0.0f);  // y�����S��theta������]

    // �A���t�@�u�����h�̗L����
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    //glDisable(GL_DEPTH_TEST);

    // �����̂̕`��
    glBegin(GL_TRIANGLES);
    for (int face = 0; face < 6; face++) {
        glColor4fv(colors[face]);
        for (int i = 0; i < 3; i++) {
            glVertex3fv(positions[indices[face * 2 + 0][i]]);
        }

        for (int i = 0; i < 3; i++) {
            glVertex3fv(positions[indices[face * 2 + 1][i]]);
        }
    }
    glEnd();

    // �A���t�@�u�����h�̖�����
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    //glEnable(GL_DEPTH_TEST);
}

// �A�j���[�V�����̂��߂̃A�b�v�f�[�g
void animate() {
    theta += 2.0f * PI / 10.0f;  // 10����1��]
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
