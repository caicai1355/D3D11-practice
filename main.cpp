//Include and link appropriate libraries and headers//
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")
///////////////**************new**************////////////////////
#pragma comment (lib, "D3D10_1.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "D2D1.lib")
#pragma comment (lib, "dwrite.lib")
///////////////**************new**************////////////////////

#include <Windows.h>
#include <D3D11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <DxErr.h>
#include <xnamath.h>
#include <exception>
#include <stdio.h>
///////////////**************new**************////////////////////
#include <D3D10_1.h>
#include <DXGI.h>
#include <D2D1.h>
#include <sstream>
#include <dwrite.h>
///////////////**************new**************////////////////////

#define WIDTH 100
#define HEIGHT 100 
#define POS_X 1200 
#define POS_Y 700 

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

//struct Vertex
//{
//	XMFLOAT3 position;
//	XMFLOAT4 color;
//};
struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT2 textureCoordinate;
};
struct ConstBufferStruct
{
	XMMATRIX WVP;
};

//int timetest()
//{
//	LARGE_INTEGER
//	QueryPerformanceCounter
//}

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
XMMATRIX positionMatrix;
ID3D11Buffer *constBuffer;
ConstBufferStruct constBufferStruct;
ID3D11ShaderResourceView * shaderResourceView;
ID3D11SamplerState * samplerState;
ID3D11BlendState * blendState;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
//XMVECTORF32 eyePos = {-0.1f,0.0f,-0.1f,0.0f};
XMVECTORF32 eyePos = {-1.0f,1.0f,-1.0f,0.0f};
XMVECTORF32 focusPos = {0.0f,0.0f,0.0f,0.0f};
XMVECTORF32 upPos = {0.0f,1.0f,0.0f,0.0f};

//textD2D
ID3D10Device1 * d3d10Device;
ID3D11Texture2D *myTestTexture;
ID3D11ShaderResourceView * textResourceView;
ID2D1RenderTarget * d2dRenderTarget;
IDWriteTextFormat *textFormat;
ID2D1SolidColorBrush *Brush;
ID3D11Buffer* textVertBuffer;
ID3D11Buffer* textIndexBuffer;
IDXGIKeyedMutex * keyMutex11;
IDXGIKeyedMutex * keyMutex10;

//FLOAT colorRGBA[4] = {0.0,1.0,0.0,1.0};	//����
FLOAT colorRGBA[4] = {0.0,0.0,0.0,1.0};	//����
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

void D2D_init(IDXGIAdapter1 *Adapter);
void IAInitText();

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
	hwnd = CreateWindow(ClassName,TEXT("windowsnametest"),WS_OVERLAPPEDWINDOW,POS_X,POS_Y,WIDTH,HEIGHT,NULL,NULL,hInstance,NULL);
	if(!hwnd)
	{
		MessageBox(NULL,TEXT("��������ʧ�ܣ�"),TEXT("��ʾ"),MB_OK);
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

	// Create DXGI factory to enumerate adapters///////////////////////////////////////////////////////////////////////////
	IDXGIFactory1 *DXGIFactory;
	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&DXGIFactory);	
	// Use the first adapter	
	IDXGIAdapter1 *Adapter;
	hr = DXGIFactory->EnumAdapters1(0, &Adapter);
	DXGIFactory->Release();	

	D3D11CreateDeviceAndSwapChain(Adapter,D3D_DRIVER_TYPE_UNKNOWN,NULL,D3D11_CREATE_DEVICE_DEBUG|D3D11_CREATE_DEVICE_BGRA_SUPPORT,NULL,NULL,D3D11_SDK_VERSION,&d3dSwapChainDesc,&d3dSwapChain,&d3dDevice,NULL,&d3dDeviceContext);	//������һ�°�adapter����NULL����ô��
	//D3D11CreateDeviceAndSwapChain(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,NULL,NULL,NULL,D3D11_SDK_VERSION,&d3dSwapChainDesc,&d3dSwapChain,&d3dDevice,NULL,&d3dDeviceContext);
	
	D2D_init(Adapter);
	Adapter->Release();

	ID3D11Texture2D * backBuffer;
	d3dSwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&backBuffer);
	
	d3dDevice->CreateRenderTargetView(backBuffer,NULL,&renderTargetView);
//����ϲ����׶�(OM)

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
	HR(D3D10CreateDevice1(Adapter,D3D10_DRIVER_TYPE_HARDWARE,NULL,D3D10_CREATE_DEVICE_DEBUG|D3D10_CREATE_DEVICE_BGRA_SUPPORT,D3D10_FEATURE_LEVEL_9_3,D3D10_1_SDK_VERSION,&d3d10Device));
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
		24.0f,
		L"en-us",
		&textFormat
		));

	HR(textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));
	HR(textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));

	d3d10Device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);	
	d3dDevice->CreateShaderResourceView(myTestTexture,NULL,&textResourceView);
}

bool RenderPipeline()
{
	HRESULT hr;
	ID3D11VertexShader* VS;
	ID3D11PixelShader* PS;

//������ɫ����������ɫ����������ɫ����
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

//���������׶�(IA)

	//Vertex vertex[] = 
	//{
	//	{XMFLOAT3(-0.5,0.5,-0.5),XMFLOAT4(1.0,1.0,1.0,1.0)},
	//	{XMFLOAT3(0.5,0.5,-0.5),XMFLOAT4(1.0,1.0,0.0,1.0)},
	//	{XMFLOAT3(-0.5,-0.5,-0.5),XMFLOAT4(1.0,0.0,1.0,1.0)},
	//	{XMFLOAT3(0.5,-0.5,-0.5),XMFLOAT4(1.0,0.0,0.0,1.0)},
	//	{XMFLOAT3(-0.5,0.5,0.5),XMFLOAT4(0.0,1.0,1.0,1.0)},
	//	{XMFLOAT3(0.5,0.5,0.5),XMFLOAT4(0.0,1.0,0.0,1.0)},
	//	{XMFLOAT3(-0.5,-0.5,0.5),XMFLOAT4(0.0,0.0,1.0,1.0)},
	//	{XMFLOAT3(0.5,-0.5,0.5),XMFLOAT4(0.0,0.0,0.0,1.0)},
	//};
	Vertex vertex[] = 
	{
		{cubeVertex0,leftUp},
		{cubeVertex1,rightUp},
		{cubeVertex2,leftDown},
		{cubeVertex3,rightDown},
	
		{cubeVertex4,leftUp},
		{cubeVertex0,rightUp},
		{cubeVertex6,leftDown},
		{cubeVertex2,rightDown},
	
		{cubeVertex4,leftUp},
		{cubeVertex5,rightUp},
		{cubeVertex0,leftDown},
		{cubeVertex1,rightDown},
	
		{cubeVertex1,leftUp},
		{cubeVertex5,rightUp},
		{cubeVertex3,leftDown},
		{cubeVertex7,rightDown},
	
		{cubeVertex7,leftUp},
		{cubeVertex6,rightUp},
		{cubeVertex3,leftDown},
		{cubeVertex2,rightDown},
	
		{cubeVertex5,leftUp},
		{cubeVertex4,rightUp},
		{cubeVertex7,leftDown},
		{cubeVertex6,rightDown},
	};
	//DWORD index[] = 
	//{
	//	0,1,2,
	//	2,1,3,
	//	4,5,0,
	//	0,5,1,
	//	4,0,6,
	//	6,0,2,
	//	1,5,3,
	//	3,5,7,
	//	7,6,3,
	//	3,6,2,
	//	5,4,7,
	//	7,4,6,
	//};
	DWORD index[] = 
	{
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
	};
	
	ID3D11InputLayout *inputLayout;

	D3D11_INPUT_ELEMENT_DESC verDesc[2] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXTURE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}
	};

	//D3D11_INPUT_ELEMENT_DESC verDesc[1] = {
	//	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0}
	//};

	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	vertexBufferDesc.ByteWidth = sizeof(vertex);	//�����ܲ��ܻ���sizeof(vertex)���� sizeof(Vertex) * ARRAYSIZE(vertex)�����ۣ����ܣ���Ϊ������Լ��趨һ��ˢ�µ����ݣ���һ��Ҫ��������
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

	indexBufferDesc.ByteWidth = sizeof(DWORD) * 36;
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
	
//������ɫ���׶�(VS)

//��դ���׶�(RS)
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

//�������沿��
	D3D11_BUFFER_DESC constBufferDesc;

	constBufferDesc.ByteWidth = sizeof(ConstBufferStruct);
	constBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constBufferDesc.CPUAccessFlags = 0;
	constBufferDesc.MiscFlags = 0;
	constBufferDesc.StructureByteStride = 0;
	d3dDevice->CreateBuffer(&constBufferDesc,NULL,&constBuffer);
	d3dDeviceContext->VSSetConstantBuffers(0,1,&constBuffer);

	worldSpace = XMMatrixIdentity();
	positionMatrix =  XMMatrixPerspectiveFovLH(0.4f*3.14f,(float)WIDTH/(float)HEIGHT,1.0f,1000.0f);

//������
	HR(D3DX11CreateShaderResourceViewFromFile(d3dDevice,L"braynzar.jpg",NULL,NULL,&shaderResourceView,NULL));
	//HR(D3DX11CreateShaderResourceViewFromFile(d3dDevice,L"bb.jpg",NULL,NULL,&shaderResourceView,NULL));
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc,sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	//������һ�»����ѡ���Ч��
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	//samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; //���������޸�һ�£�
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	//samplerDesc.BorderColor[0] = 1.0f;
	//samplerDesc.BorderColor[3] = 1.0f;
	HR(d3dDevice->CreateSamplerState(&samplerDesc,&samplerState));

	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView);
	d3dDeviceContext->PSSetSamplers(0,1,&samplerState);

//�����Ⱦ
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
	
	float blendFactor[] = {0.75f, 0.75f, 0.75f, 1.0f};
	d3dDeviceContext->OMSetBlendState(0,0,0xffffffff);
	d3dDeviceContext->OMSetBlendState(blendState,blendFactor,0xffffffff);

	IAInitText();

	return true;
}
void IAInitText()
{
	Vertex textVertex[] = 
	{
		{XMFLOAT3(-1.0,1.0,-1.0),leftUp},
		{XMFLOAT3(1.0,1.0,-1.0),rightUp},
		{XMFLOAT3(-1.0,-1.0,-1.0),leftDown},
		{XMFLOAT3(1.0,-1.0,-1.0),rightDown},
	};
	DWORD textIndex[] = 
	{
		0,1,2,
		2,1,3,
	};

	D3D11_INPUT_ELEMENT_DESC verDesc[2] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXTURE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}
	};

	//D3D11_INPUT_ELEMENT_DESC verDesc[1] = {
	//	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0}
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
//���̽���
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

	//rot1 += .0002f;
	//if(rot1 > 12.76f)rot1 = 0.1f;
	//eyePos.f[0]=eyePos.f[2]=-rot1;

	rot2 += .002f;
	if(rot2 > 3.1415 * 2) rot2 = 0.0f;

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
	d3dDeviceContext->IASetVertexBuffers(0,1,&textVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(textIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	constBufferStruct.WVP = XMMatrixTranspose(XMMatrixIdentity());
	d3dDeviceContext->UpdateSubresource(constBuffer,0,NULL,&constBufferStruct,0,0);
	d3dDeviceContext->PSSetShaderResources(0,1,&textResourceView);
	d3dDeviceContext->DrawIndexed(6,0,0);
}
void DrawScene()
{
	d3dDeviceContext->ClearRenderTargetView(renderTargetView,colorRGBA);
	d3dDeviceContext->ClearDepthStencilView(depthStencilView,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0,1,&squareVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(squareIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView);

	XMMATRIX ractangle_1 = XMMatrixRotationAxis(XMVectorSet(0,1,0,0),rot2) * XMMatrixTranslation(2,0,0);
	viewSpace = XMMatrixLookAtLH(eyePos,focusPos,upPos);
	
	d3dDeviceContext->RSSetState(rasterState_2);
	constBufferStruct.WVP = XMMatrixTranspose(worldSpace * ractangle_1 * viewSpace * positionMatrix);
	d3dDeviceContext->UpdateSubresource(constBuffer,0,NULL,&constBufferStruct,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);

	d3dDeviceContext->RSSetState(rasterState_1);
	constBufferStruct.WVP = XMMatrixTranspose(worldSpace * ractangle_1 * viewSpace * positionMatrix);
	d3dDeviceContext->UpdateSubresource(constBuffer,0,NULL,&constBufferStruct,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);

	d3dDeviceContext->RSSetState(rasterState_2);
	constBufferStruct.WVP = XMMatrixTranspose(worldSpace * viewSpace * positionMatrix);
	d3dDeviceContext->UpdateSubresource(constBuffer,0,NULL,&constBufferStruct,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);
	
	d3dDeviceContext->RSSetState(rasterState_1);
	constBufferStruct.WVP = XMMatrixTranspose(worldSpace * viewSpace * positionMatrix);
	d3dDeviceContext->UpdateSubresource(constBuffer,0,NULL,&constBufferStruct,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);

	LARGE_INTEGER ft;
	//QueryPerformanceFrequency(&ft);
	QueryPerformanceCounter(&ft);

	wchar_t timeTemp[120];
	swprintf(timeTemp,L"%Ld",ft.QuadPart);

	drawText(timeTemp);

	d3dSwapChain->Present(0,0);
}
