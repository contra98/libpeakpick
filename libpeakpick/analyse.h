/*
 * <Math containing Header file.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#pragma once

#include <Eigen/Dense>

#include <cmath>
#include <iostream>
#include <vector>

#include "savitzky.h"
#include "spectrum.h"

typedef Eigen::VectorXd Vector;

namespace PeakPick {

struct Peak {
    unsigned int start = 0;
    unsigned int max = 0;
    unsigned int min = 0;
    unsigned int end = 0;

    unsigned int int_start = 0;
    unsigned int int_end = 0;

    double deconv_x = 0;
    double deconv_y = 0;
    double integ_num = 0;
    double integ_analyt = 0;

    inline void setPeakStart(int peak_start)
    {
        start = peak_start;
        int_start = peak_start;
    }
    inline void setPeakEnd(int peak_end)
    {
        end = peak_end;
        int_end = peak_end;
    }
};

inline void Normalise(spectrum* spec, double min = 0.0, double max = 1.0)
{
    (void)min;
    double maximum = spec->Max();

#pragma omp parallel for
    for (unsigned int i = 0; i < spec->size(); ++i)
        spec->setY(i, spec->Y(i) / maximum * max);

    spec->Analyse();
}

inline void SmoothFunction(spectrum* spec, unsigned int points)
{
    double val = 0;
    Vector vector(spec->size());
    double norm = SavitzkyGolayNorm(points);
    // #pragma omp parallel for
    for (unsigned int i = 1; i < spec->size(); ++i) {
        val = 0;
        for (unsigned int j = 0; j < points; ++j) {
            double coeff = SavitzkyGolayCoefficient(points, j);
            val += coeff * spec->Y(i + j) / norm;
            if (j)
                val += coeff * spec->Y(i - j) / norm;
        }
        vector(i - 1) = val;
    }
    spec->setSpectrum(vector);
}

inline int FindMaximum(const spectrum* spec, const Peak& peak)
{
    double val = spec->Y(peak.start);
    int pos = peak.start;
    for (unsigned int i = peak.start; i < peak.end; ++i) {
        double y = spec->Y(i);
        if (val < spec->Y(i)) {
            pos = i;
            val = y;
        }
    }
    return pos;
}

inline int FindMinimum(const spectrum* spec, const Peak& peak)
{
    double val = spec->Y(peak.start);
    int pos = peak.start;
    for (unsigned int i = peak.start; i < peak.end; ++i) {
        double y = spec->Y(i);
        if (val > spec->Y(i)) {
            pos = i;
            val = y;
        }
    }
    return pos;
}

inline std::vector<Peak> PickPeaks(const spectrum* spec, double threshold, double precision = 1000, unsigned int start = 1, unsigned int end = 0, unsigned int step = 1)
{
    std::vector<Peak> peaks;
    int pos_predes = 0;
    double predes = 0, y = 0;
    Peak peak;
    unsigned int peak_open = false;
    if (end == 0)
        end = spec->size();
    for (unsigned int i = start; i < end; i += step) {
        y = round(precision * spec->Y(i)) / precision;
        if (y <= threshold) {
            if (peak_open == 1)
                peak_open = 0;
            if (peak_open == 2) {
                peak.end = i;
                peaks.push_back(peak);
                peak.setPeakStart(i);
                peak.max = i;
                peak.setPeakEnd(i);
                peak_open = 0;
            }
            pos_predes = i;
            continue;
        }

        if (y > predes) {
            if (peak_open == 1)
                peak.max = i;
            if (peak_open == 0)
                peak.start = pos_predes;
            else if (peak_open == 2) {
                peak.end = pos_predes;
                peaks.push_back(peak);
                peak.setPeakStart(i);
                peak.max = i;
                peak.setPeakEnd(i);
                peak_open = 0;
                continue;
            }
            peak_open = 1;
        }

        if (y < predes) {
            peak_open = 2;
        }
        pos_predes = i;
        predes = y;
    }
    return peaks;
}
/*
inline std::vector<Peak> PickPeaks(const spectrum* spec, double threshold, double precision = 1000, double start = 1, double end = 0, unsigned int step = 1)
{
    return PickPeaks(spec, threshold, precision, spec->XtoIndex(start), spec->XtoIndex(end), step);
}
*/
inline std::vector<Peak> Divide2Peaks(const spectrum* spec, double start, unsigned int peaks, double end = 0)
{
    std::vector<Peak> peak_list;

    int index_start = spec->XtoIndex(start);
    int diff = (spec->size() - index_start) / peaks;

    int end_range = spec->size();
    if (end > start)
        end_range = spec->XtoIndex(end);

    std::cout << index_start << " from " << start << std::endl;
    std::cout << "spec size" << spec->size() << std::endl;

    for (unsigned int i = index_start; i < end_range; i += diff) {
        if (i - 1 + diff > spec->size())
            continue;

        Peak peak;
        peak.start = i;
        peak.end = i - 1 + diff;
        std::cout << i << " .... " << i - 1 + diff << " ---- " << spec->size() << std::endl;
        peak_list.push_back(peak);
    }
    return peak_list;
}

inline double IntegrateNumerical(const std::vector<double>& x, const std::vector<double>& y, unsigned int start = 0, unsigned int end = 0, double offset = 0)
{
    if (x.size() != y.size())
        return 0;
    if (end > x.size() || x.size() < start)
        return 0;

    if (end == 0)
        end = x.size() - 1;
    double integ = 0;
#pragma omp parallel for reduction(+ \
                                   : integ)
    for (unsigned int i = start; i < end - 1; ++i) {
        double x_0 = x[i];
        double x_1 = x[i + 1];
        double y_0 = y[i] - offset;
        double y_1 = y[i + 1] - offset;
        if (std::abs(y_0) < std::abs(y_1))
            integ += (x_1 - x_0) * y_0 + (x_1 - x_0) * (y_1 - y_0) / 2.0;
        else
            integ += (x_1 - x_0) * y_1 + (x_1 - x_0) * (y_0 - y_1) / 2.0;
    }

    return integ;
}

inline double IntegrateNumerical(const spectrum* spec, unsigned int start, unsigned int end, double offset = 0)
{
    if (end <= start)
        return 0;

    if (end > spec->size() || spec->size() < start)
        return 0;

    double integ = 0;
#pragma omp parallel for reduction(+ \
                                   : integ)
    for (unsigned int i = start; i < end - 1; ++i) {
        double x_0 = spec->x()[i];
        double x_1 = spec->x()[i + 1];
        double y_0 = spec->y()[i] - offset;
        double y_1 = spec->y()[i + 1] - offset;
        if (std::abs(y_0) < std::abs(y_1))
            integ += (x_1 - x_0) * y_0 + (x_1 - x_0) * (y_1 - y_0) / 2.0;
        else
            integ += (x_1 - x_0) * y_1 + (x_1 - x_0) * (y_0 - y_1) / 2.0;
    }

    return integ;
}

inline double IntegrateNumerical(const spectrum* spec, unsigned int start, unsigned int end, const Vector coeff)
{

    if (end <= start)
        return 0;

    if (end > spec->size() || spec->size() < start)
        return 0;

    double integ = 0;
#pragma omp parallel for reduction(+ \
                                   : integ)
    for (unsigned int i = start; i < end - 1; ++i) {
        double x_0 = spec->x()[i];
        double x_1 = spec->x()[i + 1];
        double offset_0 = Polynomial(x_0, coeff);
        double offset_1 = Polynomial(x_1, coeff);
        double y_0 = spec->y()[i] - offset_0;
        double y_1 = spec->y()[i + 1] - offset_1;
        if (std::abs(y_0) < std::abs(y_1))
            integ += (x_1 - x_0) * y_0 + (x_1 - x_0) * (y_1 - y_0) / 2.0;
        else
            integ += (x_1 - x_0) * y_1 + (x_1 - x_0) * (y_0 - y_1) / 2.0;
    }

    return integ;
}

inline double IntegrateNumerical(const spectrum* spec, Peak& peak, double offset = 0)
{
    double integ = IntegrateNumerical(spec, peak.int_start, peak.int_end, offset);
    peak.integ_num = integ;
    return integ;
}

inline double IntegrateNumerical(const spectrum* spec, Peak& peak, const Vector& coeff)
{
    double integ = IntegrateNumerical(spec, peak.int_start, peak.int_end, coeff);
    peak.integ_num = integ;
    return integ;
}
}
