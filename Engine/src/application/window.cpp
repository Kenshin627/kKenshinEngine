#include "pch.h"
#include "window.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

KENSHIN_BEGIN

static constexpr u8 MAX_OS_MESSAGE_LISTENERS = 8;

Window Window::mInstance;

bool Window::init(void* configuration)
{
	KS_CORE_INFO("Initializing Window Service.");
	WindowConfiguration* config = static_cast<WindowConfiguration*>(configuration);
	if (config)
	{
		mWidth = config->width;
		mHeight = config->height;
		mName = config->name;
	}
	else
	{
		mWidth  = 1920;
		mHeight = 1080;
		mName   = "Kenshin Engine Window";
	}
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		KS_CORE_ERROR("SDL_Init failed: {}", SDL_GetError());
		return false;
	}
	mWindow = SDL_CreateWindow(mName, mWidth, mHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
	
	if (!mWindow)
	{
		KS_CORE_ERROR("SDL_CreateWindow failed: {}", SDL_GetError());
		return false;
	}
	if (!config || !config->alloc)
	{
		KS_CORE_ERROR("Window Service requires a valid Allocator in the configuration!");
		return false;
	}
	mOsMessageCallbacks.init(config->alloc, MAX_OS_MESSAGE_LISTENERS);
	mOsMessageCallbackUserData.init(config->alloc, MAX_OS_MESSAGE_LISTENERS);

	const SDL_DisplayMode* displayMode = SDL_GetCurrentDisplayMode(0);
	mRefresh = 1.0f / displayMode->refresh_rate;
	return true;
}

void Window::shutdown()
{
	mOsMessageCallbacks.shutdown();
	mOsMessageCallbackUserData.shutdown();
	SDL_DestroyWindow(mWindow);
	SDL_Quit();
	KS_CORE_INFO("Shutting down Window Service.");
}

void Window::handleMessage()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		//TODO: handle imgui event here.
		switch (e.type)
		{
		case SDL_EVENT_QUIT:
			mIsQuit = true;
			break;
		case SDL_EVENT_WINDOW_MINIMIZED:
			mIsMinimized = true;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			mWidth = static_cast<u32>(e.window.data1); //new width
			mHeight = static_cast<u32>(e.window.data2); //new height
			mIsResized = true;
			break;
		default:
			break;
		}
		sizet size = mOsMessageCallbacks.size();
		for (size_t i = 0; i < size; i++)
		{
			mOsMessageCallbacks[i](&e, mOsMessageCallbackUserData[i]);
		}
	}
}

void Window::setFullScreen(bool enable)
{
	SDL_SetWindowFullscreen(mWindow, enable);
}

void Window::registerMessageListener(OsMessageCallback cb, void* userData)
{
	mOsMessageCallbacks.pushBack(cb);
	mOsMessageCallbackUserData.pushBack(userData);
}

void Window::unregisterMessageListener(OsMessageCallback cb)
{
	sizet size = mOsMessageCallbacks.size();
	for (size_t i = 0; i < size; i++)
	{
		if (mOsMessageCallbacks[i] == cb) 
		{
			mOsMessageCallbacks.deleteSwap(i);
			mOsMessageCallbackUserData.deleteSwap(i);
			break;
		}
	}
}

void Window::centerMouse(bool draging)
{
	if (draging)
	{
		SDL_WarpMouseInWindow(mWindow, mWidth * 1.0f / 2.0f, mHeight * 1.0f / 2.0f);
		SDL_SetWindowMouseGrab(mWindow, true);
		SDL_SetWindowRelativeMouseMode(mWindow, true);
	}
	else
	{
		SDL_SetWindowMouseGrab(mWindow, false);
		SDL_SetWindowRelativeMouseMode(mWindow, false);
	}
}

Window* Window::instance()
{
	return &mInstance;
}

KENSHIN_END