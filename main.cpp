#include <iostream>
#include <map>
#include <utility>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "camera.h"
#include "shader_s.h"
#include "vertex_data.h"
#include "ndim_object.h"
#include "hypercube_objects.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void setImGuiElements();
void initializeObjects();
void cleanUpObjects();
void updateCurrentObject();

// settings
unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;

const unsigned int IMGUI_WINDOW_WIDTH = 180;
const unsigned int IMGUI_WINDOW_HEIGHT = 500;

// camera
Camera camera(glm::vec3(0.0f, 1.0f, -5.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;

float camRotRadius = 4.0f; // rotation radius (adjustable via slider)
float camRotSpeed = 0.7f; // radians per second (adjustable via slider)
float camPos = 0.0f; // camera position around origin (0-1)

// global
float rotationRate = 0.7f;
float EDGE_THICKNESS = 8.0f;
float VERTEX_SIZE = 14.0f;

// timing
float timeRatio = 1.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
// fps calculation
int frameCount = 0;
double lastFPSTime = 0.0;
double currentFPS = 0.0;

// object
NDimObjectData* currentObject;
int shapesIndex = 0;
int currentDimensionIndex = 2;  // Dropdown index (0-6 maps to dimensions 2-8)
// Map to store objects by (shapeType, dimension) key
std::map<std::pair<int, int>, NDimObjectData*> objectMap;


int main()
{
#pragma region setup
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Higher Dimensions", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // imgui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#pragma endregion

    glEnable(GL_DEPTH_TEST);

    initializeObjects();
    currentObject = &hypercube4D;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        frameCount++;
        double currentTime = glfwGetTime();
        if (currentTime - lastFPSTime >= 1.0) {  // Update every second
            currentFPS = frameCount / (currentTime - lastFPSTime);
            frameCount = 0;
            lastFPSTime = currentTime;
        }

        float angle = currentFrame * rotationRate * timeRatio;
        camera.Position.x = camRotRadius * cos(angle);
        camera.Position.z = camRotRadius * sin(angle);
        camera.LookAtTarget(glm::vec3(0.0f, 0.0f, 0.0f));

        // imgui pass 
        setImGuiElements();

        // Activate shader
        currentObject->shader->use();

        // 3D camera matrices
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // Build rotation matrix using object's method
        // Use a static buffer large enough for any dimension we support (up to 8D = 64 floats)
        static float rotationMatrix[64];
        currentObject->buildRotationMatrix(rotationMatrix, currentFrame * timeRatio);

        currentObject->shader->setFloatArray("rotationMat", rotationMatrix, currentObject->matrixSize());
        currentObject->shader->setFloat("scale", currentObject->scale);
        currentObject->shader->setMat4("view", view);
        currentObject->shader->setMat4("projection", projection);

        // draw
        currentObject->draw();

        // draw imgui
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanUpObjects();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void setImGuiElements() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(IMGUI_WINDOW_WIDTH, IMGUI_WINDOW_HEIGHT), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - IMGUI_WINDOW_WIDTH - 20, 20), ImGuiCond_Always);
    ImGui::Begin("Visualizer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("FPS: %.1f", currentFPS);
    ImGui::Spacing();

    ImGui::SeparatorText("Object");
    // family
    ImGui::Text("Family");
    ImGui::Spacing();
    const char* shapesList[] = { "Hypercube", "Simplex", "Cross-Polytope" };
    if (ImGui::Combo("##Object", &shapesIndex, shapesList, IM_ARRAYSIZE(shapesList)))
    {
        updateCurrentObject();
    }
    ImGui::Spacing();
    ImGui::Spacing();

    // dim
    ImGui::Text("Dimensions");
    ImGui::Spacing();
    const char* modelShaderNames[] = { "2D", "3D", "4D", "5D", "6D", "7D", "8D" };
    if (ImGui::Combo("##Dimensions", &currentDimensionIndex, modelShaderNames, IM_ARRAYSIZE(modelShaderNames)))
    {
        updateCurrentObject();
    }
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SeparatorText("Scene");
    // Time Speed
    ImGui::Text("Time Speed");
    ImGui::Spacing();
    ImGui::SliderFloat("##TimeRatio", &timeRatio, 0.0f, 2.0f, "%.1f");
    ImGui::Spacing();
    ImGui::Spacing();

    // Camera Zoom
    ImGui::Text("Camera Zoom");
    ImGui::Spacing();
    float inverseZoom = (91.0f - camera.Zoom) / 10.0f;  // Invert the range and scale to 1-9
    if (ImGui::SliderFloat("##Zoom", &inverseZoom, 1.0f, 9.0f, "%.1f")) {
        camera.Zoom = 91.0f - inverseZoom * 10.0f;  // Convert back to actual zoom
    }
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SeparatorText("Display");

    // Edge Thickness
    ImGui::Text("Edge Thickness");
    ImGui::Spacing();
    ImGui::SliderFloat("##EdgeThickness", &EDGE_THICKNESS, 0.1f, 30.0f, "%.1f px");
    ImGui::Spacing();
    ImGui::Spacing();

    // Vertex Size
    ImGui::Text("Vertex Size");
    ImGui::Spacing();
    ImGui::SliderFloat("##VertexSize", &VERTEX_SIZE, 0.1f, 30.0f, "%.1f px");
    ImGui::Spacing();
    ImGui::Spacing();

}

void initializeObjects() {

    hypercube2D.init();
    hypercube3D.init();
    hypercube4D.init();
    hypercube5D.init();
    hypercube6D.init();
    hypercube7D.init();
    hypercube8D.init();

    simplex2D.init();
    simplex3D.init();
    simplex4D.init();
    simplex5D.init();
    simplex6D.init();
    simplex7D.init();
    simplex8D.init();

    crossPolytope2D.init();
    crossPolytope3D.init();
    crossPolytope4D.init();
    crossPolytope5D.init();
    crossPolytope6D.init();
    crossPolytope7D.init();
    crossPolytope8D.init();

    // Populate object map with (shapeType, dimension) -> object
    // Shape types: 0 = Hypercube, 1 = Hypersphere, 2 = Simplex, 3 = Cross-Polytope
    objectMap[{0, 2}] = &hypercube2D;
    objectMap[{0, 3}] = &hypercube3D;
    objectMap[{0, 4}] = &hypercube4D;
    objectMap[{0, 5}] = &hypercube5D;
    objectMap[{0, 6}] = &hypercube6D;
    objectMap[{0, 7}] = &hypercube7D;
    objectMap[{0, 8}] = &hypercube8D;

    objectMap[{1, 2}] = &simplex2D;
    objectMap[{1, 3}] = &simplex3D;
    objectMap[{1, 4}] = &simplex4D;
    objectMap[{1, 5}] = &simplex5D;
    objectMap[{1, 6}] = &simplex6D;
    objectMap[{1, 7}] = &simplex7D;
    objectMap[{1, 8}] = &simplex8D;

    objectMap[{2, 2}] = &crossPolytope2D;
    objectMap[{2, 3}] = &crossPolytope3D;
    objectMap[{2, 4}] = &crossPolytope4D;
    objectMap[{2, 5}] = &crossPolytope5D;
    objectMap[{2, 6}] = &crossPolytope6D;
    objectMap[{2, 7}] = &crossPolytope7D;
    objectMap[{2, 8}] = &crossPolytope8D;
}
void cleanUpObjects() {

    hypercube2D.cleanup();
    hypercube3D.cleanup();
    hypercube4D.cleanup();
    hypercube5D.cleanup();
    hypercube6D.cleanup();
    hypercube7D.cleanup();
    hypercube8D.cleanup();

    simplex2D.cleanup();
    simplex3D.cleanup();
    simplex4D.cleanup();
    simplex5D.cleanup();
    simplex6D.cleanup();
    simplex7D.cleanup();
    simplex8D.cleanup();

    crossPolytope2D.cleanup();
    crossPolytope3D.cleanup();
    crossPolytope4D.cleanup();
    crossPolytope5D.cleanup();
    crossPolytope6D.cleanup();
    crossPolytope7D.cleanup();
    crossPolytope8D.cleanup();
}
// Helper function to update current object based on shape and dimension selection
void updateCurrentObject() {
    int actualDimension = currentDimensionIndex + 2;  // Convert dropdown index to actual dimension (0->2, 1->3, etc.)
    auto key = std::make_pair(shapesIndex, actualDimension);
    auto it = objectMap.find(key);
    if (it != objectMap.end()) {
        currentObject = it->second;
    }
    else {
        std::cout << "No object found for shape " << shapesIndex << ", dimension " << actualDimension << std::endl;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Adjust camera zoom based on scroll direction
    // yoffset > 0 means scroll up (zoom in), yoffset < 0 means scroll down (zoom out)
    camera.Zoom -= (float)yoffset * 3.0f;

    // Clamp zoom to valid range (1-90, which maps to slider display of 9-1)
    if (camera.Zoom < 1.0f)
        camera.Zoom = 1.0f;
    if (camera.Zoom > 90.0f)
        camera.Zoom = 90.0f;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);

    // Update global screen dimensions
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}
