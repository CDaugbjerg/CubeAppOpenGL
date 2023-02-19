#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <algorithm>
#include <cstdint>
#include <iomanip>

#include <iostream>

#include "ShaderUtility.h";

using namespace std;

void InitializeGLFW();
GLFWwindow* SetupWindow();
void LoadOpenGL();

void CreateCubeVertexBuffer(GLuint bufferObject);
void CreatePlaneVertexBuffer(GLuint bufferObject);
void CreatePlaneIndexBuffer(GLuint bufferObject);
void SetupCubeVertexArray();

void SetupTexture(GLuint texture, const char* fileName);

void RenderScene(const GLuint& shader);

void ApplyCubeTransformation(GLuint shader, glm::mat4 cameraView);

void DrawStaticObject(GLuint vertexArrayObject, GLsizei vertexCount, GLuint shader, glm::vec3 position, glm::vec3 orientation, glm::vec3 scale, glm::mat4 cameraView);

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseCallback(GLFWwindow* window, double xpos, double ypos);
void UpdateKeybaordInput(GLFWwindow* window);

//Screen
const unsigned int ScreenWidth = 1280;
const unsigned int ScreenHeight = 720;

const unsigned int ShadowMapWidth = 2048;
const unsigned int ShadowMapHeight = 2048;

//Camera
glm::vec3 _cameraPosition = glm::vec3(2.0f, 0.0f, 3.0f);
glm::vec3 _cameraForward = glm::vec3(-0.7f, 0.0f, -0.7f);
glm::vec3 _cameraForwardStart = glm::vec3(-0.7f, 0.0f, -0.7f);

glm::vec3 _playerForward = glm::vec3(-0.7f, 0.0f, -0.7f);

glm::vec3 _worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

float _yaw = -90.0f;
float _pitch = 0.0f;
float _lastX = (float)ScreenWidth / 2.0;
float _lastY = (float)ScreenHeight / 2.0;

//Timeing
float _deltaTime = 0.0f;
float _lastFrame = 0.0f;

//Buffers
GLuint _vertexBufferObjectCube;
GLuint _vertextArrayObjectCube;

GLuint _vertexBufferObjectPlane;
GLuint _indexBufferObjectPlane;
GLuint _vertexArrayObjectFloorPlane;

GLuint _depthMapFrameBufferObject;

//Shaders
GLuint _shaderProgram;
GLuint _depthShaderProgram;

//Render Textures
GLuint _depthMap;

//Textures
GLuint _textureCube;

//Lighting
glm::vec3 _lightPos(1.2f, 1.0f, 1.0f);
glm::vec3 _lightColor(1.0f, 0.95f, 0.85f);

//Cube Settings
glm::vec3 _cubeColor(1.0f, 0.95f, 0.85f);

//Files
const char* VertexShaderFileName = "shaderPhong.vs";
const char* FragmentShaderFileName = "shaderPhong.fs";

const char* DepthVertexShaderFileName = "shaderDepth.vs";
const char* DepthFragmentShaderFileName = "shaderDepth.fs";

const char* CubeTextureFileName = "Pilotage-Stretcher-Architextures.jpg";

int main() 
{
    InitializeGLFW();
    GLFWwindow* window = SetupWindow();
    LoadOpenGL();

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //Depth Test
    glEnable(GL_DEPTH_TEST);

    //Generate Buffers
    glGenBuffers(1, &_vertexBufferObjectCube);
    glGenBuffers(1, &_vertexBufferObjectPlane);
    glGenBuffers(1, &_indexBufferObjectPlane);

    //Generate Arrays
    glGenVertexArrays(1, &_vertextArrayObjectCube);
    glGenVertexArrays(1, &_vertexArrayObjectFloorPlane);

    //Generate Textures
    glGenTextures(1, &_textureCube);

    CreateCubeVertexBuffer(_vertexBufferObjectCube);
    glBindVertexArray(_vertextArrayObjectCube);

    SetupCubeVertexArray();

    CreatePlaneVertexBuffer(_vertexBufferObjectPlane);    
    glBindVertexArray(_vertexArrayObjectFloorPlane);
    CreatePlaneIndexBuffer(_indexBufferObjectPlane);
    SetupCubeVertexArray();

    SetupTexture(_textureCube, CubeTextureFileName);

    //Configure Depth Map
    glGenFramebuffers(1, &_depthMapFrameBufferObject);

    glGenTextures(1, &_depthMap);
    glBindTexture(GL_TEXTURE_2D, _depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, ShadowMapWidth, ShadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFrameBufferObject);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //Cube Shader
    _shaderProgram = CompileShaders(VertexShaderFileName, FragmentShaderFileName);
    glUseProgram(_shaderProgram);
    glUniform1i(glGetUniformLocation(_shaderProgram, "texture1"), 0);
    glUniform1i(glGetUniformLocation(_shaderProgram, "shadowMap"), 1);


    //Depth Shader
    _depthShaderProgram = CompileShaders(DepthVertexShaderFileName, DepthFragmentShaderFileName);
    glUseProgram(_depthShaderProgram);
    glUniform1i(glGetUniformLocation(_shaderProgram, "depthMap"), 0);

    //Render Loop
    while (!glfwWindowShouldClose(window))
    {
        //Timing
        float currentFrame = static_cast<float>(glfwGetTime());
        _deltaTime = currentFrame - _lastFrame;
        _lastFrame = currentFrame;

        //Input
        UpdateKeybaordInput(window);

        //Clear
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Render Depth
        glm::mat4 lightProjection;
        glm::mat4 lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 7.5f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        lightView = glm::lookAt(_lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        glUseProgram(_depthShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(_depthShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        glViewport(0, 0, ShadowMapWidth, ShadowMapHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFrameBufferObject);
        glClear(GL_DEPTH_BUFFER_BIT);
        RenderScene(_depthShaderProgram);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //Reset and Clear
        glViewport(0, 0, ScreenWidth, ScreenHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Shader
        glUseProgram(_shaderProgram);

        //Lighting
        glUniform3fv(glGetUniformLocation(_shaderProgram, "lightColor"), 1, glm::value_ptr(_lightColor));
        glUniform3fv(glGetUniformLocation(_shaderProgram, "lightPos"), 1, glm::value_ptr(_lightPos));
        glUniform3fv(glGetUniformLocation(_shaderProgram, "viewPos"), 1, glm::value_ptr(_cameraPosition));
        glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        //Texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureCube);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _depthMap);

        
        //Render
        RenderScene(_shaderProgram);

        //Swap buffer
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //Terminate glfw
    glfwTerminate();

	return 0;
}

void InitializeGLFW()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

}

GLFWwindow* SetupWindow()
{
    GLFWwindow* window = glfwCreateWindow(ScreenWidth, ScreenHeight, "Hello Claus, Jan and World", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    return window;
}

void LoadOpenGL()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(1);
    }
}

void CreateCubeVertexBuffer(GLuint bufferObject)
{
    //copied from: https://learnopengl.com/code_viewer_gh.php?code=src/2.lighting/4.2.lighting_maps_specular_map/lighting_maps_specular.cpp
    float vertices[] = 
    {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void CreatePlaneVertexBuffer(GLuint bufferObject) 
{
    float vertices[] = 
    {
         0.5f,  0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void CreatePlaneIndexBuffer(GLuint bufferObject) 
{
    unsigned int indices[] = 
    {
        0, 1, 3, 
        1, 2, 3  
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void SetupCubeVertexArray() 
{
    //Position Attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //Color Attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    //Texture Attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void SetupTexture(GLuint texture, const char* fileName) 
{
    glBindTexture(GL_TEXTURE_2D, _textureCube);

    //Wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //Load
    int width;
    int height;
    int nrChannels;

    unsigned char* data = stbi_load(fileName, &width, &height, &nrChannels, 0);
    if (data)
    {
        //Create
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        //Mipmap
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    //Free image data
    stbi_image_free(data);
}

void RenderScene(const GLuint& shader)
{
    //Camera transformation
    glm::mat4 view = glm::lookAt(_cameraPosition, _cameraPosition + _cameraForward, _worldUp);

    //Draw Cube
    ApplyCubeTransformation(shader, view);
    glBindVertexArray(_vertextArrayObjectCube);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    //Draw Planes
    DrawStaticObject(_vertexArrayObjectFloorPlane, 6, shader, glm::vec3(0.0f, 1.5f, -4.0), glm::vec3(glm::pi<float>(), 0.0f, 0.0f), glm::vec3(5.0f, 5.0, 1.0f), view);
    DrawStaticObject(_vertexArrayObjectFloorPlane, 6, shader, glm::vec3(-2.5f, 1.5f, -1.5), glm::vec3(glm::pi<float>(), -0.5f * glm::pi<float>(), 0.0f), glm::vec3(5.0f, 5.0, 5.0f), view);
    DrawStaticObject(_vertexArrayObjectFloorPlane, 6, shader, glm::vec3(0.0f, -1.0f, -1.5), glm::vec3(0.5f * glm::pi<float>(), 0.0f, 0.0f), glm::vec3(5.0f, 5.0, 5.0f), view);
}

void ApplyCubeTransformation(GLuint shader, glm::mat4 cameraView)
{
    //Reset
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    //Model
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
    glm::mat4 rotation = glm::eulerAngleXYZ(0.6f, -1.0f, -0.8f);
    model = translation * rotation;
    model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.6f, -0.3f, 0.3f));

    //View
    view = cameraView;

    //Projection
    projection = glm::perspective(glm::radians(45.0f), (float)ScreenWidth / (float)ScreenHeight, 0.1f, 100.0f);

    //Pass to shader
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void DrawStaticObject(GLuint vertexArrayObject, GLsizei vertexCount, GLuint shader, glm::vec3 position, glm::vec3 orientation, glm::vec3 scale, glm::mat4 cameraView)
{
    //Reset
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    //Model
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 rotation = glm::eulerAngleXYZ(orientation.x, orientation.y, orientation.z);
    model = translation * scaling * rotation;

    //View
    view = cameraView;

    //Projection
    projection = glm::perspective(glm::radians(45.0f), (float)ScreenWidth / (float)ScreenHeight, 0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(vertexArrayObject);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
}

void UpdateKeybaordInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = static_cast<float>(2.5 * _deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        _cameraPosition += cameraSpeed * _playerForward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        _cameraPosition -= cameraSpeed * _playerForward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        _cameraPosition -= glm::normalize(glm::cross(_playerForward, _worldUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        _cameraPosition += glm::normalize(glm::cross(_playerForward, _worldUp)) * cameraSpeed;
}

void MouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    float xoffset = xpos - _lastX;
    float yoffset = _lastY - ypos; 
    _lastX = xpos;
    _lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    _yaw += xoffset;
    _pitch += yoffset;

    _pitch = std::clamp(_pitch, -89.0f, 89.0f);

    glm::vec3 forward;
    forward.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    forward.y = sin(glm::radians(_pitch));
    forward.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    forward = glm::normalize(forward);
    forward += _cameraForwardStart;
    _cameraForward = glm::normalize(forward);

    _playerForward = glm::vec3(forward);
    _playerForward.y = 0;
    _playerForward = glm::vec3(_playerForward);
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
