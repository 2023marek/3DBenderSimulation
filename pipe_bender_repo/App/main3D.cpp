#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include "../Core/PipeAxis3D.h"
#include "../Render/GnuplotExporter3D.h"
#include "../Render/PipeRenderer.h"
#include "../Render/ShaderGL.h"
#include "../Render/CameraGL.h"
#include "../Core/Operations.h"
#include "../Render/ControlCamera.h"
#include "../Render/TubeMesh.h"
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;
double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;

bool firstMouse = true;
bool mousePressed = false;
bool leftMousePressed = false;
bool rightMousePressed = false;

ControlCamera* gCamera = nullptr;
//---------RENDER MODE----------
enum class RenderMode
{
    LINE,
    MESH
};
//---------SETUP BUFFERS----------
unsigned int VAO, VBO, EBO;
void setupBuffers()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

//---------BUILD VERTICES----------
std::vector<float> buildVertices(const PipeAxis3D& pipe)
{
    std::vector<float> v;

    for (const auto& n : pipe.getNodes())
    {
        v.push_back((float)n.pos.x);
        v.push_back((float)n.pos.y);
        v.push_back((float)n.pos.z);
    }

    return v;
}
//---------CREATE SHADERS----------
unsigned int createShader()
{
    const char* vertexShaderSrc = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 MVP;
    void main()
    {
        gl_Position = MVP * vec4(aPos, 1.0);
    }
    )";
    const char* fragmentShaderSrc = R"(
    #version 330 core
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(0.2, 0.8, 0.3, 1.0);
    }
    )";
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSrc, NULL);
    glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSrc, NULL);
    glCompileShader(fs);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

//---------MOUSE CALLBACK----------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float dx = (float)(xpos - lastX);
    float dy = (float)(lastY - ypos);

    lastX = xpos;
    lastY = ypos;

    if (!gCamera) return;

    // LMB ? rotate
    if (leftMousePressed)
        gCamera->processMouseMovement(dx, -  dy);

    // RMB ? pan
    if (rightMousePressed)
        gCamera->processPan(dx, dy);
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        leftMousePressed = (action == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        rightMousePressed = (action == GLFW_PRESS);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (gCamera)
        gCamera->processScroll((float)yoffset);
}
//----------UPDATE BUFFOR-----------

void updateBuffer(const std::vector<float>& vertices)
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
        vertices.data(), GL_DYNAMIC_DRAW);
}
//---------MVP----------
glm::mat4 getMVP(ControlCamera& camera)
{
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjection(WIDTH, HEIGHT);
   
    return proj * view * model;
}
//----------TEMPORARY HELPER--------------
void extractPointsAndTangents(
    const PipeAxis3D& pipe,
    std::vector<Vec3D>& points,
    std::vector<Vec3D>& tangents)
{
    points.clear();
    tangents.clear();

    const auto& nodes = pipe.getNodes();

    for (size_t i = 0; i < nodes.size(); i++)
    {
        points.push_back(nodes[i].pos);

        if (i < nodes.size() - 1)
        {
            Vec3D t = nodes[i + 1].pos - nodes[i].pos;

            if (length(t) > 1e-8)
                tangents.push_back(normalize(t));
            else
                tangents.push_back({ 1,0,0 }); // fallback direction
        }
        else
        {
            tangents.push_back(tangents.back());
        }
    }
}

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Pipe3D", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL();
	glEnable(GL_DEPTH_TEST);
    unsigned int shader = createShader();
    setupBuffers();


    PipeAxis3D pipe(5.0);

    pipe.addFeed(10);
    pipe.addBend(50, PI / 3);
    pipe.addRotate(PI / 3);
    pipe.addFeed(80);
    pipe.addBend(40, PI / 1);

    ControlCamera camera;
    gCamera = &camera;
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    RenderMode mode = RenderMode::MESH;
    TubeMesh tube(5.0, 16);
    while (!glfwWindowShouldClose(window))

    {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        pipe.build();
        if (mode == RenderMode::LINE)
        {
            auto vertices = buildVertices(pipe);
            updateBuffer(vertices);
        }
        else
        {
            std::vector<Vec3D> points, tangents;
            extractPointsAndTangents(pipe, points, tangents);

            tube.build(points, tangents);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER,
                tube.getVertices().size() * sizeof(float),
                tube.getVertices().data(),
                GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                tube.getIndices().size() * sizeof(unsigned int),
                tube.getIndices().data(),
                GL_DYNAMIC_DRAW);
        }
		glUseProgram(shader);
        static bool keyPressed = false;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            if (!keyPressed)
            {
                mode = (mode == RenderMode::LINE) ? RenderMode::MESH : RenderMode::LINE;
                keyPressed = true;
                std::cout << "Mode: " << (mode == RenderMode::LINE ? "LINE" : "MESH") << "\n";
            }
        }
        else
        {
            keyPressed = false;
        }
        glm::mat4 MVP = getMVP(camera);
		glUniformMatrix4fv(glGetUniformLocation(shader, "MVP"), 1, GL_FALSE, &MVP[0][0]);
		glBindVertexArray(VAO);
        if (mode == RenderMode::LINE)
        {
            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)pipe.getNodes().size());
        }
        else
        {
            glDrawElements(GL_TRIANGLES,
                (GLsizei)tube.getIndices().size(),
                GL_UNSIGNED_INT,
                0);
        }
		glfwSwapBuffers(window);
		glfwPollEvents();
    }

	glfwTerminate();


}