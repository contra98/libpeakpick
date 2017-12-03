
/*
 * <Math containing Header file.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

namespace PeakPick{
    
    struct Peak{
     int start = 0;
     int max = 0;
     double deconv_x = 0;
     double deconv_y = 0;
     int end = 0;
     double integ_num = 0;
     double integ_analyt = 0;
    };
    
    
    inline void Normalise(spectrum *spec, double min = 0.0, double max = 1.0)
    {
        double maximum = spec->Max();
        
#pragma omp parallel for
        for(int i = 0; i < spec->size(); ++i)
            spec->setY(i, spec->Y(i) / maximum  * max);
        
        spec->Analyse();
    }

    
    inline void SmoothFunction(spectrum *spec, int points)
    {
        double val = 0;
        Vector vector(spec->size());
        double norm = SavitzkyGolayNorm(points);
// #pragma omp parallel for
        for(int i = 0; i < spec->size(); ++i)
        {
            val = 0; 
            for(int j = 0; j <= points; ++j)
            {
                double coeff = SavitzkyGolayCoefficient(points, j);
                val += coeff*spec->Y(i+j)/norm;
                if(j)
                    val += coeff*spec->Y(i-j)/norm;                
            }
            vector(i) = val;
        }
        spec->setSpectrum(vector);
    }
    
    
    inline int FindMaximum(const spectrum *spec, const Peak &peak)
    {
        double val = 0;
        int pos = 0;
        for(int i = peak.start; i < peak.end; ++i)
        {
            double y = spec->Y(i);
            if(val < spec->Y(i))
            {
                pos = i;
                val = y;
            }
        }
        return pos;
    }

    inline std::vector<Peak> PickPeaks(const spectrum *spec, double threshold, double precision = 1000, int start = 0, int end = 0, int step = 1)
    {
        std::vector<Peak> peaks;
        int pos_predes = 0;
        double predes = 0, y = 0;
        Peak peak;
        int peak_open = false;
        if(end == 0)
            end = spec->size();
        for(int i = start; i < end; i += step)
        {
            y = round(precision*spec->Y(i))/precision;
            if( y <= threshold)
            {
                if(peak_open == 1)
                    peak_open = 0;
                if(peak_open == 2)
                {
                    peak.end = i;
                    peaks.push_back(peak);
                    peak.start = i;
                    peak.max = i;
                    peak.end = i;
                    peak_open = 0; 
                     
                }
                pos_predes = i;
                continue;
            }
                
            if(y > predes)
            {
                if(peak_open == 1)
                    peak.max = i;   
                if(peak_open == 0)
                    peak.start = pos_predes;
                else if(peak_open == 2 )
                {
                    peak.end = pos_predes;
                    peaks.push_back(peak);
                    peak.start = i;
                    peak.max = i;
                    peak.end = i;
                    peak_open = 0;
                    continue;
                }
                peak_open = 1;
            }
            
            if(y < predes)
            {
                peak_open = 2;
            }
            pos_predes = i;
            predes = y;
        }
        return peaks;
    }


    inline double IntegrateNumerical(const spectrum *spec, int start, int end)
    {
        if(spec->size() < (end - 1) || spec->size() < start)
            return 0;

        double integ = 0;
        for(int i = start; i < end - 1; ++i)
        {
            double x_0 = spec->X(i);
            double x_1 = spec->X(i+1);
            double y_0 = spec->Y(i);
            double y_1 = spec->Y(i+1);
            if(std::abs(y_0) < std::abs(y_1))
                integ += (x_1 - x_0)*y_0 + (x_1 - x_0)*(y_1-y_0)/2.0;
            else
                integ += (x_1 - x_0)*y_1 + (x_1 - x_0)*(y_0-y_1)/2.0;
        }

        return integ;
    }

    inline double IntegrateNumerical(const spectrum *spec, Peak &peak)
    {
        double integ = IntegrateNumerical(spec, peak.start, peak.end);
        peak.integ_num = integ;
        return integ;
    }
}
