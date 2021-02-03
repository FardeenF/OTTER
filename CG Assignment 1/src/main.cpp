#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Gameplay/Camera.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Behaviours/CameraControlBehaviour.h"
#include "Behaviours/FollowPathBehaviour.h"
#include "Behaviours/SimpleMoveBehaviour.h"
#include "Gameplay/Application.h"
#include "Gameplay/GameObjectTag.h"
#include "Gameplay/IBehaviour.h"
#include "Gameplay/Transform.h"
#include "Graphics/Texture2D.h"
#include "Graphics/Texture2DData.h"
#include "Utilities/InputHelpers.h"
#include "Utilities/MeshBuilder.h"
#include "Utilities/MeshFactory.h"
#include "Utilities/NotObjLoader.h"
#include "Utilities/ObjLoader.h"
#include "Utilities/VertexTypes.h"
#include "Gameplay/Scene.h"
#include "Gameplay/ShaderMaterial.h"
#include "Gameplay/RendererComponent.h"
#include "Gameplay/Timing.h"
#include "Graphics/TextureCubeMap.h"
#include "Graphics/TextureCubeMapData.h"
#include "Sprite.h"


#define LOG_GL_NOTIFICATIONS

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
	case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
	case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
		#ifdef LOG_GL_NOTIFICATIONS
	case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
		#endif
	default: break;
	}
}

GLFWwindow* window;

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	Application::Instance().ActiveScene->Registry().view<Camera>().each([=](Camera & cam) {
		cam.ResizeWindow(width, height);
	});
}

bool initGLFW() {
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
	
	//Create a new GLFW window
	window = glfwCreateWindow(800, 800, "The Basics", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	// Store the window in the application singleton
	Application::Instance().Window = window;

	return true;
}

bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

void InitImGui() {
	// Creates a new ImGUI context
	ImGui::CreateContext();
	// Gets our ImGUI input/output 
	ImGuiIO& io = ImGui::GetIO();
	// Enable keyboard navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// Allow docking to our window
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// Allow multiple viewports (so we can drag ImGui off our window)
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	// Allow our viewports to use transparent backbuffers
	io.ConfigFlags |= ImGuiConfigFlags_TransparentBackbuffers;

	// Set up the ImGui implementation for OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");

	// Dark mode FTW
	ImGui::StyleColorsDark();

	// Get our imgui style
	ImGuiStyle& style = ImGui::GetStyle();
	//style.Alpha = 1.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 0.8f;
	}
}

void ShutdownImGui()
{
	// Cleanup the ImGui implementation
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	// Destroy our ImGui context
	ImGui::DestroyContext();
}

std::vector<std::function<void()>> imGuiCallbacks;
void RenderImGui() {
	// Implementation new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	// ImGui context new frame
	ImGui::NewFrame();

	if (ImGui::Begin("Debug")) {
		// Render our GUI stuff
		for (auto& func : imGuiCallbacks) {
			func();
		}
		ImGui::End();
	}
	
	// Make sure ImGui knows how big our window is
	ImGuiIO& io = ImGui::GetIO();
	int width{ 0 }, height{ 0 };
	glfwGetWindowSize(window, &width, &height);
	io.DisplaySize = ImVec2((float)width, (float)height);

	// Render all of our ImGui elements
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// If we have multiple viewports enabled (can drag into a new window)
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		// Update the windows that ImGui is using
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		// Restore our gl context
		glfwMakeContextCurrent(window);
	}
}

void RenderVAO(
	const Shader::sptr& shader,
	const VertexArrayObject::sptr& vao,
	const glm::mat4& viewProjection,
	const Transform& transform)
{
	shader->SetUniformMatrix("u_ModelViewProjection", viewProjection * transform.WorldTransform());
	shader->SetUniformMatrix("u_Model", transform.WorldTransform()); 
	shader->SetUniformMatrix("u_NormalMatrix", transform.WorldNormalMatrix());
	vao->Render();
}

void SetupShaderForFrame(const Shader::sptr& shader, const glm::mat4& view, const glm::mat4& projection) {
	shader->Bind();
	// These are the uniforms that update only once per frame
	shader->SetUniformMatrix("u_View", view);
	shader->SetUniformMatrix("u_ViewProjection", projection * view);
	shader->SetUniformMatrix("u_SkyboxMatrix", projection * glm::mat4(glm::mat3(view)));
	glm::vec3 camPos = glm::inverse(view) * glm::vec4(0,0,0,1);
	shader->SetUniform("u_CamPos", camPos);
}



template<typename T>
T LERP(const T& p0, const T& p1, float t)
{
	return (1.0f - t) * p0 + t * p1;
}


//Templated Catmull-Rom function.
//(This will work for any type that supports addition and scalar multiplication.)
template<typename T>
T Catmull(const T& p0, const T& p1, const T& p2, const T& p3, float t)
{
	//TODO: Implement Catmull-Rom interpolation.
	return 0.5f * (2.f * p1 + t * (-p0 + p2)
		+ t * t * (2.f * p0 - 5.f * p1 + 4.f * p2 - p3)
		+ t * t * t * (-p0 + 3.f * p1 - 3.f * p2 + p3));
}


//Catmull-Rom Variables
float m_segmentTravelTime = 2.0f;
float m_segmentTimer;
size_t m_segmentIndex;




std::vector<glm::vec3> phantomWaypoints{ glm::vec3(-4.0f, -4.0f, 2.0f),
										 glm::vec3(4.0f, -4.0f, 2.0f),
										 glm::vec3(4.0f, 4.0f, 2.0f),
										 glm::vec3(-4.0f, 4.0f, 2.0f)};

std::vector<glm::vec3> knightWaypoints{ glm::vec3(4.0f, -4.0f, 1.0f),
										 glm::vec3(4.0f, 4.0f, 1.0f),
										 glm::vec3(-4.0f, 4.0f, 1.0f),
										 glm::vec3(-4.0f, -4.0f, 1.0f)};


//std::vector<glm::vec3> phantomWaypoints;


void UpdateCatmull(std::vector<glm::vec3> points, GameObject object, float deltaTime)
{
	if (points.size() == 0 || m_segmentTravelTime == 0)
		return;

	m_segmentTimer += deltaTime;



	if (m_segmentIndex == 0)
	{
		object.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
	}

	if (m_segmentIndex == 1)
	{
		object.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
	}

	if (m_segmentIndex == 2)
	{
		object.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
	}

	if (m_segmentIndex == 3)
	{
		object.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
	}



	while (m_segmentTimer > m_segmentTravelTime)
	{
		m_segmentTimer -= m_segmentTravelTime;

		m_segmentIndex += 1;

		if (m_segmentIndex >= points.size())
			m_segmentIndex = 0;
	}

	float t = m_segmentTimer / m_segmentTravelTime;

	if (points.size() < 4)
	{
		object.get<Transform>().SetLocalPosition(points[0]);
		return;
	}

	size_t p0_ind, p1_ind, p2_ind, p3_ind;
	glm::vec3 p0, p1, p2, p3;

	//For Catmull, the path segment between p1 and p2
	//Our segment index is gonna be p1
	p1_ind = m_segmentIndex;

	p0_ind = (p1_ind == 0) ? points.size() - 1 : p1_ind - 1;

	p2_ind = (p1_ind + 1) % points.size();

	p3_ind = (p2_ind + 1) % points.size();

	//Setting the vec3s
	p0 = points[p0_ind];
	p1 = points[p1_ind];
	p2 = points[p2_ind];
	p3 = points[p3_ind];

	object.get<Transform>().SetLocalPosition(Catmull(p0, p1, p2, p3, t));
}

void UpdateKnightCatmull(std::vector<glm::vec3> points, GameObject object, float deltaTime)
{
	if (points.size() == 0 || m_segmentTravelTime == 0)
		return;

	m_segmentTimer += deltaTime;



	if (m_segmentIndex == 0)
	{
		object.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
	}

	if (m_segmentIndex == 1)
	{
		object.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
	}

	if (m_segmentIndex == 2)
	{
		object.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
	}

	if (m_segmentIndex == 3)
	{
		object.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
	}



	while (m_segmentTimer > m_segmentTravelTime)
	{
		m_segmentTimer -= m_segmentTravelTime;

		m_segmentIndex += 1;

		if (m_segmentIndex >= points.size())
			m_segmentIndex = 0;
	}

	float t = m_segmentTimer / m_segmentTravelTime;

	if (points.size() < 4)
	{
		object.get<Transform>().SetLocalPosition(points[0]);
		return;
	}

	size_t p0_ind, p1_ind, p2_ind, p3_ind;
	glm::vec3 p0, p1, p2, p3;

	//For Catmull, the path segment between p1 and p2
	//Our segment index is gonna be p1
	p1_ind = m_segmentIndex;

	p0_ind = (p1_ind == 0) ? points.size() - 1 : p1_ind - 1;

	p2_ind = (p1_ind + 1) % points.size();

	p3_ind = (p2_ind + 1) % points.size();

	//Setting the vec3s
	p0 = points[p0_ind];
	p1 = points[p1_ind];
	p2 = points[p2_ind];
	p3 = points[p3_ind];

	object.get<Transform>().SetLocalPosition(Catmull(p0, p1, p2, p3, t));
}



int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	
	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/terrain_vert.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/terrain_frag.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		//Normal Shader
		Shader::sptr normalShader = Shader::Create();
		normalShader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		normalShader->LoadShaderPartFromFile("shaders/frag_phong.glsl", GL_FRAGMENT_SHADER);
		normalShader->Link();

		// Load our shaders
		Shader::sptr spriteShader = Shader::Create();
		spriteShader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		spriteShader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		//spriteShader->LoadShaderPartFromFile("shaders/sprite_vertex.glsl", GL_VERTEX_SHADER);
		//spriteShader->LoadShaderPartFromFile("shaders/sprite_frag.glsl", GL_FRAGMENT_SHADER);
		spriteShader->Link();
		
		//Water Shader
		Shader::sptr waterShader = Shader::Create();
		waterShader->LoadShaderPartFromFile("shaders/water_vert.glsl", GL_VERTEX_SHADER);
		waterShader->LoadShaderPartFromFile("shaders/water_frag.glsl", GL_FRAGMENT_SHADER);
		waterShader->Link();
		
		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 2.0f);
		glm::vec3 lightCol = glm::vec3(0.9f, 0.85f, 0.5f);
		float     lightAmbientPow = 1.0f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.1f;
		float     lightLinearFalloff = 0.0f;
		float     lightQuadraticFalloff = 0.0f;
		float     outlineThickness = 0.0001f;

		glm::vec3 heightCutoffs = glm::vec3(0.35f, 0.567f, 0.764f);
		glm::vec3 interpolateFactors = glm::vec3(0.04f, 0.08f, 0.08f);

		//Shader Control Booleans
		bool lightingToggle = false;
		bool ambientToggle = false;
		bool specularToggle = false;
		bool ambientSpecularToggle = false;
		bool customToggle = false;

		GLfloat waterTime = 0.0;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		shader->SetUniform("u_HeightCutoffs", heightCutoffs);
		shader->SetUniform("u_InterpolateFactors", interpolateFactors);
		shader->SetUniform("u_CustomToggle", (int)customToggle);

		normalShader->SetUniform("u_LightPos", lightPos);
		normalShader->SetUniform("u_LightCol", lightCol);
		normalShader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		normalShader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		normalShader->SetUniform("u_AmbientCol", ambientCol);
		normalShader->SetUniform("u_AmbientStrength", ambientPow);
		normalShader->SetUniform("u_OutlineThickness", outlineThickness);
		normalShader->SetUniform("u_LightToggle", (int)lightingToggle);
		normalShader->SetUniform("u_AmbientToggle", (int)ambientToggle);
		normalShader->SetUniform("u_SpecularToggle", (int)specularToggle);
		normalShader->SetUniform("u_AmbientSpecularToggle", (int)ambientSpecularToggle);
		normalShader->SetUniform("u_CustomToggle", (int)customToggle);
		//normalShader->SetUniform("u_LightAttenuationConstant", 1.0f);
		//normalShader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		//normalShader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

		spriteShader->SetUniform("u_LightPos", lightPos);
		spriteShader->SetUniform("u_LightCol", lightCol);
		spriteShader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		spriteShader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		spriteShader->SetUniform("u_AmbientCol", ambientCol);
		spriteShader->SetUniform("u_AmbientStrength", ambientPow);
		spriteShader->SetUniform("u_LightAttenuationConstant", 1.0f);
		spriteShader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		spriteShader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		
		waterShader->SetUniform("u_LightPos", lightPos);
		waterShader->SetUniform("u_LightCol", lightCol);
		waterShader->SetUniform("u_AmbientLightStrength", 0.1f);
		waterShader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		waterShader->SetUniform("u_AmbientCol", ambientCol);
		waterShader->SetUniform("u_AmbientStrength", ambientPow);
		waterShader->SetUniform("u_LightAttenuationConstant", 1.0f);
		waterShader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		waterShader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

		
		
		// We'll add some ImGui controls to control our shader
		imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Scene Level Lighting Settings"))
			{
				if (ImGui::ColorPicker3("Ambient Color", glm::value_ptr(ambientCol))) {
					shader->SetUniform("u_AmbientCol", ambientCol);
				}
				if (ImGui::SliderFloat("Fixed Ambient Power", &ambientPow, 0.01f, 1.0f)) {
					shader->SetUniform("u_AmbientStrength", ambientPow);
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Pos", glm::value_ptr(lightPos), 0.01f, -10.0f, 10.0f)) {
					shader->SetUniform("u_LightPos", lightPos);
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(lightCol))) {
					shader->SetUniform("u_LightCol", lightCol);
				}
				if (ImGui::SliderFloat("Light Ambient Power", &lightAmbientPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				}
				if (ImGui::SliderFloat("Light Specular Power", &lightSpecularPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				}
				if (ImGui::DragFloat("Light Linear Falloff", &lightLinearFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
				}
				if (ImGui::DragFloat("Light Quadratic Falloff", &lightQuadraticFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
				}
			}
			if (ImGui::SliderFloat3("Height Cutoffs", &heightCutoffs[0], 0, 1)) {
				shader->SetUniform("u_HeightCutoffs", heightCutoffs);
			}
			if (ImGui::Checkbox("No Lighting", &lightingToggle)) 
			{
				lightingToggle = true;
				ambientToggle = false;
				specularToggle = false;
				ambientSpecularToggle = false;
				customToggle = false;
				normalShader->SetUniform("u_LightToggle", (int)lightingToggle);
				normalShader->SetUniform("u_AmbientToggle", (int)ambientToggle);
				normalShader->SetUniform("u_SpecularToggle", (int)specularToggle);
				normalShader->SetUniform("u_AmbientSpecularToggle", (int)ambientSpecularToggle);
				normalShader->SetUniform("u_CustomToggle", (int)customToggle);
				shader->SetUniform("u_CustomToggle", (int)customToggle);
			}
			if (ImGui::Checkbox("Ambient Only", &ambientToggle))
			{
				lightingToggle = false;
				ambientToggle = true;
				specularToggle = false;
				ambientSpecularToggle = false;
				customToggle = false;
				normalShader->SetUniform("u_LightToggle", (int)lightingToggle);
				normalShader->SetUniform("u_AmbientToggle", (int)ambientToggle);
				normalShader->SetUniform("u_SpecularToggle", (int)specularToggle);
				normalShader->SetUniform("u_AmbientSpecularToggle", (int)ambientSpecularToggle);
				normalShader->SetUniform("u_CustomToggle", (int)customToggle);
				shader->SetUniform("u_CustomToggle", (int)customToggle);
			}
			if (ImGui::Checkbox("Specular Only", &specularToggle))
			{
				lightingToggle = false;
				ambientToggle = false;
				specularToggle = true;
				ambientSpecularToggle = false;
				customToggle = false;
				normalShader->SetUniform("u_LightToggle", (int)lightingToggle);
				normalShader->SetUniform("u_AmbientToggle", (int)ambientToggle);
				normalShader->SetUniform("u_SpecularToggle", (int)specularToggle);
				normalShader->SetUniform("u_AmbientSpecularToggle", (int)ambientSpecularToggle);
				normalShader->SetUniform("u_CustomToggle", (int)customToggle);
				shader->SetUniform("u_CustomToggle", (int)customToggle);
			}
			if (ImGui::Checkbox("Ambient + Specular", &ambientSpecularToggle))
			{
				lightingToggle = false;
				ambientToggle = false;
				specularToggle = false;
				ambientSpecularToggle = true;
				customToggle = false;
				normalShader->SetUniform("u_LightToggle", (int)lightingToggle);
				normalShader->SetUniform("u_AmbientToggle", (int)ambientToggle);
				normalShader->SetUniform("u_SpecularToggle", (int)specularToggle);
				normalShader->SetUniform("u_AmbientSpecularToggle", (int)ambientSpecularToggle);
				normalShader->SetUniform("u_CustomToggle", (int)customToggle);
				shader->SetUniform("u_CustomToggle", (int)customToggle);
			}
			if (ImGui::Checkbox("Ambient + Specular + Custom Effect", &customToggle))
			{
				lightingToggle = false;
				ambientToggle = false;
				specularToggle = false;
				ambientSpecularToggle = false;
				customToggle = true;
				normalShader->SetUniform("u_LightToggle", (int)lightingToggle);
				normalShader->SetUniform("u_AmbientToggle", (int)ambientToggle);
				normalShader->SetUniform("u_SpecularToggle", (int)specularToggle);
				normalShader->SetUniform("u_AmbientSpecularToggle", (int)ambientSpecularToggle);
				normalShader->SetUniform("u_CustomToggle", (int)customToggle);
				shader->SetUniform("u_CustomToggle", (int)customToggle);
			}
			
			/*
			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			*/
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		#pragma region TEXTURE LOADING

		// Load some textures from files
		Texture2D::sptr diffuse = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr diffuse2 = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr specular = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr reflectivity = Texture2D::LoadFromFile("images/box-reflections.bmp");

		//Load in character textures
		Texture2D::sptr characterDiffuse = Texture2D::LoadFromFile("images/CharacterTexture.png");
		Texture2D::sptr phantomDiffuse = Texture2D::LoadFromFile("images/PhantomTexture.png");
		Texture2D::sptr knightDiffuse = Texture2D::LoadFromFile("images/stone.jpg");

		//Load Height Map
		Texture2D::sptr heightMap = Texture2D::LoadFromFile("images/heightmap2.jpg");

		//Load Terrain Textures
		Texture2D::sptr sand  = Texture2D::LoadFromFile("images/sand.jpg");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr stone = Texture2D::LoadFromFile("images/stone.jpg");
		Texture2D::sptr snow  = Texture2D::LoadFromFile("images/snow.jpg");

		//Load Water Texture
		Texture2D::sptr water = Texture2D::LoadFromFile("images/water.png");

		//Load Sprite Texture
		Texture2D::sptr sprite = Texture2D::LoadFromFile("images/heart.png");
		
		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ocean.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion
		
		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr material0 = ShaderMaterial::Create();  
		material0->Shader = shader;
		material0->Set("s_HeightMap", heightMap);
		material0->Set("s_Sand", sand);
		material0->Set("s_Grass", grass);
		material0->Set("s_Rock", stone);
		material0->Set("s_Snow", snow);
		material0->Set("u_Shininess", 8.0f);
		material0->Set("u_TextureMix", 0.5f); 

		//Enable Transparency for Objects
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		
		
		ShaderMaterial::sptr waterMaterial = ShaderMaterial::Create();
		waterMaterial->Shader = waterShader;
		waterMaterial->Set("s_Water", water);
		waterMaterial->RenderLayer = 100;

		ShaderMaterial::sptr spriteMaterial = ShaderMaterial::Create();
		spriteMaterial->Shader = spriteShader;
		spriteMaterial->Set("s_Texture", sprite);
		spriteMaterial->RenderLayer = 0;
		
		ShaderMaterial::sptr phantomMat = ShaderMaterial::Create();
		phantomMat->Shader = normalShader;
		phantomMat->Set("s_Diffuse", phantomDiffuse);
		phantomMat->Set("u_LightPos", lightPos);
		phantomMat->Set("u_LightCol", lightCol);
		phantomMat->Set("u_AmbientLightStrength", lightAmbientPow);
		phantomMat->Set("u_SpecularLightStrength", lightSpecularPow);
		phantomMat->Set("u_AmbientCol", ambientCol);
		phantomMat->Set("u_AmbientStrength", ambientPow);
		phantomMat->Set("u_OutlineThickness", 0.5f);
		phantomMat->Set("u_Shininess", 8.0f);

		ShaderMaterial::sptr knightMat = ShaderMaterial::Create();
		knightMat->Shader = normalShader;
		knightMat->Set("s_Diffuse", knightDiffuse);
		knightMat->Set("u_LightPos", lightPos);
		knightMat->Set("u_LightCol", lightCol);
		knightMat->Set("u_AmbientLightStrength", lightAmbientPow);
		knightMat->Set("u_SpecularLightStrength", lightSpecularPow);
		knightMat->Set("u_AmbientCol", ambientCol);
		knightMat->Set("u_AmbientStrength", ambientPow);
		knightMat->Set("u_OutlineThickness", 0.3f);
		knightMat->Set("u_Shininess", 8.0f);
		
		// Load a second material for our reflective material!
		Shader::sptr reflectiveShader = Shader::Create();
		reflectiveShader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		reflectiveShader->LoadShaderPartFromFile("shaders/frag_reflection.frag.glsl", GL_FRAGMENT_SHADER);
		reflectiveShader->Link();

		Shader::sptr reflective = Shader::Create();
		reflective->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		reflective->LoadShaderPartFromFile("shaders/frag_blinn_phong_reflection.glsl", GL_FRAGMENT_SHADER);
		reflective->Link();
		
		
		// Scene Material
		ShaderMaterial::sptr material1 = ShaderMaterial::Create(); 
		material1->Shader = normalShader;
		material1->Set("s_Diffuse", diffuse);
		material1->Set("s_Diffuse2", diffuse2);
		material1->Set("s_Specular", specular);
		material1->Set("s_Reflectivity", reflectivity); 
		material1->Set("s_Environment", environmentMap); 
		material1->Set("u_LightPos", lightPos);
		material1->Set("u_LightCol", lightCol);
		material1->Set("u_AmbientLightStrength", lightAmbientPow); 
		material1->Set("u_SpecularLightStrength", lightSpecularPow); 
		material1->Set("u_AmbientCol", ambientCol);
		material1->Set("u_AmbientStrength", ambientPow);
		material1->Set("u_OutlineThickness", 0.00001f);
		material1->Set("u_LightAttenuationConstant", 1.0f);
		material1->Set("u_LightAttenuationLinear", lightLinearFalloff);
		material1->Set("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		material1->Set("u_Shininess", 8.0f);
		material1->Set("u_TextureMix", 0.5f);
		//material1->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
		
		ShaderMaterial::sptr reflectiveMat = ShaderMaterial::Create();
		reflectiveMat->Shader = reflectiveShader;
		reflectiveMat->Set("s_Environment", environmentMap);
		reflectiveMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
		
		
		GameObject sceneObj = scene->CreateEntity("scene_geo"); 
		{
			
			VertexArrayObject::sptr sceneVao = NotObjLoader::LoadFromFile("Sample.notobj");
			sceneObj.emplace<RendererComponent>().SetMesh(sceneVao).SetMaterial(material1);
			sceneObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 1.2f);
			//sceneObj.get<Transform>().SetLocalScale(glm::vec3(0.5f, 0.5f, 0.5f));
			
		}
	
		
		//Terrain and Water
		
		GameObject obj5 = scene->CreateEntity("Terrain Plane");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");
			
			obj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material0);
			obj5.get<Transform>().SetLocalPosition(-10.0f, -10.0f, 0.0f).SetLocalScale(glm::vec3(20.f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj5);
		}
		/*
		GameObject waterObj = scene->CreateEntity("Water Plane");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");

			waterObj.get_or_emplace<RendererComponent>().SetMesh(vao).SetMaterial(waterMaterial);
			waterObj.get<Transform>().SetLocalPosition(-40.0f, 0.0f, -10.0f).SetLocalScale(glm::vec3(20.f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(waterObj);
		}
		*/
		
		/*
		GameObject obj4 = scene->CreateEntity("moving_box");
		{
			
			// Build a mesh
			MeshBuilder<VertexPosNormTexCol> builder = MeshBuilder<VertexPosNormTexCol>();
			MeshFactory::AddCube(builder, glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f), glm::vec4(1.0f, 0.5f, 0.5f, 1.0f));
			VertexArrayObject::sptr vao = builder.Bake();
			
			obj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material0);
			obj4.get<Transform>().SetLocalPosition(-2.0f, 0.0f, 1.0f);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj4);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -4.0f, -4.0f, 0.0f });
			pathing->Points.push_back({ 4.0f, -4.0f, 0.0f });
			pathing->Points.push_back({ 4.0f,  4.0f, 0.0f });
			pathing->Points.push_back({ -4.0f,  4.0f, 0.0f });
			pathing->Speed = 2.0f;
			
		}
		*/
		
		GameObject phantomObj = scene->CreateEntity("Phantom");
		{
			
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/PhantomFixed.obj");
			phantomObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(phantomMat);
			phantomObj.get<Transform>().SetLocalPosition(-4.0f, -4.0f, 2.0f);
			phantomObj.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			phantomObj.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			
		}

		GameObject knightObj = scene->CreateEntity("Knight");
		{
			
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/scaredKnight.obj");
			knightObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(knightMat);
			knightObj.get<Transform>().SetLocalPosition(4.0f, -4.0f, 2.0f);
			knightObj.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			knightObj.get<Transform>().SetLocalScale(1.5f, 1.5f, 1.5f);

		}
		
		
		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			//cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));
			//Looking at water and terrain positions
			//cameraObject.get<Transform>().SetLocalPosition(-43.89, 25.74, 3.89).LookAt(glm::vec3(-40.69, -0.53, -7.83));
			cameraObject.get<Transform>().SetLocalPosition(1, 8, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		// Create an object to be our orthographic camera
		GameObject orthoCameraObject = scene->CreateEntity("OrthoCamera");
		{
			//cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));
			//Looking at water and terrain positions
			//cameraObject.get<Transform>().SetLocalPosition(-43.89, 25.74, 3.89).LookAt(glm::vec3(-40.69, -0.53, -7.83));
			orthoCameraObject.get<Transform>().SetLocalPosition(1, 8, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& orthoCamera = orthoCameraObject.emplace<Camera>();// Camera::Create();
			orthoCamera.SetPosition(glm::vec3(0, 3, 3));
			orthoCamera.SetUp(glm::vec3(0, 0, 1));
			orthoCamera.LookAt(glm::vec3(0));
			orthoCamera.SetFovDegrees(90.0f); // Set an initial FOV
			orthoCamera.SetOrthoHeight(3.0f);
			orthoCameraObject.get<Camera>().ToggleOrtho();
			BehaviourBinding::Bind<CameraControlBehaviour>(orthoCameraObject);
		}
		
		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			//controllables.push_back(obj2);
			//controllables.push_back(obj3);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}
		
		InitImGui();

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();


		bool move1 = true;
		bool move2 = false;
		bool move3 = false;
		bool move4 = false;

		float speed = 2.0f;

		///// Game loop /////
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});
			
			// Clear the screen
			//glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});
			
			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			// Grab out camera info from the camera object
			Transform& orthoCamTransform = orthoCameraObject.get<Transform>();
			glm::mat4 orthoView = glm::inverse(orthoCamTransform.LocalTransform());
			glm::mat4 orthoProjection = orthoCameraObject.get<Camera>().GetProjection();
			glm::mat4 orthoViewProjection = orthoProjection * orthoView;

			//Set water(wave) time
			//SetupShaderForFrame(waterShader, view, projection);
			waterShader->SetUniform("time", waterTime);
			waterTime += 0.06;

			std::cout << "Camera Position X:" << camTransform.GetLocalPosition().x << std::endl;
			std::cout << "Camera Position Y:" << camTransform.GetLocalPosition().y << std::endl;
			std::cout << "Camera Position Z:" << camTransform.GetLocalPosition().z << "\n" << std::endl;

			//std::cout << "Phantom Position X:" << phantomObj.get<Transform>().GetLocalPosition().x << std::endl;
			//std::cout << "Phantom Position Y:" << phantomObj.get<Transform>().GetLocalPosition().y << std::endl;
			//std::cout << "Phantom Position Z:" << phantomObj.get<Transform>().GetLocalPosition().z << "\n" << std::endl;
			
			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;
				
				return false;
			});


			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			
			// Iterate over the render group components and draw them
			renderGroup.each( [&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});
			

			UpdateCatmull(phantomWaypoints, phantomObj, time.DeltaTime);

			UpdateKnightCatmull(knightWaypoints, knightObj, time.DeltaTime);

			//spriteShader->Bind();
			//SetupShaderForFrame(spriteShader, orthoView, orthoProjection);
			//spriteMaterial->Apply();
			//RenderVAO(spriteShader, spriteVao, orthoViewProjection, spriteObj.get<Transform>());


			//spriteObj.get<Transform>().SetLocalPosition(cameraObject.get<Transform>().GetLocalPosition() + glm::vec3(0.0f, -5.0f, 0.0f));



			// Draw our ImGui content
			RenderImGui();

			scene->Poll();
			glfwSwapBuffers(window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}