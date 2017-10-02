//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2017 Media Design School
//
// Description  : A wrapper for GLFWwindow (basically a unique_ptr)
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

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
