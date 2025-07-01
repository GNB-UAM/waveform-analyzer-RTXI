/*
 * Copyright (C) 2025 Grupo de Neurocomputacion Biologica, Departamento de 
 * Ingenieria Informatica, Universidad Autonoma de Madrid.
 *
 * @author Alicia Garrido-Peña
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is a pluggin for RTXI 2.0 for online waveform analysis 
 */

#include "waveform-analyzer.h"
#include <iostream>
#include <main_window.h>

extern "C" Plugin::Object*
createRTXIPlugin(void)
{
  return new WaveformAnalyzer();
}

static DefaultGUIModel::variable_t vars[] = {
  // INPUT
  {"Living neuron", "Signal input to analize (V)", DefaultGUIModel::INPUT,},

  // PARAMETER
  {"Firing threshold (V)", "Threshold to declare spike beggining", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
  {"Window time (ms)", "Time around peak for the analysis (absolute window, not half)", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
  {"Slope (ms)", "Time around mid height points to calculate slope", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
  {"N Points Filter", "Number of points for the filter", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},

  //OUTPUT
  {"Filtered signal output", "Filter", DefaultGUIModel::OUTPUT,},
  {"Duration (ms) output", "Duration", DefaultGUIModel::OUTPUT,},
  {"Depol. slope output", "Calculated depol", DefaultGUIModel::OUTPUT,},
  {"Repol. slope output", "Calculated repol", DefaultGUIModel::OUTPUT,},
  {"Amplitude (V) output", "Calculated amplitude", DefaultGUIModel::OUTPUT,},
  {"Anaylis after peak output", "After peak", DefaultGUIModel::OUTPUT,},
  {"Min (V) output", "Minimun voltage in window", DefaultGUIModel::OUTPUT},
  {"Max (V) output", "Maximun voltage in window", DefaultGUIModel::OUTPUT},

  {"Mid point", "Minimun voltage in window", DefaultGUIModel::OUTPUT},
  {"Mid point 2", "Maximun voltage in window", DefaultGUIModel::OUTPUT},

  //STATE
  {"Duration (ms)", "Calculated duration (ms)", DefaultGUIModel::STATE | DefaultGUIModel::DOUBLE,},
  {"Depol. slope", "Calculated depol slope", DefaultGUIModel::STATE | DefaultGUIModel::DOUBLE,},
  {"Repol. slope", "Calculated repol slope", DefaultGUIModel::STATE | DefaultGUIModel::DOUBLE,},
  {"Amplitude (V)", "Calculated amplitude", DefaultGUIModel::STATE | DefaultGUIModel::DOUBLE,},
  {"Min (V)", "Minimun voltage in window", DefaultGUIModel::STATE | DefaultGUIModel::DOUBLE,},
  {"Max (V)", "Maximun voltage in window", DefaultGUIModel::STATE | DefaultGUIModel::DOUBLE,},
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

WaveformAnalyzer::WaveformAnalyzer(void)
  : DefaultGUIModel("WaveformAnalyzer", ::vars, ::num_vars)
{
  setWhatsThis("<p><b>WaveformAnalyzer:</b><br>Calculates onlive the duration, amplitude and slopes of the waveform</p>");
  DefaultGUIModel::createGUI(vars,
                             num_vars); // this is required to create the GUI
  customizeGUI();
  initParameters();
  update(INIT); // this is optional, you may place initialization code directly
                // into the constructor
  refresh();    // this is required to update the GUI with parameter and state
                // values
  QTimer::singleShot(0, this, SLOT(resizeMe()));
}

WaveformAnalyzer::~WaveformAnalyzer(void)
{
}


double
WaveformAnalyzer::filter(std::vector<double> signal, int cycle, double v, int n_points)
{
  // No filter case
  if (n_points == 0)
    return v;

  // Weighted average for each point and n previous
  double fv = v*0.3;
  double perc = 0.7/n_points;
  int indx;
  for (int i=1; i <= n_points; i++)
  { 
      indx = (vector_size + cycle - i) % vector_size;
      fv +=  signal[indx]*perc;
  }

  return fv;
}

double
WaveformAnalyzer::calculate_slope(double x1, double x2, double dt)
{
  return (x2-x1)/-dt;
}


double 
WaveformAnalyzer::get_slope(int point, int n_p_slope)
{
  // Get slope
  double x1 = v_buffer[(point - n_p_slope) % vector_size];
  double x2 = v_buffer[(point) % vector_size];

  return calculate_slope(x1, x2, n_p_slope*period);
}

// Retorna la distancia circular de idx1 a idx2 (hacia adelante)
int WaveformAnalyzer::circular_distance(int from, int to, int buffer_size) {
    return (to - from + buffer_size) % buffer_size;
}

int WaveformAnalyzer::get_mid_voltage_index_from_ref(int w_size, int ref)
{
  double min_val = v_buffer[ref];
  double max_val = v_buffer[ref];
  double values[w_size];
  int real_indices[w_size];

  // Single pass to:
  // - store values and their actual buffer indices
  // - find min and max values in the window
  for (int i = 0; i < w_size; i++) {
    int idx = (vector_size + ref - w_size + i) % vector_size;
    double val = v_buffer[idx];
    values[i] = val;
    real_indices[i] = idx;
    // TODO: change by previous maximum (the left is not the same point as right)
    if (val < min_val) min_val = val;
    if (val > max_val) max_val = val;
  }

  double midpoint = (min_val + max_val) / 2.0;

  // Find the index closest to the midpoint value
  // (could have been done in the previous loop, kept separate for clarity)
  double min_diff = fabs(values[0] - midpoint);
  int closest_idx = real_indices[0];

  for (int i = 1; i < w_size; i++) {
    double diff = fabs(values[i] - midpoint);
    if (diff < min_diff) {
      min_diff = diff;
      closest_idx = real_indices[i];
    }
  }

  return closest_idx;
}



void
WaveformAnalyzer::execute(void)
{
    double v = input(0);
    int w_size_points = wsize_time / period;
    int half_w = w_size_points / 2;
    int n_p_slope = time_slope / period;

    /*SAVE NEW DATA*/
    // v_buffer[cycle] = v;
    // filter signal --> if n points filter > 0 v modified
    double v_filtered = filter(v_buffer, cycle, v, n_points_filter);
    //save filtered value
    v_buffer[cycle] = v_filtered;

    output(0) = v_filtered;

    // Spike detection
    /*SPIKE DETECTED*/
    if (!got_spike) // In the peak
    {
      /*OVER THE THRESHOLD*/
      if (v > th_spike && !got_spike){
        // Change of slope
        if (v < v_buffer[cycle-3]){
            got_spike = true;
            n_waveform = half_w;
            peak_idx = cycle;
            //Init duration with the value on mid point of the units in the buffer// Guardamos los valores anteriores
            pre_mid_idx = get_mid_voltage_index_from_ref(half_w, peak_idx); //Warning: for loop
            // calculate depolarization slope
            depol_slope = get_slope(pre_mid_idx, n_p_slope);

            //Distance from peak to previous mid height spike
            int mid_duration = circular_distance(pre_mid_idx, peak_idx, vector_size);
            duration_points = mid_duration;

            v_max = v_buffer[cycle-3];

            got_spike = true;

            output(5) = 1;
        }
        else
          output(5) = 0;
      }
    }
    else //After peak
    {
      if (n_waveform < w_size_points){ //in window analysis
        n_waveform++;

        if (v < v_min)
          v_min = v;
        
        output(5) = 1;
      }
      else if (n_waveform >= w_size_points) //end of analysis
      {
        int post_mid_idx = get_mid_voltage_index_from_ref(half_w, cycle);
        
        // calculate repolarization slope
        repol_slope = get_slope(post_mid_idx + n_p_slope, n_p_slope);

        //Distance from peak to previous mid height spike
        int mid_duration2 = circular_distance(post_mid_idx, cycle, vector_size);
        
        duration_points += mid_duration2;
        duration = duration_points * period;
        
        amplitude = v_max - v_min;

        // Metrics to outputs
        output(1) = duration;
        output(2) = depol_slope;
        output(3) = repol_slope;
        output(4) = amplitude;
        output(6) = v_min;
        output(7) = v_max;

        //reset condition until next spike
        got_spike = false;

        //reset vmax/vmin
        v_max = -10000;
        v_min = 10000;

        output(5) = 0;
      }
    }
   
    /*NEXT CYCLE*/
    cycle++;
    if (vector_size == cycle){
      cycle = 0;
    }

    return;
}

void
WaveformAnalyzer::initParameters(void)
{
  //TODO fix for other period times
  vector_size = 10*4000; // 10 reads per ms * 100 ms buffer
  // vector_size = 100 /  RT::System::getInstance()->getPeriod() * 1e-6; // 0.1 ms per read * 100 ms buffer
  cycle = 0;
  v_buffer.resize(vector_size, 0);
  
  th_spike = -10;
  got_spike = false;
  n_waveform = 0;
  
  wsize_time = 100; //ms

  duration = 0;
  depol_slope = 0;
  repol_slope = 0;    
  amplitude = 0;

  duration_points = 0;
  v_max = -1000;
  v_min = 1000;

  time_slope = 2; //2 ms by default


  th_spike = 0;
  n_points_filter = 0;

}

void
WaveformAnalyzer::update(DefaultGUIModel::update_flags_t flag)
{
  switch (flag) {
    case INIT:
      period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
      setParameter("Firing threshold (V)", th_spike);
      setParameter("Window time (ms)", wsize_time);
      setParameter("Slope (ms)", time_slope);
      setParameter("N Points Filter", n_points_filter);

      setState("Duration (ms)", duration);
      setState("Depol. slope", depol_slope);
      setState("Repol. slope", repol_slope);
      setState("Amplitude (V)", amplitude);
      setState("Min (V)", v_min);
      setState("Max (V)", v_max);

      break;

    case MODIFY:
      th_spike = getParameter("Firing threshold (V)").toDouble();
      wsize_time = getParameter("Window time (ms)").toDouble();
      if (wsize_time > vector_size)
        wsize_time = vector_size - 1;

      n_points_filter = getParameter("N Points Filter").toInt();
      time_slope = getParameter("Slope (ms)").toDouble();
      
      break;

    case UNPAUSE:
      break;

    case PAUSE:
      break;

    case PERIOD:
      period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
      break;

    default:
      break;
  }
}

void
WaveformAnalyzer::customizeGUI(void)
{
}

