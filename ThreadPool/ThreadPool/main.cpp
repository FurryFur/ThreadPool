#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
//#include <vld.h>

#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "ThreadPool.h"
#include "AtomicQueue.h"
#include "ShaderHelper.h"
#include "Utils.h"

using glm::mat4;
using glm::vec3;
using glm::vec4;

#define BUFFER_OFFSET(i) ((GLvoid *)(i*sizeof(float)))

// Constants for drawing main quad
const size_t g_kNumVerts = 4;
const size_t g_kNumComponents = 5;
const size_t g_kNumTriangles = 2;
const size_t g_kVertArraySize = g_kNumVerts * g_kNumComponents;
const size_t g_kIdxArraySize = g_kNumTriangles * 3;
const size_t g_kPixelsHoriz = 16;
const size_t g_kPixelsVert = 16;

const float g_kCameraSpeed = 0.1f;

Tensor<GLubyte, g_kPixelsVert, g_kPixelsHoriz, 3> g_textureData;
std::array<std::future<void>, g_pixels_vert * g_pixels_horiz> g_futures;

bool g_movingForward = false;
bool g_movingBack = false;
bool g_movingLeft = false;
bool g_movingRight = false;

using namespace std::chrono_literals;

void errorCallback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	else if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_RELEASE))
	{
		g_movingLeft = (action == GLFW_PRESS);
	}
	else if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_RELEASE))
	{
		g_movingRight = (action == GLFW_PRESS);
	}
	else if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_RELEASE))
	{
		g_movingForward = (action == GLFW_PRESS);
	}
	else if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_RELEASE))
	{
		g_movingBack = (action == GLFW_PRESS);
	}
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// Setup VAO for simple quad
void createGeometry(GLuint& rVAO) {
	glGenVertexArrays(1, &rVAO);
	glBindVertexArray(rVAO);

	// Create vertices on CPU
	std::array<GLfloat, g_kVertArraySize> vertices {
		-1.0f,  1.0f, 0.0f,    0.0f, 1.0f, // Top left
		 1.0f,  1.0f, 0.0f,    1.0f, 1.0f, // Top right
		 1.0f, -1.0f, 0.0f,    1.0f, 0.0f, // Bottom right
		-1.0f, -1.0f, 0.0f,    0.0f, 0.0f  // Bottom left
	};

	// Create index buffer on the CPU
	std::array<GLuint, g_kIdxArraySize> indices{
		0, 1, 2,
		0, 2, 3
	};

	// Buffer vertices onto GPU
	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

	// Assign draw order (Buffer indices onto GPU)
	GLuint EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLfloat), indices.data(), GL_STATIC_DRAW);

	// Bind shader variables to the vertex data
	GLuint aPositionLocation = 0;
	glVertexAttribPointer(aPositionLocation, 3, GL_FLOAT, GL_FALSE, g_kNumComponents * sizeof(GLfloat), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(aPositionLocation);

	GLuint aTexcoordLocation = 1;
	glVertexAttribPointer(aTexcoordLocation, 2, GL_FLOAT, GL_FALSE, g_kNumComponents * sizeof(GLfloat), BUFFER_OFFSET(3));
	glEnableVertexAttribArray(aTexcoordLocation);

	glBindVertexArray(0);
}

// Setup texturing
GLuint setupTexuring(GLuint program) {
	// Buffer texture data to GPU
	GLuint texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, g_kPixelsHoriz, g_kPixelsVert, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	// Setup texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Bind shader sampler to a texture unit
	glUniform1i(glGetUniformLocation(program, "uSampler"), 0);

	return texture;
}

void sleep(std::chrono::seconds seconds)
{
	std::this_thread::sleep_for(seconds);
}

void doTransforms(GLFWwindow* window, GLuint program, float aspect_ratio)
{
	static vec3 s_cameraPos{ 0.0f, 0.0f, 3.0 };
	static vec3 s_cameraFront{ 0.0f, 0.0f, -1.0f };
	static vec3 s_cameraUp{ 0.0f, 1.0f, 0.0f };
	static double s_xpos, s_ypos, s_lastX, s_lastY, s_yaw, s_pitch;
	glfwGetCursorPos(window, &s_xpos, &s_ypos);

	static bool first_mouse = true;
	if (first_mouse) {
		s_lastX = s_xpos;
		s_lastY = s_ypos;
		first_mouse = false;
	}

	double xoffset = s_xpos - s_lastX; //+vexoffsetgives clockwise rotation
	double yoffset = s_ypos - s_lastY; //+veyoffsetgives clockwise rotation
	s_lastX = s_xpos;
	s_lastY = s_ypos;
	const double sensitivity = 0.05;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	s_yaw -= xoffset; //clockwise rotation decreases angle since
	s_pitch -= yoffset; //CCW is +verotation
	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (s_pitch > 89.0f)
		s_pitch = 89.0f;
	if (s_pitch < -89.0f)
		s_pitch = -89.0f;

	vec3 frontVector(-cos(glm::radians(s_pitch)) * sin(glm::radians(s_yaw)),
	                  sin(glm::radians(s_pitch)),
	                 -cos(glm::radians(s_pitch)) * cos(glm::radians(s_yaw)));
	s_cameraFront = glm::normalize(frontVector);
	//double currentTime = glfwGetTime();
	//currentTime = currentTime / 1000.0;
	//glUniform1d(uCurrentTimeLocation, currentTime);

	static float s_translate = 0.0f, s_rotate = 0.0f, s_scale = 1.0f;
	s_rotate += 0.6f;

	s_cameraPos += s_cameraFront * g_kCameraSpeed * static_cast<float>(g_movingForward - g_movingBack);
	s_cameraPos += glm::normalize(glm::cross(s_cameraFront, s_cameraUp)) * g_kCameraSpeed * static_cast<float>(g_movingRight - g_movingLeft);

	mat4 scale = glm::scale(mat4(), vec3{ s_scale, s_scale, s_scale });
	mat4 rotate = glm::rotate(mat4(), glm::radians(s_rotate), vec3{ 1.0f, 1.0f, 1.0f });
	mat4 translate = glm::translate(mat4(), vec3{ 0.0f, 0.0f, -5.0f });
	mat4 view = glm::lookAt(s_cameraPos, s_cameraPos + s_cameraFront, s_cameraUp);
	mat4 ortho = glm::ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, 0.1f, 100.0f);
	mat4 perspective = glm::perspective(glm::radians(60.0f), aspect_ratio, 1.0f, 100.0f);

	GLuint scaleLocation = glGetUniformLocation(program, "uScale");
	GLuint rotateLocation = glGetUniformLocation(program, "uRotate");
	GLuint translateLocation = glGetUniformLocation(program, "uTranslate");
	GLuint viewLocation = glGetUniformLocation(program, "uView");
	GLuint perspectiveLocation = glGetUniformLocation(program, "uPerspective");
	//GLuint uCurrentTimeLocation = glGetUniformLocation(program, "uCurrentTime");

	glUniformMatrix4fv(scaleLocation, 1, GL_FALSE, glm::value_ptr(scale));
	glUniformMatrix4fv(rotateLocation, 1, GL_FALSE, glm::value_ptr(rotate));
	glUniformMatrix4fv(translateLocation, 1, GL_FALSE, glm::value_ptr(translate));
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(perspectiveLocation, 1, GL_FALSE, glm::value_ptr(perspective));
}

void init(GLFWwindow*& window, GLuint& program, GLuint& VAO, GLuint& texture, float& aspect_ratio)
{
	glfwSetErrorCallback(errorCallback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	// Create opengl window and context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(800, 800, "Multithreaded Mandelbrot Fractal", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Register callbacks
	glfwSetKeyCallback(window, keyCallback);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Load opengl functinos
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Configure context
	glfwSwapInterval(1);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	// Setup opengl viewport
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	aspect_ratio = width / static_cast<float>(height);
	glViewport(0, 0, width, height);

	// Setup shaders and rendering
	compileAndLinkShaders("vertex_shader.glsl", "fragment_shader.glsl", program);
	glUseProgram(program);
	createGeometry(VAO);
	texture = setupTexuring(program);
}

void process_region(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
	for (size_t i = yoffset; i < static_cast<size_t>(yoffset + height) && i < g_textureData.size(); ++i)
	{
		for (size_t j = xoffset; j < static_cast<size_t>(xoffset + width) && j < g_textureData[i].size(); ++j)
		{
			// Convert to mandelbrot space


			g_textureData[i][j][0] = 0;
			g_textureData[i][j][1] = 255;
			g_textureData[i][j][2] = 0;
		}
	}
}

void update_texture(GLuint texture)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_kPixelsHoriz, g_kPixelsVert, GL_RGB, GL_UNSIGNED_BYTE, &g_textureData[0][0][0]);
}

void doMandelbrot(GLuint texture)
{
	// The main thread writes items to the WorkQueue
	for (int i = 0; i< g_kPixelsVert * g_kPixelsHoriz; i++)
	{
		GLsizei width = 1;
		GLsizei height = 1;
		GLint xoffset = i * width % g_kPixelsHoriz;
		GLint yoffset = i * width / g_kPixelsHoriz;
		std::future<void> future = thread_pool.submit(process_region, xoffset, yoffset, width, height);
		//std::cout << "Main Thread wrote item " << i << " to the Work Queue " << std::endl;
		//Sleep for some random time to simulate delay in arrival of work items
		//std::this_thread::sleep_for(std::chrono::milliseconds(rand()%1001));

		g_futures[i] = std::move(future);
	}

	// Wait for threads to finish
	for (auto& future : g_futures)
	{
		future.get();
	}

	// Send the new texture to the GPU
	update_texture(texture);
}

int main()
{
	GLFWwindow* window;
	GLuint program, VAO, texture;
	float aspect_ratio;

	init(window, program, VAO, texture, aspect_ratio);

	//Create a ThreadPool Object capable of holding as many threads as the number of cores
	ThreadPool threadPool;
	threadPool.start();

	doMandelbrot(threadPool, texture);

	threadPool.stop();

	// Render loop
	while (!glfwWindowShouldClose(window))
	{
		doTransforms(window, program, aspect_ratio);

		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0); // Put this texture in texture unit 0
		glBindTexture(GL_TEXTURE_2D, texture);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, g_kIdxArraySize, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}