#pragma once

class GLFWwindow;

class WinContextStore {
public:
	WinContextStore();
	~WinContextStore();

	void storeWinContext(GLFWwindow* winContext);
	GLFWwindow* getWinContext() const;
private:
	GLFWwindow* m_winContext;
};

