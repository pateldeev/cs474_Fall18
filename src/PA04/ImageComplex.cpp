#include "PA04/ImageComplex.h"
#include "PA04/HelperFunctions.h"
#include "FFT/fft2D.c"

#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>

ImageComplex::ImageComplex(int rows, int cols) : m_rows(rows), m_cols(cols), m_dataR(nullptr), m_dataI(nullptr) {
    //allocate space for real and imaginary parts of data
    m_dataR = new float* [m_rows];
    m_dataI = new float* [m_rows];
    for (int r = 0; r < m_rows; ++r) {
        m_dataR[r] = new float[m_cols];
        m_dataI[r] = new float[m_cols];
    }

    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            m_dataR[r][c] = m_dataI[r][c] = 0.f;
}

ImageComplex::ImageComplex(const ImageComplex & other) : ImageComplex(other.m_rows, other.m_cols) {
    //copy over data values
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            m_dataR[r][c] = other.m_dataR[r][c];
            m_dataI[r][c] = other.m_dataI[r][c];
        }
}

ImageComplex& ImageComplex::operator=(const ImageComplex & rhs) {
    //only defined if two images have same size
    assert(m_rows == rhs.m_rows && m_cols == rhs.m_cols);

    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            m_dataR[r][c] = rhs.m_dataR[r][c];
            m_dataI[r][c] = rhs.m_dataI[r][c];
        }

    return *this;
}

//creates complex image from ImageType variable

ImageComplex::ImageComplex(const ImageType & imgR, const ImageType & imgI) : m_rows(0), m_cols(0), m_dataR(nullptr), m_dataI(nullptr) {
    //get image info and ensure it is good
    int rowsR, rowsI, colsR, colsI, q;
    imgR.getImageInfo(rowsR, colsR, q);
    imgI.getImageInfo(rowsI, colsI, q);
    assert(rowsR == rowsI && colsR == colsI);
    m_rows = rowsR;
    m_cols = colsR;

    //allocate space for real and imaginary parts of data
    m_dataR = new float* [m_rows];
    m_dataI = new float* [m_rows];
    for (int r = 0; r < m_rows; ++r) {
        m_dataR[r] = new float[m_cols];
        m_dataI[r] = new float[m_cols];
    }

    //copy over values
    int tempR, tempI;
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            imgR.getPixelVal(r, c, tempR);
            imgI.getPixelVal(r, c, tempI);
            m_dataR[r][c] = (float) tempR;
            m_dataI[r][c] = (float) tempI;
        }
}

ImageComplex::~ImageComplex(void) {
    //free up used space
    for (int r = 0; r < m_rows; ++r) {
        delete[] m_dataR[r];
        delete[] m_dataI[r];
    }
    delete[] m_dataR;
    delete[] m_dataI;
}

void ImageComplex::getImageInfo(int & rows, int & cols) const {
    rows = m_rows;
    cols = m_cols;
}

void ImageComplex::setPixelVal(int row, int col, float valR, float valI) {
    if (valR == -0.f)
        valR = 0.0;
    if (valI == -0.f)
        valI = 0.0;
    m_dataR[row][col] = valR;
    m_dataI[row][col] = valI;
}

void ImageComplex::getPixelVal(int row, int col, float & valR, float & valI) const {
    valR = m_dataR[row][col];
    valI = m_dataI[row][col];
}

//adds pixel values of two images

void ImageComplex::operator+=(const ImageComplex & other) {
    //only defined if two images have same size
    assert(m_rows == other.m_rows && m_cols == other.m_cols);

    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            m_dataR[r][c] += other.m_dataR[r][c];
            m_dataI[r][c] += other.m_dataI[r][c];
        }
}


//adds constant value to image - needed for wiener filtering

ImageComplex ImageComplex::operator+(float val) const {
    ImageComplex sum(*this);

    //add value to each pixel
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            sum.m_dataR[r][c] += val;

    return sum;
}

//function to copy data to ImageType variable. All non integer values are rounded down

void ImageComplex::getImageType(ImageType & imgR, ImageType & imgI, bool normalize) const {
    //get image info and ensure it is good
    int rowsR, rowsI, colsR, colsI, q;
    imgR.getImageInfo(rowsR, colsR, q);
    imgI.getImageInfo(rowsI, colsI, q);
    assert(rowsR == rowsI && m_rows == rowsR && colsR == colsI && m_cols == colsR);

    //move over values. Round down to nearest integer
    int tempR, tempI;
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            tempR = (int) m_dataR[r][c];
            tempI = (int) m_dataI[r][c];
            imgR.setPixelVal(r, c, tempR);
            imgI.setPixelVal(r, c, tempI);
        }

    //renormalized values if requested
    if (normalize) {
        Helper::remapValues(imgR);
        Helper::remapValues(imgI);
    }
}

//function to get spectrum of image as ImageType variable

void ImageComplex::getSpectrum(ImageType & spectrum, bool normalize) const {
    //get image info and ensure it is good
    int spectrumR, spectrumC, q;
    spectrum.getImageInfo(spectrumR, spectrumC, q);
    assert(spectrumR == m_rows && spectrumC == m_cols);

    //function to get spectrum value - applies log transformation
    auto getNewVal = [](float real, float imaginary) ->int {
        float val = std::sqrt((double) real * real + (double) imaginary * imaginary);
        return ((int) std::log(1 + val));
    };

    //find each value of spectrum
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            spectrum.setPixelVal(r, c, getNewVal(m_dataR[r][c], m_dataI[r][c]));

    //renormalized values if requested
    if (normalize)
        Helper::remapValues(spectrum);
}

void ImageComplex::test(ImageType & spectrum) const {
    std::vector<float> vals;
    vals.resize(m_rows * m_cols);

    //function to get spectrum value - applies log transformation
    auto getNewVal = [](float real, float imaginary) ->float {
        return std::sqrt(real * real + imaginary * imaginary);
    };

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            vals[r * m_cols + c] = getNewVal(m_dataR[r][c], m_dataI[r][c]);
        }
    }

    float min = *std::min_element(vals.begin(), vals.end());
    float max = *std::max_element(vals.begin(), vals.end());

    for (float & f : vals)
        f = 255 * (f - min) / (max - min);

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            spectrum.setPixelVal(r, c, (int) vals[r * m_cols + c]);
        }
    }
}

//function to apply 2D FFT to image. Note the function internally shifts the magnitude

void ImageComplex::applyFFT(bool forward) {
    //move data into usable form - make sure to extend data to power of 2 for FFT
    int extendedR = std::pow(2, std::ceil(log(m_rows) / std::log(2)));
    int extendedC = std::pow(2, std::ceil(log(m_cols) / std::log(2)));
    std::vector<float> dataR(extendedR * extendedC, 0.f), dataI(extendedR * extendedC, 0.f);
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            //copy over data. Invert signs as necessary for shifting
            dataR[r * extendedR + c] = m_dataR[r][c] * (((r + c) % 2) ? 1 : -1);
            dataI[r * extendedR + c] = m_dataI[r][c] * (((r + c) % 2) ? 1 : -1);
        }

    //apply FFT
    fft2D(extendedR, extendedC, &dataR[0], &dataI[0], ((forward) ? -1 : 1));

    //save new values. Make sure to invert shifting transformation
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            m_dataR[r][c] = dataR[r * extendedR + c] * (((r + c) % 2) ? 1 : -1);
            m_dataI[r][c] = dataI[r * extendedR + c] * (((r + c) % 2) ? 1 : -1);
        }
}

//function to compute power spectrum of image - useful for Wiener filtering

void ImageComplex::powerSpectrum(void) {
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            m_dataR[r][c] = m_dataR[r][c] * m_dataR[r][c] + m_dataI[r][c] * m_dataI[r][c];
            m_dataI[r][c] = 0;
        }
}

//function to apply point by point complex multiplication. 

void ImageComplex::complexMultiplication(const ImageComplex & rhs, const float cutoffRadius) {
    //only defined if two images have same size
    assert(m_rows == rhs.m_rows && m_cols == rhs.m_cols);

    float tempR, tempI;
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            int u = c - m_cols / 2, v = m_rows / 2 - r; //u and v coordinates of pixel
            if (cutoffRadius < 0 || std::sqrt(u * u + v * v) <= cutoffRadius) {
                tempR = m_dataR[r][c] * rhs.m_dataR[r][c] - m_dataI[r][c] * rhs.m_dataI[r][c];
                tempI = m_dataR[r][c] * rhs.m_dataI[r][c] + m_dataI[r][c] * rhs.m_dataR[r][c];
                m_dataR[r][c] = tempR, m_dataI[r][c] = tempI;
            }
        }
}


//function to compute complex multiplicative inverse - needed to do division for inverse filtering

ImageComplex& ImageComplex::complexInverse(const float threshold) {
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            float x = m_dataR[r][c] * m_dataR[r][c] + m_dataI[r][c] * m_dataI[r][c];
            if (x > threshold)
                m_dataR[r][c] /= x, m_dataI[r][c] /= -x;
        }
    return *this;
}

//function to print pixel values. Useful for debugging

void ImageComplex::printPixelValues(void) const {
    for (int r = 0; r < m_rows; ++r) {
        std::cout << std::endl << " | ";
        for (int c = 0; c < m_cols; ++c)
            std::cout << m_dataR[r][c] << "+j" << m_dataI[r][c] << " | ";
    }
    std::cout << std::endl;
}
