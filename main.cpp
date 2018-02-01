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
#include <vector>
#include <algorithm>
///////////////**************dx_input**************////////////////////
#include <D3D10_1.h>
#include <DXGI.h>
#include <D2D1.h>
#include <sstream>
#include <dwrite.h>
///////////////**************dx_input**************////////////////////
#include <dinput.h>
///////////////**************load_model**************////////////////////
#include <fstream>

#ifdef _DEBUG
#include <comdef.h>
#endif

/*
the portion that can be improved
1.don't forget the LoadObjModel() function still can't load the polygon more than 3 edges
2.set the article struct that be used for store model data,collider data,world space,world position after calculated and so on,because world position would calculated repeatedly
3.collider should be a individual struct,can't be the derived class of the modeldata struct,if some article intends to detect collider,it must have a collider struct
*/

#define SCREEN_SIZE 2
#if SCREEN_SIZE == 1
#define WIDTH 100
#define HEIGHT 100 
#define POS_X 1200 
#define POS_Y 700
#else
#if SCREEN_SIZE == 2
#define WIDTH 500
#define HEIGHT 500 
#define POS_X 600 
#define POS_Y 200
#else
#if SCREEN_SIZE == 3
#define WIDTH 1800
#define HEIGHT 1000 
#define POS_X 10 
#define POS_Y 10
#endif
#endif
#endif

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

struct SurfaceMaterial	//group
{
    std::wstring matName;	//usemtl 的名字
    XMFLOAT4 difColor;	//mtl文件中的 ka/kd 项和 d/tr 项的结合
    int texArrayIndex;	//VertexMsgObjIndex的vector的index，从0开始
	int indexCount;	//group中vertex的数量
    bool hasTexture;	//mtl文件中是否使用texture
    bool isTransparent;	//mtl文件中是否需要blend
	bool hasNormalMap;	//mtl文件中是否有法线贴图
	ID3D11ShaderResourceView * shaderResourceView;
	ID3D11ShaderResourceView * normalMapResourceView;
};
struct VertexMsgObjIndex	//从1开始，0表示没有该值
{
	DWORD verIdx;
	DWORD texCoorIdx;
	DWORD normalIdx;
};

struct MaterialMsg	//.mtl文件的mtl数据
{
	std::wstring mtlName;	//newmtl 的名字
	XMFLOAT3 ka;
	XMFLOAT3 kd;
	XMFLOAT3 ks;
	float transparent;	//透明度，0-1，1是完全不透明
	bool hasTexture;	//是否使用texture
	bool isTransparent;	//是否需要blend
	bool hasNormalMap;	//是否有法线贴图
	ID3D11ShaderResourceView * shaderResourceView;
	ID3D11ShaderResourceView * normalMapResourceView;
};

struct Vertex
{
	Vertex(XMFLOAT3 pos = XMFLOAT3(),XMFLOAT2 texCoord = XMFLOAT2(),XMFLOAT3 nor = XMFLOAT3(),XMFLOAT3 tng = XMFLOAT3()):
		position(pos),
		textureCoordinate(texCoord),
		normal(nor),
		tangent(tng)
	{}
	XMFLOAT3 position;
	XMFLOAT2 textureCoordinate;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	int weightStart;
	int weightNum;
};
struct ConstSpace
{
	XMMATRIX WVP;
	XMMATRIX worldSpace;
	XMFLOAT4 difColor;
	BOOL hasTexture;
	BOOL hasNormalMap;
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

#define MODEL_NULL 0
#define MODEL_ISINIT 1
#define MODEL_COLLIDER 2
struct ModelData
{
	ModelData()
	{
		type = MODEL_NULL;
		modelVertexBuffer = NULL;
		modelIndexBuffer = NULL;
	}
	ID3D11Buffer *modelVertexBuffer;
	ID3D11Buffer *modelIndexBuffer;
	std::vector<SurfaceMaterial> modelSurMetVec;	//group信息
	std::vector<Vertex> vertexVec;	//生成的vertex序列
	std::vector<DWORD> indexVec;	//vertMsgVec里面的vertex信息对应的index序列（其实也是最终的用来构造index的序列）
	int type;
	std::vector<DWORD> indexNoRepeat;
	void makeIndexNoRepeat()
	{
		for(int i = 0,iLen = indexVec.size();i < iLen;i++)
		{
			if(std::find(indexNoRepeat.begin(),indexNoRepeat.end(),indexVec[i]) != indexNoRepeat.end())
			{
				indexNoRepeat.push_back(indexVec[i]);
			}
		}
	}
};

struct ModelColliderData:public ModelData
{
	XMVECTORF32 modelCentrePoint;
	float centreRadius;
	//后面还可以加group来区分leg，arm等，还能加对应的权重比
};

struct MD5meshJointData
{
	std::wstring name;
	int parentIndex;
	XMFLOAT3 jointPos;
	XMFLOAT4 jointQuat;
};

struct MD5meshWeightData
{
	int jointIndex;
	float weightValue;
	XMFLOAT3 position;
};

struct MD5meshMeshData
{
	std::wstring shaderName;
	int numverts;
	int numtris;	//the number of triangle,it's one third the number of index
	int numweights;
	ID3D11Buffer *md5meshVertexBuffer;
	ID3D11Buffer *md5meshIndexBuffer;
	std::vector<Vertex> vertexData;
	std::vector<DWORD> indexData;
	std::vector<MD5meshWeightData> weightData;
	ID3D11ShaderResourceView * shaderResourceView;
};

struct MD5meshData
{
	std::vector<MD5meshJointData> jointList;
	std::vector<MD5meshMeshData> meshList;
};

HWND hwnd;
int ClientWidth = 0;
int ClientHeight = 0;

IDXGISwapChain * d3dSwapChain;
ID3D11Device * d3dDevice;
ID3D11DeviceContext  * d3dDeviceContext;
ID3D11RenderTargetView * renderTargetView;
ID3D11DepthStencilView * depthStencilView;
ID3D11RasterizerState * rasterState_cw;
ID3D11RasterizerState * rasterState_acw;
ID3D11RasterizerState * rasterState_cwnc;
ID3D11RasterizerState * rasterState_cw_bias;
ID3D11RasterizerState * rasterState_cwnc_bias;
ID3D11Texture2D *depthStencilTexture;
ID3D11Buffer* squareVertBuffer;
ID3D11Buffer* squareIndexBuffer;
XMMATRIX worldSpace;
XMMATRIX viewSpace;
XMMATRIX projectionMatrix;
XMMATRIX inverseViewSpace;
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
ID3D11SamplerState * samplerState[2];	//1、平铺	2、不平铺
ID3D11BlendState * blendState;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
XMVECTORF32 eyePos = {-1.0f,2.0f,-1.0f,1.0f};
XMVECTORF32 focusPos = {0.0f,1.0f,0.0f,1.0f};
XMVECTORF32 upPos = {0.0f,1.0f,0.0f,1.0f};
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
bool isMouseClicked = false;
bool isMousePressed  = false;

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

//model
struct ModelData modelHouse;
struct ModelData modelGround;
struct ModelData modelBottle;
struct ModelColliderData colliderBottle;
struct MD5meshData md5Boy;
XMMATRIX scaleBoy = XMMatrixScaling( 0.01f, 0.01f, 0.01f );			// The model is a bit too large for our scene, so make it smaller
XMMATRIX translationBoy = XMMatrixTranslation( -2.0f, 3.0f, 1.5f );

//raycast(world space)
XMVECTORF32 rayPointEye;
XMVECTORF32 rayPointDir; 

//time
double timeFrequency = 0.0;	//count
double startTime = 0.0;	//count
double lastTime = 0.0;	//time
int frameCount = 0;
int fps = 0;

//FLOAT colorRGBA[4] = {0.0,1.0,0.0,1.0};	//纯绿
FLOAT colorRGBA[4] = {0.0f,0.0f,0.0f,1.0f};	//纯黑
FLOAT rot1 = 0.0f;
FLOAT rot2 = 0.0f;

XMFLOAT3 cubeVertex0 = XMFLOAT3(-0.5f,0.5f,-0.5f);
XMFLOAT3 cubeVertex1 = XMFLOAT3(0.5f,0.5f,-0.5f);
XMFLOAT3 cubeVertex2 = XMFLOAT3(-0.5f,-0.5f,-0.5f);
XMFLOAT3 cubeVertex3 = XMFLOAT3(0.5f,-0.5f,-0.5f);
XMFLOAT3 cubeVertex4 = XMFLOAT3(-0.5f,0.5f,0.5f);
XMFLOAT3 cubeVertex5 = XMFLOAT3(0.5f,0.5f,0.5f);
XMFLOAT3 cubeVertex6 = XMFLOAT3(-0.5f,-0.5f,0.5f);
XMFLOAT3 cubeVertex7 = XMFLOAT3(0.5f,-0.5f,0.5f);
#define CONTEXT_PIC_NUM 1.0f
XMFLOAT2 leftUp = XMFLOAT2(0.0f,0.0f);
XMFLOAT2 rightUp = XMFLOAT2(CONTEXT_PIC_NUM,0.0f);
XMFLOAT2 leftDown = XMFLOAT2(0.0f,CONTEXT_PIC_NUM);
XMFLOAT2 rightDown = XMFLOAT2(CONTEXT_PIC_NUM,CONTEXT_PIC_NUM);
#define SKYBOX_SIZE 1.0f
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
	Vertex(cubeVertex0,leftUp,cubeVertex0},
	Vertex(cubeVertex1,rightUp,cubeVertex1},
	Vertex(cubeVertex2,leftDown,cubeVertex2},
	Vertex(cubeVertex3,rightDown,cubeVertex3},
	
	Vertex(cubeVertex4,leftUp,cubeVertex4},
	Vertex(cubeVertex0,rightUp,cubeVertex0},
	Vertex(cubeVertex6,leftDown,cubeVertex6},
	Vertex(cubeVertex2,rightDown,cubeVertex2},
	
	Vertex(cubeVertex4,leftUp,cubeVertex4},
	Vertex(cubeVertex5,rightUp,cubeVertex5},
	Vertex(cubeVertex0,leftDown,cubeVertex0},
	Vertex(cubeVertex1,rightDown,cubeVertex1},
	
	Vertex(cubeVertex1,leftUp,cubeVertex1},
	Vertex(cubeVertex5,rightUp,cubeVertex5},
	Vertex(cubeVertex3,leftDown,cubeVertex3},
	Vertex(cubeVertex7,rightDown,cubeVertex7},
	
	Vertex(cubeVertex7,leftUp,cubeVertex7},
	Vertex(cubeVertex6,rightUp,cubeVertex6},
	Vertex(cubeVertex3,leftDown,cubeVertex3},
	Vertex(cubeVertex2,rightDown,cubeVertex2},
	
	Vertex(cubeVertex5,leftUp,cubeVertex5},
	Vertex(cubeVertex4,rightUp,cubeVertex4},
	Vertex(cubeVertex7,leftDown,cubeVertex7},
	Vertex(cubeVertex6,rightDown,cubeVertex6},
		
	Vertex(XMFLOAT3(-10.0f,0.0f,10.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f)),
	Vertex(XMFLOAT3(10.0f,0.0f,10.0f),XMFLOAT2(10.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f)),
	Vertex(XMFLOAT3(-10.0f,0.0f,-10.0f),XMFLOAT2(0.0f,10.0f),XMFLOAT3(0.0f,1.0f,0.0f)),
	Vertex(XMFLOAT3(10.0f,0.0f,-10.0f),XMFLOAT2(10.0f,10.0f),XMFLOAT3(0.0f,1.0f,0.0f)),
#endif
#ifdef LIGHT_TYPE_PLANE_NORMAL
//平面法向量
	//two boxes
	Vertex(cubeVertex0,leftUp,XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex1,rightUp,XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex2,leftDown,XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex3,rightDown,XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	
	Vertex(cubeVertex4,leftUp,XMFLOAT3(-1.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex0,rightUp,XMFLOAT3(-1.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex6,leftDown,XMFLOAT3(-1.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex2,rightDown,XMFLOAT3(-1.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	
	Vertex(cubeVertex4,leftUp,XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(cubeVertex5,rightUp,XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(cubeVertex0,leftDown,XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(cubeVertex1,rightDown,XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	
	Vertex(cubeVertex1,leftUp,XMFLOAT3(1.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex5,rightUp,XMFLOAT3(1.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex3,leftDown,XMFLOAT3(1.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex7,rightDown,XMFLOAT3(1.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	
	Vertex(cubeVertex7,leftUp,XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(cubeVertex6,rightUp,XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(cubeVertex3,leftDown,XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(cubeVertex2,rightDown,XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	
	Vertex(cubeVertex5,leftUp,XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex4,rightUp,XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex7,leftDown,XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),
	Vertex(cubeVertex6,rightDown,XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT3(0.0f,-1.0f,0.0f)),

	//grass
#define PLAIN_SIZE 100.0f		
	Vertex(XMFLOAT3(-PLAIN_SIZE,-1.0f,PLAIN_SIZE),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(XMFLOAT3(PLAIN_SIZE,-1.0f,PLAIN_SIZE),XMFLOAT2(PLAIN_SIZE,0.0f),XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(XMFLOAT3(-PLAIN_SIZE,-1.0f,-PLAIN_SIZE),XMFLOAT2(0.0f,PLAIN_SIZE),XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
	Vertex(XMFLOAT3(PLAIN_SIZE,-1.0f,-PLAIN_SIZE),XMFLOAT2(PLAIN_SIZE,PLAIN_SIZE),XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f)),
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

D3D11_INPUT_ELEMENT_DESC verDesc[4] = {
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
	{"TEXTURE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
	{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D11_INPUT_PER_VERTEX_DATA,0},
	{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0}
};

void D2D_init(IDXGIAdapter1 *Adapter);
void IAInitText();

bool InitDirectInput(HINSTANCE hInstance);
void DetectInput(double time);

void SkyBoxInit();

bool LoadObjModel(std::wstring filename,struct ModelData *modelData,bool isRHCoord,bool isCalcNormal);
bool LoadMD5Model(std::wstring filename,struct MD5meshData *md5meshData,bool isRHCoord,bool isCalcNormal);
void makeCollider(ModelData &sourceModelData,ModelColliderData &destColliderData);
void DrawModelNonBlend(struct ModelData *modelData,CXMMATRIX worldSpace,CXMMATRIX viewSpace,bool isBias);	//isBias was set to true when ModelData might be covered
void DrawModelBlend(struct ModelData *modelData,CXMMATRIX worldSpace,CXMMATRIX viewSpace,bool isBias);	//isBias was set to true when ModelData might be covered
void DrawMD5mesh(struct MD5meshData *md5meshData,CXMMATRIX worldSpace,CXMMATRIX viewSpace,bool isBias);	//isBias was set to true when MD5meshData might be covered

void DrawBottle(bool isBlend);

#define DETECT_METHOD_MODEL 0
#define DETECT_METHOD_BOUNDING_SPHERE 1
#define DETECT_METHOD_MODEL_AND_BOUNDING_SPHERE 2
void UpdateScene(double currentFrameTime);
void GetRayCast();
bool MouseHitDetect(ModelData & modelData,CXMMATRIX worldSpaceTemp,int method);
bool ColliderDetect(ModelData & srcModel,ModelData & dstModel,CXMMATRIX srcWorldSpace,CXMMATRIX dstWorldSpace,int method);
float TriangleHitDetect(XMFLOAT3 rayPointSrc,XMFLOAT3 rayPointDst,XMFLOAT3 point1,XMFLOAT3 point2,XMFLOAT3 point3);
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
	case WM_SIZE:
        ClientWidth  = LOWORD(lParam);
        ClientHeight = HIWORD(lParam);
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
			GetRayCast();
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
	d3dSwapChainDesc.Windowed = true;	//试一下直接改成false？
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

	
	hr = D3D11CreateDeviceAndSwapChain(Adapter,D3D_DRIVER_TYPE_UNKNOWN,NULL,D3D11_CREATE_DEVICE_BGRA_SUPPORT,NULL,NULL,D3D11_SDK_VERSION,&d3dSwapChainDesc,&d3dSwapChain,&d3dDevice,NULL,&d3dDeviceContext);	//后面试一下把adapter换回NULL会怎么样
	//hr = D3D11CreateDeviceAndSwapChain(Adapter,D3D_DRIVER_TYPE_UNKNOWN,NULL,D3D11_CREATE_DEVICE_DEBUG|D3D11_CREATE_DEVICE_BGRA_SUPPORT,NULL,NULL,D3D11_SDK_VERSION,&d3dSwapChainDesc,&d3dSwapChain,&d3dDevice,NULL,&d3dDeviceContext);	//debug会减慢速度，难怪帧率一直这么低。。
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

bool LoadObjModel(std::wstring filename,struct ModelData *modelData,bool isRHCoord,bool isCalcNormal)
{
	std::vector<VertexMsgObjIndex> vertMsgVec;	//obj文件里 f 字段的组合结构体以不重复的形式保存的vertex信息序列
	std::vector<XMFLOAT3> posVec;	//obj文件对应排列下来的vertex信息
	std::vector<XMFLOAT2> texCooVec;	//obj文件对应排列下来的texture coordinate信息
	std::vector<XMFLOAT3> normalVec;	//obj文件对应排列下来的normal信息

	std::wifstream modelFile(filename);
	std::vector<std::wstring> mtlLibVec;	//obj文件里引用的lib
	std::vector<MaterialMsg> mtlVec;	//mtl里的每一个不重复的mtl的属性结构体

	int indexIndex = 0;	//记录当前提取到了第几个 f (index)
	int groupIndex = 0;	//记录当前提取到了第几个 g (group)

	std::wstring wstrTemp;
	SurfaceMaterial surMetTemp;
	VertexMsgObjIndex verMsgObjTemp;
	MaterialMsg mtlMsgTemp;
	Vertex vertexTemp;
	DWORD verTemp;
	DWORD texCoorTemp;
	DWORD normalTemp;
	bool flagTemp;
	float floatTemp;

	if(modelFile)
	{
		wchar_t keyChar;
		while(modelFile)
		{
			keyChar = modelFile.get();
			switch(keyChar)
			{
			case '#':
				while(modelFile.get() != '\n' && modelFile);
				break;
			case 'v':
				keyChar = modelFile.get();
				switch(keyChar)
				{
				case ' ':	//vertex
					{
						float vx,vy,vz;
						modelFile >> vx >> vy >> vz;
						if(isRHCoord)
							posVec.push_back(XMFLOAT3(vx,vy,vz * -1.0f));
						else
							posVec.push_back(XMFLOAT3(vx,vy,vz));
						break;
					}
				case 't':	//texture coord
					{
						float vx,vy,vz;
						modelFile >> vx >> vy >> vz;
						if(isRHCoord)
							texCooVec.push_back(XMFLOAT2(vx,1.0f - vy));
						else
							texCooVec.push_back(XMFLOAT2(vx,vy));
						break;
					}
					break;
				case 'n':	//normal
					{
						float vx,vy,vz;
						modelFile >> vx >> vy >> vz;
						if(isRHCoord)
							normalVec.push_back(XMFLOAT3(vx,vy,vz * -1.0f));
						else
							normalVec.push_back(XMFLOAT3(vx,vy,vz));
						break;
					}
					break;
				}
				while(modelFile.get() != '\n' && modelFile);
				break;
			case 'g':
				if(modelData->modelSurMetVec.size() > 0)
					surMetTemp.matName == modelData->modelSurMetVec.back().matName;	//这里表示默认用上面的除非有usemtl（其实好像直接不赋值就已经是上一个了？）
				else
					surMetTemp.matName == L"";
				surMetTemp.texArrayIndex = indexIndex;
				if(modelData->modelSurMetVec.size() != 0)
					modelData->modelSurMetVec.back().indexCount = indexIndex - modelData->modelSurMetVec.back().texArrayIndex;
				surMetTemp.isTransparent = false;
				surMetTemp.hasNormalMap = false;
				surMetTemp.hasTexture = false;
				surMetTemp.normalMapResourceView = NULL;
				surMetTemp.shaderResourceView = NULL;
				surMetTemp.difColor = XMFLOAT4(0.0f,0.0f,0.0f,0.0f);
				modelData->modelSurMetVec.push_back(surMetTemp);
				groupIndex++;
				while(modelFile.get() != '\n' && modelFile);
				break;
			case 'u':	//usemtl
				if(modelFile.get() == 's')
				{
					if(modelFile.get() == 'e')
					{
						if(modelFile.get() == 'm')
						{
							if(modelFile.get() == 't')
							{
								if(modelFile.get() == 'l')
								{
									if(modelFile.get() == ' ')
									{
										modelFile >> wstrTemp; //Get next type (string)
										modelData->modelSurMetVec.back().matName = wstrTemp;
									}
								}
							}
						}
					}
				}
				while(modelFile.get() != '\n' && modelFile);
				break;
			case 'm':	//mtllib
				if(modelFile.get() == 't')
				{
					if(modelFile.get() == 'l')
					{
						if(modelFile.get() == 'l')
						{
							if(modelFile.get() == 'i')
							{
								if(modelFile.get() == 'b')
								{
									if(modelFile.get() == ' ')
									{
										modelFile >> wstrTemp; //Get next type (string)
										flagTemp = false;	//是否存在已有的mtl
										for(int i = 0,lenTemp = mtlLibVec.size();i < lenTemp;i++)
										{
											if(mtlLibVec[i] == wstrTemp)
											{
												flagTemp = true;
												break;
											}
										}
										if(flagTemp == false)
										{
											mtlLibVec.push_back(wstrTemp);
										}
									}
								}
							}
						}
					}
				}
				while(modelFile.get() != '\n' && modelFile);
				break;
			case 'f':	//暂不考虑超过3个的情况
				if(indexIndex == 0 && modelData->modelSurMetVec.empty())	//如果开始没有group，则这一部分应该也是要作为一个group的
				{	
					surMetTemp.matName = L"";
					surMetTemp.texArrayIndex = 0;
					surMetTemp.isTransparent = false;
					surMetTemp.hasNormalMap = false;
					surMetTemp.hasTexture = false;
					surMetTemp.shaderResourceView = NULL;
					surMetTemp.normalMapResourceView = NULL;
					modelData->modelSurMetVec.push_back(surMetTemp);
					groupIndex++;
				}
				for(int i = 0;i < 3;i++)	//循环获取3个vertex的属性
				{
					while((keyChar = modelFile.get()) == ' ');	//过滤开始的空格
					wstrTemp = L"";
					do
					{
						wstrTemp += keyChar;
						keyChar = modelFile.get();
					}
					while(keyChar != ' ');	//获取vertex属性存到字符串中
					verTemp = 0;
					texCoorTemp = 0;
					normalTemp = 0;
					std::wstringstream sstreamTemp(wstrTemp);
					sstreamTemp >> verTemp;
					for(unsigned i = 0,whichValue = 0,lenTemp = wstrTemp.length();i < lenTemp;i++)	//构造vertex字符串流并在解析后存到3个变量中
					{
						if(wstrTemp.c_str()[i] == '/')
						{
							sstreamTemp.get();
							whichValue++;
							if(i < wstrTemp.length() - 1 && wstrTemp.c_str()[i+1] >= '0' && wstrTemp.c_str()[i+1] <= '9' && whichValue == 1)
							{
								sstreamTemp >> texCoorTemp;
							}
							if(i < wstrTemp.length() - 1 && wstrTemp.c_str()[i+1] >= '0' && wstrTemp.c_str()[i+1] <= '9' && whichValue == 2)
							{
								sstreamTemp >> normalTemp;
							}
						}
					}
					flagTemp = false;
					if(!isCalcNormal)
					{
						for(DWORD i = 0,lenTemp = vertMsgVec.size();i < lenTemp;i++)	//判断是否已经有了一样的vertex，有的话就直接将对应index赋值过去
						{
							if(vertMsgVec[i].verIdx == verTemp && vertMsgVec[i].texCoorIdx == texCoorTemp && vertMsgVec[i].normalIdx == normalTemp)
							{
								flagTemp = true;
								modelData->indexVec.push_back(i);
								break;
							}
						}
						if(flagTemp == false)	//没有的话就创建一个VertexMsgObjIndex结构体并添加新数据，然后将对应index赋值过去
						{
							verMsgObjTemp.verIdx = verTemp;
							verMsgObjTemp.texCoorIdx = texCoorTemp;
							verMsgObjTemp.normalIdx = normalTemp;
							vertMsgVec.push_back(verMsgObjTemp);
							modelData->indexVec.push_back(vertMsgVec.size()-1);
						}
					}
					else
					{
						for(DWORD i = 0,lenTemp = vertMsgVec.size();i < lenTemp;i++)
						{
							if(vertMsgVec[i].verIdx == verTemp && vertMsgVec[i].texCoorIdx == texCoorTemp)
							{
								flagTemp = true;
								modelData->indexVec.push_back(i);
								break;
							}
						}
						if(flagTemp == false)
						{
							verMsgObjTemp.verIdx = verTemp;
							verMsgObjTemp.texCoorIdx = texCoorTemp;
							verMsgObjTemp.normalIdx = 0;
							vertMsgVec.push_back(verMsgObjTemp);
							modelData->indexVec.push_back(vertMsgVec.size()-1);
						}
					}
					indexIndex++;
				}
				while(modelFile.get() != '\n' && modelFile);
				break;
			case 's':
				while(modelFile.get() != '\n' && modelFile);
				break;
			case 'o':
				while(modelFile.get() != '\n' && modelFile);
				break;
			}
		}
		modelData->modelSurMetVec.back().indexCount = indexIndex - modelData->modelSurMetVec.back().texArrayIndex;

		//循环读取mtl文件
		for(unsigned i = 0,len = mtlLibVec.size();i < len;i++)
		{
			std::wifstream mtlFile(mtlLibVec[i]);
			if(mtlFile)
			{
				wchar_t keyChar;
				while(mtlFile)
				{
					keyChar = mtlFile.get();
					switch(keyChar)
					{
					case 'n':
						if(mtlFile.get() == 'e')
						{
							if(mtlFile.get() == 'w')
							{
								if(mtlFile.get() == 'm')
								{
									if(mtlFile.get() == 't')
									{
										if(mtlFile.get() == 'l')
										{
											if(mtlFile.get() == ' ')
											{ 
												mtlFile >> mtlMsgTemp.mtlName;
												flagTemp = true;
												for(int i = 0,len = mtlVec.size();i < len;i++)
												{
													if(mtlVec[i].mtlName == mtlMsgTemp.mtlName)
													{
														flagTemp = false;
														break;
													}
												}
												if(flagTemp == true)
												{
													mtlMsgTemp.ka = XMFLOAT3(0.0f,0.0f,0.0f);
													mtlMsgTemp.kd = XMFLOAT3(0.0f,0.0f,0.0f);
													mtlMsgTemp.ks = XMFLOAT3(0.0f,0.0f,0.0f);
													mtlMsgTemp.shaderResourceView = NULL;
													mtlMsgTemp.normalMapResourceView = NULL;
													mtlMsgTemp.transparent = 0.0f;
													mtlMsgTemp.isTransparent = false;
													mtlMsgTemp.hasNormalMap = false;
													mtlMsgTemp.hasTexture = false;
													mtlVec.push_back(mtlMsgTemp);
												}
											}
										}
									}
								}
							}
						}
						while(mtlFile.get() != '\n' && mtlFile);
						break;
					case 'K':
						switch(mtlFile.get())
						{
						case 'a':
							mtlFile >> mtlVec.back().ka.x;
							mtlFile >> mtlVec.back().ka.y;
							mtlFile >> mtlVec.back().ka.z;
							break;
						case 'd':
							mtlFile >> mtlVec.back().ka.x;
							mtlFile >> mtlVec.back().ka.y;
							mtlFile >> mtlVec.back().ka.z;
							mtlVec.back().kd.x = mtlVec.back().ka.x;
							mtlVec.back().kd.y = mtlVec.back().ka.y;
							mtlVec.back().kd.z = mtlVec.back().ka.z;
							break;
						case 's':
							mtlFile >> mtlVec.back().ks.x;
							mtlFile >> mtlVec.back().ks.y;
							mtlFile >> mtlVec.back().ks.z;
							break;
						default:
							break;
						}
						while(mtlFile.get() != '\n' && mtlFile);
						break;
					case 'd':
						mtlFile >> floatTemp;
						mtlVec.back().transparent = floatTemp;
						if(floatTemp < 1.0f)
						{
							mtlVec.back().isTransparent = true;
						}
					case 'T':
						switch(mtlFile.get())
						{
						case 'r':
							mtlFile >> floatTemp;
							mtlVec.back().transparent = 1.0f - floatTemp;
							if(floatTemp > 0.0f)
							{
								mtlVec.back().isTransparent = true;
							}
							break;
						case 'f':
							break;
						default:
							break;
						}
						while(mtlFile.get() != '\n' && mtlFile);
						break;
					case 'm':
						if(mtlFile.get() == 'a')
						{
							if(mtlFile.get() == 'p')
							{
								if(mtlFile.get() == '_')
								{
									keyChar = mtlFile.get();
									if(keyChar == 'K')
									{
										keyChar = mtlFile.get();
										if(keyChar == 'a')
										{
											mtlFile >> wstrTemp;
											if(!FAILED(D3DX11CreateShaderResourceViewFromFile(d3dDevice,wstrTemp.c_str(),NULL,NULL,&(mtlVec.back().shaderResourceView),NULL)))
											{
												mtlVec.back().hasTexture = true;
											}
										}
										else if(keyChar == 'd')
										{
											mtlFile >> wstrTemp;
											if(!FAILED(D3DX11CreateShaderResourceViewFromFile(d3dDevice,wstrTemp.c_str(),NULL,NULL,&(mtlVec.back().shaderResourceView),NULL)))
											{
												mtlVec.back().hasTexture = true;
											}
										}
									}
									else if(keyChar == 'b')
									{
										if(mtlFile.get() == 'u')
										{
											if(mtlFile.get() == 'm')
											{
												if(mtlFile.get() == 'p')
												{
													mtlFile >> wstrTemp;
													if(!FAILED(D3DX11CreateShaderResourceViewFromFile(d3dDevice,wstrTemp.c_str(),NULL,NULL,&(mtlVec.back().normalMapResourceView),NULL)))
													{
														mtlVec.back().hasNormalMap = true;
													}
												}
											}
										}
									}
									else if(keyChar == 'd')
									{
										//map_d
									}
								}
							}
						}
						while(mtlFile.get() != '\n' && mtlFile);
						break;
					case 'N':
						while(mtlFile.get() != '\n' && mtlFile);
						break;
					case 'i':
						while(mtlFile.get() != '\n' && mtlFile);
						break;
					case ' ':
					case '\t':
					case '\n':
						break;
					default:
						while(mtlFile.get() != '\n' && mtlFile);
						break;
					}
				}
			}
		}

		//group根据名字获取对应netmtl的数据
		for(int i = 0,len_surM = modelData->modelSurMetVec.size();i < len_surM;i++)
		{
			flagTemp = false;
			for(int j = 0,len_mat = mtlVec.size();j < len_mat;j++)
			{
				if(modelData->modelSurMetVec[i].matName == mtlVec[j].mtlName)
				{
					modelData->modelSurMetVec[i].hasTexture = mtlVec[j].hasTexture;
					modelData->modelSurMetVec[i].isTransparent = mtlVec[j].isTransparent;
					modelData->modelSurMetVec[i].hasNormalMap = mtlVec[j].hasNormalMap;
					modelData->modelSurMetVec[i].shaderResourceView = mtlVec[j].shaderResourceView;
					modelData->modelSurMetVec[i].normalMapResourceView = mtlVec[j].normalMapResourceView;
					modelData->modelSurMetVec[i].difColor = XMFLOAT4(mtlVec[j].ka.x,mtlVec[j].ka.y,mtlVec[j].ka.z,mtlVec[j].transparent);
					flagTemp = true;
					break;
				}
			}
			if(flagTemp == false && mtlVec.size() > 0)	//例子上写的是如果找不到匹配的话就默认用第一个（我先照着这么做。。。）
			{
				modelData->modelSurMetVec[i].hasTexture = mtlVec[0].hasTexture;
				modelData->modelSurMetVec[i].isTransparent = mtlVec[0].isTransparent;
				modelData->modelSurMetVec[i].hasNormalMap = mtlVec[0].hasNormalMap;
				modelData->modelSurMetVec[i].shaderResourceView = mtlVec[0].shaderResourceView;
				modelData->modelSurMetVec[i].normalMapResourceView = mtlVec[0].normalMapResourceView;
				modelData->modelSurMetVec[i].difColor = XMFLOAT4(mtlVec[0].ka.x,mtlVec[0].ka.y,mtlVec[0].ka.z,mtlVec[0].transparent);
			}
		}

		/*创建vertex数组，index数组，*/
		for(int i = 0,len = vertMsgVec.size();i < len;i++)
		{
			if(vertMsgVec[i].verIdx > 0)
			{
				vertexTemp.position = posVec[vertMsgVec[i].verIdx - 1];
			}

			if(vertMsgVec[i].texCoorIdx > 0)
			{
				vertexTemp.textureCoordinate = texCooVec[vertMsgVec[i].texCoorIdx - 1];
			}
			else
			{
				vertexTemp.textureCoordinate = XMFLOAT2(0.0f,0.0f);
			}

			if(vertMsgVec[i].normalIdx > 0)
			{
				vertexTemp.normal = normalVec[vertMsgVec[i].normalIdx - 1];
			}
			else
			{
				vertexTemp.normal = vertexTemp.position;
			}
			modelData->vertexVec.push_back(vertexTemp);
		}
		
		//calculate the normal(smooth vertex)
		if(isCalcNormal)
		{
			std::vector<XMFLOAT3> faceNormal;
			XMFLOAT3 normalTemp;
			XMVECTOR edge1,edge2;
			for(int i = 0,iLen = modelData->indexVec.size();i < iLen;i+=3)
			{
				edge1 = XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[i+1]].position)) - XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[i]].position));
				edge2 = XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[i+2]].position)) - XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[i]].position));
				XMStoreFloat3(&normalTemp,XMVector3Normalize(XMVector3Cross(edge1,edge2)));
				faceNormal.push_back(normalTemp);
			}
			int joinFaceNum;
			for(int i = 0,iLen = modelData->vertexVec.size();i < iLen;i++)
			{
				joinFaceNum = 0;
				normalTemp = XMFLOAT3(0.0f,0.0f,0.0f);
				for(int j = 0,k = 0,jLen = faceNormal.size();j < jLen;j++)
				{
					if(modelData->indexVec[j*3] == i || modelData->indexVec[j*3+1] == i || modelData->indexVec[j*3+2] == i)
					{
						joinFaceNum++;
						normalTemp.x += faceNormal[j].x;
						normalTemp.y += faceNormal[j].y;
						normalTemp.z += faceNormal[j].z;
					}
				}
				if(joinFaceNum > 0)
				{
					normalTemp.x /= joinFaceNum;
					normalTemp.y /= joinFaceNum;
					normalTemp.z /= joinFaceNum;
					modelData->vertexVec[i].normal = normalTemp;
				}
				else
				{
					modelData->vertexVec[i].normal = modelData->vertexVec[i].position;
				}
			}
		}

		/*计算tangent，如果原vertex已经算过了tangent，且和当前算好的不一样，则需要创建新的vertex，添加到队列的最后并修改对应的index*/
		std::vector<bool> isCalcTangent;
		float textCoorU1,textCoorU2;
		XMVECTOR wordPos1,wordPos2;
		XMVECTOR tangentTemp;
		for(int i = 0,len = modelData->vertexVec.size();i < len;i++)
		{
			isCalcTangent.push_back(false);
		}
		for(int i = 0,len = modelData->modelSurMetVec.size(),startArray;i < len;i++)
		{
			if(modelData->modelSurMetVec[i].hasNormalMap)
			{
				startArray = modelData->modelSurMetVec[i].texArrayIndex;
				for(int j = 0;j < modelData->modelSurMetVec[i].indexCount;j+=3)
				{	
					textCoorU1 = modelData->vertexVec[modelData->indexVec[startArray + j + 0]].textureCoordinate.x - modelData->vertexVec[modelData->indexVec[startArray + j + 1]].textureCoordinate.x;
					wordPos1 = XMVectorSubtract(XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[startArray + j + 0]].position)),XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[startArray + j + 1]].position)));
					textCoorU2 = modelData->vertexVec[modelData->indexVec[startArray + j + 0]].textureCoordinate.x - modelData->vertexVec[modelData->indexVec[startArray + j + 2]].textureCoordinate.x;
					wordPos2 = XMVectorSubtract(XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[startArray + j + 0]].position)),XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[startArray + j + 2]].position)));
					tangentTemp = XMVector3Normalize(XMVectorAdd(XMVectorScale(wordPos1,textCoorU2),XMVectorScale(wordPos2,-textCoorU1)));

					for(int k = 0;k < 3;k++)
					{
						if(!isCalcTangent[modelData->indexVec[startArray + j + k]])
						{
							XMStoreFloat3(&(modelData->vertexVec[modelData->indexVec[startArray + j + k]].tangent),tangentTemp);
							isCalcTangent[modelData->indexVec[startArray + j + k]] = true;
						}
						else if(!XMVector3Equal(XMLoadFloat3(&(modelData->vertexVec[modelData->indexVec[startArray + j + k]].tangent)),tangentTemp))
						{
							vertexTemp.position = modelData->vertexVec[modelData->indexVec[startArray + j + k]].position;
							vertexTemp.textureCoordinate = modelData->vertexVec[modelData->indexVec[startArray + j + k]].textureCoordinate;
							vertexTemp.normal = modelData->vertexVec[modelData->indexVec[startArray + j + k]].normal;
							XMStoreFloat3(&(vertexTemp.tangent),tangentTemp);
							modelData->vertexVec.push_back(vertexTemp);
							isCalcTangent.push_back(true);
							modelData->indexVec[startArray + j + k] = modelData->vertexVec.size() + 1;
						}
					}
				}
			}
		}
		modelData->makeIndexNoRepeat();

		HRESULT hr;
		D3D11_BUFFER_DESC vertexBufferDesc;
		D3D11_SUBRESOURCE_DATA vertexData;

		vertexBufferDesc.ByteWidth = sizeof(Vertex) * modelData->vertexVec.size();
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;
		vertexData.pSysMem = &(modelData->vertexVec[0]);
		vertexData.SysMemPitch = 0;
		vertexData.SysMemSlicePitch = 0;

		hr = d3dDevice->CreateBuffer(&vertexBufferDesc,&vertexData,&(modelData->modelVertexBuffer));
		HR(hr);

		D3D11_BUFFER_DESC indexBufferDesc;
		D3D11_SUBRESOURCE_DATA indexData;

		indexBufferDesc.ByteWidth = sizeof(DWORD) * modelData->indexVec.size();
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;
		indexData.pSysMem = &(modelData->indexVec[0]);
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		hr = d3dDevice->CreateBuffer(&indexBufferDesc,&indexData,&(modelData->modelIndexBuffer));
		HR(hr);
		modelData->type = MODEL_ISINIT;

		return true;
	}
	else
	{
		std::wstring errorString = L"open \"";
		errorString += filename;
		errorString += L"\" error!";
		MessageBox(hwnd,errorString.c_str(),L"error",MB_OK);
		return false;
	}
}


bool LoadMD5Model(std::wstring filename,struct MD5meshData *md5meshData,bool isRHCoord,bool isCalcNormal = true)
{
	std::wifstream md5File(filename);

	if(md5File)
	{
		std::wstring keyString;
		std::wstring commandline;
		wchar_t keyChar;
		int MD5Version;
		int numJoints;
		int numMeshes;
		MD5meshJointData jointTemp;
		MD5meshMeshData meshTemp;
		MD5meshWeightData weightTemp;
		Vertex vertexTemp;
		DWORD triIndexA,triIndexB,triIndexC;
		float floatNumTemp;
		XMVECTOR quanternionTemp;
		XMVECTOR vectorTemp;
		while(md5File)
		{
			md5File >> keyString;
			if(keyString == L"MD5Version")
			{
				md5File >> MD5Version;
			}
			else if(keyString == L"commandline")
			{
				while(md5File.get() != '"');
				keyString = L"";
				while((keyChar = md5File.get()) != '"')
				{
					keyString += keyChar;
				}
				commandline = keyString;
			}
			else if(keyString == L"numJoints")
			{
				md5File >> numJoints;
			}
			else if(keyString == L"numMeshes")
			{
				md5File >> numMeshes;
			}
			else if(keyString == L"joints")
			{
				md5File >> keyString;	//skip the "{"
				//query:Is it possible to have an annotated '//' symbol?	it is a possibility to map the '"' character in annotated if it has the annotate
				for(int i = 0;i < numJoints;i++)
				{
					while(md5File.get() != '"');
					keyString = L"";
					while((keyChar = md5File.get()) != '"')
					{
						keyString += keyChar;
					}
					jointTemp.name = keyString;
					md5File >> jointTemp.parentIndex;
					md5File >> keyString;	//skip the "("
					if(isRHCoord)
					{
						md5File >> jointTemp.jointPos.x >> jointTemp.jointPos.z >> jointTemp.jointPos.y;
					}
					else
					{
						md5File >> jointTemp.jointPos.x >> jointTemp.jointPos.y >> jointTemp.jointPos.z;
					}
					if(isRHCoord)
					{
						jointTemp.jointPos.z *= -1;
					}
					md5File >> keyString >> keyString;	//skip the "）" and "("
					if(isRHCoord)
					{
						md5File >> jointTemp.jointQuat.x >> jointTemp.jointQuat.z >> jointTemp.jointQuat.y;
					}
					else
					{
						md5File >> jointTemp.jointQuat.x >> jointTemp.jointQuat.y >> jointTemp.jointQuat.z;
					}
					//calculate the 'w' of quaternion
					floatNumTemp = 1.0f - jointTemp.jointQuat.x * jointTemp.jointQuat.x - jointTemp.jointQuat.y * jointTemp.jointQuat.y - jointTemp.jointQuat.z * jointTemp.jointQuat.z;
					if(floatNumTemp <= 0.0f)
					{
						jointTemp.jointQuat.w = 0.0f;
					}
					else
					{
						jointTemp.jointQuat.w = -sqrtf(floatNumTemp);	//query:why does it multiply the -1? answer:the manual said...
					}
					if(isRHCoord)
					{
						//??? how to calculate the quanternion in rh coordinate?
					}
					md5File >> keyString;	//skip the ")"
					std::getline(md5File,keyString);	//pass this line
					md5meshData->jointList.push_back(jointTemp);
				}
				while(md5File.get() != '}');	//skip the "}"
			}
			else if(keyString == L"mesh")
			{
				md5File >> keyString;	//skip the "{"
				while(md5File >> keyString && keyString != L"}")
				{
					if(keyString == L"shader")
					{
						while(md5File.get() != '"');
						keyString = L"";
						while((keyChar = md5File.get()) != '"')
						{
							keyString += keyChar;
						}
						meshTemp.shaderName = keyString;
						if(FAILED(D3DX11CreateShaderResourceViewFromFile(d3dDevice,keyString.c_str(),NULL,NULL,&(meshTemp.shaderResourceView),NULL)))
						{
							meshTemp.shaderResourceView = NULL;
						}
					}
					else if(keyString == L"numverts")
					{
						md5File >> meshTemp.numverts;
						meshTemp.vertexData.clear();
						for(int i = 0;i < meshTemp.numverts;i++)
						{
							md5File >> keyString >> keyString;	//skip the "vert" and "0"
							md5File >> keyString;	//skip the "("
							md5File >> vertexTemp.textureCoordinate.x >> vertexTemp.textureCoordinate.y;
							if(!isRHCoord)
							{
								vertexTemp.textureCoordinate.y = 1.0f - vertexTemp.textureCoordinate.y;
							}
							md5File >> keyString;	//skip the ")"
							md5File >> vertexTemp.weightStart >> vertexTemp.weightNum;
							meshTemp.vertexData.push_back(vertexTemp);
						}
					}
					else if(keyString == L"numtris")
					{
						md5File >> meshTemp.numtris;
						meshTemp.indexData.clear();
						for(int i = 0;i < meshTemp.numtris;i++)
						{
							md5File >> keyString >> keyString;	//skip the "tri" and "0"
							md5File >> triIndexA >> triIndexB >> triIndexC;
							meshTemp.indexData.push_back(triIndexA);
							meshTemp.indexData.push_back(triIndexB);
							meshTemp.indexData.push_back(triIndexC);
						}
					}
					else if(keyString == L"numweights")
					{
						md5File >> meshTemp.numweights;
						meshTemp.weightData.clear();
						for(int i = 0;i < meshTemp.numweights;i++)
						{
							md5File >> keyString >> keyString;	//skip the "weight" and "0"
							md5File >> weightTemp.jointIndex;
							md5File >> weightTemp.weightValue;
							md5File >> keyString;	//skip the "("
							if(isRHCoord)
							{
								md5File >> weightTemp.position.x >> weightTemp.position.z >> weightTemp.position.y;
							}
							else
							{
								md5File >> weightTemp.position.x >> weightTemp.position.y >> weightTemp.position.z;
							}
							md5File >> keyString;	//skip the ")"
							meshTemp.weightData.push_back(weightTemp);
						}
					}
					else
					{
						std::getline(md5File,keyString);
					}
				}
				for(int i = 0,iLen = meshTemp.numverts;i < iLen;i++)	//calculate the vertex position
				{
					meshTemp.vertexData[i].position = XMFLOAT3(0.0f,0.0f,0.0f);
					for(int j = meshTemp.vertexData[i].weightStart,jLen = meshTemp.vertexData[i].weightStart + meshTemp.vertexData[i].weightNum;j < jLen;j++)
					{
						quanternionTemp = XMLoadFloat4(&(md5meshData->jointList[meshTemp.weightData[j].jointIndex].jointQuat));
						//vectorTemp = XMQuaternionMultiply(XMQuaternionMultiply(quanternionTemp,XMLoadFloat3(&(meshTemp.weightData[j].position))),XMQuaternionConjugate(quanternionTemp));
						vectorTemp = XMQuaternionMultiply(quanternionTemp,XMLoadFloat3(&(meshTemp.weightData[j].position)));
						vectorTemp = XMQuaternionMultiply(vectorTemp,XMQuaternionConjugate(quanternionTemp));
						meshTemp.vertexData[i].position.x += (md5meshData->jointList[meshTemp.weightData[j].jointIndex].jointPos.x + XMVectorGetX(vectorTemp)) * meshTemp.weightData[j].weightValue;
						meshTemp.vertexData[i].position.y += (md5meshData->jointList[meshTemp.weightData[j].jointIndex].jointPos.y + XMVectorGetY(vectorTemp)) * meshTemp.weightData[j].weightValue;
						meshTemp.vertexData[i].position.z += (md5meshData->jointList[meshTemp.weightData[j].jointIndex].jointPos.z + XMVectorGetZ(vectorTemp)) * meshTemp.weightData[j].weightValue;
					}
				}
				if(isCalcNormal)	//calculate the normal direction
				{
					std::vector<XMFLOAT3> faceNormal;
					XMFLOAT3 normalTemp;
					XMVECTOR edge1,edge2;
					for(int i = 0,iLen = meshTemp.indexData.size();i < iLen;i+=3)
					{
						edge1 = XMLoadFloat3(&(meshTemp.vertexData[meshTemp.indexData[i+1]].position)) - XMLoadFloat3(&(meshTemp.vertexData[meshTemp.indexData[i]].position));
						edge2 = XMLoadFloat3(&(meshTemp.vertexData[meshTemp.indexData[i+2]].position)) - XMLoadFloat3(&(meshTemp.vertexData[meshTemp.indexData[i]].position));
						XMStoreFloat3(&normalTemp,XMVector3Normalize(XMVector3Cross(edge1,edge2)));
						faceNormal.push_back(normalTemp);
					}
					int joinFaceNum;
					for(int i = 0,iLen = meshTemp.vertexData.size();i < iLen;i++)
					{
						joinFaceNum = 0;
						normalTemp = XMFLOAT3(0.0f,0.0f,0.0f);
						for(int j = 0,k = 0,jLen = faceNormal.size();j < jLen;j++)
						{
							if(meshTemp.indexData[j*3] == i || meshTemp.indexData[j*3+1] == i || meshTemp.indexData[j*3+2] == i)
							{
								joinFaceNum++;
								normalTemp.x += faceNormal[j].x;
								normalTemp.y += faceNormal[j].y;
								normalTemp.z += faceNormal[j].z;
							}
						}
						if(joinFaceNum > 0)
						{
							normalTemp.x /= joinFaceNum;
							normalTemp.y /= joinFaceNum;
							normalTemp.z /= joinFaceNum;
							meshTemp.vertexData[i].normal = normalTemp;
						}
						else
						{
							meshTemp.vertexData[i].normal = meshTemp.vertexData[i].position;
						}
					}
				}
				HRESULT hr;
				D3D11_BUFFER_DESC vertexBufferDesc;
				D3D11_SUBRESOURCE_DATA vertexData;

				vertexBufferDesc.ByteWidth = sizeof(Vertex) * meshTemp.vertexData.size();
				vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
				vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				vertexBufferDesc.CPUAccessFlags = 0;
				vertexBufferDesc.MiscFlags = 0;
				vertexBufferDesc.StructureByteStride = 0;
				vertexData.pSysMem = &(meshTemp.vertexData[0]);
				vertexData.SysMemPitch = 0;
				vertexData.SysMemSlicePitch = 0;

				hr = d3dDevice->CreateBuffer(&vertexBufferDesc,&vertexData,&(meshTemp.md5meshVertexBuffer));
				HR(hr);

				D3D11_BUFFER_DESC indexBufferDesc;
				D3D11_SUBRESOURCE_DATA indexData;

				indexBufferDesc.ByteWidth = sizeof(DWORD) * meshTemp.indexData.size();
				indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
				indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				indexBufferDesc.CPUAccessFlags = 0;
				indexBufferDesc.MiscFlags = 0;
				indexBufferDesc.StructureByteStride = 0;
				indexData.pSysMem = &(meshTemp.indexData[0]);
				indexData.SysMemPitch = 0;
				indexData.SysMemSlicePitch = 0;

				hr = d3dDevice->CreateBuffer(&indexBufferDesc,&indexData,&(meshTemp.md5meshIndexBuffer));
				HR(hr);
				md5meshData->meshList.push_back(meshTemp);
			}
		}
		return true;
	}
	else
	{
		std::wstring errorString = L"open \"";
		errorString += filename;
		errorString += L"\" error!";
		MessageBox(hwnd,errorString.c_str(),L"error",MB_OK);
		return false;
	}
}

void makeCollider(ModelData &sourceModelData,ModelColliderData &destColliderData)
{
	float maxX = -FLT_MAX,minX = FLT_MAX,maxY = -FLT_MAX,minY = FLT_MAX,maxZ = -FLT_MAX,minZ = FLT_MAX;
	for(int i = 0,iLen = sourceModelData.indexVec.size();i < iLen;i++)
	{
		maxX = max(sourceModelData.vertexVec[sourceModelData.indexVec[i]].position.x,maxX);
		maxY = max(sourceModelData.vertexVec[sourceModelData.indexVec[i]].position.y,maxY);
		maxZ = max(sourceModelData.vertexVec[sourceModelData.indexVec[i]].position.z,maxZ);
		
		minX = min(sourceModelData.vertexVec[sourceModelData.indexVec[i]].position.x,minX);
		minY = min(sourceModelData.vertexVec[sourceModelData.indexVec[i]].position.y,minY);
		minZ = min(sourceModelData.vertexVec[sourceModelData.indexVec[i]].position.z,minZ);
	}
	destColliderData.modelCentrePoint.f[0] = (maxX + minX)/2.0f;
	destColliderData.modelCentrePoint.f[1] = (maxY + minY)/2.0f;
	destColliderData.modelCentrePoint.f[2] = (maxZ + minZ)/2.0f;
	destColliderData.modelCentrePoint.f[3] = 1.0f;

	XMFLOAT3 radius = XMFLOAT3(maxX - minX,maxY - minY,maxZ - minZ);
	destColliderData.centreRadius = XMVectorGetX(XMVector3Length(XMLoadFloat3(&radius)));

	//destColliderData.centreRadius = sqrtf(pow(maxX - minX,2) + pow(maxY - minY,2) + pow(maxZ - minZ,2));
	
	destColliderData.vertexVec.push_back(Vertex(XMFLOAT3(minX,maxY,minZ)));
	destColliderData.vertexVec.push_back(Vertex(XMFLOAT3(maxX,maxY,minZ)));
	destColliderData.vertexVec.push_back(Vertex(XMFLOAT3(minX,minY,minZ)));
	destColliderData.vertexVec.push_back(Vertex(XMFLOAT3(maxX,minY,minZ)));
	destColliderData.vertexVec.push_back(Vertex(XMFLOAT3(minX,maxY,maxZ)));
	destColliderData.vertexVec.push_back(Vertex(XMFLOAT3(maxX,maxY,maxZ)));
	destColliderData.vertexVec.push_back(Vertex(XMFLOAT3(minX,minY,maxZ)));
	destColliderData.vertexVec.push_back(Vertex(XMFLOAT3(maxX,minY,maxZ)));
	
	DWORD indexTemp[] = 
	{
		0, 1, 2,
		0, 2, 3,

		4, 6, 5,
		4, 7, 6,

		4, 5, 1,
		4, 1, 0,
		
		3, 2, 6,
		3, 6, 7,

		1, 5, 6,
		1, 6, 2,

		4, 0, 3, 
		4, 3, 7
	};
	for(int i = 0;i < 36;i++)
	{
		destColliderData.indexVec.push_back(indexTemp[i]);
	}
	destColliderData.makeIndexNoRepeat();
	destColliderData.type = MODEL_COLLIDER;
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
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;

	d3dDeviceContext->RSSetViewports(1,&viewPort);

	D3D11_RASTERIZER_DESC rasterStateDesc;
	ZeroMemory(&rasterStateDesc,sizeof(rasterStateDesc));
	//rasterStateDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterStateDesc.DepthClipEnable = true;
	//rasterStateDesc.DepthBiasClamp = -0.1;
	rasterStateDesc.DepthBias = 0;
	rasterStateDesc.FillMode = D3D11_FILL_SOLID;
	rasterStateDesc.CullMode = D3D11_CULL_BACK;
	rasterStateDesc.FrontCounterClockwise = false;
	d3dDevice->CreateRasterizerState(&rasterStateDesc,&rasterState_cw);
	rasterStateDesc.FrontCounterClockwise = true;
	d3dDevice->CreateRasterizerState(&rasterStateDesc,&rasterState_acw);
	rasterStateDesc.FrontCounterClockwise = false;
	rasterStateDesc.CullMode = D3D11_CULL_NONE;
	d3dDevice->CreateRasterizerState(&rasterStateDesc,&rasterState_cwnc);
	rasterStateDesc.DepthBias = 300;	//200还偶尔会出现。。。
	d3dDevice->CreateRasterizerState(&rasterStateDesc,&rasterState_cwnc_bias);
	rasterStateDesc.CullMode = D3D11_CULL_BACK;
	d3dDevice->CreateRasterizerState(&rasterStateDesc,&rasterState_cw_bias);
	//d3dDeviceContext->RSSetState(rasterState_cw);

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
	d3dDeviceContext->PSSetConstantBuffers(0,1,&constBufferSpace);

	worldSpace = XMMatrixIdentity();
	projectionMatrix =  XMMatrixPerspectiveFovLH(0.4f*3.14f,(float)WIDTH/(float)HEIGHT,0.01f,1000.0f);
	viewSpace = XMMatrixLookAtLH(eyePos,focusPos,upPos);
	cameraDir.v = focusPos.v - eyePos.v;
	cameraRotHorizontal = atan2(cameraDir.f[2],cameraDir.f[0]);
	cameraRotVertical= atan2(cameraDir.f[1],sqrtf(cameraDir.f[2] * cameraDir.f[2] + cameraDir.f[0] * cameraDir.f[0]));

	//平行光
	constBufferDesc.ByteWidth = sizeof(ConstLight);
	d3dDevice->CreateBuffer(&constBufferDesc,NULL,&constBufferLight);
	d3dDeviceContext->PSSetConstantBuffers(1,1,&constBufferLight);

	constLight.light.pad = 0.0f;
	constLight.light.ambientIntensity = XMFLOAT4(0.2f,0.2f,0.2f,1.0f);
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

	constSpotLight.spotLight.distanceAttr = XMFLOAT3(0.2f,0.1f,0.0f);
	constSpotLight.spotLight.range = 50.0f;
	constSpotLight.spotLight.pad = 0.0f;
	constSpotLight.spotLight.pos = XMFLOAT3(eyePos.f);
	constSpotLight.spotLight.dir = XMFLOAT3(cameraDir.f);
	constSpotLight.spotLight.lightIntensity = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
	constSpotLight.spotLight.deflectAttr = 10.0f;
	
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
	RenderTargetBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;//D3D11_BLEND_SRC_ALPHA;
	RenderTargetBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;;//D3D11_BLEND_DEST_ALPHA;
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
		Vertex(XMFLOAT3(-1.0,1.0,1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)),
		Vertex(XMFLOAT3(1.0,1.0,1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)),
		Vertex(XMFLOAT3(-1.0,-1.0,1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)),
		Vertex(XMFLOAT3(1.0,-1.0,1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)),
		Vertex(XMFLOAT3(-1.0,1.0,-1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)),
		Vertex(XMFLOAT3(1.0,1.0,-1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)),
		Vertex(XMFLOAT3(-1.0,-1.0,-1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)),
		Vertex(XMFLOAT3(1.0,-1.0,-1.0),XMFLOAT2(0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f)),
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
		Vertex(XMFLOAT3(-1.0f,1.0f,0.0f),leftUp,XMFLOAT3(0.0f,0.0f,-1.0f)),
		Vertex(XMFLOAT3(1.0f,1.0f,0.0f),rightUp,XMFLOAT3(0.0f,0.0f,-1.0f)),
		Vertex(XMFLOAT3(-1.0f,-1.0f,0.0f),leftDown,XMFLOAT3(0.0f,0.0f,-1.0f)),
		Vertex(XMFLOAT3(1.0f,-1.0f,0.0f),rightDown,XMFLOAT3(0.0f,0.0f,-1.0f)),
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

	LoadMD5Model(L"boy_old.md5mesh",&md5Boy,true,true);	
	LoadObjModel(L"spaceCompound.obj",&modelHouse,true,false);
	LoadObjModel(L"ground.obj",&modelGround,true,false);
	LoadObjModel(L"bottle.obj",&modelBottle,true,true);
	makeCollider(modelBottle,colliderBottle);
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
void DrawD2DText(const wchar_t * text)
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
	constSpace.WVP = XMMatrixTranspose(XMMatrixIdentity());
	constSpace.worldSpace = constSpace.WVP;
	constSpace.hasTexture = true;
	constSpace.hasNormalMap = false;
	constSpace.difColor = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->VSSetShader(VS,0,0);
	d3dDeviceContext->PSSetShader(D2D_PS,0,0);
	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView_text);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState + 1);
	d3dDeviceContext->OMSetBlendState(blendState,0,0xffffffff);
	d3dDeviceContext->DrawIndexed(6,0,0);
}
void DrawSkyBox()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0,1,&skyBoxVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(skyBoxIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	d3dDeviceContext->VSSetShader(SkyBox_VS,0,0);
	d3dDeviceContext->RSSetState(rasterState_cwnc);
	d3dDeviceContext->PSSetShader(SkyBox_PS,0,0);
	d3dDeviceContext->PSSetShaderResources(2,1,&shaderResourceView_skyBox);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState);
	d3dDeviceContext->OMSetBlendState(0,0,0xffffffff);
	
	XMMATRIX skyBoxPos = XMMatrixTranslation(eyePos.f[0],eyePos.f[1],eyePos.f[2]);
	constSpace.WVP = XMMatrixTranspose(worldSpace * XMMatrixScaling(5.0f,5.0f,5.0f) * skyBoxPos * viewSpace * projectionMatrix);
	constSpace.worldSpace = skyBoxPos;
	constSpace.hasTexture = true;
	constSpace.hasNormalMap = false;
	constSpace.difColor = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
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
	
//画模型不透明部分
	DrawModelNonBlend(&modelGround,worldSpace,viewSpace,true);
	DrawModelNonBlend(&modelHouse,worldSpace,viewSpace,false);
	DrawBottle(false);
	DrawMD5mesh(&md5Boy,scaleBoy * translationBoy * worldSpace,viewSpace,false);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0,1,&squareVertBuffer,&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(squareIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	d3dDeviceContext->VSSetShader(VS,0,0);
	d3dDeviceContext->PSSetShader(PS,0,0);
	d3dDeviceContext->OMSetBlendState(0,0,0xffffffff);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState);
	constSpotLight.spotLight.pos = XMFLOAT3(eyePos.f);
	constSpotLight.spotLight.dir = XMFLOAT3(cameraDir.f);
	d3dDeviceContext->UpdateSubresource(constBufferSpotLight,0,NULL,&constSpotLight,0,0);
	
//画草地
	constSpace.WVP = XMMatrixTranspose(worldSpace * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace);
	constSpace.hasTexture = true;
	constSpace.hasNormalMap = false;
	constSpace.difColor = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	//d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView_grass);
	//d3dDeviceContext->DrawIndexed(6,36,0);

//画两个立方体
	d3dDeviceContext->PSSetShaderResources(0,1,&shaderResourceView_brain);
	d3dDeviceContext->OMSetBlendState(blendState,0,0xffffffff);
	XMMATRIX ractangle_1 = XMMatrixRotationAxis(XMVectorSet(0,1,0,0),rot2) * XMMatrixTranslation(2,0,0);
	
	d3dDeviceContext->RSSetState(rasterState_acw);
	constSpace.WVP = XMMatrixTranspose(worldSpace * ractangle_1 * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace * ractangle_1);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);

	d3dDeviceContext->RSSetState(rasterState_cw);
	constSpace.WVP = XMMatrixTranspose(worldSpace * ractangle_1 * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace * ractangle_1);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);

	d3dDeviceContext->RSSetState(rasterState_acw);
	constSpace.WVP = XMMatrixTranspose(worldSpace * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);
	
	d3dDeviceContext->RSSetState(rasterState_cw);
	constSpace.WVP = XMMatrixTranspose(worldSpace * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace);
	d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
	d3dDeviceContext->DrawIndexed(36,0,0);

//画模型透明部分
	DrawModelBlend(&modelGround,worldSpace,viewSpace,true);
	DrawModelBlend(&modelHouse,worldSpace,viewSpace,false);
	//DrawBottle(true);

//显示文本
	wchar_t timeTemp[120];
	swprintf_s(timeTemp,L"%d",fps);
	DrawD2DText(timeTemp);

	d3dSwapChain->Present(0,0);
}

void DrawModelNonBlend(struct ModelData *modelData,CXMMATRIX worldSpace,CXMMATRIX viewSpace,bool isBias)
{
	UINT indexStart = 0;
	UINT indexCount = 0;
	//no blend
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	if(modelData->type == MODEL_NULL)return;

	d3dDeviceContext->IASetVertexBuffers(0,1,&(modelData->modelVertexBuffer),&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(modelData->modelIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	d3dDeviceContext->VSSetShader(VS,0,0);
	if(isBias)
		d3dDeviceContext->RSSetState(rasterState_cwnc_bias);
	else
		d3dDeviceContext->RSSetState(rasterState_cwnc);
	d3dDeviceContext->PSSetShader(PS,0,0);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState);
	d3dDeviceContext->OMSetBlendState(0,0,0xffffffff);
	constSpace.WVP = XMMatrixTranspose(worldSpace * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace);
	for(int i = 0,len = modelData->modelSurMetVec.size();i < len;i++)
	{
		if(modelData->modelSurMetVec[i].isTransparent == false)
		{
			indexStart = modelData->modelSurMetVec[i].texArrayIndex;
			indexCount = modelData->modelSurMetVec[i].indexCount;
			if(modelData->modelSurMetVec[i].hasTexture == true)
			{
				d3dDeviceContext->PSSetShaderResources(0,1,&(modelData->modelSurMetVec[i].shaderResourceView));
			}
			else
			{
				constSpace.difColor = modelData->modelSurMetVec[i].difColor;
			}
			if(modelData->modelSurMetVec[i].hasNormalMap)
			{
				d3dDeviceContext->PSSetShaderResources(1,1,&(modelData->modelSurMetVec[i].normalMapResourceView));
			}
			constSpace.hasTexture = modelData->modelSurMetVec[i].hasTexture;
			constSpace.hasNormalMap = modelData->modelSurMetVec[i].hasNormalMap;
			d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
			d3dDeviceContext->DrawIndexed(indexCount,indexStart,0);
		}
	}
}

void DrawModelBlend(struct ModelData *modelData,CXMMATRIX worldSpace,CXMMATRIX viewSpace,bool isBias)
{
	UINT indexStart = 0;
	UINT indexCount = 0;
	//no blend
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	if(modelData->type == MODEL_NULL)return;

	d3dDeviceContext->IASetVertexBuffers(0,1,&(modelData->modelVertexBuffer),&stride,&offset);
	d3dDeviceContext->IASetIndexBuffer(modelData->modelIndexBuffer,DXGI_FORMAT_R32_UINT,0);
	d3dDeviceContext->VSSetShader(VS,0,0);
	if(isBias)
		d3dDeviceContext->RSSetState(rasterState_cwnc_bias);
	else
		d3dDeviceContext->RSSetState(rasterState_cwnc);
	d3dDeviceContext->PSSetShader(PS,0,0);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState);
	constSpace.WVP = XMMatrixTranspose(worldSpace * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace);
	d3dDeviceContext->OMSetBlendState(blendState,0,0xffffffff);
	for(int i = 0,len = modelData->modelSurMetVec.size();i < len;i++)
	{
		if(modelData->modelSurMetVec[i].isTransparent == true)
		{
			indexStart = modelData->modelSurMetVec[i].texArrayIndex;
			indexCount = modelData->modelSurMetVec[i].indexCount;
			if(modelData->modelSurMetVec[i].hasTexture == true)
			{
				d3dDeviceContext->PSSetShaderResources(0,1,&(modelData->modelSurMetVec[i].shaderResourceView));
			}
			else
			{
				constSpace.difColor = modelData->modelSurMetVec[i].difColor;
				//constSpace.difColor = XMFLOAT4(0.5f,0.5f,0.5f,0.8f);
			}
			if(modelData->modelSurMetVec[i].hasNormalMap)
			{
				d3dDeviceContext->PSSetShaderResources(1,1,&(modelData->modelSurMetVec[i].normalMapResourceView));
			}
			constSpace.hasTexture = modelData->modelSurMetVec[i].hasTexture;
			constSpace.hasNormalMap = modelData->modelSurMetVec[i].hasNormalMap;
			d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
			d3dDeviceContext->DrawIndexed(indexCount,indexStart,0);
		}
	}
}

void DrawMD5mesh(struct MD5meshData *md5meshData,CXMMATRIX worldSpace,CXMMATRIX viewSpace,bool isBias)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	constSpace.WVP = XMMatrixTranspose(worldSpace * viewSpace * projectionMatrix);
	constSpace.worldSpace = XMMatrixTranspose(worldSpace);
	if(isBias)
		d3dDeviceContext->RSSetState(rasterState_cwnc_bias);
	else
		d3dDeviceContext->RSSetState(rasterState_cwnc);
	d3dDeviceContext->PSSetShader(PS,0,0);
	d3dDeviceContext->PSSetSamplers(0,1,samplerState);
	d3dDeviceContext->OMSetBlendState(0,0,0xffffffff);
	d3dDeviceContext->VSSetShader(VS,0,0);
	for(int i = 0,iLen = md5meshData->meshList.size();i < iLen;i++)
	{
		d3dDeviceContext->IASetVertexBuffers(0,1,&(md5meshData->meshList[i].md5meshVertexBuffer),&stride,&offset);
		d3dDeviceContext->IASetIndexBuffer(md5meshData->meshList[i].md5meshIndexBuffer,DXGI_FORMAT_R32_UINT,0);
		if(md5meshData->meshList[i].shaderResourceView != NULL)
		{
			constSpace.hasTexture = TRUE;
			d3dDeviceContext->PSSetShaderResources(0,1,&(md5meshData->meshList[i].shaderResourceView));
		}
		else
		{
			constSpace.difColor = XMFLOAT4(0.0f,0.0f,0.0f,0.0f);
			constSpace.hasTexture = FALSE;
		}
		constSpace.hasNormalMap = FALSE;
		d3dDeviceContext->UpdateSubresource(constBufferSpace,0,NULL,&constSpace,0,0);
		d3dDeviceContext->DrawIndexed(md5meshData->meshList[i].numtris * 3,0,0);
	}
}

//Input Controller
bool InitDirectInput(HINSTANCE hInstance)
{
	HR_DEBUG(DirectInput8Create(hInstance,DIRECTINPUT_VERSION,IID_IDirectInput8,(void**)&directInput,NULL));
	HR_DEBUG(directInput->CreateDevice(GUID_SysMouse,&mouseDevice,NULL));
	HR_DEBUG(directInput->CreateDevice(GUID_SysKeyboard,&keyboardDevice,NULL));
	mouseDevice->SetDataFormat(&c_dfDIMouse);
	mouseDevice->SetCooperativeLevel(hwnd,DISCL_NONEXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);
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

	isMouseClicked = false;
	if(mouseState.rgbButtons[0])
	{
		if(isMousePressed == false)
		{
			isMouseClicked = true;
		}
		isMousePressed = true;
	}
	else
	{
		isMousePressed = false;
	}

	cameraDir.v = XMVector3TransformNormal(XMVectorSet(0.0f,0.0f,1.0f,1.0f),XMMatrixRotationY(cameraRotHorizontal));
	cameraDir.v = XMVector3TransformNormal(cameraDir.v,XMMatrixRotationAxis(XMVectorSet(cameraDir.f[2],0.0f,-cameraDir.f[0],1.0f),cameraRotVertical));
	cameraDir.v = XMVector3Normalize(cameraDir);
	
	float speed = 5 * float(time);
	if(keyboardState[DIK_W] & 0x80)
	{
		XMVECTORF32 dirNormalized = {cameraDir.f[0],0.0f,cameraDir.f[2]};
		dirNormalized.v = XMVector3Normalize(dirNormalized.v);
		eyePos.v = XMVector3TransformCoord(eyePos,XMMatrixTranslation(dirNormalized.f[0] * speed,0.0f,dirNormalized.f[2] * speed));
	}
	if(keyboardState[DIK_S] & 0x80)
	{
		XMVECTORF32 dirNormalized = {cameraDir.f[0],0.0f,cameraDir.f[2]};
		dirNormalized.v = XMVector3Normalize(dirNormalized.v);
		eyePos.v = XMVector3TransformCoord(eyePos,XMMatrixTranslation(-dirNormalized.f[0] * speed,0,-dirNormalized.f[2] * speed));
	}
	if(keyboardState[DIK_A] & 0x80)
	{
		XMVECTORF32 dirNormalized = {cameraDir.f[0],0.0f,cameraDir.f[2]};
		dirNormalized.v = XMVector3Normalize(dirNormalized.v);
		eyePos.v = XMVector3TransformCoord(eyePos,XMMatrixTranslation(-dirNormalized.f[2] * speed,0,dirNormalized.f[0] * speed));
	}
	if(keyboardState[DIK_D] & 0x80)
	{
		XMVECTORF32 dirNormalized = {cameraDir.f[0],0.0f,cameraDir.f[2]};
		dirNormalized.v = XMVector3Normalize(dirNormalized.v);
		eyePos.v = XMVector3TransformCoord(eyePos,XMMatrixTranslation(dirNormalized.f[2] * speed,0,-dirNormalized.f[0] * speed));
	}
	focusPos.v = eyePos.v + cameraDir.v;
	viewSpace = XMMatrixLookAtLH(eyePos,focusPos,upPos);
}

void GetRayCast()
{
	POINT mousePos;

	/*get the screen coord*/
	GetCursorPos(&mousePos);
	ScreenToClient(hwnd, &mousePos);

	/*change to view space*/
	XMFLOAT2 ndcPoint;
	XMVECTORF32 viewPoint;
	ndcPoint.x = (mousePos.x * 2.0f / ClientWidth - 1.0f);
	ndcPoint.y = (1.0f - mousePos.y * 2.0f / ClientHeight);
	viewPoint.f[0] = ndcPoint.x / projectionMatrix(0,0);
	viewPoint.f[1] = ndcPoint.y / projectionMatrix(1,1);
	viewPoint.f[2] = 1.0f;
	viewPoint.f[3] = 1.0f;

	/*change into world space and produce two point*/
	XMVECTOR vectorTemp;
	inverseViewSpace = XMMatrixInverse(&vectorTemp,viewSpace);
	//rayPointEye.v = XMVector3TransformCoord(XMVectorZero(),inverseViewSpace);
	rayPointEye = eyePos;
	rayPointDir.v = XMVector3TransformCoord(viewPoint.v,inverseViewSpace);
}

bool MouseHitDetect(ModelData & modelData,CXMMATRIX worldSpaceTemp,int method = DETECT_METHOD_MODEL)
{
	XMVECTORF32 point1,point2,point3;
	if(modelData.type == MODEL_COLLIDER && (method == DETECT_METHOD_BOUNDING_SPHERE || method == DETECT_METHOD_MODEL_AND_BOUNDING_SPHERE))
	{
		ModelColliderData &colliderDataTemp = ((ModelColliderData&)modelData);
		float distance;
		XMVECTOR centreWorld;
		centreWorld = XMVector3TransformCoord(colliderDataTemp.modelCentrePoint.v,worldSpaceTemp);
		distance = XMVectorGetX(XMVector3Length(XMVector3Cross(centreWorld - rayPointEye.v,centreWorld - rayPointDir.v)) / XMVector3Length(rayPointDir.v - rayPointEye.v));
		if(distance > colliderDataTemp.centreRadius)
		{
			return false;
		}
		else if(method == DETECT_METHOD_BOUNDING_SPHERE)
		{
			return true;
		}
	}
	for(int i = 0,iLen = modelData.indexVec.size();i < iLen;i+=3)
	{
		point1.v =  XMVector3TransformCoord(XMLoadFloat3(&(modelData.vertexVec[modelData.indexVec[i]].position)),worldSpaceTemp);
		point2.v =  XMVector3TransformCoord(XMLoadFloat3(&(modelData.vertexVec[modelData.indexVec[i+1]].position)),worldSpaceTemp);
		point3.v =  XMVector3TransformCoord(XMLoadFloat3(&(modelData.vertexVec[modelData.indexVec[i+2]].position)),worldSpaceTemp);
		if(TriangleHitDetect(rayPointEye.f,rayPointDir.f,point1.f,point2.f,point3.f) != FLT_MAX)
		{
			return true;
		}
	}
	return false;
}

bool ColliderDetect(ModelData & srcModel,ModelData & dstModel,CXMMATRIX srcWorldSpace,CXMMATRIX dstWorldSpace,int method = DETECT_METHOD_MODEL)
{
	if(method == DETECT_METHOD_MODEL_AND_BOUNDING_SPHERE || method == DETECT_METHOD_BOUNDING_SPHERE)
	{
		if(srcModel.type == MODEL_COLLIDER && dstModel.type == MODEL_COLLIDER)	//two centre's distance checking
		{
			ModelColliderData &srcCollider = (ModelColliderData&)srcModel;
			ModelColliderData &dstCollider = (ModelColliderData&)dstModel;
			if(XMVectorGetX(XMVector3Length(XMVector3TransformCoord(srcCollider.modelCentrePoint.v,srcWorldSpace) - XMVector3TransformCoord(dstCollider.modelCentrePoint.v,dstWorldSpace))) <= srcCollider.centreRadius + dstCollider.centreRadius)
			{
				if(method == DETECT_METHOD_BOUNDING_SPHERE)
				{
					return true;
				}
			}
			else
			{
				return false;
			}
		}
		else if(srcModel.type == MODEL_COLLIDER || dstModel.type == MODEL_COLLIDER)	//one centre check one collider
		{
			XMVECTORF32 centrePoint;
			float centreRadius;
			ModelData *colliderData;
			XMMATRIX worldPosition;
			if(srcModel.type == MODEL_COLLIDER)
			{
				colliderData = &dstModel;
				centrePoint = ((ModelColliderData&)srcModel).modelCentrePoint;
				centreRadius = ((ModelColliderData&)srcModel).centreRadius;
			}
			else
			{
				colliderData = &srcModel;
				centrePoint = ((ModelColliderData&)dstModel).modelCentrePoint;
				centreRadius = ((ModelColliderData&)dstModel).centreRadius;
			}
			bool iFlag = false;
			for(int i = 0,iLen = colliderData->indexNoRepeat.size();i < iLen;i++)
			{
				if(XMVectorGetX(XMVector3Length(XMVector3TransformCoord(XMLoadFloat3(&(colliderData->vertexVec[colliderData->indexNoRepeat[i]].position)),worldPosition)-centrePoint.v)) <= centreRadius)
				{
					if(method == DETECT_METHOD_BOUNDING_SPHERE)
					{
						return true;
					}
					else
					{
						iFlag = true;
						break;
					}
				}
			}
			if(iFlag == false)
			{
				return false;
			}
		}
	}
	//model checking
	XMVECTORF32 srcPoint1,srcPoint2,srcPoint3;
	XMVECTORF32 dstPoint1,dstPoint2,dstPoint3;
	float t;
	for(int i = 0,iLen = srcModel.indexVec.size();i < iLen;i+=3)
	{
		srcPoint1.v = XMVector3TransformCoord(XMLoadFloat3(&(srcModel.vertexVec[srcModel.indexVec[i]].position)),dstWorldSpace);
		srcPoint2.v = XMVector3TransformCoord(XMLoadFloat3(&(srcModel.vertexVec[srcModel.indexVec[i+1]].position)),dstWorldSpace);
		srcPoint3.v = XMVector3TransformCoord(XMLoadFloat3(&(srcModel.vertexVec[srcModel.indexVec[i+2]].position)),dstWorldSpace);
		for(int j = 0,jLen = dstModel.indexVec.size();j < iLen;j+=3)
		{
			dstPoint1.v = XMVector3TransformCoord(XMLoadFloat3(&(dstModel.vertexVec[dstModel.indexVec[j]].position)),srcWorldSpace);
			dstPoint2.v = XMVector3TransformCoord(XMLoadFloat3(&(dstModel.vertexVec[dstModel.indexVec[j+1]].position)),srcWorldSpace);
			dstPoint3.v = XMVector3TransformCoord(XMLoadFloat3(&(dstModel.vertexVec[dstModel.indexVec[j+2]].position)),srcWorldSpace);
			t = TriangleHitDetect(srcPoint1.f,srcPoint2.f,dstPoint1.f,dstPoint2.f,dstPoint3.f);
			if(0.0f <= t && t <= 1.0f)
			{
				return true;
			}
			t = TriangleHitDetect(srcPoint2.f,srcPoint3.f,dstPoint1.f,dstPoint2.f,dstPoint3.f);
			if(0.0f <= t && t <= 1.0f)
			{
				return true;
			}
			t = TriangleHitDetect(srcPoint3.f,srcPoint1.f,dstPoint1.f,dstPoint2.f,dstPoint3.f);
			if(0.0f <= t && t <= 1.0f)
			{
				return true;
			}
		}
	}
	return false;
}

float TriangleHitDetect(XMFLOAT3 rayPointSrc,XMFLOAT3 rayPointDst,XMFLOAT3 point1,XMFLOAT3 point2,XMFLOAT3 point3)
{
	//1
	{
		XMVECTOR point1Vec,point2Vec,point3Vec;
		XMVECTORF32 dir1_2,dir1_3;
		XMVECTORF32 planeNormal;
		XMVECTOR pointEyeToPlane;
		float planeParaA,planeParaB,planeParaC,planeParaD;
		float distanceEye,distanceDir;
		float t = 0.0f;

		point1Vec =  XMVectorSetW(XMLoadFloat3(&point1),1.0f);
		point2Vec =  XMVectorSetW(XMLoadFloat3(&point2),1.0f);
		point3Vec =  XMVectorSetW(XMLoadFloat3(&point3),1.0f);

		dir1_2.v = XMVectorSet(point2.x-point1.x,point2.y-point1.y,point2.z-point1.z,0.0f);
		dir1_3.v = XMVectorSet(point3.x-point1.x,point3.y-point1.y,point3.z-point1.z,0.0f);
		planeNormal.v = XMVector3Cross(dir1_2,dir1_3);
		planeParaA = planeNormal.f[0];
		planeParaB = planeNormal.f[1];
		planeParaC = planeNormal.f[2];
		planeParaD = -(planeParaA * point1.x + planeParaB * point1.y + planeParaC * point1.z);
		distanceEye = planeParaA * rayPointSrc.x + planeParaB * rayPointSrc.y + planeParaC * rayPointSrc.z + planeParaD; 
		distanceDir = planeParaA * rayPointDst.x + planeParaB * rayPointDst.y + planeParaC * rayPointDst.z + planeParaD; 
		if(distanceEye == distanceDir)return FLT_MAX;
		t = distanceEye/(distanceEye - distanceDir);
		if(t < 0.0f)return FLT_MAX;
		pointEyeToPlane = XMLoadFloat3(&rayPointSrc) + XMVectorScale(XMVectorSubtract(XMLoadFloat3(&rayPointDst),XMLoadFloat3(&rayPointSrc)),t);

		XMVECTOR lineSeg1,lineSeg2,lineSeg3;
		XMVECTOR crossVec1,crossVec2,crossVec3;
		lineSeg1 = point1Vec - pointEyeToPlane;
		lineSeg2 = point2Vec - pointEyeToPlane;
		lineSeg3 = point3Vec - pointEyeToPlane;
		crossVec1 = XMVector3Cross(lineSeg1,lineSeg2);
		crossVec2 = XMVector3Cross(lineSeg2,lineSeg3);
		if(XMVectorGetX(XMVector3Dot(crossVec1,crossVec2)) <= 0.0f)return FLT_MAX;
		crossVec3 = XMVector3Cross(lineSeg3,lineSeg1);
		if(XMVectorGetX(XMVector3Dot(crossVec2,crossVec3)) <= 0.0f)return FLT_MAX;
		return t;
	}

	//2
	{
		//XMVECTOR pointDir1,pointDir2,pointDir3,rayDir;
		//XMVECTOR dirNormal1,dirNormal2,dirNormal3;
		//pointDir1 = XMVectorSubtract(XMLoadFloat3(&point1),rayPointEye.v);
		//pointDir2 = XMVectorSubtract(XMLoadFloat3(&point2),rayPointEye.v);
		//pointDir3 = XMVectorSubtract(XMLoadFloat3(&point3),rayPointEye.v);
		//rayDir = XMVectorSubtract(rayPointDir.v,rayPointEye.v);
	}
	//未完

	//3
	{
		//XMVECTOR point1Vec,point2Vec,point3Vec;
		//XMVECTOR pointDir1,pointDir2,pointDir3,rayDir;
		//XMMATRIX pointCoordSystem;
		//XMMATRIX triangleCoordSystem;
		//XMVECTOR pointCoord;

		//point1Vec =  XMVectorSetW(XMLoadFloat3(&point1),1.0f);
		//point2Vec =  XMVectorSetW(XMLoadFloat3(&point2),1.0f);
		//point3Vec =  XMVectorSetW(XMLoadFloat3(&point3),1.0f);

		//pointDir1 = point1Vec - rayPointEye.v;
		//pointDir2 = point2Vec - rayPointEye.v;
		//pointDir3 = point3Vec - rayPointEye.v;
		//rayDir = rayPointDir.v - rayPointEye.v;
		//XMVECTOR vectorTemp;
		//triangleCoordSystem = XMMATRIX(pointDir1,pointDir2,pointDir3,XMVectorSet(0.0f,0.0f,0.0f,1.0f));
		//pointCoordSystem = XMMatrixInverse(&vectorTemp,triangleCoordSystem);
		//if(XMMatrixIsInfinite(pointCoordSystem))
		//{
		//	return false;
		//}
		//pointCoord = XMVector3Transform(rayPointDir.v - rayPointEye.v,pointCoordSystem);
		//if(XMVectorGetX(pointCoord) > 0.0f && XMVectorGetY(pointCoord) > 0.0f && XMVectorGetZ(pointCoord) > 0.0f)
		//	return true;
		//else
		//	return false;
	}
}

#define BOTTLE_NUM 20
void DrawBottle(bool isBlend)	//bottles' controlling,condition checking and drawing
{
	static XMMATRIX worldSpaceTemp[BOTTLE_NUM + 2];
	static bool isAlive[BOTTLE_NUM + 2];
	static bool firstCall = true;
	if(firstCall == true)
	{
		for(int i = 0,len = BOTTLE_NUM;i < len;i++)
		{
			worldSpaceTemp[i] = XMMatrixTranslation(25.0f - 5.0f * (i%10 + i/10) ,2.0f , 5.0f * (i%10 - 1/10) - 75.0f);
			isAlive[i] = true;
		}
		worldSpaceTemp[0] = XMMatrixTranslation(-2.0f,2.0f,0.0f);
		firstCall = false;
	}
	//worldSpaceTemp[1] = XMMatrixTranslation(0.0f,0.0f,3.0f) * inverseViewSpace;

	if(false)
	{	
		for(int i = 0,iLen = BOTTLE_NUM;i < iLen;i++)
		{
			//if(MouseHitDetect(modelBottle,worldSpaceTemp[i],DETECT_METHOD_MODEL))
			if(MouseHitDetect(colliderBottle,worldSpaceTemp[i],DETECT_METHOD_MODEL_AND_BOUNDING_SPHERE))
			{
				isAlive[i] = false;
			}
			//remember that i have not do the depth filter of bottles
		}
	}

	//query:I have a problem,all the different checking calculates the same world position,is this repeititon unnecessary?
	if(isAlive[1] == true && isMouseClicked)
	{
		for(int i = 0,len = BOTTLE_NUM;i < len;i++)
		{
			if(i != 1 && isAlive[i] == true)
			{
				if(ColliderDetect(colliderBottle,colliderBottle,worldSpaceTemp[1],worldSpaceTemp[i],DETECT_METHOD_MODEL_AND_BOUNDING_SPHERE))
				{
					//isAlive[1] = false;
				}
			}
		}
	}

	if(isBlend)
	{
		for(int i = 0,len = BOTTLE_NUM;i < len;i++)
		{
			if(isAlive[i] == true)
			{
				DrawModelBlend(&modelBottle,worldSpaceTemp[i],viewSpace,false);
			}
		}
	}
	else
	{
		for(int i = 0,len = BOTTLE_NUM;i < len;i++)
		{
			if(isAlive[i] == true)
			{
				DrawModelNonBlend(&modelBottle,worldSpaceTemp[i],viewSpace,false);
			}
		}
	}
}