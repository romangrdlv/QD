#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QColorSpace>
#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStatusBar>
#include <math.h>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->image->setBackgroundRole(QPalette::Base);
    ui->image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->image->setScaledContents(true);
    ui->scrollArea->setBackgroundRole(QPalette::Dark);
    ui->scrollArea->setWidget(ui->image);
    ui->scrollArea->setVisible(false);
    setCentralWidget(ui->horizontalLayoutWidget);
    resize(QGuiApplication::primaryScreen()->availableSize() * (3 / 5));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::loadFile(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull())
    {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(), tr("Cannot load %1: %2").arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }
    setImage(newImage);
    setWindowFilePath(fileName);
    statusBar()->showMessage(tr("Opened \"%1\", %2x%3").arg(QDir::toNativeSeparators(fileName)).arg(qImage.width()).arg(qImage.height()));
    ui->log->append(tr("Opened \"%1\", %2x%3\n").arg(QDir::toNativeSeparators(fileName)).arg(qImage.width()).arg(qImage.height()));
    return true;
}

void MainWindow::setImage(const QImage &newImage)
{
    qImage = newImage;
    if (qImage.colorSpace().isValid())
    {
        qImage.convertToColorSpace(QColorSpace::SRgb);
    }
    ui->image->setPixmap(QPixmap::fromImage(qImage));
    scale_factor = 1.0;
    ui->scrollArea->setVisible(true);
    ui->action_Fit_to_Window->setEnabled(true);
    if (!ui->action_Fit_to_Window->isChecked())
    {
        ui->image->adjustSize();
    }
    ui->action_Select_QR_Area->setEnabled(true);
    ui->action_Normal_Size->setEnabled(true);
    ui->action_Copy->setEnabled(true);
    on_action_Fit_to_Window_triggered();
    on_action_Zoom_In_10_triggered();
    on_action_Zoom_Out_10_triggered();
    on_action_Normal_Size_triggered();
}

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;
    if (firstDialog)
    {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }
    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : supportedMimeTypes)
    {
        mimeTypeFilters.append(mimeTypeName);
    }
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/bmp");
    dialog.setAcceptMode(acceptMode);
    if (acceptMode == QFileDialog::AcceptSave)
    {
        dialog.setDefaultSuffix("bmp");
    }
}

void MainWindow::scaleImage(double factor)
{
    scale_factor *= factor;
    ui->image->resize(scale_factor * ui->image->pixmap(Qt::ReturnByValue).size());
    adjustScrollBar(ui->scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(ui->scrollArea->verticalScrollBar(), factor);
    ui->action_Zoom_In_10->setEnabled(scale_factor < 10.0);
    ui->action_Zoom_Out_10->setEnabled(scale_factor > 0.1);
}

void MainWindow::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep() / 2)));
}

QPoint MainWindow::getMouseClickPos(QMouseEvent *event)
{
    while (true)
    {
        if (event->button() == Qt::LeftButton)
        {
            QPoint point = event->pos();
            point.rx() = (point.x() - ui->toolBar->width()) / scale_factor + ui->scrollArea->horizontalScrollBar()->value() * float(ui->image->pixmap()->width()) / float(ui->scrollArea->horizontalScrollBar()->maximum() + ui->scrollArea->horizontalScrollBar()->pageStep());
            point.ry() = (point.y() - ui->menubar->height()) / scale_factor + ui->scrollArea->verticalScrollBar()->value() * float(ui->image->pixmap()->height()) / float(ui->scrollArea->verticalScrollBar()->maximum() + ui->scrollArea->verticalScrollBar()->pageStep());
            return point;
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (is_selecting_qr)
    {
        blocks[click_number] = getMouseClickPos(event);
        if (blocks[click_number].x() < 0 or blocks[click_number].y() < 0 or blocks[click_number].x() > ui->image->pixmap()->width() or blocks[click_number].y() > ui->image->pixmap()->height())
        {
            statusBar()->showMessage(tr("The point is outside the image. Try again"));
        }
        else
        {
            click_number++;
            statusBar()->showMessage(tr("Point %1 set [%2, %3]").arg(click_number).arg(blocks[click_number - 1].x()).arg(blocks[click_number - 1].y()));
            ui->log->append(tr("Point %1 set [%2, %3]\n").arg(click_number).arg(blocks[click_number - 1].x()).arg(blocks[click_number - 1].y()));
        }
        if (click_number == 3)
        {
            click_number = 0;
            is_selecting_qr = false;
            ui->action_Decode->setEnabled(true);
            statusBar()->showMessage(tr("Point 3 set [%1, %2]. Ready for decryption").arg(blocks[2].x()).arg(blocks[2].y()));
            ui->log->append(tr("Ready for decryption\n"));
        }
    }
}

#ifndef QT_NO_CLIPBOARD
static QImage clipboardImage()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData())
    {
        if (mimeData->hasImage())
        {
            const QImage image = qvariant_cast<QImage>(mimeData->imageData());
            if (!image.isNull())
            {
                return image;
            }
        }
    }
    return QImage();
}
#endif

void MainWindow::on_action_Open_triggered()
{
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);
    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().constFirst()))
    {
    }
}

void MainWindow::on_action_Exit_triggered()
{
    QWidget::close();
}

void MainWindow::on_action_Copy_triggered()
{
#ifndef QT_NO_CLIPBOARD
    QGuiApplication::clipboard()->setImage(qImage);
#endif
}

void MainWindow::on_action_Paste_triggered()
{
#ifndef QT_NO_CLIPBOARD
    const QImage newImage = clipboardImage();
    if (newImage.isNull())
    {
        statusBar()->showMessage(tr("No image in clipboard"));
    }
    else
    {
        setImage(newImage);
        setWindowFilePath(QString());
        statusBar()->showMessage(tr("Obtained image from clipboard, %1x%2, Depth: %3").arg(newImage.width()).arg(newImage.height()).arg(newImage.depth()));
        ui->log->append(tr("Obtained image from clipboard, %1x%2, Depth: %3\n").arg(newImage.width()).arg(newImage.height()).arg(newImage.depth()));
    }
#endif
}

void MainWindow::on_action_Zoom_In_10_triggered()
{
    scaleImage(1.1);
}

void MainWindow::on_action_Zoom_Out_10_triggered()
{    QPoint getMouseClickPos(QLabel label);
    scaleImage(0.9);
}

void MainWindow::on_action_About_triggered()
{
    QMessageBox::about(this, tr("About QData Decoder"),
            tr("<p>TBA<p>"));
}

void MainWindow::on_action_Fit_to_Window_triggered()
{
    bool fitToWindow = ui->action_Fit_to_Window->isChecked();
    ui->scrollArea->setWidgetResizable(fitToWindow);
}

void MainWindow::on_action_Normal_Size_triggered()
{
    scale_factor = 1.0;
    scaleImage(1.0);
}

void MainWindow::on_action_Select_QR_Area_triggered()
{
    is_selecting_qr = true;
}

void MainWindow::on_action_Decode_triggered()
{
    DecoderPopup popup;
    if (has_ever_decoded)
    {
        popup.applySettings(settings);
    }
    popup.show();
    popup.exec();
    settings = popup.getSettings();
    has_ever_decoded = true;
    Decoder *decoder = new Decoder;
    Result result = decoder->mainSequence(ui->image->pixmap(), blocks, settings);
    ui->log->append(result.log);
    ui->log->append(QString("--------------- Decoded text: ---------------\n"));
    ui->log->append(result.text);
    ui->log->append(QString("\n"));
    setImage(result.image);
}

