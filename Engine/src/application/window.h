#pragma once
#include "service.h"

struct SDL_Window;

KENSHIN_BEGIN

class Allocator;
struct WindowConfiguration
{
	Allocator* alloc;
	u32 width;
	u32 height;
	cstring name;

};

typedef void(*OsMessageCallback)(void* event, void* userData);

class Window : public Service
{
public:
	Window() = default;
	~Window() = default;
	virtual bool init(void* configuration = nullptr) override;
	virtual void shutdown() override;
	void handleMessage();
	void setFullScreen(bool enable);
	void registerMessageListener(OsMessageCallback cb, void* userData);
	void unregisterMessageListener(OsMessageCallback cb);
	void centerMouse(bool draging);
	KS_SERVICE_TYPE(Window);
	constexpr static cstring typeName = "Window Service";
public:
	cstring					 mName;
	u32						 mWidth;
	u32						 mHeight;
	SDL_Window*				 mWindow        {nullptr};
	bool					 mIsQuit		{ false };
	bool					 mIsMinimized	{ false };
	bool					 mIsResized		{ false };
	Array<OsMessageCallback> mOsMessageCallbacks;
	Array<void*>			 mOsMessageCallbackUserData;
	static Window			 mInstance;
	f32						 mRefresh;
};

KENSHIN_END