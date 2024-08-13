/*
 * @Author: DI JUNKUN
 * @Date: 2024-08-12
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _NVCODEC_API_H_
#define _NVCODEC_API_H_

#include <Windows.h>

#include <iostream>

#include "cuda.h"
#include "cuviddec.h"
#include "nvEncodeAPI.h"
#include "nvcuvid.h"

typedef CUresult (*TcuInit)(unsigned int Flags);

typedef CUresult (*TcuDeviceGet)(CUdevice *device, int ordinal);

typedef CUresult (*TcuDeviceGetCount)(int *count);

typedef CUresult (*TcuCtxCreate)(CUcontext *pctx, unsigned int flags,
                                 CUdevice dev);

typedef CUresult (*TcuGetErrorName)(CUresult error, const char **pStr);

typedef CUresult (*TcuCtxPushCurrent)(CUcontext ctx);

typedef CUresult (*TcuCtxPopCurrent)(CUcontext *pctx);

typedef CUresult (*TcuMemAlloc)(CUdeviceptr *dptr, size_t bytesize);

typedef CUresult (*TcuMemAllocPitch)(CUdeviceptr *dptr, size_t *pPitch,
                                     size_t WidthInBytes, size_t Height,
                                     unsigned int ElementSizeBytes);

typedef CUresult (*TcuMemFree)(CUdeviceptr dptr);

typedef CUresult (*TcuMemcpy2DAsync)(const CUDA_MEMCPY2D *pCopy,
                                     CUstream hStream);

typedef CUresult (*TcuStreamSynchronize)(CUstream hStream);

typedef CUresult (*TcuMemcpy2D)(const CUDA_MEMCPY2D *pCopy);

typedef CUresult (*TcuMemcpy2DUnaligned)(const CUDA_MEMCPY2D *pCopy);

// API
static TcuInit cuInit_ld;
static TcuDeviceGet cuDeviceGet_ld;
static TcuDeviceGetCount cuDeviceGetCount_ld;
static TcuCtxCreate cuCtxCreate_ld;
static TcuGetErrorName cuGetErrorName_ld;
static TcuCtxPushCurrent cuCtxPushCurrent_ld;
static TcuCtxPopCurrent cuCtxPopCurrent_ld;
static TcuMemAlloc cuMemAlloc_ld;
static TcuMemAllocPitch cuMemAllocPitch_ld;
static TcuMemFree cuMemFree_ld;
static TcuMemcpy2DAsync cuMemcpy2DAsync_ld;
static TcuStreamSynchronize cuStreamSynchronize_ld;
static TcuMemcpy2D cuMemcpy2D_ld;
static TcuMemcpy2DUnaligned cuMemcpy2DUnaligned_ld;

//
typedef CUresult (*TcuvidCtxLockCreate)(CUvideoctxlock *pLock, CUcontext ctx);
typedef CUresult (*TcuvidGetDecoderCaps)(CUVIDDECODECAPS *pdc);
typedef CUresult (*TcuvidCreateDecoder)(CUvideodecoder *phDecoder,
                                        CUVIDDECODECREATEINFO *pdci);
typedef CUresult (*TcuvidDestroyDecoder)(CUvideodecoder hDecoder);
typedef CUresult (*TcuvidDecodePicture)(CUvideodecoder hDecoder,
                                        CUVIDPICPARAMS *pPicParams);
typedef CUresult (*TcuvidGetDecodeStatus)(CUvideodecoder hDecoder, int nPicIdx,
                                          CUVIDGETDECODESTATUS *pDecodeStatus);
typedef CUresult (*TcuvidReconfigureDecoder)(
    CUvideodecoder hDecoder, CUVIDRECONFIGUREDECODERINFO *pDecReconfigParams);
typedef CUresult (*TcuvidMapVideoFrame64)(CUvideodecoder hDecoder, int nPicIdx,
                                          unsigned long long *pDevPtr,
                                          unsigned int *pPitch,
                                          CUVIDPROCPARAMS *pVPP);
typedef CUresult (*TcuvidUnmapVideoFrame64)(CUvideodecoder hDecoder,
                                            unsigned long long DevPtr);
typedef CUresult (*TcuvidCtxLockDestroy)(CUvideoctxlock lck);
typedef CUresult (*TcuvidCreateVideoParser)(CUvideoparser *pObj,
                                            CUVIDPARSERPARAMS *pParams);
typedef CUresult (*TcuvidParseVideoData)(CUvideoparser obj,
                                         CUVIDSOURCEDATAPACKET *pPacket);
typedef CUresult (*TcuvidDestroyVideoParser)(CUvideoparser obj);

//
static TcuvidCtxLockCreate cuvidCtxLockCreate_ld;
static TcuvidGetDecoderCaps cuvidGetDecoderCaps_ld;
static TcuvidCreateDecoder cuvidCreateDecoder_ld;
static TcuvidDestroyDecoder cuvidDestroyDecoder_ld;
static TcuvidDecodePicture cuvidDecodePicture_ld;
static TcuvidGetDecodeStatus cuvidGetDecodeStatus_ld;
static TcuvidReconfigureDecoder cuvidReconfigureDecoder_ld;
static TcuvidMapVideoFrame64 cuvidMapVideoFrame64_ld;
static TcuvidUnmapVideoFrame64 cuvidUnmapVideoFrame64_ld;
static TcuvidCtxLockDestroy cuvidCtxLockDestroy_ld;
static TcuvidCreateVideoParser cuvidCreateVideoParser_ld;
static TcuvidParseVideoData cuvidParseVideoData_ld;
static TcuvidDestroyVideoParser cuvidDestroyVideoParser_ld;

//
typedef NVENCSTATUS (*TNvEncodeAPICreateInstance)(
    NV_ENCODE_API_FUNCTION_LIST *functionList);
typedef NVENCSTATUS (*TNvEncodeAPIGetMaxSupportedVersion)(uint32_t *version);

//
static TNvEncodeAPICreateInstance NvEncodeAPICreateInstance_ld;
static TNvEncodeAPIGetMaxSupportedVersion NvEncodeAPIGetMaxSupportedVersion_ld;

static int InitNvCodecApi() {
  // Load library
  HMODULE nvcuda_dll = LoadLibrary(TEXT("nvcuda.dll"));
  if (nvcuda_dll == NULL) {
    std::cerr << "Unable to load nvcuda.dll!" << std::endl;
    return -1;
  }

  cuInit_ld = (TcuInit)GetProcAddress(nvcuda_dll, "cuInit");
  if (cuInit_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuDeviceGet_ld = (TcuDeviceGet)GetProcAddress(nvcuda_dll, "cuDeviceGet");
  if (cuDeviceGet_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuDeviceGetCount_ld =
      (TcuDeviceGetCount)GetProcAddress(nvcuda_dll, "cuDeviceGetCount");
  if (cuDeviceGetCount_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuCtxCreate_ld = (TcuCtxCreate)GetProcAddress(nvcuda_dll, "cuCtxCreate");
  if (cuCtxCreate_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuGetErrorName_ld =
      (TcuGetErrorName)GetProcAddress(nvcuda_dll, "cuGetErrorName");
  if (cuGetErrorName_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuCtxPushCurrent_ld =
      (TcuCtxPushCurrent)GetProcAddress(nvcuda_dll, "cuCtxPushCurrent");
  if (cuCtxPushCurrent_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuCtxPopCurrent_ld =
      (TcuCtxPopCurrent)GetProcAddress(nvcuda_dll, "cuCtxPopCurrent");
  if (cuCtxPopCurrent_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }
  cuMemAlloc_ld = (TcuMemAlloc)GetProcAddress(nvcuda_dll, "cuMemAlloc");
  if (cuMemAlloc_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuMemAllocPitch_ld =
      (TcuMemAllocPitch)GetProcAddress(nvcuda_dll, "cuMemAllocPitch");
  if (cuMemAllocPitch_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuMemFree_ld = (TcuMemFree)GetProcAddress(nvcuda_dll, "cuMemFree");
  if (cuMemFree_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuMemcpy2DAsync_ld =
      (TcuMemcpy2DAsync)GetProcAddress(nvcuda_dll, "cuMemcpy2DAsync");
  if (cuMemcpy2DAsync_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuStreamSynchronize_ld =
      (TcuStreamSynchronize)GetProcAddress(nvcuda_dll, "cuStreamSynchronize");
  if (cuStreamSynchronize_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuMemcpy2D_ld = (TcuMemcpy2D)GetProcAddress(nvcuda_dll, "cuMemcpy2D");
  if (cuMemcpy2D_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuMemcpy2DUnaligned_ld =
      (TcuMemcpy2DUnaligned)GetProcAddress(nvcuda_dll, "cuMemcpy2DUnaligned");
  if (cuMemcpy2DUnaligned_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  //
  HMODULE nvcuvid_dll = LoadLibrary(TEXT("nvcuvid.dll"));
  if (nvcuvid_dll == NULL) {
    std::cerr << "Unable to load nvcuvid.dll!" << std::endl;
    return -1;
  }

  cuvidCtxLockCreate_ld =
      (TcuvidCtxLockCreate)GetProcAddress(nvcuda_dll, "cuvidCtxLockCreate");
  if (cuvidCtxLockCreate_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidGetDecoderCaps_ld =
      (TcuvidGetDecoderCaps)GetProcAddress(nvcuda_dll, "cuvidGetDecoderCaps");
  if (cuvidGetDecoderCaps_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidCreateDecoder_ld =
      (TcuvidCreateDecoder)GetProcAddress(nvcuda_dll, "cuvidCreateDecoder");
  if (cuvidCreateDecoder_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidDestroyDecoder_ld =
      (TcuvidDestroyDecoder)GetProcAddress(nvcuda_dll, "cuvidDestroyDecoder");
  if (cuvidDestroyDecoder_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidDecodePicture_ld =
      (TcuvidDecodePicture)GetProcAddress(nvcuda_dll, "cuvidDecodePicture");
  if (cuvidDecodePicture_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidGetDecodeStatus_ld =
      (TcuvidGetDecodeStatus)GetProcAddress(nvcuda_dll, "cuvidGetDecodeStatus");
  if (cuvidGetDecodeStatus_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidReconfigureDecoder_ld = (TcuvidReconfigureDecoder)GetProcAddress(
      nvcuda_dll, "cuvidReconfigureDecoder");
  if (cuvidReconfigureDecoder_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidMapVideoFrame64_ld =
      (TcuvidMapVideoFrame64)GetProcAddress(nvcuda_dll, "cuvidMapVideoFrame64");
  if (cuvidMapVideoFrame64_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidUnmapVideoFrame64_ld = (TcuvidUnmapVideoFrame64)GetProcAddress(
      nvcuda_dll, "cuvidUnmapVideoFrame64");
  if (cuvidUnmapVideoFrame64_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidCtxLockDestroy_ld =
      (TcuvidCtxLockDestroy)GetProcAddress(nvcuda_dll, "cuvidCtxLockDestroy");
  if (cuvidCtxLockDestroy_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidCreateVideoParser_ld = (TcuvidCreateVideoParser)GetProcAddress(
      nvcuda_dll, "cuvidCreateVideoParser");
  if (cuvidCreateVideoParser_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidParseVideoData_ld =
      (TcuvidParseVideoData)GetProcAddress(nvcuda_dll, "cuvidParseVideoData");
  if (cuvidParseVideoData_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  cuvidDestroyVideoParser_ld = (TcuvidDestroyVideoParser)GetProcAddress(
      nvcuda_dll, "cuvidDestroyVideoParser");
  if (cuvidDestroyVideoParser_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  //
  HMODULE nvEncodeAPI64_dll = LoadLibrary(TEXT("nvEncodeAPI64.dll"));
  if (nvEncodeAPI64_dll == NULL) {
    std::cerr << "Unable to load nvEncodeAPI64.dll!" << std::endl;
    return -1;
  }

  NvEncodeAPICreateInstance_ld = (TNvEncodeAPICreateInstance)GetProcAddress(
      nvcuda_dll, "NvEncodeAPICreateInstance");
  if (NvEncodeAPICreateInstance_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  NvEncodeAPIGetMaxSupportedVersion_ld =
      (TNvEncodeAPIGetMaxSupportedVersion)GetProcAddress(
          nvcuda_dll, "NvEncodeAPIGetMaxSupportedVersion");
  if (NvEncodeAPIGetMaxSupportedVersion_ld == NULL) {
    std::cerr << "Unable to find function!" << std::endl;
    FreeLibrary(nvcuda_dll);
    return -1;
  }

  return 0;
}

#endif