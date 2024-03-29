#include "flutter_window.h"

#include <iostream>
#include <optional>

#include "windows.h"
#include "wrl.h"
#include "wil/com.h"
#include "WebView2.h"

#include "flutter/generated_plugin_registrant.h"

FlutterWindow::FlutterWindow(const flutter::DartProject& project)
    : project_(project) {}

FlutterWindow::~FlutterWindow() {}

static wil::com_ptr<ICoreWebView2Controller> webviewController;
static wil::com_ptr<ICoreWebView2> webview;
static LRESULT CALLBACK WebViewWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_CREATE: {
      CREATESTRUCT* pars = (CREATESTRUCT*)lparam;
      void* user_data = pars->lpCreateParams;
      SetWindowLongPtr(hwnd, 0, (LONG_PTR)user_data);
      return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    case WM_SIZE:
      if (webviewController != nullptr) {
        RECT bounds;
        GetClientRect(hwnd, &bounds);
				std::cerr << "Bounds: (" << bounds.left << "," << bounds.top << ") to (" << bounds.right << "," << bounds.bottom << ")\n";
        webviewController->put_Bounds(bounds);
      }
      break;
    case WM_DESTROY:
      webviewController = nullptr;
      webview = nullptr;
      break;
    case WM_SETFOCUS: {
      std::cerr << "Platform view window gained focus\n";
      if (webviewController != nullptr) {
        flutter::FlutterViewController* window = (flutter::FlutterViewController*)GetWindowLongPtr(hwnd, 0);
        int reason = window->engine()->QueryFocusReason();
        webviewController->MoveFocus(static_cast<COREWEBVIEW2_MOVE_FOCUS_REASON>(reason));
      }
      break;
    }
    case WM_KILLFOCUS: {
      std::cerr << "Kill focus\n";
      break;
    }
    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }
  return 0;
}

bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) {
    return false;
  }

  RECT frame = GetClientArea();

  // The size here must match the window dimensions to avoid unnecessary surface
  // creation / destruction in the startup path.
  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left, frame.bottom - frame.top, project_);
  // Ensure that basic setup of the controller was successful.
  if (!flutter_controller_->engine() || !flutter_controller_->view()) {
    return false;
  }
  RegisterPlugins(flutter_controller_->engine());
  SetChildContent(flutter_controller_->view()->GetNativeWindow());

  // Register webview class
  WNDCLASSEX wnd;
  wnd.cbSize = sizeof(wnd);
  wnd.lpszClassName = L"Webview";
  wnd.hIconSm = NULL;
  wnd.cbClsExtra = 0;
  wnd.cbWndExtra = sizeof(void*) * 1;
  wnd.lpszMenuName = NULL;
  wnd.hIcon = NULL;
  wnd.hCursor = NULL;
  wnd.hInstance = GetModuleHandle(nullptr);
  wnd.style = CS_HREDRAW | CS_VREDRAW;
  wnd.hbrBackground = (HBRUSH)(COLOR_HIGHLIGHT + 1);
  wnd.lpfnWndProc = WebViewWndProc;

  std::cerr << "Register window class returns " << RegisterClassEx(&wnd) << "\n";

  flutter_controller_->engine()->RegisterPlatformViewType("test", [](const PlatformViewCreationParams* params) {
    flutter::FlutterViewController* view_controller = (flutter::FlutterViewController*)params->user_data;
    HWND hWnd = CreateWindow(L"Webview", L"testwebview", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, params->parent, NULL, NULL, (LPVOID)view_controller);
    std::cerr << "Creating platform view #" << hWnd << " with parent " << params->parent << "\n";
    RECT rect;
    GetClientRect(params->parent, &rect);
    std::cerr << "Parent is " << rect.right << " x " << rect.bottom << "\n";

    if (webviewController == nullptr) {
      std::cerr << "Creating webview controller\n";
      CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			    [hWnd, view_controller](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

            std::cerr << "Creation callback\n";
				    // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				    env->CreateCoreWebView2Controller(hWnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					    [hWnd, view_controller](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                std::cerr << "Create core callback\n";
						    if (controller != nullptr) {
                  std::cerr << "Assigning controller\n";
							    webviewController = controller;
							    webviewController->get_CoreWebView2(&webview);
						    }

						    // Add a few settings for the webview
						    // The demo step is redundant since the values are the default settings
						    wil::com_ptr<ICoreWebView2Settings> settings;
						    webview->get_Settings(&settings);
						    settings->put_IsScriptEnabled(TRUE);
						    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						    settings->put_IsWebMessageEnabled(TRUE);

						    // Resize WebView to fit the bounds of the parent window
						    RECT bounds;
						    GetClientRect(hWnd, &bounds);
						    std::cerr << "Bounds: (" << bounds.left << "," << bounds.top << ") to (" << bounds.right << "," << bounds.bottom << ")\n";
						    webviewController->put_Bounds(bounds);

						    // Schedule an async task to navigate to Bing
						    webview->Navigate(L"https://www.google.com/");

						    // <NavigationEvents>
						    // Step 4 - Navigation events
						    // register an ICoreWebView2NavigationStartingEventHandler to cancel any non-https navigation
						    EventRegistrationToken token;
						    webview->add_NavigationStarting(Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
							    [](ICoreWebView2* webview_, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                    std::cerr << "Navigation starting\n";
								    wil::unique_cotaskmem_string uri;
								    args->get_Uri(&uri);
								    std::wstring source(uri.get());
								    if (source.substr(0, 5) != L"https") {
									    std::cerr << "Canceled\n";
									    args->put_Cancel(true);
								    } else std::cerr << "Not canceled\n";
								    return S_OK;
							    }).Get(), &token);
						    // </NavigationEvents>

						    // <Scripting>
						    // Step 5 - Scripting
						    // Schedule an async task to add initialization script that freezes the Object object
						    // webview->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
						    // Schedule an async task to get the document URL
						    webview->ExecuteScript(L"window.document.URL;", Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
							    [](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
								    LPCWSTR URL = resultObjectAsJson;
                    int len = WideCharToMultiByte(CP_UTF8, 0, URL, -1, NULL, 0, NULL, NULL);
                    char *s = new char[len];
                    WideCharToMultiByte(CP_UTF8, 0, URL, -1, s, len, NULL, NULL);
                    std::cerr << "Got URL: " << std::string(s) << "\n";
                    delete[] s;
								    return S_OK;
							    }).Get());
						    // </Scripting>

						    // <CommunicationHostWeb>
						    // Step 6 - Communication between host and web content
						    // Set an event handler for the host to return received message back to the web content
						    webview->add_WebMessageReceived(Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							    [](ICoreWebView2* webview_, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								    std::cerr << "Web message received\n";
								    wil::unique_cotaskmem_string message;
								    args->TryGetWebMessageAsString(&message);
								    // processMessage(&message);
								    webview_->PostWebMessageAsString(message.get());
								    return S_OK;
							    }).Get(), &token);

						    // Schedule an async task to add initialization script that
						    // 1) Add an listener to print message from the host
						    // 2) Post document URL to the host
						    webview->AddScriptToExecuteOnDocumentCreated(
							    L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));" \
							    L"window.chrome.webview.postMessage(window.document.URL);",
							    nullptr);
						    // </CommunicationHostWeb>


                webviewController->add_MoveFocusRequested(Microsoft::WRL::Callback<ICoreWebView2MoveFocusRequestedEventHandler>([hWnd, view_controller](ICoreWebView2Controller* sender, ICoreWebView2MoveFocusRequestedEventArgs* args){
                  COREWEBVIEW2_MOVE_FOCUS_REASON reason;
                  args->get_Reason(&reason);
                  std::cerr << "Moving focus from webview with reason " << reason << "\n";
                  view_controller->engine()->SendTabOut(hWnd, reason);
                  return S_OK;
                }).Get(), &token);

                webviewController->add_GotFocus(Microsoft::WRL::Callback<ICoreWebView2FocusChangedEventHandler>([hWnd](ICoreWebView2Controller* sender, IUnknown* args){
                  // This could probably be a method in FlutterViewController instead.
                  ::SendMessage(GetParent(hWnd), WM_KILLFOCUS, (WPARAM)hWnd, NULL);
                  return S_OK;
                }).Get(), &token);

						    return S_OK;
					    }).Get());
				    return S_OK;
			    }).Get());
    }

    /*UpdateWindow(hWnd);
    auto style = GetWindowLong(params->parent, GWL_STYLE);
    style |= WS_CLIPCHILDREN | WS_TABSTOP;
    SetWindowLong(params->parent, GWL_STYLE, style);
    UpdateWindow(params->parent);*/
    return hWnd;
  }, (void*)flutter_controller_.get());

  flutter_controller_->engine()->SetNextFrameCallback([&]() {
    this->Show();
  });

  // Flutter can complete the first frame before the "show window" callback is
  // registered. The following call ensures a frame is pending to ensure the
  // window is shown. It is a no-op if the first frame hasn't completed yet.
  flutter_controller_->ForceRedraw();

  return true;
}

void FlutterWindow::OnDestroy() {
  if (flutter_controller_) {
    flutter_controller_ = nullptr;
  }

  Win32Window::OnDestroy();
}

LRESULT
FlutterWindow::MessageHandler(HWND hwnd, UINT const message,
                              WPARAM const wparam,
                              LPARAM const lparam) noexcept {
  // Give Flutter, including plugins, an opportunity to handle window messages.
  if (flutter_controller_) {
    std::optional<LRESULT> result =
        flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam,
                                                      lparam);
    if (result) {
      return *result;
    }
  }

  switch (message) {
    case WM_FONTCHANGE:
      flutter_controller_->engine()->ReloadSystemFonts();
      break;
  }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}
