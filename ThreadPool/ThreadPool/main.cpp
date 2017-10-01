#define _USE_MATH_DEFINES
#define NANOVG_GL3_IMPLEMENTATION

#include "ThreadPool.h"
#include "WinContextStore.h"
#include "AtomicQueue.h"
#include "ShaderHelper.h"
#include "GLUtils.h"
#include "Utils.h"

#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <nanovg.h>
#include <nanovg_gl.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <complex>
#include <cmath>
//#include <mutex>
//#include <vld.h>

using glm::mat4;
using glm::vec3;
using glm::vec4;

using ThreadPoolT = ThreadPool; // ThreadPoolWithStorage<WinContextStore>;

#define BUFFER_OFFSET(i) ((GLvoid *)(i*sizeof(float)))

// Constants for drawing main quad
const size_t g_kNumVerts = 4;
const size_t g_kNumComponents = 5;
const size_t g_kNumTriangles = 2;
const size_t g_kVertArraySize = g_kNumVerts * g_kNumComponents;
const size_t g_kIdxArraySize = g_kNumTriangles * 3;
const size_t g_kPixelsHoriz = 1024;
const size_t g_kPixelsVert = 1024;
const size_t g_kRegionsHoriz = 16;
const size_t g_kRegionsVert = 16;
const size_t g_kRegionWidth = g_kPixelsHoriz / g_kRegionsHoriz;
const size_t g_kRegionHeight = g_kPixelsVert / g_kRegionsVert;
const size_t g_kFractalDomainRange = 4;
const size_t g_kStartIterations = 20;

const float g_kCameraSpeed = 0.1f;

NDArray<GLubyte, g_kPixelsVert, g_kPixelsHoriz, 3> g_textureData;
std::array<std::future<void>, g_kRegionsHoriz * g_kRegionsVert> g_futures;

bool g_movingForward = false;
bool g_movingBack = false;
bool g_movingLeft = false;
bool g_movingRight = false;
bool g_zoomingIn = false;
bool g_zoomingOut = false;
bool g_fractalRenderRequest = false;

double g_fractalZoomAmount = 1;
double g_fractalCenterRe = -0.5;
double g_fractalCenterIm = 0;

using namespace std::chrono_literals;

void errorCallback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	g_fractalRenderRequest = true;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Convert from screen coordinates to texture coordinates
	int winWidth, winHeight;
	glfwGetWindowSize(window, &winWidth, &winHeight);
	xpos = xpos / winWidth * g_kPixelsHoriz;
	ypos = ypos / winHeight * g_kPixelsVert;

	// Calculate current cursor position in the fractals current domain.
	// Set it to be the new fractal center position.
	double range = g_kFractalDomainRange / g_fractalZoomAmount;
	double minRe = g_fractalCenterRe - range / 2;
	double minIm = g_fractalCenterIm - range / 2;
	g_fractalCenterRe = minRe + xpos / (g_kPixelsHoriz - 1) * range;
	g_fractalCenterIm = minIm + ypos / (g_kPixelsVert - 1) * range;

	// Increase zoom
	const double sensitivity = 2;
	if (yoffset > 0)
		g_fractalZoomAmount *= yoffset * sensitivity;
	else
		g_fractalZoomAmount /= -yoffset * sensitivity;
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
		-1.0f,  1.0f, 0.0f,    0.0f, 0.0f, // Top left
		 1.0f,  1.0f, 0.0f,    1.0f, 0.0f, // Top right
		 1.0f, -1.0f, 0.0f,    1.0f, 1.0f, // Bottom right
		-1.0f, -1.0f, 0.0f,    0.0f, 1.0f  // Bottom left
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

	static float s_translate = 0.0f, s_rotate = 0.0f, s_scale = 1.0f;

	mat4 scale = glm::scale(mat4(), vec3{ s_scale, s_scale, s_scale });
	mat4 rotate = glm::rotate(mat4(), glm::radians(s_rotate), vec3{ 1.0f, 1.0f, 1.0f });
	mat4 translate = glm::translate(mat4(), vec3{ 0.0f, 0.0f, -5.0f });
	mat4 view = glm::lookAt(s_cameraPos, s_cameraPos + s_cameraFront, s_cameraUp);
	mat4 ortho = glm::ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, 0.1f, 100.0f);
	//mat4 perspective = glm::perspective(s_fov, aspect_ratio, 1.0f, 100.0f);

	GLuint scaleLocation = glGetUniformLocation(program, "uScale");
	GLuint rotateLocation = glGetUniformLocation(program, "uRotate");
	GLuint translateLocation = glGetUniformLocation(program, "uTranslate");
	GLuint viewLocation = glGetUniformLocation(program, "uView");
	GLuint projectionLocation = glGetUniformLocation(program, "uProjection");
	//GLuint uCurrentTimeLocation = glGetUniformLocation(program, "uCurrentTime");

	glUniformMatrix4fv(scaleLocation, 1, GL_FALSE, glm::value_ptr(scale));
	glUniformMatrix4fv(rotateLocation, 1, GL_FALSE, glm::value_ptr(rotate));
	glUniformMatrix4fv(translateLocation, 1, GL_FALSE, glm::value_ptr(translate));
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(ortho));
}

void init(GLFWwindow*& window, NVGcontext*& nvgCtx, GLuint& program, GLuint& VAO, GLuint& texture)
{
	glfwSetErrorCallback(errorCallback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	// Create opengl window and context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(g_kPixelsHoriz, g_kPixelsVert, "Multithreaded Mandelbrot Fractal", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

	// Register callbacks
	glfwSetKeyCallback(window, keyCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	// Load opengl functinos
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
	texture = setupTexuring(program);

	// Setup NanoVG
	nvgCtx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	nvgCreateFont(nvgCtx, "sans", "Assets/Font/Roboto-Regular.ttf");
	nvgCreateFont(nvgCtx, "sans-bold", "Assets/Font/example/Roboto-Bold.ttf");
}

void updateTexture(GLuint texture, size_t regionStartX, size_t regionStartY, size_t regionWidth, size_t regionHeight)
{
	// glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	// static thread_local GLFWwindow* s_threadGLCtx = glfwCreateWindow(1, 1, "ctx", nullptr, window);
	//glfwMakeContextCurrent(threadPool.getThreadLocalStorage().getWinContext());

	//glUseProgram(program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, g_kPixelsHoriz);
	glTexSubImage2D(GL_TEXTURE_2D, 0, regionStartX, regionStartY, regionWidth, regionHeight, GL_RGB, GL_UNSIGNED_BYTE, &g_textureData[regionStartY][regionStartX][0]);
}

void process_region(GLuint texture, size_t regionStartX, size_t regionStartY
                  , size_t width, size_t height)
{
	size_t numIterations = static_cast<size_t>(std::log(M_E + g_fractalZoomAmount - 1) * g_kStartIterations);

	size_t regionEndY = regionStartY + height;
	size_t regionEndX = regionStartX + width;
	for (size_t i = regionStartY; i < regionEndY && i < g_textureData.size(); ++i)
	{
		for (size_t j = regionStartX; j < regionEndX && j < g_textureData[i].size(); ++j)
		{
			// Convert from pixel coordinates (integers) to domain of the mandelbrot set 
			// being calculated (complex numbers in range [minRe, maxRe]x[minIm, maxIm])
			double range = g_kFractalDomainRange / g_fractalZoomAmount;
			double minRe = g_fractalCenterRe - range / 2;
			double minIm = g_fractalCenterIm - range / 2;
			double real = minRe + static_cast<double>(j) / (g_kPixelsHoriz - 1) * range;
			double img = minIm + static_cast<double>(i) / (g_kPixelsVert - 1) * range;
			std::complex<double> c = {real, img};
			std::complex<double> z = 0;
			double norm = 0;
			bool diverges = false;
			size_t iteration = 0;
			for (iteration = 1; iteration <= numIterations; ++iteration) {

				z = std::pow(z, 2) + c;

				norm = std::norm(z);
				if (norm > 4) {
					diverges = true;
					break;
				}
			}

			double alpha = static_cast<double>(iteration) / numIterations;

			g_textureData[i][j][0] = diverges ? alpha * 255 : 0;
			g_textureData[i][j][1] = 0;
			g_textureData[i][j][2] = 0;
		}
	}

	//GLUtils::delegateGLFn(std::bind(update_texture, texture, regionStartX, regionStartY, width, height));
}

void submitMandelbrot(ThreadPoolT& threadPool, GLuint texture, double zoomAmount)
{
	size_t regionHeight = g_kRegionHeight;
	size_t regionWidth = g_kRegionWidth;
	
	// Submit regions to the WorkQueue
	for (size_t i = 0; i < g_kRegionsVert; ++i) {
		size_t regionStartY = i * regionHeight;

		// Handle underflow by assigning more work to regions in the last row
		if ((i == g_kRegionsVert - 1) && (regionStartY + regionHeight < g_kPixelsVert))
			regionHeight = g_kPixelsVert - regionStartY;

		for (size_t j = 0; j < g_kRegionsHoriz; ++j) {
			size_t regionStartX = j * regionWidth;

			// Handle underflow by assigning more work to regions in the last column
			if ((j == g_kRegionsHoriz - 1) && (regionStartX + regionWidth < g_kPixelsHoriz))
				regionWidth = g_kPixelsHoriz - regionStartX;

			std::future<void> future = threadPool.submit(process_region, texture, regionStartX, regionStartY, regionWidth, regionHeight);
			//std::cout << "Main Thread wrote item " << i << " to the Work Queue " << std::endl;
			//Sleep for some random time to simulate delay in arrival of work items
			//std::this_thread::sleep_for(std::chrono::milliseconds(rand()%1001));

			size_t workItemIdx = i * g_kRegionsHoriz + j;
			g_futures[workItemIdx] = std::move(future);
		}
	}
}

int main()
{
	GLFWwindow* window;
	NVGcontext* nvgCtx;
	GLuint program, VAO, texture;

	init(window, nvgCtx, program, VAO, texture);

	//Create a ThreadPool Object capable of holding as many threads as the number of cores
	ThreadPoolT threadPool;

	// Add sub-window / opengl sub-context to threads
	//threadPool.getThreadLocalStorage(0).storeWinContext(window); // ID 0 is for the main thread
	//for (size_t i = 1; i <= threadPool.getNumThreads(); ++i) {   // ID 0 is for the main thread
	//															 //glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	//	GLFWwindow* winContext = glfwCreateWindow(1, 1, "ctx", nullptr, window);
	//	threadPool.getThreadLocalStorage(i).storeWinContext(winContext);
	//}

	// TODO: Read from file
	static bool s_fractalTimerRunning = true;
	using namespace std::chrono;
	double fractalTime = -1;
	threadPool.start();
	auto start = high_resolution_clock::now();
	submitMandelbrot(threadPool, texture, g_fractalZoomAmount);

	// Render loop
	while (!glfwWindowShouldClose(window))
	{
		glUseProgram(program);

		int winWidth, winHeight;
		int fbWidth, fbHeight;
		glfwGetWindowSize(window, &winWidth, &winHeight);
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		float aspectRatio = static_cast<float>(winWidth) / winHeight;
		float pxRatio = static_cast<float>(fbWidth) / winWidth;

		// Update the mandelbrot texture on CPU / GPU
		if (g_fractalRenderRequest) {

			threadPool.clearWork();
			s_fractalTimerRunning = true;
			start = high_resolution_clock::now();
			submitMandelbrot(threadPool, texture, g_fractalZoomAmount);
			g_fractalRenderRequest = false;
		}
		updateTexture(texture, 0, 0, g_kPixelsHoriz, g_kPixelsVert);
		if (s_fractalTimerRunning && futuresReady(g_futures)) {
			fractalTime = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count() / 1000000000.0;
			s_fractalTimerRunning = false;
		}

		doTransforms(window, program, aspectRatio);

		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0); // Put this texture in texture unit 0
		glBindTexture(GL_TEXTURE_2D, texture);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, g_kIdxArraySize, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
		glBindVertexArray(0);

		// Draw fractal stats
		if (fractalTime > 0) {
			nvgBeginFrame(nvgCtx, winWidth, winHeight, pxRatio);

			nvgFontFace(nvgCtx, "sans");
			nvgFontSize(nvgCtx, 24);
			nvgFillColor(nvgCtx, nvgRGBA(255, 255, 255, 255));
			nvgTextAlign(nvgCtx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgText(nvgCtx, 10, 10, ("Fractal Calc Time: " + toString(fractalTime, 9)).c_str(), nullptr);
			size_t numIterations = static_cast<size_t>(std::log(M_E + g_fractalZoomAmount - 1) * g_kStartIterations);
			nvgText(nvgCtx, 10, 40, ("Fractal Iteration Depth: " + toString(numIterations)).c_str(), nullptr);
			double range = g_kFractalDomainRange / g_fractalZoomAmount;
			nvgText(nvgCtx, 10, 70, ("Fractal Domain Size: " + toString(range, 20)).c_str(), nullptr);

			nvgEndFrame(nvgCtx);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}