/*
*  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_VIDEO_CAPTURE_SINK_WINRT_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_VIDEO_CAPTURE_SINK_WINRT_H_

#include <wrl/implements.h>

#include <mfidl.h>

#include <windows.media.h>

#include <assert.h>
#include <queue>

namespace webrtc {
class CriticalSectionWrapper;
namespace videocapturemodule {
class VideoCaptureMediaSinkWinRT;

private ref class MediaSampleEventArgs sealed
{
internal:
  MediaSampleEventArgs(Microsoft::WRL::ComPtr<IMFSample> spMediaSample) :
      _spMediaSample(spMediaSample) { }

  Microsoft::WRL::ComPtr<IMFSample> GetMediaSample() {
    return _spMediaSample;
  }

private:
  Microsoft::WRL::ComPtr<IMFSample> _spMediaSample;
};

interface class ISinkCallback
{
  void OnSample(MediaSampleEventArgs^ args);
  void OnShutdown();
};

class VideoCaptureStreamSinkWinRT :
    public IMFStreamSink,
    public IMFMediaTypeHandler {

  // State enum: Defines the current state of the stream.
  enum State {
    State_TypeNotSet = 0,    // No media type is set
    State_Ready,             // Media type is set, Start has never been called.
    State_Started,
    State_Stopped,
    State_Paused,
    State_Count              // Number of states
  };

  // StreamOperation: Defines various operations that can
  // be performed on the stream.
  enum StreamOperation {
    OpSetMediaType = 0,
    OpStart,
    OpRestart,
    OpPause,
    OpStop,
    OpProcessSample,
    Op_Count                // Number of operations
  };

  template<class T>
  class AsyncCallback : public IMFAsyncCallback {
   public:
    typedef HRESULT(T::*InvokeFn)(IMFAsyncResult *pAsyncResult);

    AsyncCallback(T *pParent, InvokeFn fn) : _pParent(pParent), _pInvokeFn(fn) {
    }

    // IUnknown
    STDMETHODIMP_(ULONG) AddRef() {
      // Delegate to parent class.
      return _pParent->AddRef();
    }
    STDMETHODIMP_(ULONG) Release() {
      // Delegate to parent class.
      return _pParent->Release();
    }
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv) {
      if (!ppv) {
        return E_POINTER;
      }
      if (iid == __uuidof(IUnknown)) {
        *ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
      } else if (iid == __uuidof(IMFAsyncCallback)) {
        *ppv = static_cast<IMFAsyncCallback*>(this);
      } else {
        *ppv = NULL;
        return E_NOINTERFACE;
      }
      AddRef();
      return S_OK;
    }

    // IMFAsyncCallback methods
    STDMETHODIMP GetParameters(DWORD*, DWORD*) {
      // Implementation of this method is optional.
      return E_NOTIMPL;
    }

    STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult) {
      return (_pParent->*_pInvokeFn)(pAsyncResult);
    }

    T *_pParent;
    InvokeFn _pInvokeFn;
  };

  // AsyncOperation:
  // Used to queue asynchronous operations. When we call MFPutWorkItem, we use this
  // object for the callback state (pState). Then, when the callback is invoked,
  // we can use the object to determine which asynchronous operation to perform.
  class AsyncOperation : public IUnknown {
   public:
    explicit AsyncOperation(StreamOperation op);

    StreamOperation m_op;   // The operation to perform.

    // IUnknown methods.
    STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

   private:
    ULONG _cRef;
    virtual ~AsyncOperation();
  };

 public:
  // IUnknown
  IFACEMETHOD(QueryInterface) (REFIID riid, void **ppv);
  IFACEMETHOD_(ULONG, AddRef) ();
  IFACEMETHOD_(ULONG, Release) ();

  // IMFMediaEventGenerator
  IFACEMETHOD(BeginGetEvent)(IMFAsyncCallback *pCallback, IUnknown *punkState);
  IFACEMETHOD(EndGetEvent) (IMFAsyncResult *pResult, IMFMediaEvent **ppEvent);
  IFACEMETHOD(GetEvent) (DWORD dwFlags, IMFMediaEvent **ppEvent);
  IFACEMETHOD(QueueEvent) (
      MediaEventType met,
      REFGUID guidExtendedType,
      HRESULT hrStatus,
      PROPVARIANT const *pvValue);

  // IMFStreamSink
  IFACEMETHOD(GetMediaSink) (IMFMediaSink **ppMediaSink);
  IFACEMETHOD(GetIdentifier) (DWORD *pdwIdentifier);
  IFACEMETHOD(GetMediaTypeHandler) (IMFMediaTypeHandler **ppHandler);
  IFACEMETHOD(ProcessSample) (IMFSample *pSample);

  IFACEMETHOD(PlaceMarker) (
    /* [in] */ MFSTREAMSINK_MARKER_TYPE eMarkerType,
    /* [in] */ PROPVARIANT const *pvarMarkerValue,
    /* [in] */ PROPVARIANT const *pvarContextValue);

  IFACEMETHOD(Flush)();

  // IMFMediaTypeHandler
  IFACEMETHOD(IsMediaTypeSupported) (
      IMFMediaType *pMediaType,
      IMFMediaType **ppMediaType);
  IFACEMETHOD(GetMediaTypeCount) (DWORD *pdwTypeCount);
  IFACEMETHOD(GetMediaTypeByIndex) (DWORD dwIndex, IMFMediaType **ppType);
  IFACEMETHOD(SetCurrentMediaType) (IMFMediaType *pMediaType);
  IFACEMETHOD(GetCurrentMediaType) (IMFMediaType **ppMediaType);
  IFACEMETHOD(GetMajorType) (GUID *pguidMajorType);

  // ValidStateMatrix: Defines a look-up table that says which operations
  // are valid from which states.
  static BOOL ValidStateMatrix[State_Count][Op_Count];

  explicit VideoCaptureStreamSinkWinRT(DWORD dwIdentifier);
  virtual ~VideoCaptureStreamSinkWinRT();

  HRESULT Initialize(VideoCaptureMediaSinkWinRT *pParent, ISinkCallback ^callback);

  HRESULT Start(MFTIME start);
  HRESULT Restart();
  HRESULT Stop();
  HRESULT Pause();
  HRESULT Shutdown();

 private:
  HRESULT ValidateOperation(StreamOperation op);

  HRESULT QueueAsyncOperation(StreamOperation op);

  HRESULT OnDispatchWorkItem(IMFAsyncResult *pAsyncResult);
  void DispatchProcessSample(AsyncOperation *pOp);

  bool DropSamplesFromQueue();
  bool SendSampleFromQueue();
  bool ProcessSamplesFromQueue(bool fFlush);
  void ProcessFormatChange(IMFMediaType *pMediaType);

  void HandleError(HRESULT hr);

 private:
  ULONG _cRef;
  CriticalSectionWrapper* _critSec;

  DWORD _dwIdentifier;
  State _state;
  bool _isShutdown;
  bool _fGetStartTimeFromSample;
  GUID _guiCurrentSubtype;

  DWORD _workQueueId;
  MFTIME _startTime;

  Microsoft::WRL::ComPtr<IMFMediaSink> _spSink;
  VideoCaptureMediaSinkWinRT* _pParent;

  Microsoft::WRL::ComPtr<IMFMediaEventQueue> _spEventQueue;
  Microsoft::WRL::ComPtr<IMFByteStream> _spByteStream;
  Microsoft::WRL::ComPtr<IMFMediaType> _spCurrentType;

  std::queue<Microsoft::WRL::ComPtr<IUnknown> > _sampleQueue;

  ISinkCallback^ _callback;
  AsyncCallback<VideoCaptureStreamSinkWinRT>  _workQueueCB;
};

class VideoCaptureMediaSinkWinRT
    : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<
             Microsoft::WRL::RuntimeClassType::WinRtClassicComMix >,
        ABI::Windows::Media::IMediaExtension,
        IMFMediaSink,
        IMFClockStateSink> {
 public:
  VideoCaptureMediaSinkWinRT();
  ~VideoCaptureMediaSinkWinRT();

  HRESULT RuntimeClassInitialize(
      ISinkCallback ^callback,
      Windows::Media::MediaProperties::IMediaEncodingProperties ^encodingProperties);

  // IMediaExtension
  IFACEMETHOD(SetProperties) (
      ABI::Windows::Foundation::Collections::IPropertySet *pConfiguration) {
    return S_OK;
  }

  // IMFMediaSink methods
  IFACEMETHOD(GetCharacteristics) (DWORD *pdwCharacteristics);

  IFACEMETHOD(AddStreamSink)(
    /* [in] */ DWORD dwStreamSinkIdentifier,
    /* [in] */ IMFMediaType *pMediaType,
    /* [out] */ IMFStreamSink **ppStreamSink);

  IFACEMETHOD(RemoveStreamSink) (DWORD dwStreamSinkIdentifier);
  IFACEMETHOD(GetStreamSinkCount) (_Out_ DWORD *pcStreamSinkCount);
  IFACEMETHOD(GetStreamSinkByIndex) (
      DWORD dwIndex,
      _Outptr_ IMFStreamSink **ppStreamSink);
  IFACEMETHOD(GetStreamSinkById) (
      DWORD dwIdentifier,
      IMFStreamSink **ppStreamSink);
  IFACEMETHOD(SetPresentationClock) (
      IMFPresentationClock *pPresentationClock);
  IFACEMETHOD(GetPresentationClock) (
      IMFPresentationClock **ppPresentationClock);
  IFACEMETHOD(Shutdown) ();

  // IMFClockStateSink methods
  IFACEMETHOD(OnClockStart) (MFTIME hnsSystemTime, LONGLONG llClockStartOffset);
  IFACEMETHOD(OnClockStop) (MFTIME hnsSystemTime);
  IFACEMETHOD(OnClockPause) (MFTIME hnsSystemTime);
  IFACEMETHOD(OnClockRestart) (MFTIME hnsSystemTime);
  IFACEMETHOD(OnClockSetRate) (MFTIME hnsSystemTime, float flRate);

 private:
  ULONG _cRef;
  CriticalSectionWrapper* _critSec;
  bool _isShutdown;
  bool _isConnected;
  LONGLONG _llStartTime;

  ISinkCallback^ _callback;

  Microsoft::WRL::ComPtr<IMFStreamSink> _spStreamSink;
  Microsoft::WRL::ComPtr<IMFPresentationClock> _spClock;
};

private ref class VideoCaptureMediaSinkProxyWinRT sealed
{
 public:
  VideoCaptureMediaSinkProxyWinRT();
  virtual ~VideoCaptureMediaSinkProxyWinRT();

  Windows::Media::IMediaExtension^ GetMFExtension();

  Windows::Foundation::IAsyncOperation<Windows::Media::IMediaExtension^>^ 
      InitializeAsync(Windows::Media::MediaProperties::IMediaEncodingProperties ^encodingProperties);

  event Windows::Foundation::EventHandler<MediaSampleEventArgs^>^ MediaSampleEvent;

 private:
  ref class VideoCaptureSinkCallback sealed : ISinkCallback
  {
   public:
    virtual void OnSample(MediaSampleEventArgs^ args)
    {
      _parent->OnSample(args);
    }

    virtual void OnShutdown()
    {
      _parent->OnShutdown();
    }

   internal:
    VideoCaptureSinkCallback(VideoCaptureMediaSinkProxyWinRT ^parent)
      : _parent(parent)
    {
    }

   private:
    VideoCaptureMediaSinkProxyWinRT^ _parent;
  };

  void OnSample(MediaSampleEventArgs^ args);

  void OnShutdown();

  void CheckShutdown();

 private:
  CriticalSectionWrapper* _critSec;
  Microsoft::WRL::ComPtr<IMFMediaSink> _mediaSink;
  bool _shutdown;
};

}  // namespace videocapturemodule
}  // namespace webrtc

#endif  // WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_VIDEO_CAPTURE_SINK_WINRT_H_
