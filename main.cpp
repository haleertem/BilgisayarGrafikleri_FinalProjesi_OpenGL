#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <GL/glut.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

unsigned int catTexture;
unsigned int pyramidTexture;

float cameraDistance = 5.0f; // Kamera ile hedef arasýndaki mesafe

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool isFalling[4] = { false, false, false, false };
float fallAngles[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
float angle = 0.0f;

enum CameraMode { FIRST_PERSON, FREE_MODE, FIXED_STORY };
CameraMode currentCamera = static_cast<CameraMode>(-1);

float yaw = -90.0f, pitch = 0.0f;
float lastX = SCR_WIDTH / 2, lastY = SCR_HEIGHT / 2;
bool firstMouse = true;
glm::vec3 cameraPos, cameraFront, cameraUp;

void renderBitmapString(float x, float y, void* font, const char* string)
{
    glRasterPos2f(x, y);
    for (const char* c = string; *c != '\0'; c++)
    {
        glutBitmapCharacter(font, *c);
    }
}

void switchCamera(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS )
    {
        if (key == GLFW_KEY_1)
            currentCamera = FIRST_PERSON;
        else if (key == GLFW_KEY_2)
            currentCamera = FREE_MODE;
        else if (key == GLFW_KEY_3)
            currentCamera = FIXED_STORY;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (currentCamera != FREE_MODE) return;

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

    yaw += xoffset;
    pitch += yoffset;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void renderText(float x, float y, std::string text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600); // Ekran çözünürlüðüne göre ayarla

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST); // 3D derinlik testini geçici kapat
    glColor3f(1.0f, 1.0f, 1.0f); // Beyaz renk

    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    glEnable(GL_DEPTH_TEST); // Geri aç
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


bool checkCollision(glm::vec3 aPos, glm::vec3 aSize, glm::vec3 bPos, glm::vec3 bSize) {
    return (abs(aPos.x - bPos.x) * 2 < (aSize.x + bSize.x)) &&
        (24 < abs(aPos.y - bPos.y)  < (aSize.y + bSize.y)*25) &&     
        (abs(aPos.z - bPos.z) * 2 < (aSize.z + bSize.z));
}


const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform int useTexture;
uniform vec3 objectColor;

void main()
{
    if (useTexture == 1)
        FragColor = texture(texture1, TexCoord);
    else
        FragColor = vec4(objectColor, 1.0);
}
)";

// Klavye kontrol
bool keys[1024];
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
    if (action == GLFW_PRESS && (currentCamera == FREE_MODE || currentCamera == FIXED_STORY))
    {
        if (key == GLFW_KEY_Z)
            cameraDistance = std::max(1.0f, cameraDistance - 0.5f); // minimum 1.0f mesafe
        if (key == GLFW_KEY_X)
            cameraDistance = std::min(100.0f, cameraDistance + 0.5f); // maksimum 100.0f mesafe
    }
}

// Hareket
void do_movement()
{
    float speed = 6.0f * deltaTime;
    if (keys[GLFW_KEY_A])
        position.z -= speed;
    if (keys[GLFW_KEY_D])
        position.z += speed;
    if (keys[GLFW_KEY_S])
        position.x -= speed;
    if (keys[GLFW_KEY_W])
        position.x += speed;

    if (currentCamera == FREE_MODE || currentCamera == FIXED_STORY)
    {
        if (keys[GLFW_KEY_Z])
            cameraDistance = std::max(1.0f, cameraDistance - speed * 5);  // daha yumuþak zoom
        if (keys[GLFW_KEY_X])
            cameraDistance = std::min(100.0f, cameraDistance + speed * 5);
    }
  
}

unsigned int createShaderProgram()
{
    int success;
    char infoLog[512];

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Error:\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Error:\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader Link Error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Dikdörtgen prizma (gövde ve bacaklar) için küp verisi
float cubeVertices[] = {
    // positions          // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};


unsigned int cubeVAO, cubeVBO;

void setupCube()
{
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture koordinatlarý
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

}

void drawCube(glm::mat4 model, unsigned int shaderProgram, int useTexture, unsigned int texture = 0, glm::vec3 color = glm::vec3(1.0f))
{
    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), useTexture);

    if (useTexture == 1 && texture != 0)
        glBindTexture(GL_TEXTURE_2D, texture);

    if (useTexture == 0)
        glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void setupCatTexture()
{
    glGenTextures(1, &catTexture);
    glBindTexture(GL_TEXTURE_2D, catTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("cat_texture.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Kedi dokusu yuklenemedi!" << std::endl;
    }
    stbi_image_free(data);
}

void setupPyramidTexture()
{
    glGenTextures(1, &pyramidTexture);
    glBindTexture(GL_TEXTURE_2D, pyramidTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("pyramidTexture.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Piramit dokusu yuklenemedi!" << std::endl;
    }
    stbi_image_free(data);
}


float groundVertices[] = {
    // positions        // texture coords
    -30.0f, -0.75f,  30.0f,  0.0f, 0.0f,
     30.0f, -0.75f,  30.0f,  1.0f, 0.0f,
     30.0f, -0.75f, -30.0f,  1.0f, 1.0f,

     30.0f, -0.75f, -30.0f,  1.0f, 1.0f,
    -30.0f, -0.75f, -30.0f,  0.0f, 1.0f,
    -30.0f, -0.75f,  30.0f,  0.0f, 0.0f
};

unsigned int groundVAO, groundVBO, groundTexture;

void setupGround()
{
    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);
    glBindVertexArray(groundVAO);

    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture yükle
    glGenTextures(1, &groundTexture);
    glBindTexture(GL_TEXTURE_2D, groundTexture);

    // sarmalama ve filtreleme
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("grass.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Zemin dokusu yüklenemedi!\n";
    }
    stbi_image_free(data);
}

float pyramidVertices[] = {
    // Taban (kare)
    -0.5f, 9.75f, -0.5f,   0.0f, 0.0f,
     0.5f, 9.75f, -0.5f,   1.0f, 0.0f,
     0.5f, 9.75f,  0.5f,   1.0f, 1.0f,
     0.5f, 9.75f,  0.5f,   1.0f, 1.0f,
    -0.5f, 9.75f,  0.5f,   0.0f, 1.0f,
    -0.5f, 9.75f, -0.5f,   0.0f, 0.0f,

    // Ön yüz
    -0.5f, 9.75f, -0.5f,   0.0f, 0.0f,
     0.5f, 9.75f, -0.5f,   1.0f, 0.0f,
     0.0f, 10.75f,  0.0f,  0.5f, 1.0f,

     // Sað yüz
      0.5f, 9.75f, -0.5f,   0.0f, 0.0f,
      0.5f, 9.75f,  0.5f,   1.0f, 0.0f,
      0.0f, 10.75f,  0.0f,  0.5f, 1.0f,

      // Arka yüz
       0.5f, 9.75f,  0.5f,   1.0f, 0.0f,
      -0.5f, 9.75f,  0.5f,   0.0f, 0.0f,
       0.0f, 10.75f,  0.0f,  0.5f, 1.0f,

       // Sol yüz
       -0.5f, 9.75f,  0.5f,   1.0f, 0.0f,
       -0.5f, 9.75f, -0.5f,   0.0f, 0.0f,
        0.0f, 10.75f,  0.0f,  0.5f, 1.0f
};


unsigned int pyramidVAO, pyramidVBO;

void setupPyramid()
{
    glGenVertexArrays(1, &pyramidVAO);
    glGenBuffers(1, &pyramidVBO);

    glBindVertexArray(pyramidVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture koordinatlarý
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void drawPyramid(glm::mat4 model, GLuint shaderProgram,  int useTexture, unsigned int texture = 0, glm::vec3 color = glm::vec3(1.0f))
{
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), useTexture); //texture

    if (useTexture == 1 && texture != 0)
        glBindTexture(GL_TEXTURE_2D, texture);

    if (useTexture == 0)
        glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));

    glBindVertexArray(pyramidVAO);
    glDrawArrays(GL_TRIANGLES, 0, 18);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CatWalker", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Window creation failed!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // Sadece bu yeterli:
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback); // Fare hareketleri
    glfwSetKeyCallback(window, [](GLFWwindow* w, int k, int s, int a, int m) {
        key_callback(w, k, s, a, m);        // WASD
        switchCamera(w, k, s, a, m);        // 1,2,3 ile kamera geçiþi
        });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "GLAD init failed!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    unsigned int shaderProgram = createShaderProgram();
    setupCube();
    setupGround();
    setupCatTexture();
    setupPyramidTexture();
    setupPyramid();


    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        do_movement();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (currentCamera == -1) // henüz seçim yapýlmadý
        {

            //glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            gluOrtho2D(0, SCR_WIDTH, 0, SCR_HEIGHT);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            glColor3f(1.0f, 1.0f, 0.0f); // Sarý renk
            renderBitmapString(SCR_WIDTH / 2 - 250, SCR_HEIGHT - 40, GLUT_BITMAP_HELVETICA_18,
               "Kamera modu secin: [1] Karakter Gozu  [2] Serbest  [3] Sabit");


            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }


        glUseProgram(shaderProgram);

        glm::vec3 lightPos = glm::vec3(0.0f, 10.0f, 0.0f); // Tepeden gelen ýþýk
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
        glm::mat4 view;

        if (currentCamera == FIRST_PERSON)
        {
            view = glm::lookAt(glm::vec3(position.x, 0.5f, position.z),
                glm::vec3(position.x + 1.0f, 0.5f, position.z),
                glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (currentCamera == FREE_MODE)
        {
            cameraPos = position - cameraFront * cameraDistance + glm::vec3(0.0f, 2.0f, 0.0f);
            cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
            view = glm::lookAt(cameraPos, position, cameraUp);
        }
        else if (currentCamera == FIXED_STORY)
        {
            glm::vec3 direction = glm::normalize(position - glm::vec3(5.0f, 0.0f, 5.0f));
            glm::vec3 fixedPos = position + (-direction * cameraDistance) + glm::vec3(0.0f, 3.0f, 0.0f);
            view = glm::lookAt(fixedPos, position, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        renderText(340, 570, "Piramite kediyle carp, ALTINI KAP !!!");

        // Zemin
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
        glBindTexture(GL_TEXTURE_2D, groundTexture);
        glBindVertexArray(groundVAO);
        glm::mat4 groundModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(groundModel));
        glDrawArrays(GL_TRIANGLES, 0, 6);


        // Gövde (uzun dikdörtgen)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(2.0f, 0.5f, 1.0f));
        drawCube(model, shaderProgram,1,catTexture);

        // Kafa (öne bir küp)
        model = glm::mat4(1.0f);
        model = glm::translate(model, position + glm::vec3(1.3f, 0.25f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        drawCube(model, shaderProgram,1,catTexture);

        // Bacaklar
        for (int i = 0; i < 4; ++i)
        {
            float x = (i < 2) ? -0.8f : 0.8f;
            float z = (i % 2 == 0) ? -0.4f : 0.4f;
            float legOffset = sin(glfwGetTime() * 5.0f) * 0.1f; //yeni
            model = glm::mat4(1.0f);
            model = glm::translate(model, position + glm::vec3(x, -0.4f + ((i % 2 == 0) ? legOffset : -legOffset), z));
            model = glm::scale(model, glm::vec3(0.2f, 0.4f, 0.2f));
            drawCube(model, shaderProgram, 1, catTexture);
        }

        // Piramitler (köþelere yakýn 4 noktaya)
            glm::vec3 corners[4] = {
            glm::vec3(-25.0f, -49.5f, -25.0f),
            glm::vec3(25.0f, -49.5f, -25.0f),
            glm::vec3(25.0f, -49.5f,  25.0f),
            glm::vec3(-25.0f, -49.5f,  25.0f),
        };

        for (int i = 0; i < 4; ++i)
        {
            glm::vec3 pyramidPos = corners[i];
            glm::vec3 pyramidSize = glm::vec3(5.0f); // ölçeklenmiþ hali
            glm::vec3 catSize = glm::vec3(2.0f, 0.5f, 1.0f); // gövdenin boyutu

            if (!isFalling[i] && checkCollision(position, catSize, pyramidPos, pyramidSize))
            {
                isFalling[i] = true;
            }

            // Devirmeyi uygula
            if (isFalling[i] && fallAngles[i] < 90.0f)
            {
                fallAngles[i] += deltaTime * 45.0f; // 45 derece/sn
            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pyramidPos);
            if (isFalling[i]) {
                model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0f)); // pivot'u yükselt
                model = glm::rotate(model, glm::radians(fallAngles[i]), glm::vec3(1.0f, 0.0f, 0.0f)); // x ekseninde
                model = glm::translate(model, glm::vec3(0.0f, -1.5f, 0.0f)); // geri indir
            }
            model = glm::scale(model, glm::vec3(5.0f));
            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.2f, 0.2f);
            drawPyramid(model, shaderProgram, 1, pyramidTexture);
        }

        // Küçük küpler
        for (int i = 0; i < 4; ++i)
        {
            glm::vec3 tip = corners[i] + glm::vec3(0.0f, 50.0f, 0.0f); // Piramidin tepe noktasý
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, tip);
            model = glm::scale(model, glm::vec3(0.5f)); // Küçük küp
            drawCube(model, shaderProgram,0,0, glm::vec3(1.0f, 1.0f, 0.0f));
        }
       
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
