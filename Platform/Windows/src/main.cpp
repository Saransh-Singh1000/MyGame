#include "init.hpp"
#include "launcher_page.hpp"

#include <windows.h>
#include <commctrl.h>

#include <iostream>

#if defined(_MSC_VER)
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#ifndef MYGAME_HAS_ULTRALIGHT
#define MYGAME_HAS_ULTRALIGHT 0
#endif

#if MYGAME_HAS_ULTRALIGHT
#include <AppCore/AppCore.h>
#include <JavaScriptCore/JavaScript.h>

#include <string>
#include <vector>

namespace {

class GameLauncher final : public ultralight::AppListener,
                           public ultralight::WindowListener,
                           public ultralight::LoadListener {
public:
    GameLauncher() {
        ultralight::Settings settings;
        settings.app_name = "MyGame Launcher";
        settings.developer_name = "MyGame";
        app_ = ultralight::App::Create(settings);
        if (!app_) {
            return;
        }

        window_ = ultralight::Window::Create(
            app_->main_monitor(), 1040, 760, false,
            ultralight::kWindowFlags_Titled | ultralight::kWindowFlags_Resizable);
        if (!window_) {
            return;
        }
        window_->SetTitle("MyGame | Graphics Setup");
        window_->MoveToCenter();

        overlay_ = ultralight::Overlay::Create(
            window_, window_->width(), window_->height(), 0, 0);
        if (!overlay_) {
            return;
        }

        activeLauncher_ = this;
        app_->set_listener(this);
        window_->set_listener(this);
        overlay_->view()->set_load_listener(this);
        overlay_->view()->LoadHTML(Launcher::page);
    }

    ~GameLauncher() override {
        if (activeLauncher_ == this) {
            activeLauncher_ = nullptr;
        }
    }

    bool run() {
        if (!app_ || !window_ || !overlay_) {
            MessageBoxW(nullptr,
                        L"Ultralight could not create the game launcher window.",
                        L"MyGame Launcher", MB_OK | MB_ICONERROR);
            return false;
        }
        app_->Run();
        return selectionMade_;
    }

    bool useOpenGL() const { return useOpenGL_; }

    void OnClose(ultralight::Window*) override {
        app_->Quit();
    }

    void OnResize(ultralight::Window*, uint32_t width, uint32_t height) override {
        overlay_->Resize(width, height);
    }

    void OnDOMReady(ultralight::View* caller, uint64_t, bool isMainFrame,
                    const ultralight::String&) override {
        if (!isMainFrame) {
            return;
        }
        auto context = caller->LockJSContext();
        JSContextRef jsContext = *context;
        JSStringRef functionName = JSStringCreateWithUTF8CString("nativePlay");
        JSObjectRef function = JSObjectMakeFunctionWithCallback(
            jsContext, functionName, &GameLauncher::onPlay);
        JSObjectSetProperty(jsContext, JSContextGetGlobalObject(jsContext),
                            functionName, function, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(functionName);
    }

    void OnFailLoading(ultralight::View*, uint64_t, bool isMainFrame,
                       const ultralight::String&, const ultralight::String& description,
                       const ultralight::String&, int) override {
        if (isMainFrame) {
            const std::string error = description.utf8().data();
            std::cerr << "Could not load Ultralight launcher: " << error << std::endl;
            MessageBoxA(nullptr, error.c_str(), "MyGame Launcher",
                        MB_OK | MB_ICONERROR);
            app_->Quit();
        }
    }

private:
    static JSValueRef onPlay(JSContextRef context, JSObjectRef, JSObjectRef,
                             size_t argumentCount, const JSValueRef arguments[],
                             JSValueRef*) {
        if (activeLauncher_ == nullptr || argumentCount != 1) {
            return JSValueMakeBoolean(context, false);
        }

        JSStringRef selectedApi = JSValueToStringCopy(context, arguments[0], nullptr);
        if (selectedApi == nullptr) {
            return JSValueMakeBoolean(context, false);
        }

        const size_t bufferSize = JSStringGetMaximumUTF8CStringSize(selectedApi);
        std::vector<char> buffer(bufferSize);
        const size_t bytesWritten =
            JSStringGetUTF8CString(selectedApi, buffer.data(), buffer.size());
        JSStringRelease(selectedApi);
        if (bytesWritten <= 1) {
            return JSValueMakeBoolean(context, false);
        }

        const std::string api(buffer.data(), bytesWritten - 1);
        if (api != "vulkan" && api != "opengl") {
            return JSValueMakeBoolean(context, false);
        }

        activeLauncher_->useOpenGL_ = api == "opengl";
        activeLauncher_->selectionMade_ = true;
        activeLauncher_->app_->Quit();
        return JSValueMakeBoolean(context, true);
    }

    static GameLauncher* activeLauncher_;
    ultralight::RefPtr<ultralight::App> app_;
    ultralight::RefPtr<ultralight::Window> window_;
    ultralight::RefPtr<ultralight::Overlay> overlay_;
    bool selectionMade_ = false;
    bool useOpenGL_ = false;
};

GameLauncher* GameLauncher::activeLauncher_ = nullptr;

bool chooseRenderer(bool& useOpenGL) {
    GameLauncher launcher;
    if (!launcher.run()) {
        return false;
    }
    useOpenGL = launcher.useOpenGL();
    return true;
}

} // namespace

#else

namespace {

bool chooseRenderer(bool& useOpenGL) {
    TASKDIALOG_BUTTON buttons[] = {
        {IDOK, L"Play"},
        {IDCANCEL, L"Cancel"}
    };
    TASKDIALOG_BUTTON radioButtons[] = {
        {101, L"Vulkan  |  Low-level performance"},
        {102, L"OpenGL  |  Broad compatibility"}
    };

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
    config.pszWindowTitle = L"MyGame  |  Graphics Setup";
    config.pszMainInstruction = L"Choose your graphics API";
    config.pszContent =
        L"Your selection will be used to start the game. You can change it later in Settings.";
    config.pszMainIcon = TD_INFORMATION_ICON;
    config.cButtons = ARRAYSIZE(buttons);
    config.pButtons = buttons;
    config.nDefaultButton = IDOK;
    config.cRadioButtons = ARRAYSIZE(radioButtons);
    config.pRadioButtons = radioButtons;
    config.nDefaultRadioButton = 101;

    int button = IDCANCEL;
    int selectedRadio = 101;
    const HRESULT result =
        TaskDialogIndirect(&config, &button, &selectedRadio, nullptr);
    if (FAILED(result)) {
        MessageBoxW(nullptr, L"Could not display the renderer selection dialog.",
                    L"MyGame Launcher", MB_OK | MB_ICONERROR);
        return false;
    }
    if (button != IDOK) {
        return false;
    }

    useOpenGL = selectedRadio == 102;
    return true;
}

} // namespace

#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    bool useOpenGL = false;
    if (!chooseRenderer(useOpenGL)) {
        return 0;
    }
    if (!Engine::run(useOpenGL)) {
        return -1;
    }
    return 0;
}