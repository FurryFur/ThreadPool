#include "GLUtils.h"

#include <queue>
#include <mutex>

namespace GLUtils {
	std::queue<std::function<void()>> g_delegatedGLFns;
	std::mutex g_mutex;
}

void GLUtils::delegateGLFn(std::function<void()> fn)
{
	std::lock_guard<std::mutex> lock{ g_mutex };
	g_delegatedGLFns.push(std::move(fn));
}

void GLUtils::invokeDelegatedGLFns()
{
	std::unique_lock<std::mutex> lock{ g_mutex };
	while (!g_delegatedGLFns.empty()) {
		std::function<void()> fn = std::move(g_delegatedGLFns.front());
		g_delegatedGLFns.pop();

		// A Function removed from the queue can be safely executed
		// asyncronously with queue modification.
		lock.unlock();
		fn();
		lock.lock();
	}
}
