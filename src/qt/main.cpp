#include <QApplication>
#include <QGLWidget>
#include <QGraphicsScene>
#include <QGraphicsView>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;

    QGraphicsView view(&scene);
    view.setWindowTitle("Mappero");
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setAttribute(Qt::WA_Maemo5AutoOrientation, true);
    view.setAttribute(Qt::WA_Maemo5NonComposited, true);
    view.setRenderHints(QPainter::Antialiasing);
    view.setViewport(new QGLWidget);
    view.show();

    return app.exec();
}
