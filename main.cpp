#include "nim.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    Nim w;
    w.show();

    return a.exec();
}
