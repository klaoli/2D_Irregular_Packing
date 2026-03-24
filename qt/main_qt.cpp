#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ILSQN Packing Visualizer");

    // 注册自定义类型用于信号/槽跨线程传输
    qRegisterMetaType<MyNest::box_t>("MyNest::box_t");
    qRegisterMetaType<std::vector<MyNest::Piece>>("std::vector<MyNest::Piece>");
    qRegisterMetaType<std::vector<MyNest::Vector>>("std::vector<MyNest::Vector>");

    MainWindow window;
    window.show();

    return app.exec();
}
