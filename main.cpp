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

#define WIDTH 100
#define HEIGHT 100 

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

HWND hwnd;
IDXGISwapChain * d3dSwapChain;
ID3D11Device * d3dDevice;
ID3D11DeviceContext  * d3dDeviceContext;
ID3D11RenderTargetView * renderTargetView;
ID3D11DepthStencilView * depthStencilView;
ID3D11RasterizerState * rasterState_1;
XMMATRIX worldSpace;
XMMATRIX viewSpace;
XMMATRIX positionMatrix;
ID3D11Buffer *constBuffer;
ConstBufferStruct constBufferStruct;
ID3D11ShaderResourceView * shaderResourceView;
ID3D11SamplerState * samplerState;
//XMVECTORF32 eyePos = {-0.1f,0.0f,-0.1f,0.0f};
XMVECTORF32 eyePos = {-2.0f,2.0f,-2.0f,0.0f};
XMVECTORF32 focusPos = {0.0f,0.0f,0.0f,0.0f};
XMVECTORF32 upPos = {0.0f,1.0f,0.0f,0.0f};

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
	hwnd = CreateWindow(ClassName,TEXT("windowsnametest"),WS_OVERLAPPEDWINDOW,1200,700,WIDTH,HEIGHT,NULL,NULL,hInstance,NULL);
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
//输出合并器阶段(OM)
	ID3D11Texture2D *texture2D;

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

	d3dDevice->CreateTexture2D(&texture2DDesc,NULL,&texture2D);
	d3dDevice->CreateDepthStencilView(texture2D,NULL,&depthStencilView);

	d3dDeviceContext->OMSetRenderTargets(1,&renderTargetView,depthStencilView);
}

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
	{cubeVertex0,XMFLOAT2(0.0,0.0)},
	{cubeVertex1,XMFLOAT2(1.0,0.0)},
	{cubeVertex2,XMFLOAT2(0.0,1.0)},
	{cubeVertex3,XMFLOAT2(1.0,1.0)},
	
	{cubeVertex4,XMFLOAT2(0.0,0.0)},
	{cubeVertex0,XMFLOAT2(1.0,0.0)},
	{cubeVertex6,XMFLOAT2(0.0,1.0)},
	{cubeVertex2,XMFLOAT2(1.0,1.0)},
	
	{cubeVertex4,XMFLOAT2(0.0,0.0)},
	{cubeVertex5,XMFLOAT2(1.0,0.0)},
	{cubeVertex0,XMFLOAT2(0.0,1.0)},
	{cubeVertex1,XMFLOAT2(1.0,1.0)},
	
	{cubeVertex1,XMFLOAT2(0.0,0.0)},
	{cubeVertex5,XMFLOAT2(1.0,0.0)},
	{cubeVertex3,XMFLOAT2(0.0,1.0)},
	{cubeVertex7,XMFLOAT2(1.0,1.0)},
	
	{cubeVertex7,XMFLOAT2(0.0,0.0)},
	{cubeVertex6,XMFLOAT2(1.0,0.0)},
	{cubeVertex3,XMFLOAT2(0.0,1.0)},
	{cubeVertex2,XMFLOAT2(1.0,1.0)},
	
	{cubeVertex5,XMFLOAT2(0.0,0.0)},
	{cubeVertex4,XMFLOAT2(1.0,0.0)},
	{cubeVertex7,XMFLOAT2(0.0,1.0)},
	{cubeVertex6,XMFLOAT2(1.0,1.0)},
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
	ID3D11Buffer* triangleIndexBuffer;
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

	vertexBufferDesc.ByteWidth = sizeof(vertex);	//看看能不能换成sizeof(vertex)或是 sizeof(Vertex) * ARRAYSIZE(vertex)。结论：不能！因为你可以自己设定一次刷新的内容，不一定要是完整的
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

	hr = d3dDevice->CreateBuffer(&indexBufferDesc,&indexData,&triangleIndexBuffer);
	HR(hr);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0,1,&triangleVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(triangleIndexBuffer,DXGI_FORMAT_R32_UINT,0);

	hr = d3dDevice->CreateInputLayout(verDesc,ARRAYSIZE(verDesc),VS_Buffer->GetBufferPointer(),VS_Buffer->GetBufferSize(),&inputLayout);
	HR(hr);

	d3dDeviceContext->IASetInputLayout(inputLayout);
	
	d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	
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
	rasterStateDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterStateDesc.CullMode = D3D11_CULL_BACK;
	d3dDevice->CreateRasterizerState(&rasterStateDesc,&rasterState_1);
	d3dDeviceContext->RSSetState(rasterState_1);

//常量缓存部分
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

//纹理部分
	HR(D3DX11CreateShaderResourceViewFromFile(d3dDevice,L"braynzar.jpg",NULL,NULL,&shaderResourceView,NULL));
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc,sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	//可以试一下换别的选项看看效果
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;	//可以试一下换别的选项看看效果
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;	//可以试一下换别的选项看看效果
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;	//可以试一下换别的选项看看效果
	//samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; //可以试着修改一下？
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(d3dDevice->CreateSamplerState(&samplerDesc,&samplerState));

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

	rot1 += .0002f;
	if(rot1 > 12.76f)rot1 = 0.1f;
	eyePos.f[0]=eyePos.f[2]=-rot1;

	rot2 += .0002f;
	if(rot2 > 3.1415 * 2) rot2 = 0.0f;

}
void DrawScene()
{
	d3dDeviceContext->ClearRenderTargetView(renderTargetView,colorRGBA);
	d3dDeviceContext->ClearDepthStencilView(depthStencilView,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
	//XMMATRIX ractangle_1 = XMMatrixTranslation(2.0f,0.0f,2.0f);
	XMMATRIX ractangle_1 = XMMatrixRotationAxis(XMVectorSet(0,1,0,0),rot2) * XMMatrixTranslation(2,0,0);
	
	viewSpace = XMMatrixLookAtLH(eyePos,focusPos,upPos);
	
	//d3dDeviceContext->RSSetState(rasterState_1);
	constBufferStruct.WVP = XMMatrixTranspose(worldSpace * viewSpace * positionMatrix);
	d3dDeviceContext->UpdateSubresource(constBuffer,0,NULL,&constBufferStruct,0,0);
	
	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView);
	d3dDeviceContext->PSSetSamplers(0,1,&samplerState);

	d3dDeviceContext->DrawIndexed(36,0,0);
	
	//d3dDeviceContext->RSSetState(0);
	constBufferStruct.WVP = XMMatrixTranspose(worldSpace * ractangle_1 * viewSpace * positionMatrix);
	d3dDeviceContext->UpdateSubresource(constBuffer,0,NULL,&constBufferStruct,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);
	d3dSwapChain->Present(0,0);
}
