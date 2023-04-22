#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool loadFile(const QString &);
private slots:
    void on_action_Open_triggered();
    void on_action_Exit_triggered();
    void on_action_Copy_triggered();
    void on_action_Paste_triggered();
    void on_action_Zoom_In_10_triggered();
    void on_action_Zoom_Out_10_triggered();
    void on_action_About_triggered();
    void on_action_Fit_to_Window_triggered();
    void on_action_Normal_Size_triggered();
    void on_action_Select_QR_Area_triggered();
    void on_action_Decode_triggered();
private:
    void setImage(const QImage &newImage);
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void mousePressEvent(QMouseEvent *event);
    QPoint getMouseClickPos(QMouseEvent *event);
private:
    Ui::MainWindow *ui;
    QImage qImage;
    double scale_factor = 1;
    bool is_selecting_qr = 0;
    QPoint blocks[3];
    int click_number = 0;
};
#endif
