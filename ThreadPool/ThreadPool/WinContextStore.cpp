#include "WinContextStore.h"

#include <GLFW\glfw3.h>


WinContextStore::WinContextStore()
	: m_winContext{ nullptr }
{
}


WinContextStore::~WinContextStore()
{
	if (m_winContext)
		glfwDestroyWindow(m_winContext);
}

void WinContextStore::storeWinContext(GLFWwindow * winContext)
{
	if (m_winContext)
		glfwDestroyWindow(m_winContext);

	m_winContext = winContext;
}

GLFWwindow* WinContextStore::getWinContext() const
{
	return m_winContext;
}
