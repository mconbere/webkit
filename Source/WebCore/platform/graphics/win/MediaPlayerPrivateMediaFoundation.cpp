/*
 * Copyright (C) 2014 Alex Christensen <achristensen@webkit.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MediaPlayerPrivateMediaFoundation.h"

#include "CachedResourceLoader.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HWndDC.h"
#include "HostWindow.h"
#include "NotImplemented.h"
#if USE(CAIRO)
#include "PlatformContextCairo.h"
#endif
#include "SoftLinking.h"

#if USE(MEDIA_FOUNDATION)

#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>

SOFT_LINK_LIBRARY(Mf);
SOFT_LINK_OPTIONAL(Mf, MFCreateSourceResolver, HRESULT, STDAPICALLTYPE, (IMFSourceResolver**));
SOFT_LINK_OPTIONAL(Mf, MFCreateMediaSession, HRESULT, STDAPICALLTYPE, (IMFAttributes*, IMFMediaSession**));
SOFT_LINK_OPTIONAL(Mf, MFCreateTopology, HRESULT, STDAPICALLTYPE, (IMFTopology**));
SOFT_LINK_OPTIONAL(Mf, MFCreateTopologyNode, HRESULT, STDAPICALLTYPE, (MF_TOPOLOGY_TYPE, IMFTopologyNode**));
SOFT_LINK_OPTIONAL(Mf, MFGetService, HRESULT, STDAPICALLTYPE, (IUnknown*, REFGUID, REFIID, LPVOID*));
SOFT_LINK_OPTIONAL(Mf, MFCreateAudioRendererActivate, HRESULT, STDAPICALLTYPE, (IMFActivate**));
SOFT_LINK_OPTIONAL(Mf, MFCreateVideoRendererActivate, HRESULT, STDAPICALLTYPE, (HWND, IMFActivate**));
SOFT_LINK_OPTIONAL(Mf, MFCreateSampleGrabberSinkActivate, HRESULT, STDAPICALLTYPE, (IMFMediaType*, IMFSampleGrabberSinkCallback*, IMFActivate**));
SOFT_LINK_OPTIONAL(Mf, MFGetSupportedMimeTypes, HRESULT, STDAPICALLTYPE, (PROPVARIANT*));

SOFT_LINK_LIBRARY(Mfplat);
SOFT_LINK_OPTIONAL(Mfplat, MFStartup, HRESULT, STDAPICALLTYPE, (ULONG, DWORD));
SOFT_LINK_OPTIONAL(Mfplat, MFShutdown, HRESULT, STDAPICALLTYPE, ());
SOFT_LINK_OPTIONAL(Mfplat, MFCreateMemoryBuffer, HRESULT, STDAPICALLTYPE, (DWORD, IMFMediaBuffer**));
SOFT_LINK_OPTIONAL(Mfplat, MFCreateSample, HRESULT, STDAPICALLTYPE, (IMFSample**));
SOFT_LINK_OPTIONAL(Mfplat, MFCreateMediaType, HRESULT, STDAPICALLTYPE, (IMFMediaType**));
SOFT_LINK_OPTIONAL(Mfplat, MFFrameRateToAverageTimePerFrame, HRESULT, STDAPICALLTYPE, (UINT32, UINT32, UINT64*));

SOFT_LINK_LIBRARY(evr);
SOFT_LINK_OPTIONAL(evr, MFCreateVideoSampleFromSurface, HRESULT, STDAPICALLTYPE, (IUnknown*, IMFSample**));

SOFT_LINK_LIBRARY(Dxva2);
SOFT_LINK_OPTIONAL(Dxva2, DXVA2CreateDirect3DDeviceManager9, HRESULT, STDAPICALLTYPE, (UINT*, IDirect3DDeviceManager9**));

SOFT_LINK_LIBRARY(D3d9);
SOFT_LINK_OPTIONAL(D3d9, Direct3DCreate9Ex, HRESULT, STDAPICALLTYPE, (UINT, IDirect3D9Ex**));

// MFSamplePresenterSampleCounter
// Data type: UINT32
//
// Version number for the video samples. When the presenter increments the version
// number, all samples with the previous version number are stale and should be
// discarded.
static const GUID MFSamplePresenterSampleCounter =
{ 0x869f1f7c, 0x3496, 0x48a9, { 0x88, 0xe3, 0x69, 0x85, 0x79, 0xd0, 0x8c, 0xb6 } };

static const double tenMegahertz = 10000000;

namespace WebCore {

MediaPlayerPrivateMediaFoundation::MediaPlayerPrivateMediaFoundation(MediaPlayer* player) 
    : m_player(player)
    , m_visible(false)
    , m_loadingProgress(false)
    , m_paused(false)
    , m_hasAudio(false)
    , m_hasVideo(false)
    , m_hwndVideo(nullptr)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_weakPtrFactory(this)
{
    createSession();
    createVideoWindow();
}

MediaPlayerPrivateMediaFoundation::~MediaPlayerPrivateMediaFoundation()
{
    notifyDeleted();
    destroyVideoWindow();
    endSession();
}

void MediaPlayerPrivateMediaFoundation::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable()) {
        registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivateMediaFoundation>(player); },
            getSupportedTypes, supportsType, 0, 0, 0, 0);
    }
}

bool MediaPlayerPrivateMediaFoundation::isAvailable() 
{
    notImplemented();
    return true;
}

static const HashSet<String, ASCIICaseInsensitiveHash>& mimeTypeCache()
{
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> cachedTypes;

    if (cachedTypes.get().size() > 0)
        return cachedTypes;

    cachedTypes.get().add(String("video/mp4"));

    if (!MFGetSupportedMimeTypesPtr())
        return cachedTypes;

    PROPVARIANT propVarMimeTypeArray;
    PropVariantInit(&propVarMimeTypeArray);

    HRESULT hr = MFGetSupportedMimeTypesPtr()(&propVarMimeTypeArray);

    if (SUCCEEDED(hr)) {
        CALPWSTR mimeTypeArray = propVarMimeTypeArray.calpwstr;
        for (unsigned i = 0; i < mimeTypeArray.cElems; i++)
            cachedTypes.get().add(mimeTypeArray.pElems[i]);
    }

    PropVariantClear(&propVarMimeTypeArray);

    return cachedTypes;
}

void MediaPlayerPrivateMediaFoundation::getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types)
{
    types = mimeTypeCache();
}

MediaPlayer::SupportsType MediaPlayerPrivateMediaFoundation::supportsType(const MediaEngineSupportParameters& parameters)
{
    if (parameters.type.isNull() || parameters.type.isEmpty())
        return MediaPlayer::IsNotSupported;

    if (mimeTypeCache().contains(parameters.type))
        return MediaPlayer::IsSupported;

    return MediaPlayer::IsNotSupported;
}

void MediaPlayerPrivateMediaFoundation::load(const String& url)
{
    startCreateMediaSource(url);
}

void MediaPlayerPrivateMediaFoundation::cancelLoad()
{
    notImplemented();
}

void MediaPlayerPrivateMediaFoundation::play()
{
    if (!m_mediaSession)
        return;

    PROPVARIANT varStart;
    PropVariantInit(&varStart);
    varStart.vt = VT_EMPTY;

    HRESULT hr = m_mediaSession->Start(nullptr, &varStart);
    ASSERT(SUCCEEDED(hr));

    PropVariantClear(&varStart);

    m_paused = !SUCCEEDED(hr);
}

void MediaPlayerPrivateMediaFoundation::pause()
{
    if (!m_mediaSession)
        return;

    m_paused = SUCCEEDED(m_mediaSession->Pause());
}

bool MediaPlayerPrivateMediaFoundation::supportsFullscreen() const
{
    return true;
}

FloatSize MediaPlayerPrivateMediaFoundation::naturalSize() const 
{
    return m_size;
}

bool MediaPlayerPrivateMediaFoundation::hasVideo() const
{
    return m_hasVideo;
}

bool MediaPlayerPrivateMediaFoundation::hasAudio() const
{
    return m_hasAudio;
}

void MediaPlayerPrivateMediaFoundation::setVisible(bool visible)
{
    m_visible = visible;
}

bool MediaPlayerPrivateMediaFoundation::seeking() const
{
    // We assume seeking is immediately complete.
    return false;
}

void MediaPlayerPrivateMediaFoundation::seekDouble(double time)
{
    PROPVARIANT propVariant;
    PropVariantInit(&propVariant);
    propVariant.vt = VT_I8;
    propVariant.hVal.QuadPart = static_cast<__int64>(time * tenMegahertz);
    
    HRESULT hr = m_mediaSession->Start(&GUID_NULL, &propVariant);
    ASSERT(SUCCEEDED(hr));
    PropVariantClear(&propVariant);

    m_player->timeChanged();
}

void MediaPlayerPrivateMediaFoundation::setRateDouble(double rate)
{
    COMPtr<IMFRateControl> rateControl;

    HRESULT hr = MFGetServicePtr()(m_mediaSession.get(), MF_RATE_CONTROL_SERVICE, IID_IMFRateControl, (void**)&rateControl);

    if (!SUCCEEDED(hr))
        return;

    BOOL reduceSamplesInStream = rate > 2.0;

    rateControl->SetRate(reduceSamplesInStream, rate);
}

double MediaPlayerPrivateMediaFoundation::durationDouble() const
{
    if (!m_mediaSource)
        return 0;

    IMFPresentationDescriptor* descriptor;
    if (!SUCCEEDED(m_mediaSource->CreatePresentationDescriptor(&descriptor)))
        return 0;
    
    UINT64 duration;
    if (!SUCCEEDED(descriptor->GetUINT64(MF_PD_DURATION, &duration)))
        duration = 0;
    descriptor->Release();
    
    return static_cast<double>(duration) / tenMegahertz;
}

float MediaPlayerPrivateMediaFoundation::currentTime() const
{
    if (!m_presenter)
        return 0.0f;

    return m_presenter->currentTime();
}

bool MediaPlayerPrivateMediaFoundation::paused() const
{
    return m_paused;
}

MediaPlayer::NetworkState MediaPlayerPrivateMediaFoundation::networkState() const
{ 
    notImplemented();
    return MediaPlayer::Empty;
}

MediaPlayer::ReadyState MediaPlayerPrivateMediaFoundation::readyState() const
{
    return m_readyState;
}

float MediaPlayerPrivateMediaFoundation::maxTimeSeekable() const
{
    return durationDouble();
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateMediaFoundation::buffered() const
{ 
    auto ranges = std::make_unique<PlatformTimeRanges>();
    if (m_presenter && m_presenter->maxTimeLoaded() > 0)
        ranges->add(MediaTime::zeroTime(), MediaTime::createWithDouble(m_presenter->maxTimeLoaded()));
    return ranges;
}

bool MediaPlayerPrivateMediaFoundation::didLoadingProgress() const
{
    return m_loadingProgress;
}

void MediaPlayerPrivateMediaFoundation::setSize(const IntSize& size)
{
    m_size = size;

    if (!m_videoDisplay)
        return;

    IntPoint positionInWindow(m_lastPaintRect.location());

    FrameView* view = nullptr;
    float deviceScaleFactor = 1.0f;
    if (m_player && m_player->cachedResourceLoader() && m_player->cachedResourceLoader()->document()) {
        view = m_player->cachedResourceLoader()->document()->view();
        deviceScaleFactor = m_player->cachedResourceLoader()->document()->deviceScaleFactor();
    }

    LayoutPoint scrollPosition;
    if (view) {
        scrollPosition = view->scrollPositionForFixedPosition();
        positionInWindow = view->convertToContainingWindow(IntPoint(m_lastPaintRect.location()));
    }

    positionInWindow.move(-scrollPosition.x().toInt(), -scrollPosition.y().toInt());

    int x = positionInWindow.x() * deviceScaleFactor;
    int y = positionInWindow.y() * deviceScaleFactor;
    int w = m_size.width() * deviceScaleFactor;
    int h = m_size.height() * deviceScaleFactor;

    if (m_hwndVideo)
        ::MoveWindow(m_hwndVideo, x, y, w, h, FALSE);

    RECT rc = { 0, 0, w, h };
    m_videoDisplay->SetVideoPosition(nullptr, &rc);
}

void MediaPlayerPrivateMediaFoundation::paint(GraphicsContext& context, const FloatRect& rect)
{
    if (context.paintingDisabled() || !m_player->visible())
        return;

    m_lastPaintRect = rect;

    if (m_presenter)
        m_presenter->paintCurrentFrame(context, rect);
}

bool MediaPlayerPrivateMediaFoundation::createSession()
{
    if (!MFStartupPtr() || !MFCreateMediaSessionPtr())
        return false;

    if (FAILED(MFStartupPtr()(MF_VERSION, MFSTARTUP_FULL)))
        return false;

    if (FAILED(MFCreateMediaSessionPtr()(nullptr, &m_mediaSession)))
        return false;

    // Get next event.
    AsyncCallback* callback = new AsyncCallback(this, true);
    HRESULT hr = m_mediaSession->BeginGetEvent(callback, nullptr);
    ASSERT(SUCCEEDED(hr));

    return true;
}

bool MediaPlayerPrivateMediaFoundation::endSession()
{
    if (m_mediaSession) {
        m_mediaSession->Shutdown();
        m_mediaSession = nullptr;
    }

    if (!MFShutdownPtr())
        return false;

    HRESULT hr = MFShutdownPtr()();
    ASSERT(SUCCEEDED(hr));

    return true;
}

bool MediaPlayerPrivateMediaFoundation::startCreateMediaSource(const String& url)
{
    if (!MFCreateSourceResolverPtr())
        return false;

    if (FAILED(MFCreateSourceResolverPtr()(&m_sourceResolver)))
        return false;

    COMPtr<IUnknown> cancelCookie;
    Vector<UChar> urlSource = url.charactersWithNullTermination();

    AsyncCallback* callback = new AsyncCallback(this, false);

    if (FAILED(m_sourceResolver->BeginCreateObjectFromURL(urlSource.data(), MF_RESOLUTION_MEDIASOURCE, nullptr, &cancelCookie, callback, nullptr)))
        return false;

    return true;
}

bool MediaPlayerPrivateMediaFoundation::endCreatedMediaSource(IMFAsyncResult* asyncResult)
{
    MF_OBJECT_TYPE objectType;
    COMPtr<IUnknown> source;

    HRESULT hr = m_sourceResolver->EndCreateObjectFromURL(asyncResult, &objectType, &source);
    if (FAILED(hr))
        return false;

    hr = source->QueryInterface(IID_PPV_ARGS(&m_mediaSource));
    if (FAILED(hr))
        return false;

    hr = asyncResult->GetStatus();
    m_loadingProgress = SUCCEEDED(hr);

    auto weakPtr = m_weakPtrFactory.createWeakPtr();
    callOnMainThread([weakPtr] {
        if (!weakPtr)
            return;
        weakPtr->onCreatedMediaSource();
    });

    return true;
}

bool MediaPlayerPrivateMediaFoundation::endGetEvent(IMFAsyncResult* asyncResult)
{
    COMPtr<IMFMediaEvent> event;

    if (!m_mediaSession)
        return false;

    // Get the event from the event queue.
    HRESULT hr = m_mediaSession->EndGetEvent(asyncResult, &event);
    if (FAILED(hr))
        return false;

    // Get the event type.
    MediaEventType mediaEventType;
    hr = event->GetType(&mediaEventType);
    if (FAILED(hr))
        return false;

    switch (mediaEventType) {
    case MESessionTopologySet: {
        auto weakPtr = m_weakPtrFactory.createWeakPtr();
        callOnMainThread([weakPtr] {
            if (!weakPtr)
                return;
            weakPtr->onTopologySet();
        });
        break;
    }

    case MESessionClosed:
        break;

    case MEMediaSample:
        break;
    }

    if (mediaEventType != MESessionClosed) {
        // For all other events, ask the media session for the
        // next event in the queue.
        AsyncCallback* callback = new AsyncCallback(this, true);

        hr = m_mediaSession->BeginGetEvent(callback, nullptr);
        if (FAILED(hr))
            return false;
    }

    return true;
}

bool MediaPlayerPrivateMediaFoundation::createTopologyFromSource()
{
    if (!MFCreateTopologyPtr())
        return false;

    // Create a new topology.
    if (FAILED(MFCreateTopologyPtr()(&m_topology)))
        return false;

    // Create the presentation descriptor for the media source.
    if (FAILED(m_mediaSource->CreatePresentationDescriptor(&m_sourcePD)))
        return false;

    // Get the number of streams in the media source.
    DWORD sourceStreams = 0;
    if (FAILED(m_sourcePD->GetStreamDescriptorCount(&sourceStreams)))
        return false;

    // For each stream, create the topology nodes and add them to the topology.
    for (DWORD i = 0; i < sourceStreams; i++) {
        if (!addBranchToPartialTopology(i))
            return false;
    }

    return true;
}

bool MediaPlayerPrivateMediaFoundation::addBranchToPartialTopology(int stream)
{
    // Get the stream descriptor for this stream.
    COMPtr<IMFStreamDescriptor> sourceSD;
    BOOL selected = FALSE;
    if (FAILED(m_sourcePD->GetStreamDescriptorByIndex(stream, &selected, &sourceSD)))
        return false;

    // Create the topology branch only if the stream is selected.
    // Otherwise, do nothing.
    if (!selected)
        return true;

    // Create a source node for this stream.
    COMPtr<IMFTopologyNode> sourceNode;
    if (!createSourceStreamNode(sourceSD, sourceNode))
        return false;

    COMPtr<IMFTopologyNode> outputNode;
    if (!createOutputNode(sourceSD, outputNode))
        return false;

    // Add both nodes to the topology.
    if (FAILED(m_topology->AddNode(sourceNode.get())))
        return false;

    if (FAILED(m_topology->AddNode(outputNode.get())))
        return false;

    // Connect the source node to the output node.
    if (FAILED(sourceNode->ConnectOutput(0, outputNode.get(), 0)))
        return false;

    return true;
}

LRESULT CALLBACK MediaPlayerPrivateMediaFoundation::VideoViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LPCWSTR MediaPlayerPrivateMediaFoundation::registerVideoWindowClass()
{
    const LPCWSTR kVideoWindowClassName = L"WebVideoWindowClass";

    static bool haveRegisteredWindowClass = false;
    if (haveRegisteredWindowClass)
        return kVideoWindowClassName;

    haveRegisteredWindowClass = true;

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_DBLCLKS;
    wcex.lpfnWndProc = VideoViewWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = nullptr;
    wcex.hIcon = nullptr;
    wcex.hCursor = ::LoadCursor(0, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = kVideoWindowClassName;
    wcex.hIconSm = nullptr;

    if (RegisterClassEx(&wcex))
        return kVideoWindowClassName;

    return nullptr;
}

void MediaPlayerPrivateMediaFoundation::createVideoWindow()
{
    HWND hWndParent = nullptr;
    FrameView* view = nullptr;
    if (!m_player || !m_player->cachedResourceLoader() || !m_player->cachedResourceLoader()->document())
        return;
    view = m_player->cachedResourceLoader()->document()->view();
    if (!view || !view->hostWindow())
        return;
    hWndParent = view->hostWindow()->platformPageClient();

    m_hwndVideo = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TRANSPARENT, registerVideoWindowClass(), 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, 0, 0, hWndParent, 0, 0, 0);
}

void MediaPlayerPrivateMediaFoundation::destroyVideoWindow()
{
    if (m_hwndVideo) {
        DestroyWindow(m_hwndVideo);
        m_hwndVideo = nullptr;
    }
}

void MediaPlayerPrivateMediaFoundation::invalidateFrameView()
{
    FrameView* view = nullptr;
    if (!m_player || !m_player->cachedResourceLoader() || !m_player->cachedResourceLoader()->document())
        return;
    view = m_player->cachedResourceLoader()->document()->view();
    if (!view)
        return;

    view->invalidate();
}

void MediaPlayerPrivateMediaFoundation::addListener(MediaPlayerListener* listener)
{
    LockHolder locker(m_mutexListeners);

    m_listeners.add(listener);
}

void MediaPlayerPrivateMediaFoundation::removeListener(MediaPlayerListener* listener)
{
    LockHolder locker(m_mutexListeners);

    m_listeners.remove(listener);
}

void MediaPlayerPrivateMediaFoundation::notifyDeleted()
{
    LockHolder locker(m_mutexListeners);

    for (HashSet<MediaPlayerListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
        (*it)->onMediaPlayerDeleted();
}

bool MediaPlayerPrivateMediaFoundation::createOutputNode(COMPtr<IMFStreamDescriptor> sourceSD, COMPtr<IMFTopologyNode>& node)
{
    if (!MFCreateTopologyNodePtr() || !MFCreateAudioRendererActivatePtr() || !MFCreateVideoRendererActivatePtr())
        return false;

    if (!sourceSD)
        return false;

#ifndef NDEBUG
    // Get the stream ID.
    DWORD streamID = 0;
    sourceSD->GetStreamIdentifier(&streamID); // Just for debugging, ignore any failures.
#endif

    COMPtr<IMFMediaTypeHandler> handler;
    if (FAILED(sourceSD->GetMediaTypeHandler(&handler)))
        return false;

    GUID guidMajorType = GUID_NULL;
    if (FAILED(handler->GetMajorType(&guidMajorType)))
        return false;

    // Create a downstream node.
    if (FAILED(MFCreateTopologyNodePtr()(MF_TOPOLOGY_OUTPUT_NODE, &node)))
        return false;

    // Create an IMFActivate object for the renderer, based on the media type.
    COMPtr<IMFActivate> rendererActivate;
    if (MFMediaType_Audio == guidMajorType) {
        // Create the audio renderer.
        if (FAILED(MFCreateAudioRendererActivatePtr()(&rendererActivate)))
            return false;
        m_hasAudio = true;
    } else if (MFMediaType_Video == guidMajorType) {
        // Create the video renderer.
        if (FAILED(MFCreateVideoRendererActivatePtr()(nullptr, &rendererActivate)))
            return false;

        m_presenter = new CustomVideoPresenter(this);
        m_presenter->SetVideoWindow(m_hwndVideo);
        if (FAILED(rendererActivate->SetUnknown(MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_ACTIVATE, static_cast<IMFActivate*>(m_presenter.get()))))
            return false;
        m_hasVideo = true;
    } else
        return false;

    // Set the IActivate object on the output node.
    if (FAILED(node->SetObject(rendererActivate.get())))
        return false;

    return true;
}

bool MediaPlayerPrivateMediaFoundation::createSourceStreamNode(COMPtr<IMFStreamDescriptor> sourceSD, COMPtr<IMFTopologyNode>& node)
{
    if (!MFCreateTopologyNodePtr())
        return false;

    if (!m_mediaSource || !m_sourcePD || !sourceSD)
        return false;

    // Create the source-stream node.
    HRESULT hr = MFCreateTopologyNodePtr()(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    if (FAILED(hr))
        return false;

    // Set attribute: Pointer to the media source.
    hr = node->SetUnknown(MF_TOPONODE_SOURCE, m_mediaSource.get());
    if (FAILED(hr))
        return false;

    // Set attribute: Pointer to the presentation descriptor.
    hr = node->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, m_sourcePD.get());
    if (FAILED(hr))
        return false;

    // Set attribute: Pointer to the stream descriptor.
    hr = node->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, sourceSD.get());
    if (FAILED(hr))
        return false;

    return true;
}

void MediaPlayerPrivateMediaFoundation::onCreatedMediaSource()
{
    if (!createTopologyFromSource())
        return;

    // Set the topology on the media session.
    HRESULT hr = m_mediaSession->SetTopology(0, m_topology.get());
    ASSERT(SUCCEEDED(hr));
}

void MediaPlayerPrivateMediaFoundation::onTopologySet()
{
    if (!MFGetServicePtr())
        return;

    if (SUCCEEDED(MFGetServicePtr()(m_mediaSession.get(), MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_videoDisplay)))) {
        ASSERT(m_videoDisplay);
        RECT rc = { 0, 0, m_size.width(), m_size.height() };
        m_videoDisplay->SetVideoPosition(nullptr, &rc);
    }

    m_readyState = MediaPlayer::HaveFutureData;

    ASSERT(m_player);
    m_player->readyStateChanged();

    play();
    m_player->playbackStateChanged();
}

MediaPlayerPrivateMediaFoundation::AsyncCallback::AsyncCallback(MediaPlayerPrivateMediaFoundation* mediaPlayer, bool event)
    : m_refCount(0)
    , m_mediaPlayer(mediaPlayer)
    , m_event(event)
{
    if (m_mediaPlayer)
        m_mediaPlayer->addListener(this);
}

MediaPlayerPrivateMediaFoundation::AsyncCallback::~AsyncCallback()
{
    if (m_mediaPlayer)
        m_mediaPlayer->removeListener(this);
}

HRESULT MediaPlayerPrivateMediaFoundation::AsyncCallback::QueryInterface(_In_ REFIID riid, __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (!IsEqualGUID(riid, IID_IMFAsyncCallback)) {
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    *ppvObject = this;
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE MediaPlayerPrivateMediaFoundation::AsyncCallback::AddRef()
{
    m_refCount++;
    return m_refCount;
}

ULONG STDMETHODCALLTYPE MediaPlayerPrivateMediaFoundation::AsyncCallback::Release()
{
    m_refCount--;
    ULONG refCount = m_refCount;
    if (!refCount)
        delete this;
    return refCount;
}

HRESULT STDMETHODCALLTYPE MediaPlayerPrivateMediaFoundation::AsyncCallback::GetParameters(__RPC__out DWORD *pdwFlags, __RPC__out DWORD *pdwQueue)
{
    // Returning E_NOTIMPL gives default values.
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MediaPlayerPrivateMediaFoundation::AsyncCallback::Invoke(__RPC__in_opt IMFAsyncResult *pAsyncResult)
{
    LockHolder locker(m_mutex);

    if (!m_mediaPlayer)
        return S_OK;

    if (m_event)
        m_mediaPlayer->endGetEvent(pAsyncResult);
    else
        m_mediaPlayer->endCreatedMediaSource(pAsyncResult);

    return S_OK;
}

void MediaPlayerPrivateMediaFoundation::AsyncCallback::onMediaPlayerDeleted()
{
    LockHolder locker(m_mutex);

    m_mediaPlayer = nullptr;
}

MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::CustomVideoPresenter(MediaPlayerPrivateMediaFoundation* mediaPlayer)
    : m_mediaPlayer(mediaPlayer)
{
    if (m_mediaPlayer)
        m_mediaPlayer->addListener(this);

    m_sourceRect.top = 0;
    m_sourceRect.left = 0;
    m_sourceRect.bottom = 1;
    m_sourceRect.right = 1;

    m_presenterEngine = std::make_unique<Direct3DPresenter>();
    if (!m_presenterEngine)
        return;

    m_scheduler.setPresenter(m_presenterEngine.get());
}

MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::~CustomVideoPresenter()
{
    if (m_mediaPlayer)
        m_mediaPlayer->removeListener(this);
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::QueryInterface(REFIID riid, __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = nullptr;
    if (IsEqualGUID(riid, IID_IMFGetService))
        *ppvObject = static_cast<IMFGetService*>(this);
    else if (IsEqualGUID(riid, IID_IMFActivate))
        *ppvObject = static_cast<IMFActivate*>(this);
    else if (IsEqualGUID(riid, IID_IMFVideoDisplayControl))
        *ppvObject = static_cast<IMFVideoDisplayControl*>(this);
    else if (IsEqualGUID(riid, IID_IMFVideoPresenter))
        *ppvObject = static_cast<IMFVideoPresenter*>(this);
    else if (IsEqualGUID(riid, IID_IMFClockStateSink))
        *ppvObject = static_cast<IMFClockStateSink*>(this);
    else if (IsEqualGUID(riid, IID_IMFVideoDeviceID))
        *ppvObject = static_cast<IMFVideoDeviceID*>(this);
    else if (IsEqualGUID(riid, IID_IMFTopologyServiceLookupClient))
        *ppvObject = static_cast<IMFTopologyServiceLookupClient*>(this);
    else if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IMFVideoPresenter*>(this);
    else if (IsEqualGUID(riid, IID_IMFAsyncCallback))
        *ppvObject = static_cast<IMFAsyncCallback*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::AddRef()
{
    m_refCount++;
    return m_refCount;
}

ULONG MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::Release()
{
    m_refCount--;
    ULONG refCount = m_refCount;
    if (!refCount)
        delete this;
    return refCount;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
    LockHolder locker(m_lock);

    // After shutdown, we cannot start.
    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    m_renderState = RenderStateStarted;

    if (isActive()) {
        if (llClockStartOffset != PRESENTATION_CURRENT_POSITION) {
            // This is a seek request, flush pending samples.
            flush();
        }
    }

    processOutputLoop();

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::OnClockStop(MFTIME hnsSystemTime)
{
    LockHolder locker(m_lock);

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    if (m_renderState != RenderStateStopped) {
        m_renderState = RenderStateStopped;
        flush();
    }

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::OnClockPause(MFTIME hnsSystemTime)
{
    LockHolder locker(m_lock);

    // After shutdown, we cannot pause.
    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    m_renderState = RenderStatePaused;

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
    LockHolder locker(m_lock);

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    ASSERT(m_renderState == RenderStatePaused);

    m_renderState = RenderStateStarted;

    processOutputLoop();

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::OnClockSetRate(MFTIME hnsSystemTime, float rate)
{
    LockHolder locker(m_lock);

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    m_rate = rate;

    m_scheduler.setClockRate(rate);

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
    LockHolder locker(m_lock);

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    switch (eMessage) {
    case MFVP_MESSAGE_FLUSH:
        hr = flush();
        break;

    case MFVP_MESSAGE_INVALIDATEMEDIATYPE:
        hr = renegotiateMediaType();
        break;

    case MFVP_MESSAGE_PROCESSINPUTNOTIFY:
        // A new input sample is available. 
        hr = processInputNotify();
        break;

    case MFVP_MESSAGE_BEGINSTREAMING:
        hr = beginStreaming();
        break;

    case MFVP_MESSAGE_ENDSTREAMING:
        hr = endStreaming();
        break;

    case MFVP_MESSAGE_ENDOFSTREAM:
        m_endStreaming = true;
        hr = checkEndOfStream();
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::GetCurrentMediaType(_Outptr_  IMFVideoMediaType **ppMediaType)
{
    LockHolder locker(m_lock);

    if (!ppMediaType)
        return E_POINTER;

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    if (!m_mediaType)
        return MF_E_NOT_INITIALIZED;

    return m_mediaType->QueryInterface(__uuidof(IMFVideoMediaType), (void**)&ppMediaType);
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::GetDeviceID(IID* pDeviceID)
{
    if (!pDeviceID)
        return E_POINTER;

    *pDeviceID = __uuidof(IDirect3DDevice9);
    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::InitServicePointers(IMFTopologyServiceLookup *pLookup)
{
    if (!pLookup)
        return E_POINTER;

    HRESULT hr = S_OK;

    LockHolder locker(m_lock);

    if (isActive())
        return MF_E_INVALIDREQUEST;

    m_clock = nullptr;
    m_mixer = nullptr;
    m_mediaEventSink = nullptr;

    // Lookup the services.

    DWORD objectCount = 1;
    hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_clock), &objectCount);
    // The clock service is optional.

    objectCount = 1;
    hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&m_mixer), &objectCount);
    if (FAILED(hr))
        return hr;

    hr = configureMixer(m_mixer.get());
    if (FAILED(hr))
        return hr;

    objectCount = 1;
    hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_mediaEventSink), &objectCount);
    if (FAILED(hr))
        return hr;

    m_renderState = RenderStateStopped;

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::ReleaseServicePointers()
{
    LockHolder locker(m_lock);

    m_renderState = RenderStateShutdown;

    flush();

    setMediaType(nullptr);

    m_clock = nullptr;
    m_mixer = nullptr;
    m_mediaEventSink = nullptr;

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::GetService(REFGUID guidService, REFIID riid, LPVOID* ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    // We only support MR_VIDEO_RENDER_SERVICE.
    if (guidService != MR_VIDEO_RENDER_SERVICE)
        return MF_E_UNSUPPORTED_SERVICE;

    HRESULT hr = m_presenterEngine->getService(guidService, riid, ppvObject);

    if (FAILED(hr))
        hr = QueryInterface(riid, ppvObject);

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::ActivateObject(REFIID riid, void **ppv)
{
    if (!ppv)
        return E_POINTER;

    if (riid == IID_IMFVideoPresenter) {
        *ppv = static_cast<IMFVideoPresenter*>(this);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::DetachObject()
{
    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::ShutdownObject()
{
    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::SetVideoWindow(HWND hwndVideo)
{
    LockHolder locker(m_lock);

    if (!IsWindow(hwndVideo))
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    HWND oldHwnd = m_presenterEngine->getVideoWindow();

    if (oldHwnd != hwndVideo) {
        // This will create a new Direct3D device.
        hr = m_presenterEngine->setVideoWindow(hwndVideo);

        notifyEvent(EC_DISPLAY_CHANGED, 0, 0);
    }

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::GetVideoWindow(HWND* phwndVideo)
{
    LockHolder locker(m_lock);

    if (!phwndVideo)
        return E_POINTER;

    *phwndVideo = m_presenterEngine->getVideoWindow();

    return S_OK;
}

static HRESULT setMixerSourceRect(IMFTransform* mixer, const MFVideoNormalizedRect& sourceRect)
{
    if (!mixer)
        return E_POINTER;

    COMPtr<IMFAttributes> attributes;

    HRESULT hr = mixer->GetAttributes(&attributes);
    if (FAILED(hr))
        return hr;

    return attributes->SetBlob(VIDEO_ZOOM_RECT, (const UINT8*)&sourceRect, sizeof(sourceRect));
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::SetVideoPosition(const MFVideoNormalizedRect* pnrcSource, const LPRECT prcDest)
{
    LockHolder locker(m_lock);

    // First, check that the parameters are valid.

    if (!pnrcSource && !prcDest)
        return E_POINTER;

    if (pnrcSource) {
        if ((pnrcSource->left > pnrcSource->right) || (pnrcSource->top > pnrcSource->bottom))
            return E_INVALIDARG;

        // The source rectangle must be normalized.
        if ((pnrcSource->left < 0) || (pnrcSource->right > 1) || (pnrcSource->top < 0) || (pnrcSource->bottom > 1))
            return E_INVALIDARG;
    }

    if (prcDest) {
        if ((prcDest->left > prcDest->right) || (prcDest->top > prcDest->bottom))
            return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    // Set the source rectangle.
    if (pnrcSource) {
        m_sourceRect = *pnrcSource;

        if (m_mixer) {
            hr = setMixerSourceRect(m_mixer.get(), m_sourceRect);
            if (FAILED(hr))
                return hr;
        }
    }

    // Set the destination rectangle.
    if (prcDest) {
        RECT rcOldDest = m_presenterEngine->getDestinationRect();

        // If the destination rectangle hasn't changed, we are done.
        if (!EqualRect(&rcOldDest, prcDest)) {
            hr = m_presenterEngine->setDestinationRect(*prcDest);
            if (FAILED(hr))
                return hr;

            // We need to change the media type when the destination rectangle has changed.
            if (m_mixer) {
                hr = renegotiateMediaType();
                if (hr == MF_E_TRANSFORM_TYPE_NOT_SET) {
                    // This is not a critical failure; the EVR will let us know when
                    // we have to set the mixer media type.
                    hr = S_OK;
                } else {
                    if (FAILED(hr))
                        return hr;

                    // We have successfully changed the media type,
                    // ask for a repaint of the current frame.
                    m_repaint = true;
                    processOutput();
                }
            }
        }
    }

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::GetVideoPosition(MFVideoNormalizedRect* pnrcSource, LPRECT prcDest)
{
    LockHolder locker(m_lock);

    if (!pnrcSource || !prcDest)
        return E_POINTER;

    *pnrcSource = m_sourceRect;
    *prcDest = m_presenterEngine->getDestinationRect();

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::RepaintVideo()
{
    LockHolder locker(m_lock);

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    // Check that at least one sample has been presented.
    if (m_prerolled) {
        m_repaint = true;
        processOutput();
    }

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::Invoke(IMFAsyncResult* pAsyncResult)
{
    return onSampleFree(pAsyncResult);
}

void MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::onMediaPlayerDeleted()
{
    m_mediaPlayer = nullptr;
}

void MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::paintCurrentFrame(GraphicsContext& context, const FloatRect& r)
{
    if (m_presenterEngine)
        m_presenterEngine->paintCurrentFrame(context, r);
}

float MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::currentTime()
{
    if (!m_clock)
        return 0.0f;

    LONGLONG clockTime;
    MFTIME systemTime;
    HRESULT hr = m_clock->GetCorrelatedTime(0, &clockTime, &systemTime);

    if (FAILED(hr))
        return 0.0f;

    // clockTime is in 100 nanoseconds, we need to convert to seconds.
    float currentTime = clockTime / tenMegahertz;

    if (currentTime > m_maxTimeLoaded)
        m_maxTimeLoaded = currentTime;

    return currentTime;
}

bool MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::isActive() const
{
    return ((m_renderState == RenderStateStarted) || (m_renderState == RenderStatePaused));
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::configureMixer(IMFTransform* mixer)
{
    COMPtr<IMFVideoDeviceID> videoDeviceID;
    HRESULT hr = mixer->QueryInterface(__uuidof(IMFVideoDeviceID), (void**)&videoDeviceID);
    if (FAILED(hr))
        return hr;

    IID deviceID = GUID_NULL;
    hr = videoDeviceID->GetDeviceID(&deviceID);
    if (FAILED(hr))
        return hr;

    // The mixer must have this device ID.
    if (!IsEqualGUID(deviceID, __uuidof(IDirect3DDevice9)))
        return MF_E_INVALIDREQUEST;

    setMixerSourceRect(mixer, m_sourceRect);

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::flush()
{
    m_prerolled = false;

    // Flush the sceduler.
    // This call will block until the scheduler thread has finished flushing.
    m_scheduler.flush();

    if (m_renderState == RenderStateStopped)
        m_presenterEngine->presentSample(nullptr, 0);

    return S_OK;
}

static bool areMediaTypesEqual(IMFMediaType* type1, IMFMediaType* type2)
{
    if (!type1 && !type2)
        return true;
    if (!type1 || !type2)
        return false;

    DWORD flags = 0;
    return S_OK == type1->IsEqual(type2, &flags);
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::setMediaType(IMFMediaType* mediaType)
{
    if (!mediaType) {
        m_mediaType = nullptr;
        releaseResources();
        return S_OK;
    }

    // If we have shut down, we cannot set the media type.
    HRESULT hr = checkShutdown();
    if (FAILED(hr)) {
        releaseResources();
        return hr;
    }

    if (areMediaTypesEqual(m_mediaType.get(), mediaType))
        return S_OK;

    m_mediaType = nullptr;
    releaseResources();

    // Get allocated samples from the presenter.
    VideoSampleList sampleQueue;
    hr = m_presenterEngine->createVideoSamples(mediaType, sampleQueue);
    if (FAILED(hr)) {
        releaseResources();
        return hr;
    }

    // Set the token counter on each sample.
    // This will help us to determine when they are invalid, and can be released.
    for (auto sample : sampleQueue) {
        hr = sample->SetUINT32(MFSamplePresenterSampleCounter, m_tokenCounter);
        if (FAILED(hr)) {
            releaseResources();
            return hr;
        }
    }

    // Add the samples to the sample pool.
    hr = m_samplePool.initialize(sampleQueue);
    if (FAILED(hr)) {
        releaseResources();
        return hr;
    }

    // Set the frame rate. 
    MFRatio fps = { 0, 0 };
    hr = MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, (UINT32*)&fps.Numerator, (UINT32*)&fps.Denominator);
    if (SUCCEEDED(hr) && fps.Numerator && fps.Denominator)
        m_scheduler.setFrameRate(fps);
    else {
        // We could not get the frame ret, use default.
        const MFRatio defaultFrameRate = { 30, 1 };
        m_scheduler.setFrameRate(defaultFrameRate);
    }

    ASSERT(mediaType);
    m_mediaType = mediaType;

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::checkShutdown() const
{
    if (m_renderState == RenderStateShutdown)
        return MF_E_SHUTDOWN;
    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::renegotiateMediaType()
{
    HRESULT hr = S_OK;

    if (!m_mixer)
        return MF_E_INVALIDREQUEST;

    // Iterate over the available output types of the mixer.

    DWORD typeIndex = 0;
    bool foundMediaType = false;
    while (!foundMediaType && (hr != MF_E_NO_MORE_TYPES)) {
        // Get the next available media type.
        COMPtr<IMFMediaType> mixerType;
        hr = m_mixer->GetOutputAvailableType(0, typeIndex++, &mixerType);
        if (FAILED(hr))
            break;

        // Do we support this media type?
        hr = isMediaTypeSupported(mixerType.get());
        if (FAILED(hr))
            break;

        // Make adjustments to proposed media type.
        COMPtr<IMFMediaType> optimalType;
        hr = createOptimalVideoType(mixerType.get(), &optimalType);
        if (FAILED(hr))
            break;

        // Test whether the mixer can accept the modified media type
        hr = m_mixer->SetOutputType(0, optimalType.get(), MFT_SET_TYPE_TEST_ONLY);
        if (FAILED(hr))
            break;

        // Try to set the new media type

        hr = setMediaType(optimalType.get());
        if (FAILED(hr))
            break;

        hr = m_mixer->SetOutputType(0, optimalType.get(), 0);

        ASSERT(SUCCEEDED(hr));

        if (FAILED(hr))
            setMediaType(nullptr);
        else
            foundMediaType = true;
    }

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::processInputNotify()
{
    // We have a new sample.
    m_sampleNotify = true;

    if (!m_mediaType) {
        // The media type is not valid.
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    } 
    
    // Invalidate the video area
    if (m_mediaPlayer) {
        auto weakPtr = m_mediaPlayer->m_weakPtrFactory.createWeakPtr();
        callOnMainThread([weakPtr] {
            if (weakPtr)
                weakPtr->invalidateFrameView();
        });
    }

    // Process sample
    processOutputLoop();

    return S_OK;
}

static float MFOffsetToFloat(const MFOffset& offset)
{
    const int denominator = std::numeric_limits<WORD>::max() + 1;
    return offset.value + (float(offset.fract) / denominator);
}

static MFOffset MakeOffset(float v)
{
    // v = offset.value + (offset.fract / denominator), where denominator = 65536.0f.
    const int denominator = std::numeric_limits<WORD>::max() + 1;
    MFOffset offset;
    offset.value = short(v);
    offset.fract = WORD(denominator * (v - offset.value));
    return offset;
}

static MFVideoArea MakeArea(float x, float y, DWORD width, DWORD height)
{
    MFVideoArea area;
    area.OffsetX = MakeOffset(x);
    area.OffsetY = MakeOffset(y);
    area.Area.cx = width;
    area.Area.cy = height;
    return area;
}

static HRESULT validateVideoArea(const MFVideoArea& area, UINT32 width, UINT32 height)
{
    float fOffsetX = MFOffsetToFloat(area.OffsetX);
    float fOffsetY = MFOffsetToFloat(area.OffsetY);

    if (((LONG)fOffsetX + area.Area.cx > width) || ((LONG)fOffsetY + area.Area.cy > height))
        return MF_E_INVALIDMEDIATYPE;
    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::beginStreaming()
{
    return m_scheduler.startScheduler(m_clock.get());
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::endStreaming()
{
    return m_scheduler.stopScheduler();
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::checkEndOfStream()
{
    if (!m_endStreaming) {
        // We have not received the end-of-stream message from the EVR.
        return S_OK;
    }

    if (m_sampleNotify) {
        // There is still input samples available for the mixer. 
        return S_OK;
    }

    if (m_samplePool.areSamplesPending()) {
        // There are samples scheduled for rendering.
        return S_OK;
    }

    // We are done, notify the EVR.
    notifyEvent(EC_COMPLETE, (LONG_PTR)S_OK, 0);
    m_endStreaming = false;
    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::isMediaTypeSupported(IMFMediaType* mediaType)
{
    COMPtr<IMFMediaType> proposedVideoType = mediaType;

    // We don't support compressed media types.
    BOOL compressed = FALSE;
    HRESULT hr = proposedVideoType->IsCompressedFormat(&compressed);
    if (FAILED(hr))
        return hr;
    if (compressed)
        return MF_E_INVALIDMEDIATYPE;

    // Validate the format.
    GUID guidSubType = GUID_NULL;
    hr = proposedVideoType->GetGUID(MF_MT_SUBTYPE, &guidSubType);
    if (FAILED(hr))
        return hr;
    D3DFORMAT d3dFormat = (D3DFORMAT)guidSubType.Data1;

    // Check if the format can be used as backbuffer format.
    hr = m_presenterEngine->checkFormat(d3dFormat);
    if (FAILED(hr))
        return hr;

    // Check interlaced formats.
    MFVideoInterlaceMode interlaceMode = MFVideoInterlace_Unknown;
    hr = proposedVideoType->GetUINT32(MF_MT_INTERLACE_MODE, (UINT32*)&interlaceMode);
    if (FAILED(hr))
        return hr;

    if (interlaceMode != MFVideoInterlace_Progressive)
        return MF_E_INVALIDMEDIATYPE;

    UINT32 width = 0, height = 0;
    hr = MFGetAttributeSize(proposedVideoType.get(), MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr))
        return hr;

    // Validate apertures.
    MFVideoArea videoCropArea;
    if (SUCCEEDED(proposedVideoType->GetBlob(MF_MT_PAN_SCAN_APERTURE, (UINT8*)&videoCropArea, sizeof(MFVideoArea), nullptr)))
        validateVideoArea(videoCropArea, width, height);
    if (SUCCEEDED(proposedVideoType->GetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&videoCropArea, sizeof(MFVideoArea), nullptr)))
        validateVideoArea(videoCropArea, width, height);
    if (SUCCEEDED(proposedVideoType->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)&videoCropArea, sizeof(MFVideoArea), nullptr)))
        validateVideoArea(videoCropArea, width, height);
    
    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::createOptimalVideoType(IMFMediaType* proposedType, IMFMediaType** optimalType)
{
    COMPtr<IMFMediaType> optimalVideoType;
    HRESULT hr = MFCreateMediaTypePtr()(&optimalVideoType);
    if (FAILED(hr))
        return hr;
    hr = optimalVideoType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr))
        return hr;

    hr = proposedType->CopyAllItems(optimalVideoType.get());
    if (FAILED(hr))
        return hr;

    // We now modify the new media type.

    // We assume that the monitor's pixel aspect ratio is 1:1,
    // and that the pixel aspect ratio is preserved by the presenter.
    hr = MFSetAttributeRatio(optimalVideoType.get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (FAILED(hr))
        return hr;

    // Get the output rectangle.
    RECT rcOutput = m_presenterEngine->getDestinationRect();
    if (IsRectEmpty(&rcOutput)) {
        hr = calculateOutputRectangle(proposedType, rcOutput);
        if (FAILED(hr))
            return hr;
    }

    hr = optimalVideoType->SetUINT32(MF_MT_YUV_MATRIX, MFVideoTransferMatrix_BT709);
    if (FAILED(hr))
        return hr;

    hr = optimalVideoType->SetUINT32(MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_709);
    if (FAILED(hr))
        return hr;

    hr = optimalVideoType->SetUINT32(MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_BT709);
    if (FAILED(hr))
        return hr;

    hr = optimalVideoType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_16_235);
    if (FAILED(hr))
        return hr;

    hr = optimalVideoType->SetUINT32(MF_MT_VIDEO_LIGHTING, MFVideoLighting_dim);
    if (FAILED(hr))
        return hr;

    hr = MFSetAttributeSize(optimalVideoType.get(), MF_MT_FRAME_SIZE, rcOutput.right, rcOutput.bottom);
    if (FAILED(hr))
        return hr;

    MFVideoArea displayArea = MakeArea(0, 0, rcOutput.right, rcOutput.bottom);

    hr = optimalVideoType->SetUINT32(MF_MT_PAN_SCAN_ENABLED, FALSE);
    if (FAILED(hr))
        return hr;

    hr = optimalVideoType->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&displayArea, sizeof(MFVideoArea));
    if (FAILED(hr))
        return hr;

    hr = optimalVideoType->SetBlob(MF_MT_PAN_SCAN_APERTURE, (UINT8*)&displayArea, sizeof(MFVideoArea));
    if (FAILED(hr))
        return hr;

    hr = optimalVideoType->SetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)&displayArea, sizeof(MFVideoArea));
    if (FAILED(hr))
        return hr;

    *optimalType = optimalVideoType.leakRef();

    return S_OK;
}

static RECT correctAspectRatio(const RECT& src, const MFRatio& srcPAR, const MFRatio& destPAR)
{
    RECT rc = { 0, 0, src.right - src.left, src.bottom - src.top };

    if ((srcPAR.Numerator * destPAR.Denominator) != (srcPAR.Denominator * destPAR.Numerator)) {
        // The source and destination aspect ratios are different

        // Transform the source aspect ratio to 1:1
        if (srcPAR.Numerator > srcPAR.Denominator)
            rc.right = MulDiv(rc.right, srcPAR.Numerator, srcPAR.Denominator);
        else if (srcPAR.Numerator < srcPAR.Denominator)
            rc.bottom = MulDiv(rc.bottom, srcPAR.Denominator, srcPAR.Numerator);


        // Transform to destination aspect ratio.
        if (destPAR.Numerator > destPAR.Denominator)
            rc.bottom = MulDiv(rc.bottom, destPAR.Numerator, destPAR.Denominator);
        else if (destPAR.Numerator < destPAR.Denominator)
            rc.right = MulDiv(rc.right, destPAR.Denominator, destPAR.Numerator);

    }

    return rc;
}

static HRESULT GetVideoDisplayArea(IMFMediaType* type, MFVideoArea* area)
{
    if (!type || !area)
        return E_POINTER;

    HRESULT hr = S_OK;
    UINT32 width = 0, height = 0;

    BOOL bPanScan = MFGetAttributeUINT32(type, MF_MT_PAN_SCAN_ENABLED, FALSE);

    if (bPanScan)
        hr = type->GetBlob(MF_MT_PAN_SCAN_APERTURE, (UINT8*)area, sizeof(MFVideoArea), nullptr);

    if (!bPanScan || hr == MF_E_ATTRIBUTENOTFOUND) {
        hr = type->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)area, sizeof(MFVideoArea), nullptr);

        if (hr == MF_E_ATTRIBUTENOTFOUND)
            hr = type->GetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)area, sizeof(MFVideoArea), nullptr);

        if (hr == MF_E_ATTRIBUTENOTFOUND) {
            hr = MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height);
            if (SUCCEEDED(hr))
                *area = MakeArea(0.0, 0.0, width, height);
        }
    }

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::calculateOutputRectangle(IMFMediaType* proposedType, RECT& outputRect)
{
    COMPtr<IMFMediaType> proposedVideoType = proposedType;

    UINT32 srcWidth = 0, srcHeight = 0;
    HRESULT hr = MFGetAttributeSize(proposedVideoType.get(), MF_MT_FRAME_SIZE, &srcWidth, &srcHeight);
    if (FAILED(hr))
        return hr;

    MFVideoArea displayArea;
    ZeroMemory(&displayArea, sizeof(displayArea));

    hr = GetVideoDisplayArea(proposedVideoType.get(), &displayArea);
    if (FAILED(hr))
        return hr;

    LONG offsetX = (LONG)MFOffsetToFloat(displayArea.OffsetX);
    LONG offsetY = (LONG)MFOffsetToFloat(displayArea.OffsetY);

    // Check if the display area is valid.
    // If it is valid, we use it. If not, we use the frame dimensions.

    RECT rcOutput;

    if (displayArea.Area.cx != 0
        && displayArea.Area.cy != 0
        && offsetX + displayArea.Area.cx <= srcWidth
        && offsetY + displayArea.Area.cy <= srcHeight) {
        rcOutput.left = offsetX;
        rcOutput.right = offsetX + displayArea.Area.cx;
        rcOutput.top = offsetY;
        rcOutput.bottom = offsetY + displayArea.Area.cy;
    } else {
        rcOutput.left = 0;
        rcOutput.top = 0;
        rcOutput.right = srcWidth;
        rcOutput.bottom = srcHeight;
    }

    // Correct aspect ratio.

    MFRatio inputPAR = { 1, 1 };
    MFRatio outputPAR = { 1, 1 }; // We assume the monitor's pixels are square.
    MFGetAttributeRatio(proposedVideoType.get(), MF_MT_PIXEL_ASPECT_RATIO, (UINT32*)&inputPAR.Numerator, (UINT32*)&inputPAR.Denominator);
    outputRect = correctAspectRatio(rcOutput, inputPAR, outputPAR);

    return S_OK;
}

void MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::processOutputLoop()
{
    // Get video frames from the mixer and schedule them for presentation.
    HRESULT hr = S_OK;

    while (hr == S_OK) {
        if (!m_sampleNotify) {
            // Currently no more input samples.
            hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
            break;
        }

        // We break from the loop if we fail to process a sample.
        hr = processOutput();
    }

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        checkEndOfStream();
}

static HRESULT setDesiredSampleTime(IMFSample* sample, const LONGLONG& sampleTime, const LONGLONG& duration)
{
    // To tell the mixer to give us an earlier frame for repainting, we can set the desired sample time.
    // We have to clear the desired sample time before reusing the sample.

    if (!sample)
        return E_POINTER;

    COMPtr<IMFDesiredSample> desired;

    HRESULT hr = sample->QueryInterface(__uuidof(IMFDesiredSample), (void**)&desired);

    if (SUCCEEDED(hr))
        desired->SetDesiredSampleTimeAndDuration(sampleTime, duration);

    return hr;
}

static HRESULT clearDesiredSampleTime(IMFSample* sample)
{
    if (!sample)
        return E_POINTER;

    // We need to retrieve some attributes we have set on the sample before we call 
    // IMFDesiredSample::Clear(), and set them once more, since they are cleared by
    // the Clear() call.

    UINT32 counter = MFGetAttributeUINT32(sample, MFSamplePresenterSampleCounter, (UINT32)-1);

    COMPtr<IMFDesiredSample> desired;
    HRESULT hr = sample->QueryInterface(__uuidof(IMFDesiredSample), (void**)&desired);
    if (SUCCEEDED(hr)) {
        desired->Clear();

        hr = sample->SetUINT32(MFSamplePresenterSampleCounter, counter);
        if (FAILED(hr))
            return hr;
    }

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::processOutput()
{
    // This method will try to get a new sample from the mixer.
    // It is called when the mixer has a new sample, or when repainting the last frame.

    ASSERT(m_sampleNotify || m_repaint);

    LONGLONG mixerStartTime = 0, mixerEndTime = 0;
    MFTIME systemTime = 0;
    bool repaint = m_repaint;  

    // If the clock has not started, we only present the first sample. 

    if ((m_renderState != RenderStateStarted) && !m_repaint && m_prerolled)
        return S_FALSE;

    if (!m_mixer)
        return MF_E_INVALIDREQUEST;

    // Get a free sample from the pool.
    COMPtr<IMFSample> sample;
    HRESULT hr = m_samplePool.getSample(sample);
    if (hr == MF_E_SAMPLEALLOCATOR_EMPTY)
        return S_FALSE; // We will try again later when there are free samples

    if (FAILED(hr))
        return hr;

    ASSERT(sample);

    ASSERT(MFGetAttributeUINT32(sample.get(), MFSamplePresenterSampleCounter, (UINT32)-1) == m_tokenCounter);

    if (m_repaint) {
        // Get the most recent sample from the mixer.
        setDesiredSampleTime(sample.get(), m_scheduler.lastSampleTime(), m_scheduler.frameDuration());
        m_repaint = false;
    } else {
        // Clear the desired sample time to get the next sample in the stream.
        clearDesiredSampleTime(sample.get());

        if (m_clock) {
            // Get the starting time of the ProcessOutput call.
            m_clock->GetCorrelatedTime(0, &mixerStartTime, &systemTime);
        }
    }

    // Get a sample from the mixer. 
    MFT_OUTPUT_DATA_BUFFER dataBuffer;
    ZeroMemory(&dataBuffer, sizeof(dataBuffer));

    dataBuffer.dwStreamID = 0;
    dataBuffer.pSample = sample.get();
    dataBuffer.dwStatus = 0;

    DWORD status = 0;
    hr = m_mixer->ProcessOutput(0, 1, &dataBuffer, &status);

    // Release events. There are usually no events returned,
    // but in case there are, we should release them.
    if (dataBuffer.pEvents)
        dataBuffer.pEvents->Release();

    if (FAILED(hr)) {
        HRESULT hr2 = m_samplePool.returnSample(sample.get());
        if (FAILED(hr2))
            return hr2;

        if (hr == MF_E_TRANSFORM_TYPE_NOT_SET) {
            // The media type has not been set, renegotiate.
            hr = renegotiateMediaType();
        } else if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
            // The media type changed, reset it.
            setMediaType(nullptr);
        } else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            // The mixer needs more input.
            m_sampleNotify = false;
        }
    } else {
        // We have got a sample from the mixer.

        if (m_clock && !repaint) {
            // Notify the EVR about latency.
            m_clock->GetCorrelatedTime(0, &mixerEndTime, &systemTime);

            LONGLONG latencyTime = mixerEndTime - mixerStartTime;
            notifyEvent(EC_PROCESSING_LATENCY, (LONG_PTR)&latencyTime, 0);
        }

        // Make sure we are notified when the sample is released
        hr = trackSample(sample.get());
        if (FAILED(hr))
            return hr;

        // Deliver the sample for scheduling
        hr = deliverSample(sample.get(), repaint);
        if (FAILED(hr))
            return hr;

        // At least one sample has been presented now.
        m_prerolled = true;
    }

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::deliverSample(IMFSample* sample, bool repaint)
{
    if (!sample)
        return E_POINTER;

    Direct3DPresenter::DeviceState state = Direct3DPresenter::DeviceOK;

    // Determine if the sample should be presented immediately.
    bool presentNow = ((m_renderState != RenderStateStarted) || isScrubbing() || repaint);

    HRESULT hr = m_presenterEngine->checkDeviceState(state);

    if (SUCCEEDED(hr))
        hr = m_scheduler.scheduleSample(sample, presentNow);

    if (FAILED(hr)) {
        // Streaming has failed, notify the EVR.
        notifyEvent(EC_ERRORABORT, hr, 0);
    } else if (state == Direct3DPresenter::DeviceReset)
        notifyEvent(EC_DISPLAY_CHANGED, S_OK, 0);

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::trackSample(IMFSample* sample)
{
    if (!sample)
        return E_POINTER;

    COMPtr<IMFTrackedSample> tracked;

    HRESULT hr = sample->QueryInterface(__uuidof(IMFTrackedSample), (void**)&tracked);
    if (FAILED(hr))
        return hr;

    if (!tracked)
        return E_POINTER;

    // Set callback object on which the onSampleFree method is invoked when the sample is no longer used.
    return tracked->SetAllocator(this, nullptr);
}

void MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::releaseResources()
{
    // The token counter is incremented to indicate that existing samples are
    // invalid and can be disposed in the method onSampleFree.
    m_tokenCounter++;

    flush();

    m_samplePool.clear();

    if (m_presenterEngine)
        m_presenterEngine->releaseResources();
}

HRESULT MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::onSampleFree(IMFAsyncResult* result)
{
    if (!result)
        return E_POINTER;

    COMPtr<IUnknown> object;
    HRESULT hr = result->GetObject(&object);
    if (FAILED(hr)) {
        notifyEvent(EC_ERRORABORT, hr, 0);
        return hr;
    }

    COMPtr<IMFSample> sample;
    hr = object->QueryInterface(__uuidof(IMFSample), (void**)&sample);
    if (FAILED(hr)) {
        notifyEvent(EC_ERRORABORT, hr, 0);
        return hr;
    }

    m_lock.lock();

    if (MFGetAttributeUINT32(sample.get(), MFSamplePresenterSampleCounter, (UINT32)-1) == m_tokenCounter) {
        hr = m_samplePool.returnSample(sample.get());

        // Do more processing, since a free sample is available
        if (SUCCEEDED(hr))
            processOutputLoop();
    }

    m_lock.unlock();

    if (FAILED(hr))
        notifyEvent(EC_ERRORABORT, hr, 0);

    return hr;
}

void MediaPlayerPrivateMediaFoundation::CustomVideoPresenter::notifyEvent(long EventCode, LONG_PTR Param1, LONG_PTR Param2)
{
    if (m_mediaEventSink)
        m_mediaEventSink->Notify(EventCode, Param1, Param2);
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoSamplePool::getSample(COMPtr<IMFSample>& sample)
{
    LockHolder locker(m_lock);

    if (!m_initialized)
        return MF_E_NOT_INITIALIZED;

    if (m_videoSampleQueue.isEmpty())
        return MF_E_SAMPLEALLOCATOR_EMPTY;

    sample = m_videoSampleQueue.takeFirst();

    m_pending++;

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoSamplePool::returnSample(IMFSample* sample)
{
    if (!sample)
        return E_POINTER;

    LockHolder locker(m_lock);

    if (!m_initialized)
        return MF_E_NOT_INITIALIZED;

    m_videoSampleQueue.append(sample);
    m_pending--;
    return S_OK;
}

bool MediaPlayerPrivateMediaFoundation::VideoSamplePool::areSamplesPending()
{
    LockHolder locker(m_lock);

    if (!m_initialized)
        return FALSE;

    return (m_pending > 0);
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoSamplePool::initialize(VideoSampleList& samples)
{
    LockHolder locker(m_lock);

    if (m_initialized)
        return MF_E_INVALIDREQUEST;

    // Copy the samples
    for (auto sample : samples)
        m_videoSampleQueue.append(sample);

    m_initialized = true;
    samples.clear();

    return S_OK;
}

void MediaPlayerPrivateMediaFoundation::VideoSamplePool::clear()
{
    LockHolder locker(m_lock);

    m_videoSampleQueue.clear();
    m_initialized = false;
    m_pending = 0;
}


// Scheduler thread messages.

enum ScheduleEvent {
    EventTerminate = WM_USER,
    EventSchedule,
    EventFlush
};

void MediaPlayerPrivateMediaFoundation::VideoScheduler::setFrameRate(const MFRatio& fps)
{
    UINT64 avgTimePerFrame = 0;
    MFFrameRateToAverageTimePerFramePtr()(fps.Numerator, fps.Denominator, &avgTimePerFrame);

    m_frameDuration = (MFTIME)avgTimePerFrame;
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoScheduler::startScheduler(IMFClock* clock)
{
    if (m_schedulerThread.isValid())
        return E_UNEXPECTED;

    HRESULT hr = S_OK;

    m_clock = clock;

    // Use high timer resolution.
    timeBeginPeriod(1);

    // Create an event to signal that the scheduler thread has started.
    m_threadReadyEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_threadReadyEvent.isValid())
        return HRESULT_FROM_WIN32(GetLastError());

    // Create an event to signal that the flush has completed.
    m_flushEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_flushEvent.isValid())
        return HRESULT_FROM_WIN32(GetLastError());

    // Start scheduler thread.
    DWORD threadID = 0;
    m_schedulerThread = ::CreateThread(nullptr, 0, schedulerThreadProc, (LPVOID)this, 0, &threadID);
    if (!m_schedulerThread.isValid())
        return HRESULT_FROM_WIN32(GetLastError());

    HANDLE hObjects[] = { m_threadReadyEvent.get(), m_schedulerThread.get() };

    // Wait for the thread to start
    DWORD result = ::WaitForMultipleObjects(2, hObjects, FALSE, INFINITE);
    if (WAIT_OBJECT_0 != result) {
        // The thread has terminated.
        m_schedulerThread.clear();
        return E_UNEXPECTED;
    }

    m_threadID = threadID;

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoScheduler::stopScheduler()
{
    if (!m_schedulerThread.isValid())
        return S_OK;

    // Terminate the scheduler thread
    stopThread();
    ::PostThreadMessage(m_threadID, EventTerminate, 0, 0);

    // Wait for the scheduler thread to finish.
    ::WaitForSingleObject(m_schedulerThread.get(), INFINITE);

    LockHolder locker(m_lock);

    m_scheduledSamples.clear();
    m_schedulerThread.clear();
    m_flushEvent.clear();

    // Clear previously set timer resolution.
    timeEndPeriod(1);

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoScheduler::flush()
{
    // This method will wait for the flush to finish on the worker thread.

    if (m_schedulerThread.isValid()) {
        ::PostThreadMessage(m_threadID, EventFlush, 0, 0);

        HANDLE objects[] = { m_flushEvent.get(), m_schedulerThread.get() };

        const int schedulerTimeout = 5000;

        // Wait for the flush to finish or the thread to terminate.
        ::WaitForMultipleObjects(2, objects, FALSE, schedulerTimeout);
    }

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoScheduler::scheduleSample(IMFSample* sample, bool presentNow)
{
    if (!sample)
        return E_POINTER;

    if (!m_presenter)
        return MF_E_NOT_INITIALIZED;

    if (!m_schedulerThread.isValid())
        return MF_E_NOT_INITIALIZED;

    DWORD exitCode = 0;
    ::GetExitCodeThread(m_schedulerThread.get(), &exitCode);

    if (exitCode != STILL_ACTIVE)
        return E_FAIL;

    if (presentNow || !m_clock)
        m_presenter->presentSample(sample, 0);
    else {
        // Submit the sample for scheduling.
        LockHolder locker(m_lock);
        m_scheduledSamples.append(sample);

        ::PostThreadMessage(m_threadID, EventSchedule, 0, 0);
    }

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoScheduler::processSamplesInQueue(LONG& nextSleep)
{
    HRESULT hr = S_OK;
    LONG wait = 0;

    // Process samples as long as there are samples in the queue, and they have not arrived too early.

    while (!m_exitThread) {
        COMPtr<IMFSample> sample;

        if (true) {
            LockHolder locker(m_lock);
            if (m_scheduledSamples.isEmpty())
                break;
            sample = m_scheduledSamples.takeFirst();
        }

        // Process the sample.
        // If the sample has arrived too early, wait will be > 0,
        // and the scheduler should go to sleep.
        hr = processSample(sample.get(), wait);

        if (FAILED(hr))
            break;

        if (wait > 0)
            break;
    }

    if (!wait) {
        // The queue is empty. Sleep until the next message arrives.
        wait = INFINITE;
    }

    nextSleep = wait;
    return hr;
}

// MFTimeToMilliseconds: Convert 100-nanosecond time to milliseconds.
static LONG MFTimeToMilliseconds(const LONGLONG& time)
{
    return (time / 10000);
}

HRESULT MediaPlayerPrivateMediaFoundation::VideoScheduler::processSample(IMFSample* sample, LONG& nextSleep)
{
    if (!sample)
        return E_POINTER;

    HRESULT hr = S_OK;

    LONGLONG presentationTime = 0;
    LONGLONG timeNow = 0;
    MFTIME systemTime = 0;

    bool presentNow = true;
    LONG nextSleepTime = 0;

    if (m_clock) {
        // Get the time stamp of the sample.
        // A sample can possibly have no time stamp.
        hr = sample->GetSampleTime(&presentationTime);

        // Get the clock time.
        // If the sample does not have a time stamp, the clock time is not needed.
        if (SUCCEEDED(hr))
            hr = m_clock->GetCorrelatedTime(0, &timeNow, &systemTime);

        // Determine the time until the sample should be presented.
        // Samples arriving late, will have negative values.
        LONGLONG timeDelta = presentationTime - timeNow;
        if (m_playbackRate < 0) {
            // Reverse delta for reverse playback.
            timeDelta = -timeDelta;
        }

        LONGLONG frameDurationOneFourth = m_frameDuration / 4;

        if (timeDelta < -frameDurationOneFourth) {
            // The sample has arrived late. 
            presentNow = true;
        } else if (timeDelta > (3 * frameDurationOneFourth)) {
            // We can sleep, the sample has arrived too early.
            nextSleepTime = MFTimeToMilliseconds(timeDelta - (3 * frameDurationOneFourth));

            // Since sleeping is using the system clock, we need to convert the sleep time
            // from presentation time to system time.
            nextSleepTime = (LONG)(nextSleepTime / fabsf(m_playbackRate));

            presentNow = false;
        }
    }

    if (presentNow)
        hr = m_presenter->presentSample(sample, presentationTime);
    else {
        // Return the sample to the queue, since it is not ready.
        LockHolder locker(m_lock);
        m_scheduledSamples.prepend(sample);
    }

    nextSleep = nextSleepTime;

    return hr;
}

DWORD WINAPI MediaPlayerPrivateMediaFoundation::VideoScheduler::schedulerThreadProc(LPVOID lpParameter)
{
    VideoScheduler* scheduler = reinterpret_cast<VideoScheduler*>(lpParameter);
    if (!scheduler)
        return static_cast<DWORD>(-1);
    return scheduler->schedulerThreadProcPrivate();
}

DWORD MediaPlayerPrivateMediaFoundation::VideoScheduler::schedulerThreadProcPrivate()
{
    HRESULT hr = S_OK;

    // This will force a message queue to be created for the thread.
    MSG msg;
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    // The thread is ready.
    SetEvent(m_threadReadyEvent.get());

    LONG wait = INFINITE;
    m_exitThread = false;
    while (!m_exitThread) {
        // Wait for messages
        DWORD result = MsgWaitForMultipleObjects(0, nullptr, FALSE, wait, QS_POSTMESSAGE);

        if (result == WAIT_TIMEOUT) {
            hr = processSamplesInQueue(wait);
            if (FAILED(hr))
                m_exitThread = true;
        }

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            bool processSamples = true;

            switch (msg.message) {
            case EventTerminate:
                m_exitThread = true;
                break;

            case EventFlush:
                {
                    LockHolder lock(m_lock);
                    m_scheduledSamples.clear();
                }
                wait = INFINITE;
                SetEvent(m_flushEvent.get());
                break;

            case EventSchedule:
                if (processSamples) {
                    hr = processSamplesInQueue(wait);
                    if (FAILED(hr))
                        m_exitThread = true;
                    processSamples = (wait != INFINITE);
                }
                break;
            }
        }
    }
    return (SUCCEEDED(hr) ? 0 : 1);
}

static HRESULT findAdapter(IDirect3D9* direct3D9, HMONITOR monitor, UINT& adapterID)
{
    HRESULT hr = E_FAIL;

    UINT adapterCount = direct3D9->GetAdapterCount();
    for (UINT i = 0; i < adapterCount; i++) {
        HMONITOR monitorTmp = direct3D9->GetAdapterMonitor(i);

        if (!monitorTmp)
            break;

        if (monitorTmp == monitor) {
            adapterID = i;
            hr = S_OK;
            break;
        }
    }

    return hr;
}

MediaPlayerPrivateMediaFoundation::Direct3DPresenter::Direct3DPresenter()
{
    SetRectEmpty(&m_destRect);

    ZeroMemory(&m_displayMode, sizeof(m_displayMode));

    HRESULT hr = initializeD3D();

    if (FAILED(hr))
        return;

    createD3DDevice();
}

MediaPlayerPrivateMediaFoundation::Direct3DPresenter::~Direct3DPresenter()
{
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::getService(REFGUID guidService, REFIID riid, void** ppv)
{
    ASSERT(ppv);

    HRESULT hr = S_OK;

    if (riid == __uuidof(IDirect3DDeviceManager9)) {
        if (!m_deviceManager)
            hr = MF_E_UNSUPPORTED_SERVICE;
        else {
            *ppv = m_deviceManager.get();
            m_deviceManager->AddRef();
        }
    } else
        hr = MF_E_UNSUPPORTED_SERVICE;

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::checkFormat(D3DFORMAT format)
{
    HRESULT hr = S_OK;

    UINT adapter = D3DADAPTER_DEFAULT;
    D3DDEVTYPE type = D3DDEVTYPE_HAL;

    if (m_device) {
        D3DDEVICE_CREATION_PARAMETERS params;
        hr = m_device->GetCreationParameters(&params);
        if (FAILED(hr))
            return hr;

        adapter = params.AdapterOrdinal;
        type = params.DeviceType;
    }

    D3DDISPLAYMODE mode;
    hr = m_direct3D9->GetAdapterDisplayMode(adapter, &mode);
    if (FAILED(hr))
        return hr;

    return m_direct3D9->CheckDeviceType(adapter, type, mode.Format, format, TRUE);
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::setVideoWindow(HWND hwnd)
{
    ASSERT(IsWindow(hwnd));
    ASSERT(hwnd != m_hwnd);

    {
        LockHolder locker(m_lock);
        m_hwnd = hwnd;
        updateDestRect();
    }

    return createD3DDevice();
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::setDestinationRect(const RECT& rcDest)
{
    if (EqualRect(&rcDest, &m_destRect))
        return S_OK;

    LockHolder locker(m_lock);

    m_destRect = rcDest;
    updateDestRect();

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::createVideoSamples(IMFMediaType* format, VideoSampleList& videoSampleQueue)
{
    // Create video samples matching the supplied format.
    // A swap chain with a single back buffer will be created for each video sample.
    // The mixer will render to the back buffer through a surface kept by the sample.
    // The surface can be rendered to a window by presenting the swap chain.
    // In our case the surface is transferred to system memory, and rendered to a graphics context.

    if (!m_hwnd)
        return MF_E_INVALIDREQUEST;

    if (!format)
        return MF_E_UNEXPECTED;


    LockHolder locker(m_lock);

    releaseResources();

    D3DPRESENT_PARAMETERS presentParameters;
    HRESULT hr = getSwapChainPresentParameters(format, &presentParameters);
    if (FAILED(hr)) {
        releaseResources();
        return hr;
    }

    updateDestRect();

    static const int presenterBufferCount = 3;

    for (int i = 0; i < presenterBufferCount; i++) {
        COMPtr<IDirect3DSwapChain9> swapChain;
        hr = m_device->CreateAdditionalSwapChain(&presentParameters, &swapChain);
        if (FAILED(hr)) {
            releaseResources();
            return hr;
        }

        COMPtr<IMFSample> videoSample;
        hr = createD3DSample(swapChain.get(), videoSample);
        if (FAILED(hr)) {
            releaseResources();
            return hr;
        }

        videoSampleQueue.append(videoSample);
    }

    return hr;
}

void MediaPlayerPrivateMediaFoundation::Direct3DPresenter::releaseResources()
{
    m_surfaceRepaint = nullptr;
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::checkDeviceState(DeviceState& state)
{
    LockHolder locker(m_lock);

    HRESULT hr = m_device->CheckDeviceState(m_hwnd);

    state = DeviceOK;

    // Not all failure codes are critical.

    switch (hr) {
    case S_OK:
    case S_PRESENT_OCCLUDED:
    case S_PRESENT_MODE_CHANGED:
        hr = S_OK;
        break;

    case D3DERR_DEVICELOST:
    case D3DERR_DEVICEHUNG:
        hr = createD3DDevice();
        if (FAILED(hr))
            return hr;
        state = DeviceReset;
        hr = S_OK;
        break;

    case D3DERR_DEVICEREMOVED:
        state = DeviceRemoved;
        break;

    case E_INVALIDARG:
        // This might happen if the window has been destroyed, or is not valid.
        // A new device will be created if a new window is set.
        hr = S_OK;
    }

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::presentSample(IMFSample* sample, LONGLONG targetPresentationTime)
{
    HRESULT hr = S_OK;

    LockHolder locker(m_lock);

    COMPtr<IDirect3DSurface9> surface;

    if (sample) {
        COMPtr<IMFMediaBuffer> buffer;
        hr = sample->GetBufferByIndex(0, &buffer);
        hr = MFGetServicePtr()(buffer.get(), MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&surface);
    } else if (m_surfaceRepaint) {
        // Use the last surface.
        surface = m_surfaceRepaint;
    }

    if (surface) {
        UINT width = m_destRect.right - m_destRect.left;
        UINT height = m_destRect.bottom - m_destRect.top;

        if (width > 0 && height > 0) {
            if (!m_memSurface || m_width != width || m_height != height) {
                D3DFORMAT format = D3DFMT_A8R8G8B8;
                D3DSURFACE_DESC desc;
                if (SUCCEEDED(surface->GetDesc(&desc)))
                    format = desc.Format;
                hr = m_device->CreateOffscreenPlainSurface(width, height, format, D3DPOOL_SYSTEMMEM, &m_memSurface, nullptr);
                m_width = width;
                m_height = height;
            }
            // Copy data from video memory to system memory
            hr = m_device->GetRenderTargetData(surface.get(), m_memSurface.get());
            if (FAILED(hr)) {
                m_memSurface = nullptr;
                hr = S_OK;
            }
        }

        // Since we want to draw to the GraphicsContext provided in the paint method,
        // and not draw directly to the window, we skip presenting the swap chain:

        // COMPtr<IDirect3DSwapChain9> swapChain;
        // hr = surface->GetContainer(__uuidof(IDirect3DSwapChain9), (LPVOID*)&swapChain));
        // hr = presentSwapChain(swapChain, surface));

        // Keep the last surface for repaints.
        m_surfaceRepaint = surface;
    }

    if (FAILED(hr)) {
        if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET || hr == D3DERR_DEVICEHUNG) {
            // Ignore this error. We have to reset or recreate the device.
            // The presenter will handle this when checking the device state the next time.
            hr = S_OK;
        }
    }
    return hr;
}

void MediaPlayerPrivateMediaFoundation::Direct3DPresenter::paintCurrentFrame(WebCore::GraphicsContext& context, const WebCore::FloatRect& destRect)
{
    UINT width = m_destRect.right - m_destRect.left;
    UINT height = m_destRect.bottom - m_destRect.top;

    if (!width || !height)
        return;

    LockHolder locker(m_lock);

    if (!m_memSurface)
        return;

    D3DLOCKED_RECT lockedRect;
    if (SUCCEEDED(m_memSurface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY))) {
        void* data = lockedRect.pBits;
        int pitch = lockedRect.Pitch;
#if USE(CAIRO)
        D3DFORMAT format = D3DFMT_UNKNOWN;
        D3DSURFACE_DESC desc;
        if (SUCCEEDED(m_memSurface->GetDesc(&desc)))
            format = desc.Format;

        cairo_format_t cairoFormat = CAIRO_FORMAT_INVALID;

        switch (format) {
        case D3DFMT_A8R8G8B8:
            cairoFormat = CAIRO_FORMAT_ARGB32;
            break;
        case D3DFMT_X8R8G8B8:
            cairoFormat = CAIRO_FORMAT_RGB24;
            break;
        }

        ASSERT(cairoFormat != CAIRO_FORMAT_INVALID);

        cairo_surface_t* image = nullptr;
        if (cairoFormat != CAIRO_FORMAT_INVALID)
            image = cairo_image_surface_create_for_data(static_cast<unsigned char*>(data), cairoFormat, width, height, pitch);

        FloatRect srcRect(0, 0, width, height);
        if (image) {
            WebCore::PlatformContextCairo* ctxt = context.platformContext();
            ctxt->drawSurfaceToContext(image, destRect, srcRect, context);
            cairo_surface_destroy(image);
        }
#else
#error "Platform needs to implement drawing of Direct3D surface to graphics context!"
#endif
        m_memSurface->UnlockRect();
    }
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::initializeD3D()
{
    ASSERT(!m_direct3D9);
    ASSERT(!m_deviceManager);

    HRESULT hr = Direct3DCreate9ExPtr()(D3D_SDK_VERSION, &m_direct3D9);
    if (FAILED(hr))
        return hr;

    return DXVA2CreateDirect3DDeviceManager9Ptr()(&m_deviceResetToken, &m_deviceManager);
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::createD3DDevice()
{
    HRESULT hr = S_OK;
    UINT adapterID = D3DADAPTER_DEFAULT;

    LockHolder locker(m_lock);

    if (!m_direct3D9 || !m_deviceManager)
        return MF_E_NOT_INITIALIZED;

    HWND hwnd = GetDesktopWindow();

    // We create additional swap chains to present the video frames,
    // and do not use the implicit swap chain of the device.
    // The size of the back buffer is 1 x 1.

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));

    pp.BackBufferWidth = 1;
    pp.BackBufferHeight = 1;
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_COPY;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.hDeviceWindow = hwnd;
    pp.Flags = D3DPRESENTFLAG_VIDEO;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    if (m_hwnd) {
        HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);

        hr = findAdapter(m_direct3D9.get(), monitor, adapterID);
        if (FAILED(hr))
            return hr;
    }

    D3DCAPS9 ddCaps;
    ZeroMemory(&ddCaps, sizeof(ddCaps));

    hr = m_direct3D9->GetDeviceCaps(adapterID, D3DDEVTYPE_HAL, &ddCaps);
    if (FAILED(hr))
        return hr;

    DWORD flags = D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;

    if (ddCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        flags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        flags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    COMPtr<IDirect3DDevice9Ex> device;
    hr = m_direct3D9->CreateDeviceEx(adapterID, D3DDEVTYPE_HAL, pp.hDeviceWindow, flags, &pp, nullptr, &device);
    if (FAILED(hr))
        return hr;

    hr = m_direct3D9->GetAdapterDisplayMode(adapterID, &m_displayMode);
    if (FAILED(hr))
        return hr;

    hr = m_deviceManager->ResetDevice(device.get(), m_deviceResetToken);
    if (FAILED(hr))
        return hr;

    m_device = device;

    return hr;
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::createD3DSample(IDirect3DSwapChain9* swapChain, COMPtr<IMFSample>& videoSample)
{
    COMPtr<IDirect3DSurface9> surface;
    HRESULT hr = swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &surface);
    if (FAILED(hr))
        return hr;

    D3DCOLOR colorBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);
    hr = m_device->ColorFill(surface.get(), nullptr, colorBlack);
    if (FAILED(hr))
        return hr;

    return MFCreateVideoSampleFromSurfacePtr()(surface.get(), &videoSample);
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::presentSwapChain(IDirect3DSwapChain9* swapChain, IDirect3DSurface9* surface)
{
    if (!m_hwnd)
        return MF_E_INVALIDREQUEST;

    return swapChain->Present(nullptr, &m_destRect, m_hwnd, nullptr, 0);
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::getSwapChainPresentParameters(IMFMediaType* type, D3DPRESENT_PARAMETERS* presentParams)
{
    if (!m_hwnd)
        return MF_E_INVALIDREQUEST;

    COMPtr<IMFMediaType> videoType = type;

    UINT32 width = 0, height = 0;
    HRESULT hr = MFGetAttributeSize(videoType.get(), MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr))
        return hr;

    GUID guidSubType = GUID_NULL;
    hr = videoType->GetGUID(MF_MT_SUBTYPE, &guidSubType);
    if (FAILED(hr))
        return hr;

    DWORD d3dFormat = guidSubType.Data1;

    ZeroMemory(presentParams, sizeof(D3DPRESENT_PARAMETERS));
    presentParams->BackBufferWidth = width;
    presentParams->BackBufferHeight = height;
    presentParams->Windowed = TRUE;
    presentParams->SwapEffect = D3DSWAPEFFECT_COPY;
    presentParams->BackBufferFormat = (D3DFORMAT)d3dFormat;
    presentParams->hDeviceWindow = m_hwnd;
    presentParams->Flags = D3DPRESENTFLAG_VIDEO;
    presentParams->PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    D3DDEVICE_CREATION_PARAMETERS params;
    hr = m_device->GetCreationParameters(&params);
    if (FAILED(hr))
        return hr;

    if (params.DeviceType != D3DDEVTYPE_HAL)
        presentParams->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    return S_OK;
}

HRESULT MediaPlayerPrivateMediaFoundation::Direct3DPresenter::updateDestRect()
{
    if (!m_hwnd)
        return S_FALSE;

    RECT rcView;
    GetClientRect(m_hwnd, &rcView);

    // Clip to the client area of the window.
    if (m_destRect.right > rcView.right)
        m_destRect.right = rcView.right;

    if (m_destRect.bottom > rcView.bottom)
        m_destRect.bottom = rcView.bottom;

    return S_OK;
}

} // namespace WebCore

#endif
