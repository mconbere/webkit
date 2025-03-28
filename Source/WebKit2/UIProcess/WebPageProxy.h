/*
 * Copyright (C) 2010, 2011, 2014-2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebPageProxy_h
#define WebPageProxy_h

#include "APIObject.h"
#include "APISession.h"
#include "AssistedNodeInformation.h"
#include "AutoCorrectionCallback.h"
#include "Connection.h"
#include "ContextMenuContextData.h"
#include "DownloadID.h"
#include "DragControllerAction.h"
#include "DrawingAreaProxy.h"
#include "EditingRange.h"
#include "EditorState.h"
#include "GeolocationPermissionRequestManagerProxy.h"
#include "LayerTreeContext.h"
#include "MessageSender.h"
#include "NotificationPermissionRequestManagerProxy.h"
#include "PageLoadState.h"
#include "ProcessThrottler.h"
#include "SandboxExtension.h"
#include "ShareableBitmap.h"
#include "UserMediaPermissionRequestManagerProxy.h"
#include "VisibleContentRectUpdateInfo.h"
#include "WKBase.h"
#include "WKPagePrivate.h"
#include "WebColorPicker.h"
#include "WebContextMenuItemData.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrameProxy.h"
#include "WebPageCreationParameters.h"
#include "WebPageDiagnosticLoggingClient.h"
#include "WebPageInjectedBundleClient.h"
#include "WebPreferences.h"
#include <WebCore/AlternativeTextClient.h> // FIXME: Needed by WebPageProxyMessages.h for DICTATION_ALTERNATIVES.
#include "WebPageProxyMessages.h"
#include "WebPopupMenuProxy.h"
#include "WebProcessLifetimeTracker.h"
#include <WebCore/Color.h>
#include <WebCore/DragActions.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/MediaProducer.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformScreen.h>
#include <WebCore/ScrollTypes.h>
#include <WebCore/SearchPopupMenu.h>
#include <WebCore/TextChecking.h>
#include <WebCore/TextGranularity.h>
#include <WebCore/ViewState.h>
#include <WebCore/VisibleSelection.h>
#include <memory>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Ref.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

OBJC_CLASS NSView;
OBJC_CLASS _WKRemoteObjectRegistry;

#if ENABLE(DRAG_SUPPORT)
#include <WebCore/DragActions.h>
#endif

#if ENABLE(TOUCH_EVENTS)
#include "NativeWebTouchEvent.h"
#endif

#if PLATFORM(EFL)
#include "WKPageEfl.h"
#include "WebUIPopupMenuClient.h"
#include <Evas.h>
#endif

#if PLATFORM(COCOA)
#include "LayerRepresentation.h"
#endif

#if PLATFORM(MAC)
#include "AttributedString.h"
#endif

#if PLATFORM(IOS)
#include "ProcessThrottler.h"
#endif

#if PLATFORM(GTK)
#include "ArgumentCodersGtk.h"
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
#include <WebCore/MediaPlaybackTargetPicker.h>
#include <WebCore/WebMediaSessionManagerClient.h>
#endif

#if USE(APPLE_INTERNAL_SDK)
#include <WebKitAdditions/WebPageProxyIncludes.h>
#endif

#if ENABLE(MEDIA_SESSION)
namespace WebCore {
class MediaSessionMetadata;
}
#endif

namespace API {
class ContextMenuClient;
class FindClient;
class FindMatchesClient;
class FormClient;
class HistoryClient;
class LoaderClient;
class Navigation;
class NavigationClient;
class PolicyClient;
class UIClient;
class URLRequest;
}

namespace IPC {
class ArgumentDecoder;
class Connection;
}

namespace WebCore {
class AuthenticationChallenge;
class Cursor;
class DragData;
class FloatRect;
class GraphicsLayer;
class IntSize;
class MediaConstraintsImpl;
class ProtectionSpace;
class RunLoopObserver;
class SharedBuffer;
class TextIndicator;
struct DictionaryPopupInfo;
struct ExceptionDetails;
struct FileChooserSettings;
struct SecurityOriginData;
struct TextAlternativeWithRange;
struct TextCheckingResult;
struct ViewportAttributes;
struct WindowFeatures;
}

#if PLATFORM(GTK)
typedef GtkWidget* PlatformWidget;
#endif

namespace WebKit {

class CertificateInfo;
class NativeWebGestureEvent;
class NativeWebKeyboardEvent;
class NativeWebMouseEvent;
class NativeWebWheelEvent;
class PageClient;
class RemoteLayerTreeScrollingPerformanceData;
class RemoteLayerTreeTransaction;
class RemoteScrollingCoordinatorProxy;
class StringPairVector;
class UserData;
class ViewSnapshot;
class VisitedLinkStore;
class WebBackForwardList;
class WebBackForwardListItem;
class WebContextMenuProxy;
class WebEditCommandProxy;
class WebFullScreenManagerProxy;
class WebNavigationState;
class WebVideoFullscreenManagerProxy;
class WebKeyboardEvent;
class WebMouseEvent;
class WebOpenPanelResultListenerProxy;
class WebPageGroup;
class WebProcessProxy;
class WebUserContentControllerProxy;
class WebWheelEvent;
class WebsiteDataStore;
struct AttributedString;
struct ColorSpaceData;
struct EditingRange;
struct EditorState;
struct PlatformPopupMenuData;
struct PrintInfo;
struct WebPopupItem;

#if ENABLE(VIBRATION)
class WebVibrationProxy;
#endif

#if USE(QUICK_LOOK)
class QuickLookDocumentData;
#endif

typedef GenericCallback<uint64_t> UnsignedCallback;
typedef GenericCallback<EditingRange> EditingRangeCallback;
typedef GenericCallback<const String&> StringCallback;
typedef GenericCallback<API::SerializedScriptValue*, bool, const WebCore::ExceptionDetails&> ScriptValueCallback;

#if PLATFORM(GTK)
typedef GenericCallback<API::Error*> PrintFinishedCallback;
#endif

#if ENABLE(TOUCH_EVENTS)
struct QueuedTouchEvents {
    QueuedTouchEvents(const NativeWebTouchEvent& event)
        : forwardedEvent(event)
    {
    }
    NativeWebTouchEvent forwardedEvent;
    Vector<NativeWebTouchEvent> deferredTouchEvents;
};
#endif

typedef GenericCallback<const String&, bool, int32_t> ValidateCommandCallback;
typedef GenericCallback<const WebCore::IntRect&, const EditingRange&> RectForCharacterRangeCallback;

#if PLATFORM(MAC)
typedef GenericCallback<const AttributedString&, const EditingRange&> AttributedStringForCharacterRangeCallback;
typedef GenericCallback<const String&, double, bool> FontAtSelectionCallback;
#endif

#if PLATFORM(IOS)
typedef GenericCallback<const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t> GestureCallback;
typedef GenericCallback<const WebCore::IntPoint&, uint32_t> TouchesCallback;
struct NodeAssistanceArguments {
    AssistedNodeInformation m_nodeInformation;
    bool m_userIsInteracting;
    bool m_blurPreviousNode;
    RefPtr<API::Object> m_userData;
};
#endif

#if PLATFORM(COCOA)
typedef GenericCallback<const WebCore::MachSendRight&> MachSendRightCallback;
#endif

class WebPageProxy : public API::ObjectImpl<API::Object::Type::Page>
#if ENABLE(INPUT_TYPE_COLOR)
    , public WebColorPicker::Client
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    , public WebCore::WebMediaSessionManagerClient
#endif
    , public WebPopupMenuProxy::Client
    , public IPC::MessageReceiver
    , public IPC::MessageSender {
public:
    static Ref<WebPageProxy> create(PageClient&, WebProcessProxy&, uint64_t pageID, Ref<API::PageConfiguration>&&);
    virtual ~WebPageProxy();

    const API::PageConfiguration& configuration() const;

    uint64_t pageID() const { return m_pageID; }
    WebCore::SessionID sessionID() const { return m_sessionID; }
    void setSessionID(WebCore::SessionID);

    WebFrameProxy* mainFrame() const { return m_mainFrame.get(); }
    WebFrameProxy* focusedFrame() const { return m_focusedFrame.get(); }
    WebFrameProxy* frameSetLargestFrame() const { return m_frameSetLargestFrame.get(); }

    DrawingAreaProxy* drawingArea() const { return m_drawingArea.get(); }

    WebNavigationState& navigationState() { return *m_navigationState.get(); }

    WebsiteDataStore& websiteDataStore() { return m_websiteDataStore; }

#if ENABLE(DATA_DETECTION)
    NSArray *dataDetectionResults() { return m_dataDetectionResults.get(); }
#endif
        
#if ENABLE(ASYNC_SCROLLING)
    RemoteScrollingCoordinatorProxy* scrollingCoordinatorProxy() const { return m_scrollingCoordinatorProxy.get(); }
#endif

    WebBackForwardList& backForwardList() { return m_backForwardList; }

    bool addsVisitedLinks() const { return m_addsVisitedLinks; }
    void setAddsVisitedLinks(bool addsVisitedLinks) { m_addsVisitedLinks = addsVisitedLinks; }

    void fullscreenMayReturnToInline();
    void didEnterFullscreen();
    void didExitFullscreen();

    WebInspectorProxy* inspector();

#if ENABLE(REMOTE_INSPECTOR)
    bool allowsRemoteInspection() const { return m_allowsRemoteInspection; }
    void setAllowsRemoteInspection(bool);
    String remoteInspectionNameOverride() const { return m_remoteInspectionNameOverride; }
    void setRemoteInspectionNameOverride(const String&);
#endif

#if ENABLE(VIBRATION)
    WebVibrationProxy* vibration() { return m_vibration.get(); }
#endif

#if ENABLE(FULLSCREEN_API)
    WebFullScreenManagerProxy* fullScreenManager();
#endif
#if PLATFORM(IOS) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))
    RefPtr<WebVideoFullscreenManagerProxy> videoFullscreenManager();
#endif

#if PLATFORM(IOS)
    bool allowsMediaDocumentInlinePlayback() const;
    void setAllowsMediaDocumentInlinePlayback(bool);
#endif

#if ENABLE(CONTEXT_MENUS)
    API::ContextMenuClient& contextMenuClient() { return *m_contextMenuClient; }
    void setContextMenuClient(std::unique_ptr<API::ContextMenuClient>);
#endif
    API::FindClient& findClient() { return *m_findClient; }
    void setFindClient(std::unique_ptr<API::FindClient>);
    API::FindMatchesClient& findMatchesClient() { return *m_findMatchesClient; }
    void setFindMatchesClient(std::unique_ptr<API::FindMatchesClient>);
    API::DiagnosticLoggingClient& diagnosticLoggingClient() { return *m_diagnosticLoggingClient; }
    void setDiagnosticLoggingClient(std::unique_ptr<API::DiagnosticLoggingClient>);
    void setFormClient(std::unique_ptr<API::FormClient>);
    void setNavigationClient(std::unique_ptr<API::NavigationClient>);
    void setHistoryClient(std::unique_ptr<API::HistoryClient>);
    void setLoaderClient(std::unique_ptr<API::LoaderClient>);
    void setPolicyClient(std::unique_ptr<API::PolicyClient>);
    void setInjectedBundleClient(const WKPageInjectedBundleClientBase*);
    WebPageInjectedBundleClient* injectedBundleClient() { return m_injectedBundleClient.get(); }

    API::UIClient& uiClient() { return *m_uiClient; }
    void setUIClient(std::unique_ptr<API::UIClient>);
#if PLATFORM(EFL)
    void initializeUIPopupMenuClient(const WKPageUIPopupMenuClientBase*);
#endif

    void initializeWebPage();

    void close();
    bool tryClose();
    bool isClosed() const { return m_isClosed; }

    RefPtr<API::Navigation> loadRequest(const WebCore::ResourceRequest&, WebCore::ShouldOpenExternalURLsPolicy = WebCore::ShouldOpenExternalURLsPolicy::ShouldAllowExternalSchemes, API::Object* userData = nullptr);
    RefPtr<API::Navigation> loadFile(const String& fileURL, const String& resourceDirectoryURL, API::Object* userData = nullptr);
    RefPtr<API::Navigation> loadData(API::Data*, const String& MIMEType, const String& encoding, const String& baseURL, API::Object* userData = nullptr);
    RefPtr<API::Navigation> loadHTMLString(const String& htmlString, const String& baseURL, API::Object* userData = nullptr);
    void loadAlternateHTMLString(const String& htmlString, const String& baseURL, const String& unreachableURL, API::Object* userData = nullptr);
    void loadPlainTextString(const String&, API::Object* userData = nullptr);
    void loadWebArchiveData(API::Data*, API::Object* userData = nullptr);
    void navigateToPDFLinkWithSimulatedClick(const String& url, WebCore::IntPoint documentPoint, WebCore::IntPoint screenPoint);

    void stopLoading();
    RefPtr<API::Navigation> reload(bool reloadFromOrigin, bool contentBlockersEnabled);

    RefPtr<API::Navigation> goForward();
    RefPtr<API::Navigation> goBack();

    RefPtr<API::Navigation> goToBackForwardItem(WebBackForwardListItem*);
    void tryRestoreScrollPosition();
    void didChangeBackForwardList(WebBackForwardListItem* addedItem, Vector<RefPtr<WebBackForwardListItem>> removed);
    void willGoToBackForwardListItem(uint64_t itemID, const UserData&);

    bool shouldKeepCurrentBackForwardListItemInList(WebBackForwardListItem*);

    bool willHandleHorizontalScrollEvents() const;

    bool canShowMIMEType(const String& mimeType);

    bool drawsBackground() const { return m_drawsBackground; }
    void setDrawsBackground(bool);

    float topContentInset() const { return m_topContentInset; }
    void setTopContentInset(float);

    WebCore::Color underlayColor() const { return m_underlayColor; }
    void setUnderlayColor(const WebCore::Color&);

    // At this time, m_pageExtendedBackgroundColor can be set via pageExtendedBackgroundColorDidChange() which is a message
    // from the UIProcess, or by didCommitLayerTree(). When PLATFORM(MAC) adopts UI side compositing, we should get rid of
    // the message entirely.
    WebCore::Color pageExtendedBackgroundColor() const { return m_pageExtendedBackgroundColor; }

    void viewWillStartLiveResize();
    void viewWillEndLiveResize();

    void setInitialFocus(bool forward, bool isKeyboardEventValid, const WebKeyboardEvent&, std::function<void (CallbackBase::Error)>);
    
    void clearSelection();
    void restoreSelectionInFocusedEditableElement();

    void setViewNeedsDisplay(const WebCore::IntRect&);
    void displayView();
    bool canScrollView();
    void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset); // FIXME: CoordinatedGraphics should use requestScroll().
    void requestScroll(const WebCore::FloatPoint& scrollPosition, const WebCore::IntPoint& scrollOrigin, bool isProgrammaticScroll);
    
    void setDelegatesScrolling(bool delegatesScrolling) { m_delegatesScrolling = delegatesScrolling; }
    bool delegatesScrolling() const { return m_delegatesScrolling; }

    enum class ViewStateChangeDispatchMode { Deferrable, Immediate };
    void viewStateDidChange(WebCore::ViewState::Flags mayHaveChanged, bool wantsSynchronousReply = false, ViewStateChangeDispatchMode = ViewStateChangeDispatchMode::Deferrable);
    bool isInWindow() const { return m_viewState & WebCore::ViewState::IsInWindow; }
    void waitForDidUpdateViewState();
    void didUpdateViewState() { m_waitingForDidUpdateViewState = false; }

    void layerHostingModeDidChange();

    WebCore::IntSize viewSize() const;
    bool isViewVisible() const { return m_viewState & WebCore::ViewState::IsVisible; }
    bool isViewWindowActive() const;

    void addMIMETypeWithCustomContentProvider(const String& mimeType);

    void executeEditCommand(const String& commandName, const String& argument = String());
    void validateCommand(const String& commandName, std::function<void (const String&, bool, int32_t, CallbackBase::Error)>);

    const EditorState& editorState() const { return m_editorState; }
    bool canDelete() const { return hasSelectedRange() && isContentEditable(); }
    bool hasSelectedRange() const { return m_editorState.selectionIsRange; }
    bool isContentEditable() const { return m_editorState.isContentEditable; }
    
    bool maintainsInactiveSelection() const { return m_maintainsInactiveSelection; }
    void setMaintainsInactiveSelection(bool);
    void setEditable(bool);
    bool isEditable() const { return m_isEditable; }

#if PLATFORM(IOS)
    void executeEditCommand(const String& commandName, std::function<void (CallbackBase::Error)>);
    double displayedContentScale() const { return m_lastVisibleContentRectUpdate.scale(); }
    const WebCore::FloatRect& exposedContentRect() const { return m_lastVisibleContentRectUpdate.exposedRect(); }
    const WebCore::FloatRect& unobscuredContentRect() const { return m_lastVisibleContentRectUpdate.unobscuredRect(); }

    void updateVisibleContentRects(const WebCore::FloatRect& exposedRect, const WebCore::FloatRect& unobscuredRect, const WebCore::FloatRect& unobscuredRectInScrollViewCoordinates, const WebCore::FloatRect& customFixedPositionRect, double scale, bool inStableState, bool isChangingObscuredInsetsInteractively, bool allowShrinkToFit, double timestamp, double horizontalVelocity, double verticalVelocity, double scaleChangeRate);
    void resendLastVisibleContentRects();

    enum class UnobscuredRectConstraint { ConstrainedToDocumentRect, Unconstrained };
    WebCore::FloatRect computeCustomFixedPositionRect(const WebCore::FloatRect& unobscuredContentRect, double displayedContentScale, UnobscuredRectConstraint = UnobscuredRectConstraint::Unconstrained) const;
    void overflowScrollViewWillStartPanGesture();
    void overflowScrollViewDidScroll();
    void overflowScrollWillStartScroll();
    void overflowScrollDidEndScroll();

    void dynamicViewportSizeUpdate(const WebCore::FloatSize& minimumLayoutSize, const WebCore::FloatSize& maximumUnobscuredSize, const WebCore::FloatRect& targetExposedContentRect, const WebCore::FloatRect& targetUnobscuredRect, const WebCore::FloatRect& targetUnobscuredRectInScrollViewCoordinates, double targetScale, int32_t deviceOrientation);
    void synchronizeDynamicViewportUpdate();

    void setViewportConfigurationMinimumLayoutSize(const WebCore::FloatSize&);
    void setMaximumUnobscuredSize(const WebCore::FloatSize&);
    void setDeviceOrientation(int32_t);
    int32_t deviceOrientation() const { return m_deviceOrientation; }
    void willCommitLayerTree(uint64_t transactionID);

    void selectWithGesture(const WebCore::IntPoint, WebCore::TextGranularity, uint32_t gestureType, uint32_t gestureState, bool isInteractingWithAssistedNode, std::function<void (const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t, CallbackBase::Error)>);
    void updateSelectionWithTouches(const WebCore::IntPoint, uint32_t touches, bool baseIsStart, std::function<void (const WebCore::IntPoint&, uint32_t, CallbackBase::Error)>);
    void selectWithTwoTouches(const WebCore::IntPoint from, const WebCore::IntPoint to, uint32_t gestureType, uint32_t gestureState, std::function<void (const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t, CallbackBase::Error)>);
    void updateBlockSelectionWithTouch(const WebCore::IntPoint, uint32_t touch, uint32_t handlePosition);
    void extendSelection(WebCore::TextGranularity);
    void selectWordBackward();
    void moveSelectionByOffset(int32_t offset, std::function<void (CallbackBase::Error)>);
    void selectTextWithGranularityAtPoint(const WebCore::IntPoint, WebCore::TextGranularity, bool isInteractingWithAssistedNode, std::function<void (CallbackBase::Error)>);
    void selectPositionAtPoint(const WebCore::IntPoint, bool isInteractingWithAssistedNode, std::function<void (CallbackBase::Error)>);
    void selectPositionAtBoundaryWithDirection(const WebCore::IntPoint, WebCore::TextGranularity, WebCore::SelectionDirection, bool isInteractingWithAssistedNode, std::function<void (CallbackBase::Error)>);
    void moveSelectionAtBoundaryWithDirection(WebCore::TextGranularity, WebCore::SelectionDirection, std::function<void(CallbackBase::Error)>);
    void beginSelectionInDirection(WebCore::SelectionDirection, std::function<void (uint64_t, CallbackBase::Error)>);
    void updateSelectionWithExtentPoint(const WebCore::IntPoint, bool isInteractingWithAssistedNode, std::function<void (uint64_t, CallbackBase::Error)>);
    void updateSelectionWithExtentPointAndBoundary(const WebCore::IntPoint, WebCore::TextGranularity, bool isInteractingWithAssistedNode, std::function<void(uint64_t, CallbackBase::Error)>);
    void requestAutocorrectionData(const String& textForAutocorrection, std::function<void (const Vector<WebCore::FloatRect>&, const String&, double, uint64_t, CallbackBase::Error)>);
    void applyAutocorrection(const String& correction, const String& originalText, std::function<void (const String&, CallbackBase::Error)>);
    bool applyAutocorrection(const String& correction, const String& originalText);
    void requestAutocorrectionContext(std::function<void (const String&, const String&, const String&, const String&, uint64_t, uint64_t, CallbackBase::Error)>);
    void getAutocorrectionContext(String& contextBefore, String& markedText, String& selectedText, String& contextAfter, uint64_t& location, uint64_t& length);
    void requestDictationContext(std::function<void (const String&, const String&, const String&, CallbackBase::Error)>);
    void replaceDictatedText(const String& oldText, const String& newText);
    void replaceSelectedText(const String& oldText, const String& newText);
    void didReceivePositionInformation(const InteractionInformationAtPosition&);
    void getPositionInformation(const WebCore::IntPoint&, InteractionInformationAtPosition&);
    void requestPositionInformation(const WebCore::IntPoint&);
    void startInteractionWithElementAtPosition(const WebCore::IntPoint&);
    void stopInteraction();
    void performActionOnElement(uint32_t action);
    void saveImageToLibrary(const SharedMemory::Handle& imageHandle, uint64_t imageSize);
    void didUpdateBlockSelectionWithTouch(uint32_t touch, uint32_t flags, float growThreshold, float shrinkThreshold);
    void focusNextAssistedNode(bool isForward, std::function<void (CallbackBase::Error)>);
    void setAssistedNodeValue(const String&);
    void setAssistedNodeValueAsNumber(double);
    void setAssistedNodeSelectedIndex(uint32_t index, bool allowMultipleSelection = false);
    void applicationDidEnterBackground();
    void applicationWillEnterForeground();
    void applicationWillResignActive();
    void applicationDidBecomeActive();
    void zoomToRect(WebCore::FloatRect, double minimumScale, double maximumScale);
    void commitPotentialTapFailed();
    void didNotHandleTapAsClick(const WebCore::IntPoint&);
    void disableDoubleTapGesturesDuringTapIfNecessary(uint64_t requestID);
    void didFinishDrawingPagesToPDF(const IPC::DataReference&);
    void contentSizeCategoryDidChange(const String& contentSizeCategory);
    void getLookupContextAtPoint(const WebCore::IntPoint&, std::function<void(const String&, CallbackBase::Error)>);
#endif
#if ENABLE(DATA_DETECTION)
    void setDataDetectionResult(const DataDetectionResult&);
#endif
    void didCommitLayerTree(const WebKit::RemoteLayerTreeTransaction&);

#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
    void didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect);
    void commitPageTransitionViewport();
#endif
#if PLATFORM(EFL)
    void setThemePath(const String&);
#endif

#if PLATFORM(GTK)
    void setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    void confirmComposition(const String& compositionString, int64_t selectionStart, int64_t selectionLength);
    void cancelComposition();
#endif

#if PLATFORM(GTK)
    void setInputMethodState(bool enabled);
#endif

#if PLATFORM (GTK) && HAVE(GTK_GESTURES)
    void getCenterForZoomGesture(const WebCore::IntPoint& centerInViewCoordinates, WebCore::IntPoint& center);
#endif

#if PLATFORM(COCOA)
    void windowAndViewFramesChanged(const WebCore::FloatRect& viewFrameInWindowCoordinates, const WebCore::FloatPoint& accessibilityViewCoordinates);
    void setMainFrameIsScrollable(bool);

    void sendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput);
    bool shouldDelayWindowOrderingForEvent(const WebMouseEvent&);
    bool acceptsFirstMouse(int eventNumber, const WebMouseEvent&);

    void setAcceleratedCompositingRootLayer(LayerOrView*);
    LayerOrView* acceleratedCompositingRootLayer() const;

    void insertTextAsync(const String& text, const EditingRange& replacementRange, bool registerUndoGroup = false, EditingRangeIsRelativeTo = EditingRangeIsRelativeTo::EditableRoot);
    void getMarkedRangeAsync(std::function<void (EditingRange, CallbackBase::Error)>);
    void getSelectedRangeAsync(std::function<void (EditingRange, CallbackBase::Error)>);
    void characterIndexForPointAsync(const WebCore::IntPoint&, std::function<void (uint64_t, CallbackBase::Error)>);
    void firstRectForCharacterRangeAsync(const EditingRange&, std::function<void (const WebCore::IntRect&, const EditingRange&, CallbackBase::Error)>);
    void setCompositionAsync(const String& text, Vector<WebCore::CompositionUnderline> underlines, const EditingRange& selectionRange, const EditingRange& replacementRange);
    void confirmCompositionAsync();

    void setScrollPerformanceDataCollectionEnabled(bool);
    bool scrollPerformanceDataCollectionEnabled() const { return m_scrollPerformanceDataCollectionEnabled; }
    RemoteLayerTreeScrollingPerformanceData* scrollingPerformanceData() { return m_scrollingPerformanceData.get(); }
#endif // PLATFORM(COCOA)

#if PLATFORM(MAC)
    void insertDictatedTextAsync(const String& text, const EditingRange& replacementRange, const Vector<WebCore::TextAlternativeWithRange>& dictationAlternatives, bool registerUndoGroup);
    void attributedSubstringForCharacterRangeAsync(const EditingRange&, std::function<void (const AttributedString&, const EditingRange&, CallbackBase::Error)>);
    void setFont(const String& fontFamily, double fontSize, uint64_t fontTraits);
    void fontAtSelection(std::function<void (const String&, double, bool, CallbackBase::Error)>);

    void startWindowDrag();
    NSWindow *platformWindow();

#if WK_API_ENABLED
    NSView *inspectorAttachmentView();
    _WKRemoteObjectRegistry *remoteObjectRegistry();
#endif

    void intrinsicContentSizeDidChange(const WebCore::IntSize& intrinsicContentSize);
    CGRect boundsOfLayerInLayerBackedWindowCoordinates(CALayer *) const;
#endif // PLATFORM(MAC)

#if PLATFORM(EFL)
    void handleInputMethodKeydown(bool& handled);
    void confirmComposition(const String&);
    void setComposition(const String&, Vector<WebCore::CompositionUnderline>&, int);
    void cancelComposition();
#endif

#if PLATFORM(GTK)
    PlatformWidget viewWidget();
    const WebCore::Color& backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const WebCore::Color& color) { m_backgroundColor = color; }
#endif

    void handleMouseEvent(const NativeWebMouseEvent&);
    void handleWheelEvent(const NativeWebWheelEvent&);
    void handleKeyboardEvent(const NativeWebKeyboardEvent&);

#if ENABLE(MAC_GESTURE_EVENTS)
    void handleGestureEvent(const NativeWebGestureEvent&);
#endif

#if ENABLE(IOS_TOUCH_EVENTS)
    void handleTouchEventSynchronously(const NativeWebTouchEvent&);
    void handleTouchEventAsynchronously(const NativeWebTouchEvent&);

#elif ENABLE(TOUCH_EVENTS)
    void handleTouchEvent(const NativeWebTouchEvent&);
#endif

    void scrollBy(WebCore::ScrollDirection, WebCore::ScrollGranularity);
    void centerSelectionInVisibleArea();

    const String& toolTip() const { return m_toolTip; }

    const String& userAgent() const { return m_userAgent; }
    void setApplicationNameForUserAgent(const String&);
    const String& applicationNameForUserAgent() const { return m_applicationNameForUserAgent; }
    void setCustomUserAgent(const String&);
    const String& customUserAgent() const { return m_customUserAgent; }
    static String standardUserAgent(const String& applicationName = String());

    bool supportsTextEncoding() const;
    void setCustomTextEncodingName(const String&);
    String customTextEncodingName() const { return m_customTextEncodingName; }

    bool areActiveDOMObjectsAndAnimationsSuspended() const { return m_isPageSuspended; }
    void resumeActiveDOMObjectsAndAnimations();
    void suspendActiveDOMObjectsAndAnimations();

    double estimatedProgress() const;

    void terminateProcess();

    SessionState sessionState(const std::function<bool (WebBackForwardListItem&)>& = nullptr) const;
    RefPtr<API::Navigation> restoreFromSessionState(SessionState, bool navigate);

    bool supportsTextZoom() const;
    double textZoomFactor() const { return m_textZoomFactor; }
    void setTextZoomFactor(double);
    double pageZoomFactor() const;
    void setPageZoomFactor(double);
    void setPageAndTextZoomFactors(double pageZoomFactor, double textZoomFactor);

    void scalePage(double scale, const WebCore::IntPoint& origin);
    void scalePageInViewCoordinates(double scale, const WebCore::IntPoint& centerInViewCoordinates);
    double pageScaleFactor() const;
    double viewScaleFactor() const { return m_viewScaleFactor; }
    void scaleView(double scale);
    void setShouldScaleViewToFitDocument(bool);
    
    float deviceScaleFactor() const;
    void setIntrinsicDeviceScaleFactor(float);
    void setCustomDeviceScaleFactor(float);
    void windowScreenDidChange(PlatformDisplayID);

    void setUseFixedLayout(bool);
    void setFixedLayoutSize(const WebCore::IntSize&);
    bool useFixedLayout() const { return m_useFixedLayout; };
    const WebCore::IntSize& fixedLayoutSize() const { return m_fixedLayoutSize; };

    void listenForLayoutMilestones(WebCore::LayoutMilestones);

    bool hasHorizontalScrollbar() const { return m_mainFrameHasHorizontalScrollbar; }
    bool hasVerticalScrollbar() const { return m_mainFrameHasVerticalScrollbar; }

    void setSuppressScrollbarAnimations(bool);
    bool areScrollbarAnimationsSuppressed() const { return m_suppressScrollbarAnimations; }

    bool isPinnedToLeftSide() const { return m_mainFrameIsPinnedToLeftSide; }
    bool isPinnedToRightSide() const { return m_mainFrameIsPinnedToRightSide; }
    bool isPinnedToTopSide() const { return m_mainFrameIsPinnedToTopSide; }
    bool isPinnedToBottomSide() const { return m_mainFrameIsPinnedToBottomSide; }

    bool rubberBandsAtLeft() const;
    void setRubberBandsAtLeft(bool);
    bool rubberBandsAtRight() const;
    void setRubberBandsAtRight(bool);
    bool rubberBandsAtTop() const;
    void setRubberBandsAtTop(bool);
    bool rubberBandsAtBottom() const;
    void setRubberBandsAtBottom(bool);

    void setShouldUseImplicitRubberBandControl(bool shouldUseImplicitRubberBandControl) { m_shouldUseImplicitRubberBandControl = shouldUseImplicitRubberBandControl; }
    bool shouldUseImplicitRubberBandControl() const { return m_shouldUseImplicitRubberBandControl; }
        
    void setEnableVerticalRubberBanding(bool);
    bool verticalRubberBandingIsEnabled() const;
    void setEnableHorizontalRubberBanding(bool);
    bool horizontalRubberBandingIsEnabled() const;
        
    void setBackgroundExtendsBeyondPage(bool);
    bool backgroundExtendsBeyondPage() const;

    void setPaginationMode(WebCore::Pagination::Mode);
    WebCore::Pagination::Mode paginationMode() const { return m_paginationMode; }
    void setPaginationBehavesLikeColumns(bool);
    bool paginationBehavesLikeColumns() const { return m_paginationBehavesLikeColumns; }
    void setPageLength(double);
    double pageLength() const { return m_pageLength; }
    void setGapBetweenPages(double);
    double gapBetweenPages() const { return m_gapBetweenPages; }
    void setPaginationLineGridEnabled(bool);
    bool paginationLineGridEnabled() const { return m_paginationLineGridEnabled; }
    unsigned pageCount() const { return m_pageCount; }

#if PLATFORM(COCOA)
    // Called by the web process through a message.
    void registerWebProcessAccessibilityToken(const IPC::DataReference&);
    // Called by the UI process when it is ready to send its tokens to the web process.
    void registerUIProcessAccessibilityTokens(const IPC::DataReference& elemenToken, const IPC::DataReference& windowToken);
    bool readSelectionFromPasteboard(const String& pasteboardName);
    String stringSelectionForPasteboard();
    RefPtr<WebCore::SharedBuffer> dataSelectionForPasteboard(const String& pasteboardType);
    void makeFirstResponder();

    ColorSpaceData colorSpace();
#endif

#if ENABLE(SERVICE_CONTROLS)
    void replaceSelectionWithPasteboardData(const Vector<String>& types, const IPC::DataReference&);
#endif

    void pageScaleFactorDidChange(double);
    void pluginScaleFactorDidChange(double);
    void pluginZoomFactorDidChange(double);

    // Find.
    void findString(const String&, FindOptions, unsigned maxMatchCount);
    void findStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void getImageForFindMatch(int32_t matchIndex);
    void selectFindMatch(int32_t matchIndex);
    void didGetImageForFindMatch(const ShareableBitmap::Handle& contentImageHandle, uint32_t matchIndex);
    void hideFindUI();
    void countStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void didCountStringMatches(const String&, uint32_t matchCount);
    void setTextIndicator(const WebCore::TextIndicatorData&, uint64_t /* WebCore::TextIndicatorWindowLifetime */ lifetime = 0 /* Permanent */);
    void setTextIndicatorAnimationProgress(float);
    void clearTextIndicator();
    void didFindString(const String&, const Vector<WebCore::IntRect>&, uint32_t matchCount, int32_t matchIndex);
    void didFailToFindString(const String&);
    void didFindStringMatches(const String&, const Vector<Vector<WebCore::IntRect>>& matchRects, int32_t firstIndexAfterSelection);

    void getContentsAsString(std::function<void (const String&, CallbackBase::Error)>);
    void getBytecodeProfile(std::function<void (const String&, CallbackBase::Error)>);
    void isWebProcessResponsive(std::function<void (bool isWebProcessResponsive)>);

#if ENABLE(MHTML)
    void getContentsAsMHTMLData(std::function<void (API::Data*, CallbackBase::Error)>);
#endif
    void getMainResourceDataOfFrame(WebFrameProxy*, std::function<void (API::Data*, CallbackBase::Error)>);
    void getResourceDataFromFrame(WebFrameProxy*, API::URL*, std::function<void (API::Data*, CallbackBase::Error)>);
    void getRenderTreeExternalRepresentation(std::function<void (const String&, CallbackBase::Error)>);
    void getSelectionOrContentsAsString(std::function<void (const String&, CallbackBase::Error)>);
    void getSelectionAsWebArchiveData(std::function<void (API::Data*, CallbackBase::Error)>);
    void getSourceForFrame(WebFrameProxy*, std::function<void (const String&, CallbackBase::Error)>);
    void getWebArchiveOfFrame(WebFrameProxy*, std::function<void (API::Data*, CallbackBase::Error)>);
    void runJavaScriptInMainFrame(const String&, std::function<void (API::SerializedScriptValue*, bool hadException, const WebCore::ExceptionDetails&, CallbackBase::Error)> callbackFunction);
    void forceRepaint(PassRefPtr<VoidCallback>);

    float headerHeight(WebFrameProxy*);
    float footerHeight(WebFrameProxy*);
    void drawHeader(WebFrameProxy*, const WebCore::FloatRect&);
    void drawFooter(WebFrameProxy*, const WebCore::FloatRect&);

#if PLATFORM(COCOA)
    // Dictionary.
    void performDictionaryLookupAtLocation(const WebCore::FloatPoint&);
    void performDictionaryLookupOfCurrentSelection();
#endif

    void receivedPolicyDecision(WebCore::PolicyAction, WebFrameProxy*, uint64_t listenerID, API::Navigation* navigationID);

    void backForwardRemovedItem(uint64_t itemID);

#if ENABLE(DRAG_SUPPORT)    
    // Drag and drop support.
    void dragEntered(WebCore::DragData&, const String& dragStorageName = String());
    void dragUpdated(WebCore::DragData&, const String& dragStorageName = String());
    void dragExited(WebCore::DragData&, const String& dragStorageName = String());
    void performDragOperation(WebCore::DragData&, const String& dragStorageName, const SandboxExtension::Handle&, const SandboxExtension::HandleArray&);

    void didPerformDragControllerAction(uint64_t dragOperation, bool mouseIsOverFileInput, unsigned numberOfItemsToBeAccepted);
    void dragEnded(const WebCore::IntPoint& clientPosition, const WebCore::IntPoint& globalPosition, uint64_t operation);
#if PLATFORM(COCOA)
    void setDragImage(const WebCore::IntPoint& clientPosition, const ShareableBitmap::Handle& dragImageHandle, bool isLinkDrag);
    void setPromisedDataForImage(const String& pasteboardName, const SharedMemory::Handle& imageHandle, uint64_t imageSize, const String& filename, const String& extension,
                         const String& title, const String& url, const String& visibleURL, const SharedMemory::Handle& archiveHandle, uint64_t archiveSize);
#if ENABLE(ATTACHMENT_ELEMENT)
    void setPromisedDataForAttachment(const String& pasteboardName, const String& filename, const String& extension, const String& title, const String& url, const String& visibleURL);
#endif
#endif
#if PLATFORM(GTK)
    void startDrag(const WebCore::DragData&, const ShareableBitmap::Handle& dragImage);
#endif
#endif

    void processDidBecomeUnresponsive();
    void processDidBecomeResponsive();
    void processDidCrash();
    void willChangeProcessIsResponsive();
    void didChangeProcessIsResponsive();

#if PLATFORM(IOS)
    void processWillBecomeSuspended();
    void processWillBecomeForeground();
#endif

    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&);
    void willEnterAcceleratedCompositingMode();

    enum UndoOrRedo { Undo, Redo };
    void addEditCommand(WebEditCommandProxy*);
    void removeEditCommand(WebEditCommandProxy*);
    bool isValidEditCommand(WebEditCommandProxy*);
    void registerEditCommand(PassRefPtr<WebEditCommandProxy>, UndoOrRedo);

#if PLATFORM(COCOA)
    void registerKeypressCommandName(const String& name) { m_knownKeypressCommandNames.add(name); }
    bool isValidKeypressCommandName(const String& name) const { return m_knownKeypressCommandNames.contains(name); }
#endif

    WebProcessProxy& process() { return m_process; }
    pid_t processIdentifier() const;

    WebPreferences& preferences() { return m_preferences; }
    void setPreferences(WebPreferences&);

    WebPageGroup& pageGroup() { return m_pageGroup; }

    bool isValid() const;

#if ENABLE(DRAG_SUPPORT)
    WebCore::DragOperation currentDragOperation() const { return m_currentDragOperation; }
    bool currentDragIsOverFileInput() const { return m_currentDragIsOverFileInput; }
    unsigned currentDragNumberOfFilesToBeAccepted() const { return m_currentDragNumberOfFilesToBeAccepted; }
    void resetCurrentDragInformation()
    {
        m_currentDragOperation = WebCore::DragOperationNone;
        m_currentDragIsOverFileInput = false;
        m_currentDragNumberOfFilesToBeAccepted = 0;
    }
#endif

    void preferencesDidChange();

#if ENABLE(CONTEXT_MENUS)
    // Called by the WebContextMenuProxy.
    void contextMenuItemSelected(const WebContextMenuItemData&);
#endif

    // Called by the WebOpenPanelResultListenerProxy.
#if PLATFORM(IOS)
    void didChooseFilesForOpenPanelWithDisplayStringAndIcon(const Vector<String>&, const String& displayString, const API::Data* iconData);
#endif
    void didChooseFilesForOpenPanel(const Vector<String>&);
    void didCancelForOpenPanel();

    WebPageCreationParameters creationParameters();

#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
    void findZoomableAreaForPoint(const WebCore::IntPoint&, const WebCore::IntSize&);
#endif

    void handleDownloadRequest(DownloadProxy*);

    void advanceToNextMisspelling(bool startBeforeSelection);
    void changeSpellingToWord(const String& word);
#if USE(APPKIT)
    void uppercaseWord();
    void lowercaseWord();
    void capitalizeWord();
#endif

#if PLATFORM(COCOA)
    bool isSmartInsertDeleteEnabled() const { return m_isSmartInsertDeleteEnabled; }
    void setSmartInsertDeleteEnabled(bool);
#endif

#if PLATFORM(GTK)
    String accessibilityPlugID() const { return m_accessibilityPlugID; }
#endif

    void setCanRunModal(bool);
    bool canRunModal();

    void beginPrinting(WebFrameProxy*, const PrintInfo&);
    void endPrinting();
    void computePagesForPrinting(WebFrameProxy*, const PrintInfo&, PassRefPtr<ComputedPagesCallback>);
#if PLATFORM(COCOA)
    void drawRectToImage(WebFrameProxy*, const PrintInfo&, const WebCore::IntRect&, const WebCore::IntSize&, PassRefPtr<ImageCallback>);
    void drawPagesToPDF(WebFrameProxy*, const PrintInfo&, uint32_t first, uint32_t count, PassRefPtr<DataCallback>);
#elif PLATFORM(GTK)
    void drawPagesForPrinting(WebFrameProxy*, const PrintInfo&, PassRefPtr<PrintFinishedCallback>);
#endif

    PageLoadState& pageLoadState() { return m_pageLoadState; }

#if PLATFORM(COCOA)
    void handleAlternativeTextUIResult(const String& result);
#endif

    void saveDataToFileInDownloadsFolder(const String& suggestedFilename, const String& mimeType, const String& originatingURLString, API::Data*);
    void savePDFToFileInDownloadsFolder(const String& suggestedFilename, const String& originatingURLString, const IPC::DataReference&);
#if PLATFORM(COCOA)
    void savePDFToTemporaryFolderAndOpenWithNativeApplicationRaw(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size, const String& pdfUUID);
    void savePDFToTemporaryFolderAndOpenWithNativeApplication(const String& suggestedFilename, const String& originatingURLString, const IPC::DataReference&, const String& pdfUUID);
    void openPDFFromTemporaryFolderWithNativeApplication(const String& pdfUUID);
#endif

    WebCore::IntRect visibleScrollerThumbRect() const { return m_visibleScrollerThumbRect; }

    uint64_t renderTreeSize() const { return m_renderTreeSize; }

    void printMainFrame();
    
    void setMediaVolume(float);
    void setMuted(bool);
    void setMayStartMediaWhenInWindow(bool);
    bool mayStartMediaWhenInWindow() const { return m_mayStartMediaWhenInWindow; }
        
#if ENABLE(MEDIA_SESSION)
    bool hasMediaSessionWithActiveMediaElements() const { return m_hasMediaSessionWithActiveMediaElements; }
    void handleMediaEvent(WebCore::MediaEventType);
    void setVolumeOfMediaElement(double, uint64_t);
#endif

    // WebPopupMenuProxy::Client
    virtual NativeWebMouseEvent* currentlyProcessedMouseDownEvent() override;

    void setSuppressVisibilityUpdates(bool flag);
    bool suppressVisibilityUpdates() { return m_suppressVisibilityUpdates; }

#if PLATFORM(IOS)
    void willStartUserTriggeredZooming();

    void potentialTapAtPosition(const WebCore::FloatPoint&, uint64_t& requestID);
    void commitPotentialTap();
    void cancelPotentialTap();
    void tapHighlightAtPosition(const WebCore::FloatPoint&, uint64_t& requestID);
    void handleTap(const WebCore::FloatPoint&);

    void inspectorNodeSearchMovedToPosition(const WebCore::FloatPoint&);
    void inspectorNodeSearchEndedAtPosition(const WebCore::FloatPoint&);

    void blurAssistedNode();
#endif

    void postMessageToInjectedBundle(const String& messageName, API::Object* messageBody);

#if ENABLE(INPUT_TYPE_COLOR)
    void setColorPickerColor(const WebCore::Color&);
    void endColorPicker();
#endif

    WebCore::IntSize minimumLayoutSize() const { return m_minimumLayoutSize; }
    void setMinimumLayoutSize(const WebCore::IntSize&);

    bool autoSizingShouldExpandToViewHeight() const { return m_autoSizingShouldExpandToViewHeight; }
    void setAutoSizingShouldExpandToViewHeight(bool);

    void didReceiveAuthenticationChallengeProxy(uint64_t frameID, PassRefPtr<AuthenticationChallengeProxy>);

    int64_t spellDocumentTag();
    void didFinishCheckingText(uint64_t requestID, const Vector<WebCore::TextCheckingResult>&);
    void didCancelCheckingText(uint64_t requestID);

    void connectionWillOpen(IPC::Connection&);
    void webProcessWillShutDown();

    void processDidFinishLaunching();

    void didSaveToPageCache();
        
    void setScrollPinningBehavior(WebCore::ScrollPinningBehavior);
    WebCore::ScrollPinningBehavior scrollPinningBehavior() const { return m_scrollPinningBehavior; }

    void setOverlayScrollbarStyle(WTF::Optional<WebCore::ScrollbarOverlayStyle>);
    WTF::Optional<WebCore::ScrollbarOverlayStyle> overlayScrollbarStyle() const { return m_scrollbarOverlayStyle; }

    bool shouldRecordNavigationSnapshots() const { return m_shouldRecordNavigationSnapshots; }
    void setShouldRecordNavigationSnapshots(bool shouldRecordSnapshots) { m_shouldRecordNavigationSnapshots = shouldRecordSnapshots; }
    void recordNavigationSnapshot();
    void recordNavigationSnapshot(WebBackForwardListItem&);

#if PLATFORM(COCOA)
    PassRefPtr<ViewSnapshot> takeViewSnapshot();
#endif

#if ENABLE(SUBTLE_CRYPTO)
    void wrapCryptoKey(const Vector<uint8_t>&, bool& succeeded, Vector<uint8_t>&);
    void unwrapCryptoKey(const Vector<uint8_t>&, bool& succeeded, Vector<uint8_t>&);
#endif

    void takeSnapshot(WebCore::IntRect, WebCore::IntSize bitmapSize, SnapshotOptions, std::function<void (const ShareableBitmap::Handle&, CallbackBase::Error)>);

    void navigationGestureDidBegin();
    void navigationGestureWillEnd(bool willNavigate, WebBackForwardListItem&);
    void navigationGestureDidEnd(bool willNavigate, WebBackForwardListItem&);
    void navigationGestureDidEnd();
    void navigationGestureSnapshotWasRemoved();
    void willRecordNavigationSnapshot(WebBackForwardListItem&);

    bool isShowingNavigationGestureSnapshot() const { return m_isShowingNavigationGestureSnapshot; }

    bool isPlayingAudio() const { return !!(m_mediaState & WebCore::MediaProducer::IsPlayingAudio); }
    void isPlayingMediaDidChange(WebCore::MediaProducer::MediaStateFlags, uint64_t);

#if ENABLE(MEDIA_SESSION)
    void hasMediaSessionWithActiveMediaElementsDidChange(bool);
    void mediaSessionMetadataDidChange(const WebCore::MediaSessionMetadata&);
    void focusedContentMediaElementDidChange(uint64_t);
#endif

#if PLATFORM(MAC)
    void removeNavigationGestureSnapshot();

    API::HitTestResult* lastMouseMoveHitTestResult() const { return m_lastMouseMoveHitTestResult.get(); }
    void performImmediateActionHitTestAtLocation(WebCore::FloatPoint);

    void immediateActionDidUpdate();
    void immediateActionDidCancel();
    void immediateActionDidComplete();

    void* immediateActionAnimationControllerForHitTestResult(RefPtr<API::HitTestResult>, uint64_t, RefPtr<API::Object>);

    void installViewStateChangeCompletionHandler(void(^completionHandler)());

    void handleAcceptedCandidate(WebCore::TextCheckingResult);
    void didHandleAcceptedCandidate();
#endif

#if PLATFORM(EFL) && HAVE(ACCESSIBILITY) && defined(HAVE_ECORE_X)
    bool accessibilityObjectReadByPoint(const WebCore::IntPoint&);
    bool accessibilityObjectReadPrevious();
    bool accessibilityObjectReadNext();
#endif

#if USE(UNIFIED_TEXT_CHECKING)
    void checkTextOfParagraph(const String& text, uint64_t checkingTypes, Vector<WebCore::TextCheckingResult>& results);
#endif
    void getGuessesForWord(const String& word, const String& context, Vector<String>& guesses);

    void setShouldDispatchFakeMouseMoveEvents(bool);

    // Diagnostic messages logging.
    void logDiagnosticMessage(const String& message, const String& description, bool shouldSample);
    void logDiagnosticMessageWithResult(const String& message, const String& description, uint32_t result, bool shouldSample);
    void logDiagnosticMessageWithValue(const String& message, const String& description, const String& value, bool shouldSample);
    void logSampledDiagnosticMessage(const String& message, const String& description);
    void logSampledDiagnosticMessageWithResult(const String& message, const String& description, uint32_t result);
    void logSampledDiagnosticMessageWithValue(const String& message, const String& description, const String& value);

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    void addPlaybackTargetPickerClient(uint64_t);
    void removePlaybackTargetPickerClient(uint64_t);
    void showPlaybackTargetPicker(uint64_t, const WebCore::FloatRect&, bool hasVideo);
    void playbackTargetPickerClientStateDidChange(uint64_t, WebCore::MediaProducer::MediaStateFlags);
    void setMockMediaPlaybackTargetPickerEnabled(bool);
    void setMockMediaPlaybackTargetPickerState(const String&, WebCore::MediaPlaybackTargetContext::State);

    // WebMediaSessionManagerClient
    virtual void setPlaybackTarget(uint64_t, Ref<WebCore::MediaPlaybackTarget>&&) override;
    virtual void externalOutputDeviceAvailableDidChange(uint64_t, bool) override;
    virtual void setShouldPlayToPlaybackTarget(uint64_t, bool) override;
#endif

    void didChangeBackgroundColor();
    void didLayoutForCustomContentProvider();

    // For testing
    void clearWheelEventTestTrigger();
    void callAfterNextPresentationUpdate(std::function<void (CallbackBase::Error)>);

    void didLayout(uint32_t layoutMilestones);

    void didRestoreScrollPosition();

private:
    WebPageProxy(PageClient&, WebProcessProxy&, uint64_t pageID, Ref<API::PageConfiguration>&&);
    void platformInitialize();

    void updateViewState(WebCore::ViewState::Flags flagsToUpdate = WebCore::ViewState::AllFlags);
    void updateActivityToken();
    void updateProccessSuppressionState();

    enum class ResetStateReason {
        PageInvalidated,
        WebProcessExited,
    };
    void resetState(ResetStateReason);
    void resetStateAfterProcessExited();

    void setUserAgent(const String&);

    // IPC::MessageReceiver
    // Implemented in generated WebPageProxyMessageReceiver.cpp
    virtual void didReceiveMessage(IPC::Connection&, IPC::MessageDecoder&) override;
    virtual void didReceiveSyncMessage(IPC::Connection&, IPC::MessageDecoder&, std::unique_ptr<IPC::MessageEncoder>&) override;

    // IPC::MessageSender
    virtual bool sendMessage(std::unique_ptr<IPC::MessageEncoder>, unsigned messageSendFlags) override;
    virtual IPC::Connection* messageSenderConnection() override;
    virtual uint64_t messageSenderDestinationID() override;

    // WebPopupMenuProxy::Client
    virtual void valueChangedForPopupMenu(WebPopupMenuProxy*, int32_t newSelectedIndex) override;
    virtual void setTextFromItemForPopupMenu(WebPopupMenuProxy*, int32_t index) override;
#if PLATFORM(GTK)
    virtual void failedToShowPopupMenu() override;
#endif

    void didCreateMainFrame(uint64_t frameID);
    void didCreateSubframe(uint64_t frameID);

    void didStartProvisionalLoadForFrame(uint64_t frameID, uint64_t navigationID, const String& url, const String& unreachableURL, const UserData&);
    void didReceiveServerRedirectForProvisionalLoadForFrame(uint64_t frameID, uint64_t navigationID, const String&, const UserData&);
    void didChangeProvisionalURLForFrame(uint64_t frameID, uint64_t navigationID, const String& url);
    void didFailProvisionalLoadForFrame(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, uint64_t navigationID, const String& provisionalURL, const WebCore::ResourceError&, const UserData&);
    void didCommitLoadForFrame(uint64_t frameID, uint64_t navigationID, const String& mimeType, bool frameHasCustomContentProvider, bool mainFramePluginHandlesPageScaleGesture, uint32_t frameLoadType, const WebCore::CertificateInfo&, bool containsPluginDocument, const UserData&);
    void didFinishDocumentLoadForFrame(uint64_t frameID, uint64_t navigationID, const UserData&);
    void didFinishLoadForFrame(uint64_t frameID, uint64_t navigationID, const UserData&);
    void didFailLoadForFrame(uint64_t frameID, uint64_t navigationID, const WebCore::ResourceError&, const UserData&);
    void didSameDocumentNavigationForFrame(uint64_t frameID, uint64_t navigationID, uint32_t sameDocumentNavigationType, const String&, const UserData&);
    void didReceiveTitleForFrame(uint64_t frameID, const String&, const UserData&);
    void didFirstLayoutForFrame(uint64_t frameID, const UserData&);
    void didFirstVisuallyNonEmptyLayoutForFrame(uint64_t frameID, const UserData&);
    void didDisplayInsecureContentForFrame(uint64_t frameID, const UserData&);
    void didRunInsecureContentForFrame(uint64_t frameID, const UserData&);
    void didDetectXSSForFrame(uint64_t frameID, const UserData&);
    void frameDidBecomeFrameSet(uint64_t frameID, bool);
    void didStartProgress();
    void didChangeProgress(double);
    void didFinishProgress();
    void setNetworkRequestsInProgress(bool);

    void didDestroyNavigation(uint64_t navigationID);

    void decidePolicyForNavigationAction(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, uint64_t navigationID, const NavigationActionData&, uint64_t originatingFrameID, const WebCore::SecurityOriginData& originatingFrameSecurityOrigin, const WebCore::ResourceRequest& originalRequest, const WebCore::ResourceRequest&, uint64_t listenerID, const UserData&, bool& receivedPolicyAction, uint64_t& newNavigationID, uint64_t& policyAction, DownloadID&);
    void decidePolicyForNewWindowAction(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const NavigationActionData&, const WebCore::ResourceRequest&, const String& frameName, uint64_t listenerID, const UserData&);
    void decidePolicyForResponse(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, bool canShowMIMEType, uint64_t listenerID, const UserData&);
    void decidePolicyForResponseSync(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, bool canShowMIMEType, uint64_t listenerID, const UserData&, bool& receivedPolicyAction, uint64_t& policyAction, DownloadID&);
    void unableToImplementPolicy(uint64_t frameID, const WebCore::ResourceError&, const UserData&);

    void willSubmitForm(uint64_t frameID, uint64_t sourceFrameID, const Vector<std::pair<String, String>>& textFieldValues, uint64_t listenerID, const UserData&);

    // History client
    void didNavigateWithNavigationData(const WebNavigationDataStore&, uint64_t frameID);
    void didPerformClientRedirect(const String& sourceURLString, const String& destinationURLString, uint64_t frameID);
    void didPerformServerRedirect(const String& sourceURLString, const String& destinationURLString, uint64_t frameID);
    void didUpdateHistoryTitle(const String& title, const String& url, uint64_t frameID);

    // UI client
    void createNewPage(uint64_t frameID, const WebCore::SecurityOriginData&, const WebCore::ResourceRequest&, const WebCore::WindowFeatures&, const NavigationActionData&, uint64_t& newPageID, WebPageCreationParameters&);
    void showPage();
    void closePage(bool stopResponsivenessTimer);
    void runJavaScriptAlert(uint64_t frameID, const WebCore::SecurityOriginData&, const String&, RefPtr<Messages::WebPageProxy::RunJavaScriptAlert::DelayedReply>);
    void runJavaScriptConfirm(uint64_t frameID, const WebCore::SecurityOriginData&, const String&, RefPtr<Messages::WebPageProxy::RunJavaScriptConfirm::DelayedReply>);
    void runJavaScriptPrompt(uint64_t frameID, const WebCore::SecurityOriginData&, const String&, const String&, RefPtr<Messages::WebPageProxy::RunJavaScriptPrompt::DelayedReply>);
    void setStatusText(const String&);
    void mouseDidMoveOverElement(const WebHitTestResultData&, uint32_t modifiers, const UserData&);

#if ENABLE(NETSCAPE_PLUGIN_API)
    void unavailablePluginButtonClicked(uint32_t opaquePluginUnavailabilityReason, const String& mimeType, const String& pluginURLString, const String& pluginsPageURLString, const String& frameURLString, const String& pageURLString);
#endif // ENABLE(NETSCAPE_PLUGIN_API)
#if ENABLE(WEBGL)
    void webGLPolicyForURL(const String& url, uint32_t& loadPolicy);
    void resolveWebGLPolicyForURL(const String& url, uint32_t& loadPolicy);
#endif // ENABLE(WEBGL)
    void setToolbarsAreVisible(bool toolbarsAreVisible);
    void getToolbarsAreVisible(bool& toolbarsAreVisible);
    void setMenuBarIsVisible(bool menuBarIsVisible);
    void getMenuBarIsVisible(bool& menuBarIsVisible);
    void setStatusBarIsVisible(bool statusBarIsVisible);
    void getStatusBarIsVisible(bool& statusBarIsVisible);
    void setIsResizable(bool isResizable);
    void getIsResizable(bool& isResizable);
    void setWindowFrame(const WebCore::FloatRect&);
    void getWindowFrame(WebCore::FloatRect&);
    void screenToRootView(const WebCore::IntPoint& screenPoint, WebCore::IntPoint& windowPoint);
    void rootViewToScreen(const WebCore::IntRect& viewRect, WebCore::IntRect& result);
#if PLATFORM(IOS)
    void accessibilityScreenToRootView(const WebCore::IntPoint& screenPoint, WebCore::IntPoint& windowPoint);
    void rootViewToAccessibilityScreen(const WebCore::IntRect& viewRect, WebCore::IntRect& result);
#endif
    void runBeforeUnloadConfirmPanel(const String& message, uint64_t frameID, RefPtr<Messages::WebPageProxy::RunBeforeUnloadConfirmPanel::DelayedReply>);
    void didChangeViewportProperties(const WebCore::ViewportAttributes&);
    void pageDidScroll();
    void runOpenPanel(uint64_t frameID, const WebCore::FileChooserSettings&);
    void printFrame(uint64_t frameID);
    void exceededDatabaseQuota(uint64_t frameID, const String& originIdentifier, const String& databaseName, const String& displayName, uint64_t currentQuota, uint64_t currentOriginUsage, uint64_t currentDatabaseUsage, uint64_t expectedUsage, PassRefPtr<Messages::WebPageProxy::ExceededDatabaseQuota::DelayedReply>);
    void reachedApplicationCacheOriginQuota(const String& originIdentifier, uint64_t currentQuota, uint64_t totalBytesNeeded, PassRefPtr<Messages::WebPageProxy::ReachedApplicationCacheOriginQuota::DelayedReply>);
    void requestGeolocationPermissionForFrame(uint64_t geolocationID, uint64_t frameID, String originIdentifier);

    void requestUserMediaPermissionForFrame(uint64_t userMediaID, uint64_t frameID, String originIdentifier, const Vector<String>& audioDeviceUIDs, const Vector<String>& videoDeviceUIDs);
    void checkUserMediaPermissionForFrame(uint64_t userMediaID, uint64_t frameID, String originIdentifier);

    void runModal();
    void notifyScrollerThumbIsVisibleInRect(const WebCore::IntRect&);
    void recommendedScrollbarStyleDidChange(int32_t newStyle);
    void didChangeScrollbarsForMainFrame(bool hasHorizontalScrollbar, bool hasVerticalScrollbar);
    void didChangeScrollOffsetPinningForMainFrame(bool pinnedToLeftSide, bool pinnedToRightSide, bool pinnedToTopSide, bool pinnedToBottomSide);
    void didChangePageCount(unsigned);
    void pageExtendedBackgroundColorDidChange(const WebCore::Color&);
#if ENABLE(NETSCAPE_PLUGIN_API)
    void didFailToInitializePlugin(const String& mimeType, const String& frameURLString, const String& pageURLString);
    void didBlockInsecurePluginVersion(const String& mimeType, const String& pluginURLString, const String& frameURLString, const String& pageURLString, bool replacementObscured);
#endif // ENABLE(NETSCAPE_PLUGIN_API)
    void setCanShortCircuitHorizontalWheelEvents(bool canShortCircuitHorizontalWheelEvents) { m_canShortCircuitHorizontalWheelEvents = canShortCircuitHorizontalWheelEvents; }

    void reattachToWebProcess();
    RefPtr<API::Navigation> reattachToWebProcessForReload();
    RefPtr<API::Navigation> reattachToWebProcessWithItem(WebBackForwardListItem*);

    void requestNotificationPermission(uint64_t notificationID, const String& originString);
    void showNotification(const String& title, const String& body, const String& iconURL, const String& tag, const String& lang, const String& dir, const String& originString, uint64_t notificationID);
    void cancelNotification(uint64_t notificationID);
    void clearNotifications(const Vector<uint64_t>& notificationIDs);
    void didDestroyNotification(uint64_t notificationID);

#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
    void pageDidRequestScroll(const WebCore::IntPoint&);
    void pageTransitionViewportReady();
    void didFindZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&);
#endif

    void didChangeContentSize(const WebCore::IntSize&);

#if ENABLE(INPUT_TYPE_COLOR)
    void showColorPicker(const WebCore::Color& initialColor, const WebCore::IntRect&);
    virtual void didChooseColor(const WebCore::Color&) override;
    virtual void didEndColorPicker() override;
#endif

    void editorStateChanged(const EditorState&);
    void compositionWasCanceled(const EditorState&);

    // Back/Forward list management
    void backForwardAddItem(uint64_t itemID);
    void backForwardGoToItem(uint64_t itemID, SandboxExtension::Handle&);
    void backForwardItemAtIndex(int32_t index, uint64_t& itemID);
    void backForwardBackListCount(int32_t& count);
    void backForwardForwardListCount(int32_t& count);
    void backForwardClear();

    // Undo management
    void registerEditCommandForUndo(uint64_t commandID, uint32_t editAction);
    void registerInsertionUndoGrouping();
    void clearAllEditCommands();
    void canUndoRedo(uint32_t action, bool& result);
    void executeUndoRedo(uint32_t action, bool& result);

    // Keyboard handling
#if PLATFORM(COCOA)
    void executeSavedCommandBySelector(const String& selector, bool& handled);
#endif

#if PLATFORM(GTK)
    void getEditorCommandsForKeyEvent(const AtomicString&, Vector<String>&);
    void bindAccessibilityTree(const String&);
#endif
#if PLATFORM(EFL)
    void getEditorCommandsForKeyEvent(Vector<String>&);
#endif

    // Popup Menu.
    void showPopupMenu(const WebCore::IntRect& rect, uint64_t textDirection, const Vector<WebPopupItem>& items, int32_t selectedIndex, const PlatformPopupMenuData&);
    void hidePopupMenu();

#if ENABLE(CONTEXT_MENUS)
    void showContextMenu(const ContextMenuContextData&, const UserData&);
    void internalShowContextMenu(const ContextMenuContextData&, const UserData&);
#endif

#if ENABLE(TELEPHONE_NUMBER_DETECTION)
#if PLATFORM(MAC)
    void showTelephoneNumberMenu(const String& telephoneNumber, const WebCore::IntPoint&);
#endif
#endif

    // Search popup results
    void saveRecentSearches(const String&, const Vector<WebCore::RecentSearch>&);
    void loadRecentSearches(const String&, Vector<WebCore::RecentSearch>&);

#if PLATFORM(COCOA)
    // Speech.
    void getIsSpeaking(bool&);
    void speak(const String&);
    void stopSpeaking();

    // Spotlight.
    void searchWithSpotlight(const String&);
        
    void searchTheWeb(const String&);

    // Dictionary.
    void didPerformDictionaryLookup(const WebCore::DictionaryPopupInfo&);
#endif

#if PLATFORM(MAC)
    bool appleMailPaginationQuirkEnabled();
#endif

    // Spelling and grammar.
    void checkSpellingOfString(const String& text, int32_t& misspellingLocation, int32_t& misspellingLength);
    void checkGrammarOfString(const String& text, Vector<WebCore::GrammarDetail>&, int32_t& badGrammarLocation, int32_t& badGrammarLength);
    void spellingUIIsShowing(bool&);
    void updateSpellingUIWithMisspelledWord(const String& misspelledWord);
    void updateSpellingUIWithGrammarString(const String& badGrammarPhrase, const WebCore::GrammarDetail&);
    void learnWord(const String& word);
    void ignoreWord(const String& word);
    void requestCheckingOfString(uint64_t requestID, const WebCore::TextCheckingRequestData&);

    void setFocus(bool focused);
    void takeFocus(uint32_t direction);
    void setToolTip(const String&);
    void setCursor(const WebCore::Cursor&);
    void setCursorHiddenUntilMouseMoves(bool);

    void didReceiveEvent(uint32_t opaqueType, bool handled);
    void stopResponsivenessTimer();

    void voidCallback(uint64_t);
    void dataCallback(const IPC::DataReference&, uint64_t);
    void imageCallback(const ShareableBitmap::Handle&, uint64_t);
    void stringCallback(const String&, uint64_t);
    void scriptValueCallback(const IPC::DataReference&, bool hadException, const WebCore::ExceptionDetails&, uint64_t);
    void computedPagesCallback(const Vector<WebCore::IntRect>&, double totalScaleFactorForPrinting, uint64_t);
    void validateCommandCallback(const String&, bool, int, uint64_t);
    void unsignedCallback(uint64_t, uint64_t);
    void editingRangeCallback(const EditingRange&, uint64_t);
#if PLATFORM(COCOA)
    void machSendRightCallback(const WebCore::MachSendRight&, uint64_t);
#endif
    void rectForCharacterRangeCallback(const WebCore::IntRect&, const EditingRange&, uint64_t);
#if PLATFORM(MAC)
    void attributedStringForCharacterRangeCallback(const AttributedString&, const EditingRange&, uint64_t);
    void fontAtSelectionCallback(const String&, double, bool, uint64_t);
#endif
#if PLATFORM(IOS)
    void gestureCallback(const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t, uint64_t);
    void touchesCallback(const WebCore::IntPoint&, uint32_t, uint64_t);
    void autocorrectionDataCallback(const Vector<WebCore::FloatRect>&, const String&, float, uint64_t, uint64_t);
    void autocorrectionContextCallback(const String&, const String&, const String&, const String&, uint64_t, uint64_t, uint64_t);
    void dictationContextCallback(const String&, const String&, const String&, uint64_t);
    void interpretKeyEvent(const EditorState&, bool isCharEvent, bool& handled);
    void showPlaybackTargetPicker(bool hasVideo, const WebCore::IntRect& elementRect);
#endif
#if PLATFORM(GTK)
    void printFinishedCallback(const WebCore::ResourceError&, uint64_t);
#endif

    void focusedFrameChanged(uint64_t frameID);
    void frameSetLargestFrameChanged(uint64_t frameID);

    void canAuthenticateAgainstProtectionSpaceInFrame(uint64_t frameID, const WebCore::ProtectionSpace&, bool& canAuthenticate);
    void didReceiveAuthenticationChallenge(uint64_t frameID, const WebCore::AuthenticationChallenge&, uint64_t challengeID);

    void didFinishLoadingDataForCustomContentProvider(const String& suggestedFilename, const IPC::DataReference&);

#if PLATFORM(COCOA)
    void pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus);
    void setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, uint64_t complexTextInputState);
#endif

    bool maybeInitializeSandboxExtensionHandle(const WebCore::URL&, SandboxExtension::Handle&);

#if PLATFORM(MAC)
    void substitutionsPanelIsShowing(bool&);
    void showCorrectionPanel(int32_t panelType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings);
    void dismissCorrectionPanel(int32_t reason);
    void dismissCorrectionPanelSoon(int32_t reason, String& result);
    void recordAutocorrectionResponse(int32_t responseType, const String& replacedString, const String& replacementString);

#if USE(DICTATION_ALTERNATIVES)
    void showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext);
    void removeDictationAlternatives(uint64_t dictationContext);
    void dictationAlternatives(uint64_t dictationContext, Vector<String>& result);
#endif
#endif // PLATFORM(MAC)

#if PLATFORM(IOS)
    WebCore::FloatSize screenSize();
    WebCore::FloatSize availableScreenSize();
    float textAutosizingWidth();

    void dynamicViewportUpdateChangedTarget(double newTargetScale, const WebCore::FloatPoint& newScrollPosition, uint64_t dynamicViewportSizeUpdateID);
    void couldNotRestorePageState();
    void restorePageState(const WebCore::FloatRect&, double scale);
    void restorePageCenterAndScale(const WebCore::FloatPoint&, double scale);

    void didGetTapHighlightGeometries(uint64_t requestID, const WebCore::Color& color, const Vector<WebCore::FloatQuad>& geometries, const WebCore::IntSize& topLeftRadius, const WebCore::IntSize& topRightRadius, const WebCore::IntSize& bottomLeftRadius, const WebCore::IntSize& bottomRightRadius);

    void startAssistingNode(const AssistedNodeInformation&, bool userIsInteracting, bool blurPreviousNode, const UserData&);
    void stopAssistingNode();

    void showInspectorHighlight(const WebCore::Highlight&);
    void hideInspectorHighlight();

    void showInspectorIndication();
    void hideInspectorIndication();

    void enableInspectorNodeSearch();
    void disableInspectorNodeSearch();
#endif // PLATFORM(IOS)

#if ENABLE(DATA_DETECTION)
    RetainPtr<NSArray> m_dataDetectionResults;
#endif

    void clearLoadDependentCallbacks();

    void performDragControllerAction(DragControllerAction, WebCore::DragData&, const String& dragStorageName, const SandboxExtension::Handle&, const SandboxExtension::HandleArray&);

    void updateBackingStoreDiscardableState();

    void setRenderTreeSize(uint64_t treeSize) { m_renderTreeSize = treeSize; }

#if PLUGIN_ARCHITECTURE(X11)
    void createPluginContainer(uint64_t& windowID);
    void windowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID);
    void windowedPluginVisibilityDidChange(bool isVisible, uint64_t windowID);
#endif

    void processNextQueuedWheelEvent();
    void sendWheelEvent(const WebWheelEvent&);

#if ENABLE(TOUCH_EVENTS)
    bool shouldStartTrackingTouchEvents(const WebTouchEvent&) const;
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
    void findPlugin(const String& mimeType, uint32_t processType, const String& urlString, const String& frameURLString, const String& pageURLString, bool allowOnlyApplicationPlugins, uint64_t& pluginProcessToken, String& newMIMEType, uint32_t& pluginLoadPolicy, String& unavailabilityDescription);
#endif

#if USE(QUICK_LOOK)
    void didStartLoadForQuickLookDocumentInMainFrame(const String& fileName, const String& uti);
    void didFinishLoadForQuickLookDocumentInMainFrame(const QuickLookDocumentData&);
#endif

#if ENABLE(CONTENT_FILTERING)
    void contentFilterDidBlockLoadForFrame(const WebCore::ContentFilterUnblockHandler&, uint64_t frameID);
#endif

    uint64_t generateNavigationID();

    WebPreferencesStore preferencesStore() const;

    void dispatchViewStateChange();
    void viewDidLeaveWindow();
    void viewDidEnterWindow();

#if PLATFORM(MAC)
    void didPerformImmediateActionHitTest(const WebHitTestResultData&, bool contentPreventsDefault, const UserData&);
#endif

    void useFixedLayoutDidChange(bool useFixedLayout) { m_useFixedLayout = useFixedLayout; }
    void fixedLayoutSizeDidChange(WebCore::IntSize fixedLayoutSize) { m_fixedLayoutSize = fixedLayoutSize; }

    void imageOrMediaDocumentSizeChanged(const WebCore::IntSize&);
#if ENABLE(VIDEO)
#if USE(GSTREAMER)
    void requestInstallMissingMediaPlugins(const String& details, const String& description);
#endif
#endif

    void handleAutoFillButtonClick(const UserData&);

    void finishInitializingWebPageAfterProcessLaunch();

    void handleMessage(IPC::Connection&, const String& messageName, const UserData& messageBody);
    void handleSynchronousMessage(IPC::Connection&, const String& messageName, const UserData& messageBody, UserData& returnUserData);

    PageClient& m_pageClient;
    Ref<API::PageConfiguration> m_configuration;

    std::unique_ptr<API::LoaderClient> m_loaderClient;
    std::unique_ptr<API::PolicyClient> m_policyClient;
    std::unique_ptr<API::NavigationClient> m_navigationClient;
    std::unique_ptr<API::HistoryClient> m_historyClient;
    std::unique_ptr<API::FormClient> m_formClient;
    std::unique_ptr<API::UIClient> m_uiClient;
#if PLATFORM(EFL)
    WebUIPopupMenuClient m_uiPopupMenuClient;
#endif
    std::unique_ptr<API::FindClient> m_findClient;
    std::unique_ptr<API::FindMatchesClient> m_findMatchesClient;
    std::unique_ptr<API::DiagnosticLoggingClient> m_diagnosticLoggingClient;
#if ENABLE(CONTEXT_MENUS)
    std::unique_ptr<API::ContextMenuClient> m_contextMenuClient;
#endif
    std::unique_ptr<WebPageInjectedBundleClient> m_injectedBundleClient;

    std::unique_ptr<WebNavigationState> m_navigationState;
    String m_failingProvisionalLoadURL;

    std::unique_ptr<DrawingAreaProxy> m_drawingArea;
#if ENABLE(ASYNC_SCROLLING)
    std::unique_ptr<RemoteScrollingCoordinatorProxy> m_scrollingCoordinatorProxy;
#endif

    Ref<WebProcessProxy> m_process;
    Ref<WebPageGroup> m_pageGroup;
    Ref<WebPreferences> m_preferences;

    WebProcessLifetimeTracker m_webProcessLifetimeTracker { *this };

    const RefPtr<WebUserContentControllerProxy> m_userContentController;
    Ref<VisitedLinkStore> m_visitedLinkStore;
    Ref<WebsiteDataStore> m_websiteDataStore;

    RefPtr<WebFrameProxy> m_mainFrame;
    RefPtr<WebFrameProxy> m_focusedFrame;
    RefPtr<WebFrameProxy> m_frameSetLargestFrame;

    String m_userAgent;
    String m_applicationNameForUserAgent;
    String m_customUserAgent;
    String m_customTextEncodingName;

    bool m_treatsSHA1CertificatesAsInsecure;

    RefPtr<WebInspectorProxy> m_inspector;

#if ENABLE(FULLSCREEN_API)
    RefPtr<WebFullScreenManagerProxy> m_fullScreenManager;
#endif
#if PLATFORM(IOS) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))
    RefPtr<WebVideoFullscreenManagerProxy> m_videoFullscreenManager;
#endif
#if PLATFORM(IOS)
    VisibleContentRectUpdateInfo m_lastVisibleContentRectUpdate;
    bool m_hasReceivedLayerTreeTransactionAfterDidCommitLoad;
    uint64_t m_firstLayerTreeTransactionIdAfterDidCommitLoad;
    int32_t m_deviceOrientation;
    bool m_dynamicViewportSizeUpdateWaitingForTarget;
    bool m_dynamicViewportSizeUpdateWaitingForLayerTreeCommit;
    uint64_t m_dynamicViewportSizeUpdateLayerTreeTransactionID;
    uint64_t m_layerTreeTransactionIdAtLastTouchStart;
    uint64_t m_currentDynamicViewportSizeUpdateID { 0 };
    bool m_hasNetworkRequestsOnSuspended;
#endif

#if ENABLE(VIBRATION)
    RefPtr<WebVibrationProxy> m_vibration;
#endif

#if USE(APPLE_INTERNAL_SDK)
#include <WebKitAdditions/WebPageProxyMembers.h>
#endif

    CallbackMap m_callbacks;
    HashSet<uint64_t> m_loadDependentStringCallbackIDs;

    HashSet<WebEditCommandProxy*> m_editCommandSet;

#if PLATFORM(COCOA)
    HashSet<String> m_knownKeypressCommandNames;
#endif

    RefPtr<WebPopupMenuProxy> m_activePopupMenu;
#if ENABLE(CONTEXT_MENUS)
    std::unique_ptr<WebContextMenuProxy> m_activeContextMenu;
    ContextMenuContextData m_activeContextMenuContextData;
#endif
    RefPtr<API::HitTestResult> m_lastMouseMoveHitTestResult;

    RefPtr<WebOpenPanelResultListenerProxy> m_openPanelResultListener;
    GeolocationPermissionRequestManagerProxy m_geolocationPermissionRequestManager;
    NotificationPermissionRequestManagerProxy m_notificationPermissionRequestManager;

    UserMediaPermissionRequestManagerProxy m_userMediaPermissionRequestManager;

    WebCore::ViewState::Flags m_viewState;
    bool m_viewWasEverInWindow;
#if PLATFORM(IOS)
    bool m_allowsMediaDocumentInlinePlayback { false };
    bool m_alwaysRunsAtForegroundPriority;
    ProcessThrottler::ForegroundActivityToken m_activityToken;
#endif
        
    Ref<WebBackForwardList> m_backForwardList;
        
    bool m_maintainsInactiveSelection;

    String m_toolTip;

    EditorState m_editorState;
    bool m_isEditable;

    double m_textZoomFactor;
    double m_pageZoomFactor;
    double m_pageScaleFactor;
    double m_pluginZoomFactor;
    double m_pluginScaleFactor;
    double m_viewScaleFactor { 1 };
    float m_intrinsicDeviceScaleFactor;
    float m_customDeviceScaleFactor;
    float m_topContentInset;

    LayerHostingMode m_layerHostingMode;

    bool m_drawsBackground;

    WebCore::Color m_underlayColor;
    WebCore::Color m_pageExtendedBackgroundColor;

    bool m_useFixedLayout;
    WebCore::IntSize m_fixedLayoutSize;

    bool m_suppressScrollbarAnimations;

    WebCore::Pagination::Mode m_paginationMode;
    bool m_paginationBehavesLikeColumns;
    double m_pageLength;
    double m_gapBetweenPages;
    bool m_paginationLineGridEnabled;
        
    // If the process backing the web page is alive and kicking.
    bool m_isValid;

    // Whether WebPageProxy::close() has been called on this page.
    bool m_isClosed;

    // Whether it can run modal child web pages.
    bool m_canRunModal;

    bool m_needsToFinishInitializingWebPageAfterProcessLaunch { false };

    bool m_isInPrintingMode;
    bool m_isPerformingDOMPrintOperation;

    bool m_inDecidePolicyForResponseSync;
    const WebCore::ResourceRequest* m_decidePolicyForResponseRequest;
    bool m_syncMimeTypePolicyActionIsValid;
    WebCore::PolicyAction m_syncMimeTypePolicyAction;
    DownloadID m_syncMimeTypePolicyDownloadID;

    bool m_inDecidePolicyForNavigationAction;
    bool m_syncNavigationActionPolicyActionIsValid;
    WebCore::PolicyAction m_syncNavigationActionPolicyAction;
    DownloadID m_syncNavigationActionPolicyDownloadID;
    bool m_shouldSuppressAppLinksInNextNavigationPolicyDecision { false };

    Deque<NativeWebKeyboardEvent> m_keyEventQueue;
    Deque<NativeWebWheelEvent> m_wheelEventQueue;
    Deque<std::unique_ptr<Vector<NativeWebWheelEvent>>> m_currentlyProcessedWheelEvents;
#if ENABLE(MAC_GESTURE_EVENTS)
    Deque<NativeWebGestureEvent> m_gestureEventQueue;
#endif

    bool m_processingMouseMoveEvent;
    std::unique_ptr<NativeWebMouseEvent> m_nextMouseMoveEvent;
    std::unique_ptr<NativeWebMouseEvent> m_currentlyProcessedMouseDownEvent;

#if ENABLE(TOUCH_EVENTS)
    bool m_isTrackingTouchEvents;
#endif
#if ENABLE(TOUCH_EVENTS) && !ENABLE(IOS_TOUCH_EVENTS)
    Deque<QueuedTouchEvents> m_touchEventQueue;
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    RefPtr<WebColorPicker> m_colorPicker;
#endif

    const uint64_t m_pageID;
    WebCore::SessionID m_sessionID;

    bool m_isPageSuspended;
    bool m_addsVisitedLinks;

#if ENABLE(REMOTE_INSPECTOR)
    bool m_allowsRemoteInspection;
    String m_remoteInspectionNameOverride;
#endif

#if PLATFORM(COCOA)
    bool m_isSmartInsertDeleteEnabled;
#endif

#if PLATFORM(GTK)
    String m_accessibilityPlugID;
    WebCore::Color m_backgroundColor;
#endif

    int64_t m_spellDocumentTag;
    bool m_hasSpellDocumentTag;
    unsigned m_pendingLearnOrIgnoreWordMessageCount;

    bool m_mainFrameHasCustomContentProvider;

#if ENABLE(DRAG_SUPPORT)
    // Current drag destination details are delivered as an asynchronous response,
    // so we preserve them to be used when the next dragging delegate call is made.
    WebCore::DragOperation m_currentDragOperation;
    bool m_currentDragIsOverFileInput;
    unsigned m_currentDragNumberOfFilesToBeAccepted;
#endif

    PageLoadState m_pageLoadState;
    
    bool m_delegatesScrolling;

    bool m_mainFrameHasHorizontalScrollbar;
    bool m_mainFrameHasVerticalScrollbar;

    // Whether horizontal wheel events can be handled directly for swiping purposes.
    bool m_canShortCircuitHorizontalWheelEvents;

    bool m_mainFrameIsPinnedToLeftSide;
    bool m_mainFrameIsPinnedToRightSide;
    bool m_mainFrameIsPinnedToTopSide;
    bool m_mainFrameIsPinnedToBottomSide;

    bool m_shouldUseImplicitRubberBandControl;
    bool m_rubberBandsAtLeft;
    bool m_rubberBandsAtRight;
    bool m_rubberBandsAtTop;
    bool m_rubberBandsAtBottom;
        
    bool m_enableVerticalRubberBanding;
    bool m_enableHorizontalRubberBanding;

    bool m_backgroundExtendsBeyondPage;

    bool m_shouldRecordNavigationSnapshots;
    bool m_isShowingNavigationGestureSnapshot;

    bool m_mainFramePluginHandlesPageScaleGesture { false };

    unsigned m_pageCount;

    WebCore::IntRect m_visibleScrollerThumbRect;

    uint64_t m_renderTreeSize;
    uint64_t m_sessionRestorationRenderTreeSize;
    bool m_wantsSessionRestorationRenderTreeSizeThresholdEvent;
    bool m_hitRenderTreeSizeThreshold;

    bool m_suppressVisibilityUpdates;
    bool m_autoSizingShouldExpandToViewHeight;
    WebCore::IntSize m_minimumLayoutSize;

    float m_mediaVolume;
    bool m_muted;
    bool m_mayStartMediaWhenInWindow;

    bool m_waitingForDidUpdateViewState;

    bool m_shouldScaleViewToFitDocument { false };
    bool m_suppressNavigationSnapshotting { false };

#if PLATFORM(COCOA)
    HashMap<String, String> m_temporaryPDFFiles;
    std::unique_ptr<WebCore::RunLoopObserver> m_viewStateChangeDispatcher;

    std::unique_ptr<RemoteLayerTreeScrollingPerformanceData> m_scrollingPerformanceData;
    bool m_scrollPerformanceDataCollectionEnabled;
#endif
    UserObservablePageToken m_pageIsUserObservableCount;
    ProcessSuppressionDisabledToken m_preventProcessSuppressionCount;
        
    WebCore::ScrollPinningBehavior m_scrollPinningBehavior;
    WTF::Optional<WebCore::ScrollbarOverlayStyle> m_scrollbarOverlayStyle;

    uint64_t m_navigationID;

    WebPreferencesStore::ValueMap m_configurationPreferenceValues;
    WebCore::ViewState::Flags m_potentiallyChangedViewStateFlags;
    bool m_viewStateChangeWantsSynchronousReply;
    Vector<uint64_t> m_nextViewStateChangeCallbacks;

    WebCore::MediaProducer::MediaStateFlags m_mediaState { WebCore::MediaProducer::IsNotPlaying };

#if ENABLE(MEDIA_SESSION)
    bool m_hasMediaSessionWithActiveMediaElements { false };
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    bool m_requiresTargetMonitoring { false };
#endif

#if PLATFORM(IOS)
    bool m_hasDeferredStartAssistingNode { false };
    std::unique_ptr<NodeAssistanceArguments> m_deferredNodeAssistanceArguments;
#endif
};

} // namespace WebKit

#endif // WebPageProxy_h
