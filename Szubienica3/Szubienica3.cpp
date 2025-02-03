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
#include <cstring>
#include <fstream>   


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


static float ParseScaleAttribute(const char* scaleAttr)
{
    if (!scaleAttr) {
        std::cerr << "[ParseScaleAttribute] Missing scale attribute. Defaulting to 1.0\n";
        return 1.0f;
    }
    float scale = static_cast<float>(std::atof(scaleAttr));
    std::cout << "[ParseScaleAttribute] Parsed scale: " << scale << "\n";
    return scale;
}


void LoadURDF(const std::string& urdfPath)
{
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



static void DrawPrimitiveImmediate(const tinygltf::Model& model,
    const tinygltf::Primitive& primitive)
{
    if (primitive.indices < 0) {
        return;
    }

    const tinygltf::Accessor& indexAccessor = model.accessors[size_t(primitive.indices)];
    const tinygltf::BufferView& indexBufView = model.bufferViews[size_t(indexAccessor.bufferView)];
    const tinygltf::Buffer& indexBuf = model.buffers[size_t(indexBufView.buffer)];

    const unsigned char* indexData = indexBuf.data.data()
        + indexBufView.byteOffset
        + indexAccessor.byteOffset;

    auto attrPosIt = primitive.attributes.find("POSITION");
    if (attrPosIt == primitive.attributes.end()) {
        return;
    }

    int posAccId = attrPosIt->second;
    const tinygltf::Accessor& posAccessor = model.accessors[size_t(posAccId)];
    const tinygltf::BufferView& posBufView = model.bufferViews[size_t(posAccessor.bufferView)];
    const tinygltf::Buffer& posBuf = model.buffers[size_t(posBufView.buffer)];

    const unsigned char* posData = posBuf.data.data()
        + posBufView.byteOffset
        + posAccessor.byteOffset;


    glBegin(GL_TRIANGLES);


    const size_t MAX_DEBUG = 10;
    size_t printedCount = 0;

    for (size_t i = 0; i < indexAccessor.count; i++) {

        uint16_t idx = 0;
        std::memcpy(&idx, indexData + i * sizeof(uint16_t), sizeof(uint16_t));

        size_t vertexOffset = idx * 3; 

        float vx, vy, vz;
        std::memcpy(&vx, posData + (vertexOffset + 0) * sizeof(float), sizeof(float));
        std::memcpy(&vy, posData + (vertexOffset + 1) * sizeof(float), sizeof(float));
        std::memcpy(&vz, posData + (vertexOffset + 2) * sizeof(float), sizeof(float));
/*/
        if (printedCount < MAX_DEBUG) {
            std::cout << "[DrawPrimitiveImmediate] i=" << i
                << ", idx=" << idx
                << " => (vx, vy, vz) = ("
                << vx << ", " << vy << ", " << vz << ")\n";
            printedCount++;
        }
        /*/
        glVertex3f(vx, vy, vz);
    }

    if (indexAccessor.count > MAX_DEBUG) {
        std::cout << "[DrawPrimitiveImmediate] (Printed first "
            << MAX_DEBUG << " vertices of " << indexAccessor.count << " total.)\n";
    }

    glEnd();
}

static void RenderGltfModelImmediate(const tinygltf::Model& model)
{
    for (size_t m = 0; m < model.meshes.size(); m++) {
        const tinygltf::Mesh& mesh = model.meshes[m];
        for (size_t p = 0; p < mesh.primitives.size(); p++) {
            const tinygltf::Primitive& primitive = mesh.primitives[p];
            if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
                DrawPrimitiveImmediate(model, primitive);
            }
        }
    }
}


void RenderURDF()
{

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);

    glColor3f(1.0f, 0.0f, 0.0f);


    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for (auto& link : g_links) {
        std::cout << "[RenderURDF] Rendering link: " << link.name << std::endl;

        std::ifstream testFile(link.meshFile);
        if (!testFile.good()) {
            std::cerr << "[CheckFile] Could NOT open file: " << link.meshFile << "\n";
        }
        else {
            std::cout << "[CheckFile] File " << link.meshFile << " is found.\n";
        }

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
                std::cout << "[LoadGLTF] Successfully loaded glTF: "
                    << link.meshFile
                    << " (nodes: " << model.nodes.size()
                    << ", meshes: " << model.meshes.size()
                    << ")\n";
            }

            glPushMatrix();

      
            glTranslatef(link.translationX, link.translationY, link.translationZ);

            glRotatef(link.rotationAngle, link.rotationX, link.rotationY, link.rotationZ);

            glScalef(link.scaleX, link.scaleY, link.scaleZ);

          
            glScalef(100.0f, 100.0f, 100.0f);

            RenderGltfModelImmediate(model);

            glPopMatrix();

        }
        else {
            std::cerr << "[RenderURDF] Link " << link.name << " has no mesh file.\n";
        }
    }
    std::cout << "[RenderURDF] Rendering completed." << std::endl;
}


void SetupCamera()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 800.0 / 600.0, 0.1, 250000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        65000.0, 11000.0, 10000.0,  // eye
        0.0, 0.0, 0.0,   // center
        0.0, 1.0, 0.0    // up
    );
    std::cout << "[SetupCamera] Camera setup completed.\n";
}


bool InitializeOpenGL()
{
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);

    std::cout << "[Main] Starting application...\n";

    if (!InitializeOpenGL()) {
        MessageBoxA(NULL, "Could not initialize OpenGL!", "Error", MB_ICONERROR);
        return -1;
    }

    std::string urdfPath = "C:/Users/misko/Downloads/Grafika Projekt Pliki/Szubienica-Kielbasy-master/x64/Debug/Szubienica-Urdf-Plik-master/Szubienica-Urdf-Plik-master/szub.urdf";
    LoadURDF(urdfPath);

    SetupCamera();

  
    GLFWwindow* window = glfwGetCurrentContext();
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderURDF();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "[Main] Closing application.\n";
    glfwTerminate();
    return 0;
}
