#pragma once

#include <functional>

namespace GLUtils {
	void delegateGLFn(std::function<void()> fn);
	void invokeDelegatedGLFns();
}