#include "decoderpopup.h"
#include "ui_decoderpopup.h"

DecoderPopup::DecoderPopup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DecoderPopup)
{
    ui->setupUi(this);
    connect(ui->sizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(checkSize()));
}

Settings DecoderPopup::getSettings()
{
    Settings s;
    s.qr_size = ui->sizeSpinBox->value();
    s.generate_image = ui->imageCheckBox->isChecked();
    s.generate_text = ui->textCheckBox->isChecked();
    s.generate_log = ui->logCheckBox->isChecked();
    s.mode = ui->modeComboBox->currentIndex();
    return s;
}

void DecoderPopup::applySettings(Settings s)
{
    ui->sizeSpinBox->setValue(s.qr_size);
    ui->imageCheckBox->setChecked(s.generate_image);
    ui->textCheckBox->setChecked(s.generate_text);
    ui->logCheckBox->setChecked(s.generate_log);
    ui->modeComboBox->setCurrentIndex(s.mode);
}

void DecoderPopup::checkSize()
{
    if ((ui->sizeSpinBox->value() - 21) % 4 != 0 or ui->sizeSpinBox->value() < 21 or ui->sizeSpinBox->value() > 177)
    {
        ui->status->setText("Invalid QR Size\nProceed on your own risk");
        ui->status->setStyleSheet("QLabel { color : red; }");
    }
    else
    {
        ui->status->setText("QR version: " + QVariant((ui->sizeSpinBox->value() - 21) / 4 + 1).toString());
        ui->status->setStyleSheet("QLabel { color : default; }");
    }
}

DecoderPopup::~DecoderPopup()
{
    delete ui;
}
