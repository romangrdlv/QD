#ifndef DECODERPOPUP_H
#define DECODERPOPUP_H

#include <QDialog>

struct Settings
{
    int qr_size;
    bool generate_image;
    bool generate_text;
    bool generate_log;
    int mode;
};

namespace Ui {
class DecoderPopup;
}

class DecoderPopup : public QDialog
{
    Q_OBJECT
public:
    explicit DecoderPopup(QWidget *parent = nullptr);
    Settings getSettings();
    void applySettings(Settings s);
    ~DecoderPopup();
public slots:
    void checkSize();
private:
    Ui::DecoderPopup *ui;
};

#endif
