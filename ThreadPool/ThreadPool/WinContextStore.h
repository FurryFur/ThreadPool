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

#pragma once

class GLFWwindow;

class WinContextStore {
public:
	WinContextStore();
	~WinContextStore();
	WinContextStore(const WinContextStore&) = delete;
	WinContextStore& operator=(const WinContextStore&) = delete;
	
	void storeWinContext(GLFWwindow* winContext);
	GLFWwindow* getWinContext() const;
private:
	GLFWwindow* m_winContext;
};

