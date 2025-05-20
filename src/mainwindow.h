#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "audioprocessor.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartStopClicked();
    void onClearText();
    void onNewTranslation(const QString &text);
    void onStatusChanged(const QString &status);
    void onError(const QString &message);
    void onSaveConfig();
    void onConfigChanged();

private:
    void setupConnections();
    void updateButtonState();
    void loadConfig();
    void saveConfig();

    Ui::MainWindow *ui;
    AudioProcessor *audioProcessor;
    bool isRecording;
};

#endif // MAINWINDOW_H 