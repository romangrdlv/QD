#include "decoder.h"
#include <math.h>

#include <iostream>

using namespace std;

//  B               A
//  # # # # # # # # #
//  #             #
//  #           #
//  #         #
//  #       #
//  #     #
//  #   #
//  # #
//  #
//  C
//  lengths: [AB, BC, CA]

Decoder::Decoder()
{

}

QPixmap Decoder::mainSequence(const QPixmap* image, QPoint* blocks, int size)
{
    this->defineLengths(blocks);
    float angle = defineRotationAngle(blocks, defineQRQuadrant(blocks));

    QTransform tr;
    tr.translate(blocks[1].x(), blocks[1].y());
    tr.rotate(angle * 180 / M_PI);
    tr.translate(-blocks[1].x(), -blocks[1].y());
    QPixmap newImage = image->transformed(tr);

    blocks[0] = rotatePoint(blocks[0], image, newImage, angle);
    blocks[1] = rotatePoint(blocks[1], image, newImage, angle);
    blocks[2] = rotatePoint(blocks[2], image, newImage, angle);
    cout << blocks[0].x() << " " << blocks[0].y() << " " << blocks[1].x() << " " << blocks[1].y() << " " << blocks[2].x() << " " << blocks[2].y() << "   " << angle * 180 / M_PI << endl;
    newImage = qrToMatrix(newImage, blocks, size);
    cout << blocks[0].x() << " " << blocks[0].y() << " " << blocks[1].x() << " " << blocks[1].y() << " " << blocks[2].x() << " " << blocks[2].y() << "   " << angle * 180 / M_PI << endl;
    return newImage;
}

void Decoder::defineLengths(QPoint* blocks)
{
    lengths[0] = sqrt(pow((blocks[0].x() - blocks[1].x()), 2) + pow((blocks[0].y() - blocks[1].y()), 2));
    lengths[1] = sqrt(pow((blocks[1].x() - blocks[2].x()), 2) + pow((blocks[1].y() - blocks[2].y()), 2));
    lengths[2] = sqrt(pow((blocks[2].x() - blocks[0].x()), 2) + pow((blocks[2].y() - blocks[0].y()), 2));
    if (lengths[0] > lengths[1] and lengths[0] > lengths[2])
    {
        float buff1 = lengths[2];
        lengths[2] = lengths[0];
        lengths[0] = buff1;

        QPoint buff2 = blocks[2];
        blocks[2] = blocks[1];
        blocks[1] = buff2;
    }
    else if (lengths[1] > lengths[0] and lengths[1] > lengths[2])
    {
        float buff = lengths[2];
        lengths[2] = lengths[1];
        lengths[1] = buff;

        QPoint buff2 = blocks[0];
        blocks[0] = blocks[1];
        blocks[1] = buff2;
    }
}

int Decoder::defineQRQuadrant(QPoint* blocks)
{
    int ax = blocks[0].x() - blocks[1].x();
    int ay = blocks[0].y() - blocks[1].y();
    //  bx = 0;
    //  by = 0;
    int cx = blocks[2].x() - blocks[1].x();
    int cy = blocks[2].y() - blocks[1].y();
    if (ax <= 0 and ay <= 0 and cx >= 0 and cy <= 0) // rot to quadrant 1
    {
        return 1;
    }
    if (cx <= 0 and cy <= 0 and ax >= 0 and ay <= 0)
    {
        QPoint buff = blocks[2];
        blocks[2] = blocks[0];
        blocks[0] = buff;

        return 1;
    }
    if (ax >= 0 and ay <= 0 and cx >= 0 and cy >= 0) // rot to quadrant 4
    {
        return 0;
    }
    if (cx >= 0 and cy <= 0 and ax >= 0 and ay >= 0)
    {
        QPoint buff = blocks[2];
        blocks[2] = blocks[0];
        blocks[0] = buff;

        return 0;
    }
    if (ax >= 0 and ay >= 0 and cx <= 0 and cy >= 0) // rot to quadrant 3
    {
        return 3;
    }
    if (cx >= 0 and cy >= 0 and ax <= 0 and ay >= 0)
    {
        QPoint buff = blocks[2];
        blocks[2] = blocks[0];
        blocks[0] = buff;

        return 3;
    }
    if (ax <= 0 and ay >= 0 and cx <= 0 and cy <= 0) // rot to quadrant 2
    {
        return 2;
    }
    if (cx <= 0 and cy >= 0 and ax <= 0 and ay <= 0)
    {
        QPoint buff = blocks[2];
        blocks[2] = blocks[0];
        blocks[0] = buff;

        return 2;
    }
    else
    {
        return -1;
    }
}

float Decoder::defineRotationAngle(QPoint* blocks, int quadrant)
{
    float anglea = atan2((blocks[0].y() - blocks[1].y()), (blocks[0].x() - blocks[1].x())) * -1;
    float anglec = atan2((blocks[2].y() - blocks[1].y()), (blocks[2].x() - blocks[1].x())) * -1;
    cout << anglea * 180 / M_PI << " " << anglec * 180 / M_PI << " " << quadrant << endl;
    if (anglea < 0)
    {
        anglea += M_PI / 2;
    }
    if (anglec < 0)
    {
        anglec += M_PI / 2;
    }
    cout << anglea * 180 / M_PI << " " << anglec * 180 / M_PI << " " << quadrant << endl;
    return anglec + quadrant * (M_PI / 2);
}

QPoint Decoder::rotatePoint(QPoint point, const QPixmap* image, QPixmap newImage, float angle)
{
    int x = point.x() - image->width() / 2;
    int y = point.y() - image->height() / 2;
    int rx, ry;

    if (angle >= 0)
    {
        rx = float(x) * cos(angle) - float(y) * sin(angle) + image->width() / 2 + (newImage.width() - image->width()) / 2;
        ry = float(y) * cos(angle) + float(x) * sin(angle) + image->height() / 2 + (newImage.height() - image->height()) / 2;
    }
    else
    {
        rx = float(x) * cos(angle) + float(y) * sin(angle) + image->width() / 2 + (newImage.width() - image->width()) / 2;
        ry = float(y) * cos(angle) - float(x) * sin(angle) + image->height() / 2 + (newImage.height() - image->height()) / 2;
    }
    return QPoint(rx, ry);
}

QPixmap Decoder::qrToMatrix(QPixmap image, QPoint* blocks, int size)
{
    int sumMatrix[size][size];
    int cntMatrix[size][size];
    bool modMatrix[size][size];
    int window_size = 4;
    int size_pixels = ((blocks[0].x() - blocks[1].x()) + (blocks[2].y() - blocks[1].y())) / 2;
    int pixels_per_module = ceil(float(size_pixels) / float(size - 1));
    if (pixels_per_module % 2 != 0)
    {
        pixels_per_module++;
    }
    int border = pixels_per_module * 0.4;

    float image_scale_factor = float(pixels_per_module) / (float(size_pixels) / float(size - 1));
    QImage qImage = image.scaled(int(image.width() * image_scale_factor), int(image.height() * image_scale_factor)).toImage();
    blocks[0] = QPoint(blocks[0].x() * image_scale_factor, blocks[0].y() * image_scale_factor);
    blocks[1] = QPoint(blocks[1].x() * image_scale_factor, blocks[1].y() * image_scale_factor);
    blocks[2] = QPoint(blocks[2].x() * image_scale_factor, blocks[2].y() * image_scale_factor);

    int start_x = blocks[1].x() - pixels_per_module / 2;
    int start_y = blocks[1].y() - pixels_per_module / 2;
    float area = window_size * window_size;
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            cntMatrix[y][x] = 0;
        }
    }
    for (int module_y = 0; module_y < size; module_y++)
    {
        for (int module_x = 0; module_x < size; module_x++)
        {
            sumMatrix[module_y][module_x] = 0;
            int pixels_checked = 0;
            int i;
            int left = border;
            int right = pixels_per_module - border;
            int top = border;
            int bottom = pixels_per_module - border;

            while (left <= right && top <= bottom)
            {
                for (i = left; i <= right; ++i)
                {
                    int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + top));
                    if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                    {
                        element = sumMatrix[module_y][module_x] / pixels_checked;
                    }
                    sumMatrix[module_y][module_x] += element;
                    qImage.setPixelColor(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + top, Qt::blue);
                    pixels_checked++;
                }
                top++;
                for (i = top; i <= bottom; ++i)
                {
                    int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + right, start_y + module_y * pixels_per_module + i));
                    if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                    {
                        element = sumMatrix[module_y][module_x] / pixels_checked;
                    }
                    sumMatrix[module_y][module_x] += element;
                    qImage.setPixelColor(start_x + module_x * pixels_per_module + right, start_y + module_y * pixels_per_module + i, Qt::blue);
                    pixels_checked++;
                }
                right--;
                if (top <= bottom)
                {
                    for (i = right; i >= left; --i)
                    {
                        int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + bottom));
                        if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                        {
                            element = sumMatrix[module_y][module_x] / pixels_checked;
                        }
                        sumMatrix[module_y][module_x] += element;
                        qImage.setPixelColor(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + bottom, Qt::blue);
                        pixels_checked++;
                    }
                    bottom--;
                }
                if (left <= right) {
                    for (i = bottom; i >= top; --i)
                    {
                        int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + left, start_y + module_y * pixels_per_module + i));
                        if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                        {
                            element = sumMatrix[module_y][module_x] / pixels_checked;
                        }
                        sumMatrix[module_y][module_x] += element;
                        qImage.setPixelColor(start_x + module_x * pixels_per_module + left, start_y + module_y * pixels_per_module + i, Qt::blue);
                        pixels_checked++;
                    }
                    left++;
                }
            }
//            for (int pixel_y = border; pixel_y < pixels_per_module - border; pixel_y++)
//            {
//                for (int pixel_x = border; pixel_x < pixels_per_module - border; pixel_x++)
//                {
//                    sumMatrix[module_y][module_x] += qGray(qImage.pixel(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y));
////                    if (pixel_x == pixels_per_module / 2 - 1 and pixel_y == pixels_per_module / 2 - 1)
////                    {
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x - 1, start_y + module_y * pixels_per_module + pixel_y - 1, Qt::red);
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x - 1, start_y + module_y * pixels_per_module + pixel_y, Qt::red);
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x - 1, start_y + module_y * pixels_per_module + pixel_y + 1, Qt::red);
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y - 1, Qt::red);
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y, Qt::red);
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y + 1, Qt::red);
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x + 1, start_y + module_y * pixels_per_module + pixel_y - 1, Qt::red);
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x + 1, start_y + module_y * pixels_per_module + pixel_y, Qt::red);
////                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x + 1, start_y + module_y * pixels_per_module + pixel_y + 1, Qt::red);
////                    }
//                    qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y, Qt::blue);
//                    if (pixel_x == 0 or pixel_x == pixels_per_module - 1 or pixel_y == 0 or pixel_y == pixels_per_module - 1)
//                    {
//                        qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y, Qt::green);
//                    }
//                }
//            }
        }
    }
    for (int window_y = 0; window_y < size - window_size + 1; window_y++)
    {
        for (int window_x = 0; window_x < size - window_size + 1; window_x++)
        {
            float sum = 0;
            for (int module_y = 0; module_y < window_size; module_y++)
            {
                for (int module_x = 0; module_x < window_size; module_x++)
                {
                    sum += sumMatrix[window_y + module_y][window_x + module_x];
                }
            }
            int filter = sum / area;
            for (int module_y = 0; module_y < window_size; module_y++)
            {
                for (int module_x = 0; module_x < window_size; module_x++)
                {
                    if (sumMatrix[window_y + module_y][window_x + module_x] > filter)
                    {
                        cntMatrix[window_y + module_y][window_x + module_x] -= 1;
                    }
                    else
                    {
                        cntMatrix[window_y + module_y][window_x + module_x] += 1;
                    }
                }
            }
        }
    }
    for (int module_y = 0; module_y < size; module_y++)
    {
        for (int module_x = 0; module_x < size; module_x++)
        {
            if (cntMatrix[module_y][module_x] <= 0)
            {
                modMatrix[module_y][module_x] = 0;
            }
            else
            {
                modMatrix[module_y][module_x] = 1;
            }
        }
    }
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            cout << cntMatrix[y][x] << " ";
        }
        cout << endl;
    }
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            if (modMatrix[y][x])
            {
                cout << "██";
            }
            else
            {
                cout << "  ";
            }
        }
        cout << endl;
    }
    cout << endl;
    cout << start_x << " " << start_y << " " << pixels_per_module << " " << image_scale_factor << " " << size_pixels << endl;
    return QPixmap::fromImage(qImage);
}
