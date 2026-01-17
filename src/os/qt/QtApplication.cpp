/*
 * Copyright (c) 2025-present Henri Michelon
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
module;
#include "Libraries.h"
#include <QLabel>
#include <QMessageBox>
#include <QVulkanInstance>
module samples.qt;

import vireo.tools;

namespace samples {

    std::shared_ptr<Application> QtApplication::app{};
    PlatformWindowHandle QtApplication::windowHandle{};

    class VulkanWidget : public QWidget {
    public:
        VulkanWidget(QVulkanInstance* inst) : m_inst(inst) {
            setAttribute(Qt::WA_PaintOnScreen);
            setAttribute(Qt::WA_NativeWindow);
        }

        void initVulkanSurface() {
            WId windowHandle = this->winId();

            VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(this->windowHandle());
        }

    protected:
        // Empêche Qt de tenter de dessiner par-dessus votre rendu
        QPaintEngine* paintEngine() const override { return nullptr; }

    private:
        QVulkanInstance* m_inst;
    };


    int QtApplication::run(
        const std::shared_ptr<Application>& app,
        const uint32_t width,
        const uint32_t height,
        const std::string& name,
        int argc, char** argv) {
        QApplication qtApp(argc, argv);

        if (!dirExists("shaders")) {
            QMessageBox::information(
                nullptr,
                "Error",
                "Shaders directory not found, please run the application from the root of the project");
            return 1;
        }

        std::string title = name;

        QtApplication::app = app;
        constexpr auto backend = vireo::Backend::VULKAN;
        title.append("Vulkan 1.3");

        QWidget window;
        window.setFixedSize(width, height);
        window.setWindowTitle(title.c_str());
        window.show();

        // app->init(backend, hwnd);

        return qtApp.exec();
        // // SetWindowText(hwnd, title.c_str());
        //
        // try {
        //     app->onInit();
        //     g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
        //     status = g_application_run (G_APPLICATION (app), argc, argv);
        //     g_object_unref (app);
        //
        //     // ShowWindow(hwnd, nCmdShow);
        //     // auto msg = MSG{};
        //     // while (msg.message != WM_QUIT) {
        //         // if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        //             // TranslateMessage(&msg);
        //             // DispatchMessage(&msg);
        //         // }
        //     // }
        //     app->onDestroy();
        //     // return static_cast<char>(msg.wParam);
        //     return 0;
        // } catch (vireo::Exception& e) {
        //     const std::string msg = e.what();
        //     const std::string msgbox = "zenity --error "
        //       "--title=\"Error\" "
        //       "--text=\"" + msg + "\"";
        //     std::system(msgbox.c_str());
        //     return 1;
        // }
        return 0;
    }

    bool QtApplication::dirExists(const std::string& path) {
        namespace fs = std::filesystem;
        return fs::exists(path) && fs::is_directory(path);
    }

    // vireo::Backend QtApplication::backendSelectorDialog(const std::string& title) {
    //     QDialog dialog;
    //     dialog.setWindowTitle(title.c_str());
    //     dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    //
    //     auto* layout = new QVBoxLayout(&dialog);
    //     layout->addWidget(new QLabel("Please choose a backend:"));
    //
    //     auto* buttonLayout = new QHBoxLayout();
    //     auto* btnVulkan = new QPushButton("Vulkan");
    //     auto* btnDirectX = new QPushButton("DirectX");
    //
    //     buttonLayout->addWidget(btnVulkan);
    //     buttonLayout->addWidget(btnDirectX);
    //     layout->addLayout(buttonLayout);
    //
    //     auto selected = vireo::Backend::UNDEFINED;
    //
    //     QObject::connect(btnVulkan, &QPushButton::clicked, [&]() {
    //         selected = vireo::Backend::VULKAN;
    //         dialog.accept();
    //     });
    //
    //     QObject::connect(btnDirectX, &QPushButton::clicked, [&]() {
    //         selected = vireo::Backend::DIRECTX;
    //         dialog.accept();
    //     });
    //
    //     if (dialog.exec() == QDialog::Accepted) {
    //         return selected;
    //     }
    //
    //     return vireo::Backend::UNDEFINED;
    // }



}
