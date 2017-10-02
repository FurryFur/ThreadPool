//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2017 Media Design School
//
// Description  : Collection of opengl related utility functions
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

#pragma once

#include <functional>

namespace GLUtils {
	// Delegate an opengl function for calling on the main thread
	void delegateGLFn(std::function<void()> fn);

	// Remove and invoke the queued opengl function.
	// This should be called on the main thread.
	void invokeDelegatedGLFns();
}