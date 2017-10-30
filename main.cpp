//#pragma comment(lib, "d3d11.lib")
//#pragma comment(lib, "d3dx11.lib")
//#pragma comment(lib, "d3dx10.lib")

#include <Windows.h>
#include <D3D11.h>
#include <d3dx11.h>
#include <DxErr.h>
#include <xnamath.h>
#include <exception>
#include <stdio.h>

#define WIDTH 300
#define HEIGHT 300 

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

const TCHAR ClassName[]=TEXT("dx_world");

HWND hwnd;
IDXGISwapChain * d3dSwapChain;
ID3D11Device * d3dDevice;
ID3D11DeviceContext  * d3dDeviceContext;
ID3D11RenderTargetView * renderTargetView;

//FLOAT colorRGBA[4] = {0.0,1.0,0.0,1.0};	//纯绿
FLOAT colorRGBA[4] = {0.0,0.0,0.0,1.0};	//纯黑


void UpdateScene();
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

void messageLoop()
{
	MSG msg;
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
			UpdateScene();
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
	hwnd = CreateWindow(ClassName,TEXT("windowsnametest"),WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,WIDTH,HEIGHT,NULL,NULL,hInstance,NULL);
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

	D3D11CreateDeviceAndSwapChain(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,NULL,NULL,NULL,D3D11_SDK_VERSION,&d3dSwapChainDesc,&d3dSwapChain,&d3dDevice,NULL,&d3dDeviceContext);

	ID3D11Texture2D * backBuffer;
	d3dSwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&backBuffer);
	
	d3dDevice->CreateRenderTargetView(backBuffer,NULL,&renderTargetView);

	d3dDeviceContext->OMSetRenderTargets(1,&renderTargetView,NULL);

}

//struct Vertex
//{
//	XMFLOAT3 position;
//	XMFLOAT4 color;
//};
struct Vertex
{
	XMFLOAT3 position;
};

Vertex vertex[] = 
{
	{XMFLOAT3(0.0,0.0,0.5)},
	{XMFLOAT3(0.0,0.5,0.5)},
	{XMFLOAT3(0.5,0.0,0.5)},
	{XMFLOAT3(0.5,0.5,0.5)},
	{XMFLOAT3(0.5,0.5,0.5)},
};
bool RenderPipeline()
{
	HRESULT hr;
	ID3D11VertexShader* VS;
	ID3D11PixelShader* PS;
	ID3D10Blob* VS_Buffer;
	ID3D10Blob* PS_Buffer;

//创建着色器（顶点着色器和像素着色器）
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "VS", "vs_4_0", 0, 0, 0, &VS_Buffer, 0, 0);
	HR(hr);
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "PS", "ps_4_0", 0, 0, 0, &PS_Buffer, 0, 0);
	HR(hr);
	
	hr = d3dDevice->CreateVertexShader(VS_Buffer->GetBufferPointer(),VS_Buffer->GetBufferSize(),NULL,&VS);
	HR(hr);
	hr = d3dDevice->CreatePixelShader(PS_Buffer->GetBufferPointer(),PS_Buffer->GetBufferSize(),NULL,&PS);
	HR(hr);
	
	d3dDeviceContext->VSSetShader(VS,0,0);
	d3dDeviceContext->PSSetShader(PS,0,0);

//输入汇编器阶段(IA)

	ID3D11Buffer* triangleVertBuffer;
	ID3D11InputLayout *inputLayout;

	//D3D11_INPUT_ELEMENT_DESC verDesc[2] = {
	//	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
	//	{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}
	//};

	//Vertex vertex[] = 
	//{
	//	{XMFLOAT3(0.0,0.0,0.5),XMFLOAT4(0.0,0.5,0.5,0.5)},
	//	{XMFLOAT3(0.0,0.5,0.5),XMFLOAT4(0.0,0.5,0.5,0.5)},
	//	{XMFLOAT3(0.5,0.0,0.5),XMFLOAT4(0.0,0.5,0.5,0.5)}
	//};

	D3D11_INPUT_ELEMENT_DESC verDesc[1] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0}
	};

	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	vertexBufferDesc.ByteWidth = sizeof(Vertex) * ARRAYSIZE(vertex);	//看看能不能换成sizeof(vertex)或是 sizeof(Vertex) * ARRAYSIZE(vertex)
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertex;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	hr = d3dDevice->CreateBuffer(&vertexBufferDesc,&vertexData,&triangleVertBuffer);
	HR(hr);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0,1,&triangleVertBuffer,&stride,&offset);

	hr = d3dDevice->CreateInputLayout(verDesc,ARRAYSIZE(verDesc),VS_Buffer->GetBufferPointer(),VS_Buffer->GetBufferSize(),&inputLayout);
	HR(hr);

	d3dDeviceContext->IASetInputLayout(inputLayout);
	
	//d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	
//顶点着色器阶段(VS)

//光栅化阶段(RS)
	D3D11_VIEWPORT viewPort;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = WIDTH;
	viewPort.Height = HEIGHT;
	viewPort.MaxDepth = 0;
	viewPort.MinDepth = 0;

	d3dDeviceContext->RSSetViewports(1,&viewPort);

	return true;
}

int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	WindowInit(hInstance);

	DirectxInit();
	
	RenderPipeline();

	messageLoop();
	return 0;
}

void UpdateScene()
{
//白绿渐变
	//static bool colorDirection = true;
	//if(colorDirection)
	//{
	//	colorRGBA[0] -= 1.0 / 38000;
	//	colorRGBA[2] = colorRGBA[0];
	//	if(colorRGBA[0] < 0.0)colorDirection = false;
	//}
	//else
	//{
	//	colorRGBA[0] += 1.0 / 38000;
	//	colorRGBA[2] = colorRGBA[0];
	//	if(colorRGBA[0] > 1.0)colorDirection = true;
	//}

}
void DrawScene()
{
	d3dDeviceContext->ClearRenderTargetView(renderTargetView,colorRGBA);
	d3dDeviceContext->Draw(ARRAYSIZE(vertex),0);
	d3dSwapChain->Present(0,0);
}
