//Include and link appropriate libraries and headers//
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")
///////////////**************d2d_texture**************////////////////////
#pragma comment (lib, "D3D10_1.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "D2D1.lib")
#pragma comment (lib, "dwrite.lib")
///////////////**************dx_input**************////////////////////
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

#include <Windows.h>
#include <D3D11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <DxErr.h>
#include <xnamath.h>
#include <exception>
#include <stdio.h>
#include <math.h>
///////////////**************dx_input**************////////////////////
#include <D3D10_1.h>
#include <DXGI.h>
#include <D2D1.h>
#include <sstream>
#include <dwrite.h>
///////////////**************dx_input**************////////////////////
#include <dinput.h>

#ifdef _DEBUG
#include <comdef.h>
#endif

#define WIDTH 100
#define HEIGHT 100 
#define POS_X 1200 
#define POS_Y 700

//#define WIDTH 500
//#define HEIGHT 500 
//#define POS_X 600 
//#define POS_Y 200

//#define LIGHT_TYPE_VERTEX_NORMAL
#define LIGHT_TYPE_PLANE_NORMAL

#if defined(DEBUG) | defined(_DEBUG)
    #ifndef HR
    #define HR(x)                                                 \
    {                                                             \
        HRESULT DefHr = (x);                                      \
        if(FAILED(DefHr))                                         \
        {                                                         \
            DXTrace(__FILE__, (DWORD)__LINE__, DefHr, L#x, true); \
        }                                                         \
    }
    #endif
#else
    #ifndef HR
    #define HR(x) (x)
    #endif
#endifdif
#endif

#if defined(DEBUG) | defined(_DEBUG)
	#ifndef HR_DEBUG
	#define HR_DEBUG(x)								\
	{												\
		if(FAILED(x))								\
		{											\
			_com_error err(x);						\
			LPCTSTR errMsg = err.ErrorMessage();	\
			OutputDebugString(errMsg);				\
		}											\
	}
	#endif
#else
	#ifndef HR_DEBUG
	#define HR_DEBUG(hr) (hr)
	#endif
#endif
//#define HR_DEBUG(hr) (hr)

const TCHAR ClassName[]=TEXT("dx_world");

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT2 textureCoordinate;
	XMFLOAT3 normal;
};
struct ConstSpace
{
	XMMATRIX WVP;
	XMMATRIX worldSpace;
};
struct Light
{
	XMFLOAT3 dir;
	float pad;
	XMFLOAT4 ambientIntensity;
	XMFLOAT4 lightIntensity;
};
struct PointLight
{
	XMFLOAT3 pos;
	float range;
	XMFLOAT4 lightIntensity;
	XMFLOAT3 attr;
	float pad_2;
};
struct SpotLight
{
	XMFLOAT3 pos;
	float range;
	XMFLOAT4 lightIntensity;
	XMFLOAT3 distanceAttr;
	float pad;
	XMFLOAT3 dir;
	float deflectAttr;
};
struct ConstLight
{
	Light light;
};
struct ConstPointLight
{
	PointLight pointLight;
};
struct ConstSpotLight
{
	SpotLight spotLight;
};

HWND hwnd;
IDXGISwapChain * d3dSwapChain;
ID3D11Device * d3dDevice;
ID3D11DeviceContext  * d3dDeviceContext;
ID3D11RenderTargetView * renderTargetView;
ID3D11DepthStencilView * depthStencilView;
ID3D11RasterizerState * rasterState_1;
ID3D11RasterizerState * rasterState_2;
ID3D11Texture2D *depthStencilTexture;
ID3D11Buffer* squareVertBuffer;
ID3D11Buffer* squareIndexBuffer;
XMMATRIX worldSpace;
XMMATRIX viewSpace;
XMMATRIX projectionMatrix;
ID3D11Buffer *constBufferSpace;
ID3D11Buffer *constBufferLight;
ID3D11Buffer *constBufferPointLight;
ID3D11Buffer *constBufferSpotLight;
ConstSpace constSpace;
ConstLight constLight;
ConstPointLight constPointLight;
ConstSpotLight constSpotLight;
ID3D11ShaderResourceView * shaderResourceView_brain;
ID3D11ShaderResourceView * shaderResourceView_grass;
ID3D11SamplerState * samplerState[2];	//1、重复	2、不重复
ID3D11BlendState * blendState;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
XMVECTORF32 eyePos = {-1.0f,1.0f,-1.0f,0.0f};
XMVECTORF32 focusPos = {0.0f,1.0f,0.0f,0.0f};
XMVECTORF32 upPos = {0.0f,1.0f,0.0f,0.0f};
FLOAT cameraRotHorizontal = 0.0f;
FLOAT cameraRotVertical = 0.0f;
XMVECTORF32 cameraDir;
//XMFLOAT3 cameraPos;

//textD2D
ID3D10Device1 * d3d10Device;
ID3D11Texture2D *myTestTexture;
ID3D10Blob* D2D_PS_Buffer;
ID3D11PixelShader* D2D_PS;
ID3D11ShaderResourceView * shaderResourceView_text;
ID2D1RenderTarget * d2dRenderTarget;
IDWriteTextFormat *textFormat;
ID2D1SolidColorBrush *Brush;
ID3D11Buffer* textVertBuffer;
ID3D11Buffer* textIndexBuffer;
IDXGIKeyedMutex * keyMutex11;
IDXGIKeyedMutex * keyMutex10;

//directInput
LPDIRECTINPUT8 directInput;
IDirectInputDevice8 * mouseDevice;
IDirectInputDevice8 * keyboardDevice;

//skyBox
ID3D10Blob* SkyBox_VS_Buffer;
ID3D10Blob* SkyBox_PS_Buffer;
ID3D11VertexShader* SkyBox_VS;
ID3D11PixelShader* SkyBox_PS;
ID3D11Texture2D * skyBoxTexture;
ID3D11ShaderResourceView * shaderResourceView_skyBox;
ID3D11Buffer* skyBoxVertBuffer;
ID3D11Buffer* skyBoxIndexBuffer;
ID3D11DepthStencilState  *skyboxDepthStencilState;


//time
double timeFrequency = 0.0;	//count
double startTime = 0.0;	//count
double lastTime = 0.0;	//time
int frameCount = 0;
int fps = 0;

//FLOAT colorRGBA[4] = {0.0,1.0,0.0,1.0};	//纯绿
FLOAT colorRGBA[4] = {0.0,0.0,0.0,1.0};	//纯黑
FLOAT rot1 = 0.0f;
FLOAT rot2 = 0.0f;

XMFLOAT3 cubeVertex0 = XMFLOAT3(-0.5,0.5,-0.5);
XMFLOAT3 cubeVertex1 = XMFLOAT3(0.5,0.5,-0.5);
XMFLOAT3 cubeVertex2 = XMFLOAT3(-0.5,-0.5,-0.5);
XMFLOAT3 cubeVertex3 = XMFLOAT3(0.5,-0.5,-0.5);
XMFLOAT3 cubeVertex4 = XMFLOAT3(-0.5,0.5,0.5);
XMFLOAT3 cubeVertex5 = XMFLOAT3(0.5,0.5,0.5);
XMFLOAT3 cubeVertex6 = XMFLOAT3(-0.5,-0.5,0.5);
XMFLOAT3 cubeVertex7 = XMFLOAT3(0.5,-0.5,0.5);
#define CONTEXT_PIC_NUM 1.0
XMFLOAT2 leftUp = XMFLOAT2(0.0,0.0);
XMFLOAT2 rightUp = XMFLOAT2(CONTEXT_PIC_NUM,0.0);
XMFLOAT2 leftDown = XMFLOAT2(0.0,CONTEXT_PIC_NUM);
XMFLOAT2 rightDown = XMFLOAT2(CONTEXT_PIC_NUM,CONTEXT_PIC_NUM);
#define SKYBOX_SIZE 1.0
XMFLOAT3 skyBox0 = XMFLOAT3(-SKYBOX_SIZE,SKYBOX_SIZE,-SKYBOX_SIZE);
XMFLOAT3 skyBox1 = XMFLOAT3(SKYBOX_SIZE,SKYBOX_SIZE,-SKYBOX_SIZE);
XMFLOAT3 skyBox2 = XMFLOAT3(-SKYBOX_SIZE,-SKYBOX_SIZE,-SKYBOX_SIZE);
XMFLOAT3 skyBox3 = XMFLOAT3(SKYBOX_SIZE,-SKYBOX_SIZE,-SKYBOX_SIZE);
XMFLOAT3 skyBox4 = XMFLOAT3(-SKYBOX_SIZE,SKYBOX_SIZE,SKYBOX_SIZE);
XMFLOAT3 skyBox5 = XMFLOAT3(SKYBOX_SIZE,SKYBOX_SIZE,SKYBOX_SIZE);
XMFLOAT3 skyBox6 = XMFLOAT3(-SKYBOX_SIZE,-SKYBOX_SIZE,SKYBOX_SIZE);
XMFLOAT3 skyBox7 = XMFLOAT3(SKYBOX_SIZE,-SKYBOX_SIZE,SKYBOX_SIZE);

Vertex vertex[] = 
{
#ifdef LIGHT_TYPE_VERTEX_NORMAL
//顶点法向量
	{cubeVertex0,leftUp,cubeVertex0},
	{cubeVertex1,rightUp,cubeVertex1},
	{cubeVertex2,leftDown,cubeVertex2},
	{cubeVertex3,rightDown,cubeVertex3},
	
	{cubeVertex4,leftUp,cubeVertex4},
	{cubeVertex0,rightUp,cubeVertex0},
	{cubeVertex6,leftDown,cubeVertex6},
	{cubeVertex2,rightDown,cubeVertex2},
	
	{cubeVertex4,leftUp,cubeVertex4},
	{cubeVertex5,rightUp,cubeVertex5},
	{cubeVertex0,leftDown,cubeVertex0},
	{cubeVertex1,rightDown,cubeVertex1},
	
	{cubeVertex1,leftUp,cubeVertex1},
	{cubeVertex5,rightUp,cubeVertex5},
	{cubeVertex3,leftDown,cubeVertex3},
	{cubeVertex7,rightDown,cubeVertex7},
	
	{cubeVertex7,leftUp,cubeVertex7},
	{cubeVertex6,rightUp,cubeVertex6},
	{cubeVertex3,leftDown,cubeVertex3},
	{cubeVertex2,rightDown,cubeVertex2},
	
	{cubeVertex5,leftUp,cubeVertex5},
	{cubeVertex4,rightUp,cubeVertex4},
	{cubeVertex7,leftDown,cubeVertex7},
	{cubeVertex6,rightDown,cubeVertex6},
		
	{XMFLOAT3(-10.0f,0.0f,10.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f)},
	{XMFLOAT3(10.0f,0.0f,10.0f),XMFLOAT2(10.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f)},
	{XMFLOAT3(-10.0f,0.0f,-10.0f),XMFLOAT2(0.0f,10.0f),XMFLOAT3(0.0f,1.0f,0.0f)},
	{XMFLOAT3(10.0f,0.0f,-10.0f),XMFLOAT2(10.0f,10.0f),XMFLOAT3(0.0f,1.0f,0.0f)},
#endif
#ifdef LIGHT_TYPE_PLANE_NORMAL
//平面法向量
	//two boxes
	{cubeVertex0,leftUp,XMFLOAT3(0.0f,0.0f,-1.0f)},
	{cubeVertex1,rightUp,XMFLOAT3(0.0f,0.0f,-1.0f)},
	{cubeVertex2,leftDown,XMFLOAT3(0.0f,0.0f,-1.0f)},
	{cubeVertex3,rightDown,XMFLOAT3(0.0f,0.0f,-1.0f)},
	
	{cubeVertex4,leftUp,XMFLOAT3(-1.0f,0.0f,0.0f)},
	{cubeVertex0,rightUp,XMFLOAT3(-1.0f,0.0f,0.0f)},
	{cubeVertex6,leftDown,XMFLOAT3(-1.0f,0.0f,0.0f)},
	{cubeVertex2,rightDown,XMFLOAT3(-1.0f,0.0f,0.0f)},
	
	{cubeVertex4,leftUp,XMFLOAT3(0.0f,1.0f,0.0f)},
	{cubeVertex5,rightUp,XMFLOAT3(0.0f,1.0f,0.0f)},
	{cubeVertex0,leftDown,XMFLOAT3(0.0f,1.0f,0.0f)},
	{cubeVertex1,rightDown,XMFLOAT3(0.0f,1.0f,0.0f)},
	
	{cubeVertex1,leftUp,XMFLOAT3(1.0f,0.0f,0.0f)},
	{cubeVertex5,rightUp,XMFLOAT3(1.0f,0.0f,0.0f)},
	{cubeVertex3,leftDown,XMFLOAT3(1.0f,0.0f,0.0f)},
	{cubeVertex7,rightDown,XMFLOAT3(1.0f,0.0f,0.0f)},
	
	{cubeVertex7,leftUp,XMFLOAT3(0.0f,-1.0f,0.0f)},
	{cubeVertex6,rightUp,XMFLOAT3(0.0f,-1.0f,0.0f)},
	{cubeVertex3,leftDown,XMFLOAT3(0.0f,-1.0f,0.0f)},
	{cubeVertex2,rightDown,XMFLOAT3(0.0f,-1.0f,0.0f)},
	
	{cubeVertex5,leftUp,XMFLOAT3(0.0f,0.0f,1.0f)},
	{cubeVertex4,rightUp,XMFLOAT3(0.0f,0.0f,1.0f)},
	{cubeVertex7,leftDown,XMFLOAT3(0.0f,0.0f,1.0f)},
	{cubeVertex6,rightDown,XMFLOAT3(0.0f,0.0f,1.0f)},

	//grass
#define PLAIN_SIZE 100.0f		
	{XMFLOAT3(-PLAIN_SIZE,-1.0f,PLAIN_SIZE),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f)},
	{XMFLOAT3(PLAIN_SIZE,-1.0f,PLAIN_SIZE),XMFLOAT2(PLAIN_SIZE,0.0f),XMFLOAT3(0.0f,1.0f,0.0f)},
	{XMFLOAT3(-PLAIN_SIZE,-1.0f,-PLAIN_SIZE),XMFLOAT2(0.0f,PLAIN_SIZE),XMFLOAT3(0.0f,1.0f,0.0f)},
	{XMFLOAT3(PLAIN_SIZE,-1.0f,-PLAIN_SIZE),XMFLOAT2(PLAIN_SIZE,PLAIN_SIZE),XMFLOAT3(0.0f,1.0f,0.0f)},
#endif
};
DWORD index[] = 
{
	//two boxes
	0,1,2,
	2,1,3,

	4,5,6,
	6,5,7,
	
	8,9,10,
	10,9,11,
	
	12,13,14,
	14,13,15,
	
	16,17,18,
	18,17,19,
	
	20,21,22,
	22,21,23,

	//grass
	24,25,26,
	26,25,27,
};

D3D11_INPUT_ELEMENT_DESC verDesc[3] = {
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
	{"TEXTURE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
	{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D11_INPUT_PER_VERTEX_DATA,0}
};

void D2D_init(IDXGIAdapter1 *Adapter);
void IAInitText();

bool InitDirectInput(HINSTANCE hInstance);
void DetectInput(double time);

void SkyBoxInit();

void UpdateScene(double currentFrameTime);
void DrawScene();

LRESULT CALLBACK WinProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE ){
			DestroyWindow(hwnd);
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd,message,wParam,lParam);
	}
}

void startTimer()
{
	LARGE_INTEGER ft,ct;
	QueryPerformanceFrequency(&ft);
	QueryPerformanceCounter(&ct);
	timeFrequency = (double)ft.QuadPart;
	startTime = (double)ct.QuadPart;
}
double getTime()
{
	LARGE_INTEGER ct;
	QueryPerformanceCounter(&ct);
	return (ct.QuadPart - startTime)/timeFrequency;
}
double getFrameTime()
{
	double ct,currentFrameTime;
	ct = getTime();
	currentFrameTime =  ct - lastTime;
	if(currentFrameTime < 0.0)currentFrameTime = 0.0;
	lastTime = ct;
	return currentFrameTime;
}

void messageLoop()
{
	MSG msg;
	double currentFrameTime;
	ZeroMemory(&msg,sizeof(MSG));
	while(msg.message != WM_QUIT)
	{
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			frameCount++;
			if(getTime() > 1.0f)
			{
				fps = frameCount;
				frameCount = 0;
				startTimer();
			}
			currentFrameTime = getFrameTime();
			DetectInput(currentFrameTime);
			UpdateScene(currentFrameTime);
			DrawScene();
		}
	}
}

void WindowInit(HINSTANCE hInstance)
{
	WNDCLASS wc;
	
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground =  (HBRUSH)(GetStockObject(WHITE_BRUSH));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = ClassName;
	wc.lpfnWndProc = WinProc;
	OutputDebugString(L"1");

	RegisterClass(&wc);
	hwnd = CreateWindow(ClassName,TEXT("windowsnametest"),WS_OVERLAPPEDWINDOW,POS_X,POS_Y,WIDTH,HEIGHT,NULL,NULL,hInstance,NULL);
	if(!hwnd)
	{
		MessageBox(NULL,TEXT("创建窗口失败！"),TEXT("提示"),MB_OK);
		UnregisterClass(ClassName,hInstance);
		return;
	}
	OutputDebugString(L"2");
	ShowWindow(hwnd,SW_SHOW);
	UpdateWindow(hwnd);
}

void DirectxInit()
{
	DXGI_MODE_DESC d3dModeDesc;
	HRESULT hr;
	ZeroMemory(&d3dModeDesc, sizeof(DXGI_MODE_DESC));
	d3dModeDesc.Width = WIDTH;
	d3dModeDesc.Height = HEIGHT;
	d3dModeDesc.RefreshRate.Denominator = 1;
	d3dModeDesc.RefreshRate.Numerator = 60;
	d3dModeDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

	DXGI_SWAP_CHAIN_DESC d3dSwapChainDesc;
	ZeroMemory(&d3dSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	d3dSwapChainDesc.BufferDesc = d3dModeDesc;
	d3dSwapChainDesc.BufferCount = 1;
	d3dSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	d3dSwapChainDesc.OutputWindow = hwnd;
	d3dSwapChainDesc.Windowed = true;
	d3dSwapChainDesc.SampleDesc.Count = 1;
	d3dSwapChainDesc.SampleDesc.Quality = 0;

	// Create DXGI factory to enumerate adapters///////////////////////////////////////////////////////////////////////////
	IDXGIFactory1 *DXGIFactory;
	HR(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&DXGIFactory));	
	// Use the first adapter
	IDXGIAdapter1 *Adapter;
	for(int i = 0;DXGIFactory->EnumAdapters1(i, &Adapter) == DXGI_ERROR_NOT_FOUND;i++);
	//DXGIFactory->EnumAdapters1(4, &Adapter);
	DXGIFactory->Release();	

	hr = D3D11CreateDeviceAndSwapChain(Adapter,D3D_DRIVER_TYPE_UNKNOWN,NULL,D3D11_CREATE_DEVICE_DEBUG|D3D11_CREATE_DEVICE_BGRA_SUPPORT,NULL,NULL,D3D11_SDK_VERSION,&d3dSwapChainDesc,&d3dSwapChain,&d3dDevice,NULL,&d3dDeviceContext);	//后面试一下把adapter换回NULL会怎么样
	//hr = D3D11CreateDeviceAndSwapChain(Adapter,D3D_DRIVER_TYPE_UNKNOWN,NULL,D3D11_CREATE_DEVICE_BGRA_SUPPORT,NULL,NULL,D3D11_SDK_VERSION,&d3dSwapChainDesc,&d3dSwapChain,&d3dDevice,NULL,&d3dDeviceContext);	//后面试一下把adapter换回NULL会怎么样
	HR_DEBUG(hr);
	//HR(D3D11CreateDeviceAndSwapChain(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,NULL,NULL,NULL,D3D11_SDK_VERSION,&d3dSwapChainDesc,&d3dSwapChain,&d3dDevice,NULL,&d3dDeviceContext));
	
	D2D_init(Adapter);
	Adapter->Release();

	ID3D11Texture2D * backBuffer;
	d3dSwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&backBuffer);
	
	d3dDevice->CreateRenderTargetView(backBuffer,NULL,&renderTargetView);
//输出合并器阶段(OM)

	D3D11_TEXTURE2D_DESC texture2DDesc;

	texture2DDesc.Width = WIDTH;
	texture2DDesc.Height = HEIGHT;
	texture2DDesc.MipLevels = 1;
	texture2DDesc.ArraySize = 1;
	texture2DDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	texture2DDesc.SampleDesc.Count = 1;
	texture2DDesc.SampleDesc.Quality = 0;
	texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
	texture2DDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	texture2DDesc.CPUAccessFlags = 0;
	texture2DDesc.MiscFlags = 0;

	d3dDevice->CreateTexture2D(&texture2DDesc,NULL,&depthStencilTexture);
	d3dDevice->CreateDepthStencilView(depthStencilTexture,NULL,&depthStencilView);

	d3dDeviceContext->OMSetRenderTargets(1,&renderTargetView,depthStencilView);
}

void D2D_init(IDXGIAdapter1 *Adapter)
{
	HRESULT hr;
	hr = D3D10CreateDevice1(Adapter,D3D10_DRIVER_TYPE_HARDWARE,NULL,D3D10_CREATE_DEVICE_DEBUG|D3D10_CREATE_DEVICE_BGRA_SUPPORT,D3D10_FEATURE_LEVEL_9_3,D3D10_1_SDK_VERSION,&d3d10Device);
	HR_DEBUG(hr);
	D3D11_TEXTURE2D_DESC texture2DDescTemp;
	IDXGIResource * sharedResource;
	HANDLE sharedHandle;

	texture2DDescTemp.Width = WIDTH;
	texture2DDescTemp.Height = HEIGHT;
	texture2DDescTemp.MipLevels = 1;
	texture2DDescTemp.ArraySize = 1;
	texture2DDescTemp.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texture2DDescTemp.SampleDesc.Count = 1;
	texture2DDescTemp.SampleDesc.Quality = 0;
	texture2DDescTemp.Usage = D3D11_USAGE_DEFAULT;
	texture2DDescTemp.BindFlags = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
	texture2DDescTemp.CPUAccessFlags = 0;
	texture2DDescTemp.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
	
	d3dDevice->CreateTexture2D(&texture2DDescTemp,NULL,&myTestTexture);
	myTestTexture->QueryInterface(__uuidof(IDXGIKeyedMutex),(void**)&keyMutex11);
	myTestTexture->QueryInterface(__uuidof(IDXGIResource),(void**)&sharedResource);
	sharedResource->GetSharedHandle(&sharedHandle);
	sharedResource->Release();

	// Open the surface for the shared texture in D3D10.1///////////////////////////////////////////////////////////////////
	IDXGISurface1 *sharedSurface;
	d3d10Device->OpenSharedResource(sharedHandle,__uuidof(IDXGISurface1),(void**)&sharedSurface);
	sharedSurface->QueryInterface(__uuidof(IDXGIKeyedMutex),(void**)&keyMutex10);

	// Create D2D factory///////////////////////////////////////////////////////////////////////////////////////////////////
	ID2D1Factory *d2dFactory; 
	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,__uuidof(ID2D1Factory),(void**)&d2dFactory));

	D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties;

	ZeroMemory(&renderTargetProperties, sizeof(renderTargetProperties));

	renderTargetProperties.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;
	renderTargetProperties.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED);	

	HR(d2dFactory->CreateDxgiSurfaceRenderTarget(sharedSurface, &renderTargetProperties, &d2dRenderTarget));

	sharedSurface->Release();
	d2dFactory->Release();	

	HR(d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 0.0f, 1.0f), &Brush));

	//DirectWrite///////////////////////////////////////////////////////////////////////////////////////////////////////////
	IDWriteFactory *writeFactory;
	HR(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(&writeFactory)));

	HR(writeFactory->CreateTextFormat(
		L"Script",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		20.0f,
		L"en-us",
		&textFormat
		));

	HR(textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));
	HR(textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));

	d3d10Device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);	
	d3dDevice->CreateShaderResourceView(myTestTexture,NULL,&shaderResourceView_text);
}

bool RenderPipeline()
{
	HRESULT hr;

//创建着色器（顶点着色器和像素着色器）
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "VS", "vs_4_0", 0, 0, 0, &VS_Buffer, 0, 0);
	HR(hr);
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "PS", "ps_4_0", D3D11_SHADER_DEBUG_REG_INTERFACE_POINTERS, 0, 0, &PS_Buffer, 0, 0);
	HR(hr);
	
	hr = d3dDevice->CreateVertexShader(VS_Buffer->GetBufferPointer(),VS_Buffer->GetBufferSize(),NULL,&VS);
	HR(hr);
	hr = d3dDevice->CreatePixelShader(PS_Buffer->GetBufferPointer(),PS_Buffer->GetBufferSize(),NULL,&PS);
	HR(hr);
	
	d3dDeviceContext->VSSetShader(VS,0,0);

//输入汇编器阶段(IA)
	
	ID3D11InputLayout *inputLayout;

	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	vertexBufferDesc.ByteWidth = sizeof(vertex);	//看看能不能换成sizeof(vertex)或是 sizeof(Vertex) * ARRAYSIZE(vertex)。结论：不能！因为你可以自己设定一次刷新的内容，不一定要是完整的
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertex;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	hr = d3dDevice->CreateBuffer(&vertexBufferDesc,&vertexData,&squareVertBuffer);
	HR(hr);

	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA indexData;

	indexBufferDesc.ByteWidth = sizeof(index);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = index;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	hr = d3dDevice->CreateBuffer(&indexBufferDesc,&indexData,&squareIndexBuffer);
	HR(hr);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0,1,&squareVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(squareIndexBuffer,DXGI_FORMAT_R32_UINT,0);

	hr = d3dDevice->CreateInputLayout(verDesc,ARRAYSIZE(verDesc),VS_Buffer->GetBufferPointer(),VS_Buffer->GetBufferSize(),&inputLayout);
	HR(hr);

	d3dDeviceContext->IASetInputLayout(inputLayout);
	
	d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
//顶点着色器阶段(VS)

//光栅化阶段(RS)
	D3D11_VIEWPORT viewPort;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = WIDTH;
	viewPort.Height = HEIGHT;
	viewPort.MaxDepth = 1.0;
	viewPort.MinDepth = 0.0;

	d3dDeviceContext->RSSetViewports(1,&viewPort);

	D3D11_RASTERIZER_DESC rasterStateDesc;
	ZeroMemory(&rasterStateDesc,sizeof(rasterStateDesc));
	//rasterStateDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterStateDesc.FillMode = D3D11_FILL_SOLID;
	rasterStateDesc.CullMode = D3D11_CULL_BACK;
	rasterStateDesc.FrontCounterClockwise = false;
	d3dDevice->CreateRasterizerState(&rasterStateDesc,&rasterState_1);
	rasterStateDesc.FrontCounterClockwise = true;
	d3dDevice->CreateRasterizerState(&rasterStateDesc,&rasterState_2);
	//d3dDeviceContext->RSSetState(rasterState_1);

//常量缓存部分（空间变换和光照）
	D3D11_BUFFER_DESC constBufferDesc;

	//透视投影矩阵
	constBufferDesc.ByteWidth = sizeof(ConstSpace);
	constBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constBufferDesc.CPUAccessFlags = 0;
	constBufferDesc.MiscFlags = 0;
	constBufferDesc.StructureByteStride = 0;
	d3dDevice->CreateBuffer(&constBufferDesc,NULL,&constBufferSpace);
	d3dDeviceContext->VSSetConstantBuffers(0,1,&constBufferSpace);

	worldSpace = XMMatrixIdentity();
	projectionMatrix =  XMMatrixPerspectiveFovLH(0.4f*3.14f,(float)WIDTH/(float)HEIGHT,1.0f,1000.0f);
	viewSpace = XMMatrixLookAtLH(eyePos,focusPos,upPos);
	cameraDir.v = focusPos.v - eyePos.v;
	cameraRotHorizontal = atan2(cameraDir.f[2],cameraDir.f[0]);
	cameraRotVertical= atan2(cameraDir.f[1],sqrt(cameraDir.f[2] * cameraDir.f[2] + cameraDir.f[0] * cameraDir.f[0]));

	//平行光
	constBufferDesc.ByteWidth = sizeof(ConstLight);
	d3dDevice->CreateBuffer(&constBufferDesc,NULL,&constBufferLight);
	d3dDeviceContext->PSSetConstantBuffers(1,1,&constBufferLight);

	constLight.light.pad = 0.0f;
	constLight.light.ambientIntensity = XMFLOAT4(0.2f,0.2f,0.2f,0.2f);
	//constLight.light.ambientIntensity = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);	//这个其实是环境光，就先合并到平行光里了
	constLight.light.lightIntensity = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
	constLight.light.dir = XMFLOAT3(5.3f,-1.3f,1.0f);

	d3dDeviceContext->UpdateSubresource(constBufferLight,0,NULL,&constLight,0,0);

	//点光
	constBufferDesc.ByteWidth = sizeof(ConstPointLight);
	d3dDevice->CreateBuffer(&constBufferDesc,NULL,&constBufferPointLight);
	d3dDeviceContext->PSSetConstantBuffers(2,1,&constBufferPointLight);

	constPointLight.pointLight.attr = XMFLOAT3(0.0f,0.5f,0.0f);
	constPointLight.pointLight.range = 100.0f;
	constPointLight.pointLight.pad_2 = 0.0f;
	constPointLight.pointLight.pos = XMFLOAT3(1.3f,1.2f,-1.0f);
	constPointLight.pointLight.lightIntensity = XMFLOAT4(1.0f,0.0f,0.0f,1.0f);
	
	d3dDeviceContext->UpdateSubresource(constBufferPointLight,0,NULL,&constPointLight,0,0);

	//聚光灯光
	constBufferDesc.ByteWidth = sizeof(ConstSpotLight);
	d3dDevice->CreateBuffer(&constBufferDesc,NULL,&constBufferSpotLight);
	d3dDeviceContext->PSSetConstantBuffers(3,1,&constBufferSpotLight);

	constSpotLight.spotLight.distanceAttr = XMFLOAT3(0.0f,0.5f,0.0f);
	constSpotLight.spotLight.range = 5.0f;
	constSpotLight.spotLight.pad = 0.0f;
	constSpotLight.spotLight.pos = XMFLOAT3(eyePos.f);
	constSpotLight.spotLight.dir = XMFLOAT3(cameraDir.f);
	constSpotLight.spotLight.lightIntensity = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
	constSpotLight.spotLight.deflectAttr = 5.0f;
	
	d3dDeviceContext->UpdateSubresource(constBufferSpotLight,0,NULL,&constSpotLight,0,0);

//纹理部分
	HR(D3DX11CreateShaderResourceViewFromFile(d3dDevice,L"braynzar.jpg",NULL,NULL,&shaderResourceView_brain,NULL));
	HR(D3DX11CreateShaderResourceViewFromFile(d3dDevice,L"grass.jpg",NULL,NULL,&shaderResourceView_grass,NULL));
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc,sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	//可以试一下换别的选项看看效果
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; //可以试着修改一下？
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(d3dDevice->CreateSamplerState(&samplerDesc,samplerState));

	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView_brain);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState);
	
//混合渲染
	D3D11_BLEND_DESC blendDesc;
	D3D11_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc;

	RenderTargetBlendDesc.BlendEnable = true;
	RenderTargetBlendDesc.SrcBlend = D3D11_BLEND_SRC_COLOR;
	RenderTargetBlendDesc.DestBlend = D3D11_BLEND_BLEND_FACTOR;
	RenderTargetBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
	RenderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	RenderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	RenderTargetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	RenderTargetBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ZeroMemory(&blendDesc,sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = true;
	blendDesc.RenderTarget[0] = RenderTargetBlendDesc;

	d3dDevice->CreateBlendState(&blendDesc,&blendState);
	
	IAInitText();
	SkyBoxInit();

	return true;
}
void SkyBoxInit()
{
	HRESULT hr;

//创建天空盒的顶点着色器和像素着色器
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "SKYBOX_VS", "vs_4_0", 0, 0, 0, &SkyBox_VS_Buffer, 0, 0);
	HR(hr);
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "SKYBOX_PS", "ps_4_0", 0, 0, 0, &SkyBox_PS_Buffer, 0, 0);
	HR(hr);
	
	hr = d3dDevice->CreateVertexShader(SkyBox_VS_Buffer->GetBufferPointer(),SkyBox_VS_Buffer->GetBufferSize(),NULL,&SkyBox_VS);
	HR(hr);
	hr = d3dDevice->CreatePixelShader(SkyBox_PS_Buffer->GetBufferPointer(),SkyBox_PS_Buffer->GetBufferSize(),NULL,&SkyBox_PS);
	HR(hr);

	Vertex skyBoxVertex[] = 
	{	
		{XMFLOAT3(-1.0,1.0,1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)},
		{XMFLOAT3(1.0,1.0,1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)},
		{XMFLOAT3(-1.0,-1.0,1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)},
		{XMFLOAT3(1.0,-1.0,1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)},
		{XMFLOAT3(-1.0,1.0,-1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)},
		{XMFLOAT3(1.0,1.0,-1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)},
		{XMFLOAT3(-1.0,-1.0,-1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)},
		{XMFLOAT3(1.0,-1.0,-1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)},
	};
	UINT skyBoxIndex[] = 
	{
		0,1,2,
		2,1,3,

		4,5,0,
		0,5,1,

		2,3,6,
		6,3,7,

		5,4,7,
		7,4,6,

		4,0,6,
		6,0,2,

		1,5,3,
		3,5,7,
	};

//天空盒纹理
	HR(D3DX11CreateTextureFromFile(d3dDevice,L"skymap.dds",NULL,NULL,(ID3D11Resource**)&skyBoxTexture,NULL));

	D3D11_SHADER_RESOURCE_VIEW_DESC skyBoxViewDesc;
	D3D11_TEXTURE2D_DESC textureDesc;
	skyBoxTexture->GetDesc(&textureDesc);
	skyBoxViewDesc.Format = textureDesc.Format;
	skyBoxViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	skyBoxViewDesc.TextureCube.MipLevels = textureDesc.MipLevels;
	skyBoxViewDesc.TextureCube.MostDetailedMip = 0;

	d3dDevice->CreateShaderResourceView(skyBoxTexture,&skyBoxViewDesc,&shaderResourceView_skyBox);
//天空盒顶点缓冲区和索引缓冲区
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	vertexBufferDesc.ByteWidth = sizeof(skyBoxVertex);
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = skyBoxVertex;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	hr = d3dDevice->CreateBuffer(&vertexBufferDesc,&vertexData,&skyBoxVertBuffer);
	HR(hr);

	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA indexData;

	indexBufferDesc.ByteWidth = sizeof(skyBoxIndex);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = skyBoxIndex;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	hr = d3dDevice->CreateBuffer(&indexBufferDesc,&indexData,&skyBoxIndexBuffer);
	HR(hr);

//设置深度模板状态（是为了让天空盒在最远的深度上能够显示，所以深度方法要设置成 D3D11_COMPARISON_LESS_EQUAL 小等于）
	D3D11_DEPTH_STENCIL_DESC dssDesc;
	ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	d3dDevice->CreateDepthStencilState(&dssDesc, &skyboxDepthStencilState);
}
void IAInitText()
{
	Vertex textVertex[] = 
	{
		{XMFLOAT3(-1.0,1.0,-1.0),leftUp,XMFLOAT3(0.0f,0.0f,-1.0f)},
		{XMFLOAT3(1.0,1.0,-1.0),rightUp,XMFLOAT3(0.0f,0.0f,-1.0f)},
		{XMFLOAT3(-1.0,-1.0,-1.0),leftDown,XMFLOAT3(0.0f,0.0f,-1.0f)},
		{XMFLOAT3(1.0,-1.0,-1.0),rightDown,XMFLOAT3(0.0f,0.0f,-1.0f)},
	};
	DWORD textIndex[] = 
	{
		0,1,2,
		2,1,3,
	};

	//D3D11_INPUT_ELEMENT_DESC verDesc[2] = {
	//	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
	//	{"TEXTURE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}
	//};

	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	vertexBufferDesc.ByteWidth = sizeof(textVertex);
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = textVertex;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HR(d3dDevice->CreateBuffer(&vertexBufferDesc,&vertexData,&textVertBuffer));

	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA indexData;

	indexBufferDesc.ByteWidth = sizeof(DWORD) * 6;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = textIndex;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	HR(d3dDevice->CreateBuffer(&indexBufferDesc,&indexData,&textIndexBuffer));

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc,sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	//可以试一下换别的选项看看效果
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; //可以试着修改一下？
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(d3dDevice->CreateSamplerState(&samplerDesc,samplerState + 1));

//D2D像素着色器
	D3DX11CompileFromFile(L"Effects.fx",NULL,NULL,"D2D_PS","ps_4_0",NULL,NULL,NULL,&D2D_PS_Buffer,NULL,NULL);
	d3dDevice->CreatePixelShader(D2D_PS_Buffer->GetBufferPointer(),D2D_PS_Buffer->GetBufferSize(),NULL,&D2D_PS);
}

int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	WindowInit(hInstance);

	DirectxInit();
	InitDirectInput(hInstance);
	RenderPipeline();
	messageLoop();
	return 0;
}

void UpdateScene(double currentFrameTime)
{
	//rot2 += .002;
	rot2 += (float)currentFrameTime * 3.1416f;
	if(rot2 > 3.1416f * 2) rot2 -= 3.1416f * 2;
}
void drawText(const wchar_t * text)
{
	//Release the D3D 11 Device
	keyMutex11->ReleaseSync(0);

	//Use D3D10.1 device
	keyMutex10->AcquireSync(0, 5);			

	//Draw D2D content		
	d2dRenderTarget->BeginDraw();	

	//Clear D2D Background
	d2dRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

	//Set the brush color D2D will use to draw with
	Brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));	

	//Draw the Text
	d2dRenderTarget->DrawText(
	text,
	wcslen(text),
	textFormat,
	D2D1::RectF(0, 0, WIDTH, HEIGHT),
	Brush
	);

	d2dRenderTarget->EndDraw();

	//Release the D3D10.1 Device
	keyMutex10->ReleaseSync(1);

	//Use the D3D11 Device
	keyMutex11->AcquireSync(1, 5);

	//Use the shader resource representing the direct2d render target
	//to texture a square which is rendered in screen space so it
	//overlays on top of our entire scene. We use alpha blending so
	//that the entire background of the D2D render target is "invisible",
	//And only the stuff we draw with D2D will be visible (the text)

	//Set the blend state for D2D render target texture objects
	//d3d11DevCon->OMSetBlendState(Transparency, NULL, 0xffffffff);
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	float blendFactor[] = {0.75f, 0.75f, 0.75f, 1.0f};
	d3dDeviceContext->IASetVertexBuffers(0,1,&textVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(textIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	constSpace.WVP = XMMatrixTranspose(XMMatrixIdentity());
	constSpace.worldSpace = constSpace.WVP;
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView_text);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState + 1);
	d3dDeviceContext->PSSetShader(D2D_PS,0,0);
	d3dDeviceContext->OMSetBlendState(blendState,blendFactor,0xffffffff);
	d3dDeviceContext->DrawIndexed(6,0,0);
}
void DrawSkyBox()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0,1,&skyBoxVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(skyBoxIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	d3dDeviceContext->VSSetShader(SkyBox_VS,0,0);
	d3dDeviceContext->PSSetShader(SkyBox_PS,0,0);
	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView_skyBox);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState);
	d3dDeviceContext->OMSetBlendState(0,0,0xffffffff);
	
	XMMATRIX skyBoxPos = XMMatrixTranslation(eyePos.f[0],eyePos.f[1],eyePos.f[2]);
	constSpace.WVP = XMMatrixTranspose(worldSpace * XMMatrixScaling(5.0f,5.0f,5.0f) * skyBoxPos * viewSpace * projectionMatrix);
	constSpace.worldSpace = skyBoxPos;
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);

	d3dDeviceContext->OMSetDepthStencilState(skyboxDepthStencilState,0);
	d3dDeviceContext->DrawIndexed(36,0,0);
	d3dDeviceContext->OMSetDepthStencilState(NULL,0);
}
void DrawScene()
{
	d3dDeviceContext->ClearRenderTargetView(renderTargetView,colorRGBA);
	d3dDeviceContext->ClearDepthStencilView(depthStencilView,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
	
//画天空盒
	DrawSkyBox();
//画两个立方体
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0,1,&squareVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(squareIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	d3dDeviceContext->VSSetShader(VS,0,0);
	d3dDeviceContext->PSSetShader(PS,0,0);
	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView_brain);
	d3dDeviceContext->OMSetBlendState(0,0,0xffffffff);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState);
	constSpotLight.spotLight.pos = XMFLOAT3(eyePos.f);
	constSpotLight.spotLight.dir = XMFLOAT3(cameraDir.f);
	d3dDeviceContext->UpdateSubresource(constBufferSpotLight,0,NULL,&constSpotLight,0,0);

	XMMATRIX ractangle_1 = XMMatrixRotationAxis(XMVectorSet(0,1,0,0),rot2) * XMMatrixTranslation(2,0,0);
	
	d3dDeviceContext->RSSetState(rasterState_2);
	constSpace.WVP = XMMatrixTranspose(worldSpace * ractangle_1 * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace * ractangle_1);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);

	d3dDeviceContext->RSSetState(rasterState_1);
	constSpace.WVP = XMMatrixTranspose(worldSpace * ractangle_1 * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace * ractangle_1);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);

	d3dDeviceContext->RSSetState(rasterState_2);
	constSpace.WVP = XMMatrixTranspose(worldSpace * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);
	
	d3dDeviceContext->RSSetState(rasterState_1);
	constSpace.WVP = XMMatrixTranspose(worldSpace * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);
//画草地
	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView_grass);
	d3dDeviceContext->DrawIndexed(42,0,0);

//显示文本
	wchar_t timeTemp[120];
	swprintf(timeTemp,L"%d",fps);
	drawText(timeTemp);

	d3dSwapChain->Present(0,0);
}

//Input Controller
bool InitDirectInput(HINSTANCE hInstance)
{
	HR_DEBUG(DirectInput8Create(hInstance,DIRECTINPUT_VERSION,IID_IDirectInput8,(void**)&directInput,NULL));
	HR_DEBUG(directInput->CreateDevice(GUID_SysMouse,&mouseDevice,NULL));
	HR_DEBUG(directInput->CreateDevice(GUID_SysKeyboard,&keyboardDevice,NULL));
	mouseDevice->SetDataFormat(&c_dfDIMouse);
	mouseDevice->SetCooperativeLevel(hwnd,DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);
	keyboardDevice->SetDataFormat(&c_dfDIKeyboard);
	keyboardDevice->SetCooperativeLevel(hwnd,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	return true;
}
void DetectInput(double time)
{
	DIMOUSESTATE mouseState;
	BYTE keyboardState[256];

	mouseDevice->Acquire();
	keyboardDevice->Acquire();
	mouseDevice->GetDeviceState(sizeof(DIMOUSESTATE),&mouseState);
	keyboardDevice->GetDeviceState(sizeof(keyboardState),&keyboardState);
	
	cameraRotHorizontal += mouseState.lX*0.001f;
	if(cameraRotHorizontal < -6.2832f || cameraRotHorizontal > 6.2832f)cameraRotHorizontal = fmod(cameraRotHorizontal,6.2832f);
	cameraRotVertical += mouseState.lY*0.001f;
	if(cameraRotVertical < -1.5707f || cameraRotVertical > 1.5707f)
	{
		if(mouseState.lY > 0)
			cameraRotVertical = 1.5707f;
		else
			cameraRotVertical = -1.5707f;
			//cameraRotVertical = fmod(cameraRotVertical,6.2832f);
	}

	cameraDir.v = XMVector3TransformCoord(XMVectorSet(0.0f,0.0f,1.0f,1.0f),XMMatrixRotationY(cameraRotHorizontal));

	cameraDir.v = XMVector3TransformCoord(cameraDir.v,XMMatrixRotationAxis(XMVectorSet(cameraDir.f[2],0.0f,-cameraDir.f[0],1.0f),cameraRotVertical));
	cameraDir.v = XMVector3Normalize(cameraDir);
	
	float speed = 5 * float(time);
	if(keyboardState[DIK_W] & 0x80)
	{
		eyePos.v = XMVector3Transform(eyePos,XMMatrixTranslation(cameraDir.f[0] * speed,0,cameraDir.f[2] * speed));
	}
	if(keyboardState[DIK_S] & 0x80)
	{
		eyePos.v = XMVector3Transform(eyePos,XMMatrixTranslation(-cameraDir.f[0] * speed,0,-cameraDir.f[2] * speed));
	}
	if(keyboardState[DIK_A] & 0x80)
	{
		eyePos.v = XMVector3Transform(eyePos,XMMatrixTranslation(-cameraDir.f[2] * speed,0,cameraDir.f[0] * speed));
	}
	if(keyboardState[DIK_D] & 0x80)
	{
		eyePos.v = XMVector3Transform(eyePos,XMMatrixTranslation(cameraDir.f[2] * speed,0,-cameraDir.f[0] * speed));
	}
	focusPos.v = eyePos.v + cameraDir.v;
	viewSpace = XMMatrixLookAtLH(eyePos,focusPos,upPos);
}