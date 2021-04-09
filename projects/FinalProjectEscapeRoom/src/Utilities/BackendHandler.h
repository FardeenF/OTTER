#pragma once



#include <iostream>
#include <Logging.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>



#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define LOG_GL_NOTIFICATIONS

class BackendHandler abstract
{
public:
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
	static void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

	//Initialize everything
	static bool InitAll();

	//Window resize callback
	static void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height);

	//Backend Graphic Init Functions
	static bool InitGLFW();
	static bool InitGLAD();

	//ImGui Init Functions
	static void InitImGui();
	static void ShutdownImGui();
	static void RenderImGui();



	static GLFWwindow* window;
	static std::vector<std::function<void()>> imGuiCallbacks;
};