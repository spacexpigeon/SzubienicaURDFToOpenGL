#include <Windows.h>       
#include <GL/glew.h>       
#include <GLFW/glfw3.h>    
#include <GL/glu.h>        
#include <tinyxml2.h>     
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>         

struct URDFLink {
    std::string name;
    std::string meshFile;
    float scaleX{ 1.f };
    float scaleY{ 1.f };
    float scaleZ{ 1.f };
    float translationX{ 0.f };
    float translationY{ 0.f };
    float translationZ{ 0.f };
    float rotationAngle{ 0.f };
    float rotationX{ 0.f };
    float rotationY{ 1.f };
    float rotationZ{ 0.f };
};

static std::vector<URDFLink> g_links;

static float ParseScaleAttribute(const char* scaleAttr) {
    if (!scaleAttr) {
        std::cerr << "[ParseScaleAttribute] Missing scale attribute. Defaulting to 1.0\n";
        return 1.0f;
    }
    float scale = static_cast<float>(std::atof(scaleAttr));
    std::cout << "[ParseScaleAttribute] Parsed scale: " << scale << "\n";
    return scale;
}

void LoadURDF(const std::string& urdfPath) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(urdfPath.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "[LoadURDF] Failed to load URDF: " << urdfPath << std::endl;
        MessageBoxA(NULL, "Failed to load URDF file.", "Error", MB_ICONERROR);
        return;
    }
    std::cout << "[LoadURDF] URDF file loaded: " << urdfPath << "\n";

    tinyxml2::XMLElement* robot = doc.FirstChildElement("robot");
    if (!robot) {
        std::cerr << "[LoadURDF] No <robot> element in URDF.\n";
        MessageBoxA(NULL, "No <robot> element in URDF.", "Error", MB_ICONERROR);
        return;
    }

    for (tinyxml2::XMLElement* link = robot->FirstChildElement("link"); link; link = link->NextSiblingElement("link")) {
        URDFLink urdfLink;

        if (const char* linkName = link->Attribute("name")) {
            urdfLink.name = linkName;
        }
        else {
            std::cerr << "[LoadURDF] Link missing name attribute.\n";
        }

        tinyxml2::XMLElement* visual = link->FirstChildElement("visual");
        if (visual) {
            tinyxml2::XMLElement* geometry = visual->FirstChildElement("geometry");
            if (geometry) {
                tinyxml2::XMLElement* mesh = geometry->FirstChildElement("mesh");
                if (mesh) {
                    if (const char* fn = mesh->Attribute("filename")) {
                        urdfLink.meshFile = fn;
                    }
                    else {
                        std::cerr << "[LoadURDF] Mesh missing filename attribute.\n";
                    }

                    if (const char* sc = mesh->Attribute("scale")) {
                        float s = ParseScaleAttribute(sc);
                        urdfLink.scaleX = s;
                        urdfLink.scaleY = s;
                        urdfLink.scaleZ = s;
                    }
                }
            }
        }

        g_links.push_back(urdfLink);
        std::cout << "[LoadURDF] Found link: " << urdfLink.name
            << ", meshFile=" << urdfLink.meshFile
            << ", scale=" << urdfLink.scaleX << "\n";
    }

    std::cout << "[LoadURDF] All links loaded successfully. Total links: " << g_links.size() << "\n";
    MessageBoxA(NULL, "URDF loaded successfully!", "Info", MB_OK);
}

void RenderURDF() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto& link : g_links) {
        std::cout << "[RenderURDF] Rendering link: " << link.name << "\n";

        if (!link.meshFile.empty()) {
            tinygltf::Model model;
            tinygltf::TinyGLTF loader;
            std::string err, warn;

            bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, link.meshFile);
            if (!ret) {
                std::cerr << "[LoadGLTF] Failed to load glTF: " << link.meshFile
                    << "\nError: " << err << "\nWarning: " << warn << std::endl;
                continue;
            }
            else {
                std::cout << "[LoadGLTF] Successfully loaded glTF: " << link.meshFile
                    << " (nodes: " << model.nodes.size()
                    << ", meshes: " << model.meshes.size() << ")\n";
            }

            // Debug: Sprawdzenie danych geometrii
            for (const auto& mesh : model.meshes) {
                std::cout << "[RenderURDF] Mesh: " << mesh.name
                    << ", Primitives: " << mesh.primitives.size() << "\n";
                for (const auto& primitive : mesh.primitives) {
                    if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                        std::cout << "[RenderURDF] Primitive has POSITION attribute.\n";
                    }
                    else {
                        std::cerr << "[RenderURDF] Primitive missing POSITION attribute!\n";
                    }
                }
            }
        }
        else {
            std::cerr << "[RenderURDF] Link " << link.name << " has no mesh file.\n";
        }
    }

    glfwSwapBuffers(glfwGetCurrentContext());
    std::cout << "[RenderURDF] Rendering completed.\n";
}


bool InitializeOpenGL() {
    if (!glfwInit()) {
        std::cerr << "[InitializeOpenGL] Failed to init GLFW.\n";
        return false;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Quadrotor URDF", nullptr, nullptr);
    if (!window) {
        std::cerr << "[InitializeOpenGL] Failed to create GLFW window.\n";
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "[InitializeOpenGL] Failed to init GLEW.\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    std::cout << "[InitializeOpenGL] OpenGL initialized successfully.\n";
    return true;
}

void SetupCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 800.0 / 600.0, 0.1, 1000.0);
    //160x80
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 200.0, 500.0,   
        0.0, 0.0, 0.0,       
        0.0, 1.0, 0.0);      
    std::cout << "[SetupCamera] Camera setup completed.\n";
}

void SetupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat ambientLight[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat diffuseLight[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    GLfloat lightPosition[] = { 200.0f, 200.0f, 200.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    std::cout << "[SetupLighting] Lighting setup completed.\n";
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);

    std::cout << "[Main] Starting application...\n";

    if (!InitializeOpenGL()) {
        MessageBoxA(NULL, "Could not initialize OpenGL!", "Error", MB_ICONERROR);
        return -1;
    }

    std::string urdfPath = "C:/Users/misko/source/repos/Szubienica3/x64/Debug/SzubienicaURDF/szub.urdf";
    LoadURDF(urdfPath);

    SetupLighting();
    SetupCamera();
    RenderURDF();

    GLFWwindow* window = glfwGetCurrentContext();
    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();
    }

    std::cout << "[Main] Closing application.\n";
    glfwTerminate();
    return 0;
}
