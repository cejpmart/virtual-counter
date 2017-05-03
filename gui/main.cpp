#include "mainwindow.h"
#include <QApplication>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "\x1b[35;1mWDebug:\x1b[0;m");
        break;
    case QtInfoMsg:
        fprintf(stderr, "\x1b[36;1mInfo:\x1b[0;m");
        break;
    case QtWarningMsg:
        fprintf(stderr, "\x1b[33;1mWarning:\x1b[0;m");
        break;
    case QtCriticalMsg:
        fprintf(stderr, "\x1b[31;1mCritical:\x1b[0;m");
        break;
    case QtFatalMsg:
        fprintf(stderr, "\x1b[31;1mFatal:\x1b[0;m");
        break;
    }
    fprintf(stderr, " %s", localMsg.constData());
    if (context.file)
        fprintf(stderr, " (%s:%u, %s)", context.file, context.line, context.function);

    fprintf(stderr, "\n");

    if (type == QtFatalMsg)
        abort();
}


int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
