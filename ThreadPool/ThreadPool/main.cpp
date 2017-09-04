#include <iostream>
#include <array>
//#include <vld.h>

#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include "ThreadPool.h"
#include "WorkQueue.h"
#include "Task.h"
#include "ShaderHelper.h"
#include "Utils.h"

#define BUFFER_OFFSET(i) ((GLvoid *)(i*sizeof(float)))

// Constants for drawing main quad
const size_t g_kNumVerts = 4;
const size_t g_kNumComponents = 5;
const size_t g_kNumTriangles = 2;
const size_t g_kVertArraySize = g_kNumVerts * g_kNumComponents;
const size_t g_kIdxArraySize = g_kNumTriangles * 3;
const size_t g_kPixelsHoriz = 4;
const size_t g_kPixelsVert = 4;

Tensor<GLubyte, g_kPixelsVert, g_kPixelsHoriz, 3> g_textureData;

void errorCallback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
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
	// Create texture on CPU
	for (int i = 0; i < g_textureData.size(); ++i)
	{
		for (int j = 0; j < g_textureData[i].size(); ++j)
		{
			g_textureData[i][j][0] = (i + j) % 2 == 0 ? 255 : 0;
			g_textureData[i][j][1] = 0;
			g_textureData[i][j][2] = (i + j) % 2 == 0 ? 0 : 255;
		}
	}

	// Buffer texture data to GPU
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_kPixelsHoriz, g_kPixelsVert, 0, GL_RGB, GL_UNSIGNED_BYTE, &g_textureData[0][0][0]);

	// Setup texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Bind shader variable to texture unit
	glUniform1i(glGetUniformLocation(program, "texSampler"), 0);

	return texture;
}

int main()
{
	GLuint program;
	GLuint VAO;
	GLFWwindow* window;

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
	glViewport(0, 0, width, height);

	// Setup shaders and rendering
	compileAndLinkShaders("vertex_shader.glsl", "fragment_shader.glsl", program);
	glUseProgram(program);
	createGeometry(VAO);
	GLuint texture = setupTexuring(program);

	// Render loop
	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, g_kIdxArraySize, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//srand((unsigned int)time(0));
	//const int kiTOTALITEMS = 20;
	////Create a ThreadPool Object capable of holding as many threads as the number of cores
	//ThreadPool& threadPool = ThreadPool::GetInstance();
	////Initialize the pool
	//threadPool.Initialize();
	//threadPool.Start();
	//// The main thread writes items to the WorkQueue
	//for(int i =0; i< kiTOTALITEMS; i++)
	//{
	//	threadPool.Submit(CTask(i));
	//	std::cout << "Main Thread wrote item " << i << " to the Work Queue " << std::endl;
	//	//Sleep for some random time to simulate delay in arrival of work items
	//	//std::this_thread::sleep_for(std::chrono::milliseconds(rand()%1001));
	//}
	//if (threadPool.getItemsProcessed() == kiTOTALITEMS)
	//{
	//	threadPool.Stop();
	//}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}