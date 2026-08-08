#include "gme.h"

#define WIN32_LEAN_AND_MEAN
#include <initguid.h> // Should come first

#include <d3dcompiler.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <stdint.h>
#include <windows.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")

#define SCR_W 600
#define SCR_H 800

#define BUFFER_COUNT 2

#define DEBUG_INTERFACE

#define COM(obj, method, ...) (obj)->lpVtbl->method(obj, __VA_ARGS__)
#define COM_OK(obj, method, ...) SUCCEEDED(COM(obj, method, __VA_ARGS__))
#define COM_CHK(obj, method, ...) if (FAILED(COM(obj, method, __VA_ARGS__))) return 1

static IDXGIFactory4             * d3d_factory;
static IDXGIAdapter1             * d3d_adapter;
static ID3D12Device              * d3d_device;
static ID3D12CommandQueue        * d3d_queue;
static IDXGISwapChain3           * d3d_swc;
static ID3D12DescriptorHeap      * d3d_rtv_heap;
static ID3D12CommandAllocator    * d3d_cmd_alloc;
static ID3D12GraphicsCommandList * d3d_cmd_list;
static ID3D12RootSignature       * d3d_root_sign;
static ID3D12PipelineState       * d3d_pso;

static ID3D12Resource * d3d_rt[BUFFER_COUNT];
static ID3D12Resource * d3d_buffer;

static ID3D12Fence * d3d_fence;
static unsigned      d3d_frame_idx;
static HANDLE        d3d_fence_event;
static uint64_t      d3d_fence_value;

static void d3d_release(void * obj) {
  if (obj) COM((IUnknown *)obj, Release);
}

typedef void (STDMETHODCALLTYPE * d3d_get_cpu_desc_t)(ID3D12DescriptorHeap *, D3D12_CPU_DESCRIPTOR_HANDLE *);
static D3D12_CPU_DESCRIPTOR_HANDLE d3d_get_cpu_desc(ID3D12DescriptorHeap * heap) {
  D3D12_CPU_DESCRIPTOR_HANDLE h;
  ((d3d_get_cpu_desc_t)heap->lpVtbl->GetCPUDescriptorHandleForHeapStart)(heap, &h);
  return h;
}

static int d3d_debug() {
#ifdef DEBUG_INTERFACE
  ID3D12Debug * debug;
  if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, (void **)&debug))) {
    COM(debug, EnableDebugLayer);
    return DXGI_CREATE_FACTORY_DEBUG;
  }
#endif
  return 0;
}

static inline int d3d_enum_adapter_by_gpu(IDXGIFactory6 * f6, unsigned i) {
  return COM_OK(f6, EnumAdapterByGpuPreference, i, DXGI_GPU_PREFERENCE_UNSPECIFIED, &IID_IDXGIAdapter1, (void **)&d3d_adapter);
}
static inline int d3d_enum_adapter(unsigned i) {
  return COM_OK(d3d_factory, EnumAdapters1, i, &d3d_adapter);
}
static inline int d3d_adapter_is_software(void) {
  DXGI_ADAPTER_DESC1 desc;
  COM(d3d_adapter, GetDesc1, &desc);
  return desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
}
static inline int d3d_create_device(void) {
  return FAILED(D3D12CreateDevice((IUnknown *)d3d_adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&d3d_device));
}
static int d3d_init_adapter(void) {
  IDXGIFactory6 * factory6;
  if (COM_OK(d3d_factory, QueryInterface, &IID_IDXGIFactory6, (void **)&factory6)) {
    for (unsigned i = 0; d3d_enum_adapter_by_gpu(factory6, i); i++) {
      if (d3d_adapter_is_software()) continue;
      if (0 == d3d_create_device()) return 0;
    }
  }

  for (unsigned i = 0; d3d_enum_adapter(i); i++) {
    if (d3d_adapter_is_software()) continue;
    if (0 == d3d_create_device()) return 0;
  }

  COM_CHK(d3d_factory, EnumWarpAdapter, &IID_IDXGIAdapter1, (void **)&d3d_adapter);
  return d3d_create_device();
}

static int d3d_init_queue(void) {
  D3D12_COMMAND_QUEUE_DESC desc = {0};
  COM_CHK(d3d_device, CreateCommandQueue, &desc, &IID_ID3D12CommandQueue, (void **)&d3d_queue);
  return 0;
}

static int d3d_init_swapchain(HWND hwnd) {
  IDXGISwapChain1 * swc;
  DXGI_SWAP_CHAIN_DESC1 desc = {
    .Width       = SCR_W,
    .Height      = SCR_H,
    .Format      = DXGI_FORMAT_R8G8B8A8_UNORM,
    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
    .BufferCount = BUFFER_COUNT,
    .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD,

    .SampleDesc = (DXGI_SAMPLE_DESC) {
      .Count = 1,
    },
  };
  COM_CHK(d3d_factory, CreateSwapChainForHwnd, (IUnknown *)d3d_queue, hwnd, &desc, NULL, NULL, &swc);
  COM_CHK(swc, QueryInterface, &IID_IDXGISwapChain3, (void **)&d3d_swc);
  return 0;
}

static int d3d_init_rtv_heap(void) {
  D3D12_DESCRIPTOR_HEAP_DESC desc = {
    .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
    .NumDescriptors = BUFFER_COUNT,
  };
  COM_CHK(d3d_device, CreateDescriptorHeap, &desc, &IID_ID3D12DescriptorHeap, (void **)&d3d_rtv_heap);
  return 0;
}

static D3D12_CPU_DESCRIPTOR_HANDLE d3d_get_rtv_cpu_desc(int i) {
  D3D12_CPU_DESCRIPTOR_HANDLE h = d3d_get_cpu_desc(d3d_rtv_heap);
  h.ptr += i * COM(d3d_device, GetDescriptorHandleIncrementSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  return h;
}

static int d3d_init_rtv(void) {
  for (int i = 0; i < BUFFER_COUNT; i++) {
    COM_CHK(d3d_swc, GetBuffer, i, &IID_ID3D12Resource, (void **)&d3d_rt[i]);
    COM(d3d_device, CreateRenderTargetView, d3d_rt[i], NULL, d3d_get_rtv_cpu_desc(i));
  }
  return 0;
}

static int d3d_init_cmdlist() {
  COM_CHK(d3d_device, CreateCommandList, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d_cmd_alloc, d3d_pso, &IID_ID3D12GraphicsCommandList, (void **)&d3d_cmd_list);
  COM_CHK(d3d_cmd_list, Close);
  return 0;
}

static void d3d_report_err(ID3DBlob * err) {
  if (!err) return;
  const char * txt = COM(err, GetBufferPointer);
  MessageBox(NULL, txt, "Direct3D error", MB_ICONERROR);
}

static int d3d_init_root_signature() {
  ID3DBlob * blob;
  ID3DBlob * err;
  D3D12_ROOT_SIGNATURE_DESC desc = {
    .NumParameters      = 2,
    .pParameters        = (D3D12_ROOT_PARAMETER[]) {{
      .ParameterType    = D3D12_ROOT_PARAMETER_TYPE_SRV,
    }, {
      .ParameterType    = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
      .Constants        = (D3D12_ROOT_CONSTANTS) {
        .Num32BitValues = sizeof(gme_upc_t) / 4,
      },
    }},
  };
  if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &err))) return (d3d_report_err(err), 1);
  if (err) return (d3d_report_err(err), 1);

  const void * data = COM(blob, GetBufferPointer);
  size_t        len = COM(blob, GetBufferSize);
  COM_CHK(d3d_device, CreateRootSignature, 0, data, len, &IID_ID3D12RootSignature, (void **)&d3d_root_sign);

  d3d_release(blob);
  d3d_release(err);
  return 0;
}

static ID3DBlob * d3d_compile(const char * tgt, const char * name) {
  HRSRC r = FindResource(NULL, name, "hlsl");
  HGLOBAL g = LoadResource(NULL, r);
  void * ptr = LockResource(g);
  unsigned sz = SizeofResource(NULL, r);

  ID3DBlob * blob;
  ID3DBlob * err;
  if (FAILED(D3DCompile(ptr, sz, NULL, NULL, NULL, "main", tgt, 0, 0, &blob, &err))) return (d3d_report_err(err), NULL);
  // If you want warnings as well:
  // if (err) return (d3d_report_err(err), NULL);
  return blob;
}
static D3D12_SHADER_BYTECODE d3d_blob2shader(ID3DBlob * blob) {
  return (D3D12_SHADER_BYTECODE){ COM(blob, GetBufferPointer), COM(blob, GetBufferSize) };
}
static int d3d_init_pso() {
  ID3DBlob * vs = d3d_compile("vs_5_0", "shader.vert");
  ID3DBlob * ps = d3d_compile("ps_5_0", "shader.frag");
  if (!vs || !ps) return 1;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
    .pRootSignature        = d3d_root_sign,
    .VS                    = d3d_blob2shader(vs),
    .PS                    = d3d_blob2shader(ps),
    .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
    .SampleMask            = UINT_MAX,
    .NumRenderTargets      = 1,

    .RasterizerState = (D3D12_RASTERIZER_DESC) {
      .FillMode = D3D12_FILL_MODE_SOLID,
      .CullMode = D3D12_CULL_MODE_NONE,
    },
    .SampleDesc = (DXGI_SAMPLE_DESC) {
      .Count = 1,
    },
  };
  desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  COM_CHK(d3d_device, CreateGraphicsPipelineState, &desc, &IID_ID3D12PipelineState, (void **)&d3d_pso);

  d3d_release(vs);
  d3d_release(ps);
  return 0;
}

static int d3d_init_buffer(void) {
  int size = GME_BUF_SIZE;

  D3D12_HEAP_PROPERTIES heap = {
    .Type = D3D12_HEAP_TYPE_UPLOAD,
  };
  D3D12_RESOURCE_DESC res = {
    .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
    .Width            = size,
    .Height           = 1,
    .DepthOrArraySize = 1,
    .MipLevels        = 1,
    .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    .SampleDesc       = (DXGI_SAMPLE_DESC) {
      .Count          = 1,
    },
  };
  COM_CHK(d3d_device, CreateCommittedResource,
      &heap, D3D12_HEAP_FLAG_NONE, &res, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, 
      &IID_ID3D12Resource, (void **)&d3d_buffer);

  return 0;
}

int d3d_init(HWND hwnd) {
  if (FAILED(CreateDXGIFactory2(d3d_debug(), &IID_IDXGIFactory4, (void **)&d3d_factory))) return 1;

  if (d3d_init_adapter())       return 1;
  if (d3d_init_queue())         return 1;
  if (d3d_init_swapchain(hwnd)) return 1;

  COM_CHK(d3d_factory, MakeWindowAssociation, hwnd, DXGI_MWA_NO_ALT_ENTER);

  if (d3d_init_rtv_heap()) return 1;
  if (d3d_init_rtv())      return 1;

  COM_CHK(d3d_device, CreateCommandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, (void **)&d3d_cmd_alloc);

  if (d3d_init_root_signature()) return 1;
  if (d3d_init_pso())            return 1;
  if (d3d_init_cmdlist())        return 1;

  if (d3d_init_buffer()) return 1;

  COM_CHK(d3d_device, CreateFence, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&d3d_fence);
  d3d_frame_idx   = COM(d3d_swc, GetCurrentBackBufferIndex);
  d3d_fence_value = 1;
  d3d_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (!d3d_fence_event) return 1;

  return 0;
}

static int d3d_wait() {
  uint64_t v = d3d_fence_value;
  COM_CHK(d3d_queue, Signal, d3d_fence, v);
  d3d_fence_value++;

  if (COM(d3d_fence, GetCompletedValue) < v) {
    COM(d3d_fence, SetEventOnCompletion, v, d3d_fence_event);
    WaitForSingleObject(d3d_fence_event, INFINITE);
  }

  d3d_frame_idx = COM(d3d_swc, GetCurrentBackBufferIndex);
  return 0;
}

void d3d_deinit(void) {
  d3d_wait();

  for (int i = 0; i < BUFFER_COUNT; i++) d3d_release(d3d_rt[i]);

  d3d_release(d3d_fence);

  d3d_release(d3d_buffer);
  d3d_release(d3d_cmd_list);
  d3d_release(d3d_pso);
  d3d_release(d3d_root_sign);
  d3d_release(d3d_cmd_alloc);
  d3d_release(d3d_rtv_heap);
  d3d_release(d3d_swc);
  d3d_release(d3d_queue);
  d3d_release(d3d_device);
  d3d_release(d3d_adapter);
  d3d_release(d3d_factory);

  CloseHandle(d3d_fence_event);
}

static void d3d_cmd_transition_barrier(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
  D3D12_RESOURCE_BARRIER b = {
    .Transition    = {
      .pResource   = d3d_rt[d3d_frame_idx],
      .StateBefore = before,
      .StateAfter  = after,
    }
  };
  COM(d3d_cmd_list, ResourceBarrier, 1, &b);
}
int d3d_frame(void) {
  COM_CHK(d3d_cmd_alloc, Reset);
  COM_CHK(d3d_cmd_list, Reset, d3d_cmd_alloc, d3d_pso);

  COM(d3d_cmd_list, SetGraphicsRootSignature, d3d_root_sign);
  COM(d3d_cmd_list, SetGraphicsRootShaderResourceView, 0, COM(d3d_buffer, GetGPUVirtualAddress));
  COM(d3d_cmd_list, SetGraphicsRoot32BitConstants, 1, sizeof(gme_upc_t) / 4, &gme_pc, 0);

  D3D12_VIEWPORT vp = { 0, 0, SCR_W, SCR_H };
  COM(d3d_cmd_list, RSSetViewports, 1, &vp);
  D3D12_RECT     sc = { 0, 0, SCR_W, SCR_H };
  COM(d3d_cmd_list, RSSetScissorRects, 1, &sc);

  d3d_cmd_transition_barrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

  D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d_get_rtv_cpu_desc(d3d_frame_idx);
  COM(d3d_cmd_list, OMSetRenderTargets, 1, &rtv, FALSE, NULL);

  float colour[] = { 0.1, 0.2, 0.3, 1.0 };
  COM(d3d_cmd_list, ClearRenderTargetView, rtv, colour, 0, NULL);
  COM(d3d_cmd_list, IASetPrimitiveTopology, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  COM(d3d_cmd_list, DrawInstanced, 3, 1, 0, 0);

  d3d_cmd_transition_barrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

  COM_CHK(d3d_cmd_list, Close);

  ID3D12CommandList * cmd_list = (ID3D12CommandList *)d3d_cmd_list;
  COM(d3d_queue, ExecuteCommandLists, 1, &cmd_list);

  COM_CHK(d3d_swc, Present, 1, 0);
  d3d_wait();

  return 0;
}

static LRESULT window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_KEYDOWN:
      if (HIWORD(l_param) & KF_REPEAT) return 0;

      switch (LOWORD(w_param)) {
        case VK_LEFT:  gme_left();     break;
        case VK_RIGHT: gme_right();    break;
        case VK_UP:    gme_up();       break;
        case VK_DOWN:  gme_down();     break;
        case VK_SPACE: gme_new_game(); break;
      }

      return 0;
    case WM_PAINT:
      // TODO: map once on init
      void * buf;
      COM_CHK(d3d_buffer, Map, 0, NULL, &buf);
      gme_load(buf);
      COM(d3d_buffer, Unmap, 0, NULL);

      gme_frame();
      if (d3d_frame()) PostQuitMessage(1);
      return 0;
  }
  return DefWindowProc(hwnd, msg, w_param, l_param);
}

int WinMain(HINSTANCE h_instance, HINSTANCE h_prev, LPSTR cmd_line, int cmd_show) {
  HICON h_icon = LoadIcon(h_instance, "IDI_APPICON");

  WNDCLASSEX wcex  = {
    .cbSize        = sizeof(WNDCLASSEX),
    .style         = CS_HREDRAW | CS_VREDRAW,
    .lpfnWndProc   = &window_proc,
    .hInstance     = h_instance,
    .hIcon         = h_icon,
    .hCursor       = LoadCursor(NULL, IDC_ARROW),
    .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
    .lpszClassName = "m4c0-snake-window",
    .hIconSm       = h_icon,
  };
  if (!RegisterClassEx(&wcex)) {
    MessageBox(NULL, "Failed to register window class", "Unhandled error", 0);
    return 1;
  }

  DWORD style = WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX ^ WS_MAXIMIZEBOX;

  HWND hwnd = CreateWindow(
      "m4c0-snake-window",
      "Casually Casual Snake Game",
      style, CW_USEDEFAULT, CW_USEDEFAULT,
      SCR_W, SCR_H, 
      NULL, NULL, h_instance, NULL);
  if (!hwnd) {
    MessageBox(NULL, "Failed to create window", "Unhandled error", 0);
    return 1;
  }

  if (d3d_init(hwnd)) return 1;
  gme_init();
  gme_resize(SCR_W, SCR_H);

  ShowWindow(hwnd, cmd_show);
  UpdateWindow(hwnd);

  MSG msg;
  while (GetMessage(&msg, 0, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  
  gme_deinit();
  d3d_deinit();
  return msg.wParam;
}
