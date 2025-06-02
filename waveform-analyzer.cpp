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
 * This is pluggin for RTXI 2.0 for online waveform analysis 
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
  {
    "Waveform analyzer", "Analyze spike waveform from an input signal",
    DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
  },
  {
    "A State", "Tooltip description", DefaultGUIModel::STATE,
  },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

WaveformAnalyzer::WaveformAnalyzer(void)
  : DefaultGUIModel("WaveformAnalyzer with Custom GUI", ::vars, ::num_vars)
{
  setWhatsThis("<p><b>WaveformAnalyzer:</b><br>QWhatsThis description.</p>");
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

void
WaveformAnalyzer::execute(void)
{
    double v = input(0);
    int w_size_points = wsize_time / period;
    int allow_reset = 1;

    /*SAVE NEW DATA*/
    // v_list[cycle] = v;
    // filter signal --> if n points filter > 0 v modified
    double v_filtered = filter(v_list, cycle, v, n_points);
    //save filtered value
    v_list[cycle] = v_filtered;

    output(0) = v_filtered;

    // int n_p_slope = 15;

    // Calculate slope 
    double x1 = v_list[(vector_size + cycle) % vector_size];
    double x2 = v_list[(vector_size + cycle - n_p_slope) % vector_size];
    curr_slope = calculate_slope(x1, x2, n_p_slope * period);

    

    output(4) = curr_slope;

    // Spike detection

    /*OVER THE THRESHOLD*/
    if (v > th_spike && switch_th == true){
      if (v < v_list[cycle-3]){

        n_spikes++;

        /*SPIKE DETECTED*/
        if (wsize_time < 0)
        {
          updatable = true;
          // cycle --;
        }

        // Save threshold values for next spike

        // Get threshold for V
        switch_th = false;
        th_calculated = v_list[(vector_size + cycle - wsize_points) % vector_size];
        output(1) = th_calculated;

        // Get slope
        x1 = v_list[(vector_size + cycle - wsize_points ) % vector_size];
        x2 = v_list[(vector_size + cycle - wsize_points -n_p_slope) % vector_size];
      
        sl_calculated = calculate_slope(x1, x2, n_p_slope*period);
        output(2) = sl_calculated;

        //Get threshold for sum 
        th_sum_calculated = sum_list[(vector_size + cycle - wsize_points) % vector_size];
        //Get threshold by mean of 3 last spikes. 
        //TODO: define lim of buffer and size. (use more points ¿?)
        th_sum_buff[n_spikes%10] = th_sum_calculated;
        th_sum_calculated = (th_sum_calculated + th_sum_buff[(n_spikes%10)-1] + th_sum_buff[(n_spikes%10)-2])/3;


        allow_reset = 1;
      }
    }
    
    // Update states
    if (updatable)
    {
      // output(6) = sum <= th_sum_param; //Area threshold crossed
      output(6) = sum < th_sum_calculated; //Area threshold crossed
      output(7) = v > th_calculated; //Voltage threshold crossed
      output(8) = curr_slope > sl_calculated; //Current threshold crossed
    }
    else
    {
      output(6) = 0;
      output(7) = 0;
      output(8) = 0;
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
  some_parameter = 0;
  some_state = 0;
}

void
WaveformAnalyzer::update(DefaultGUIModel::update_flags_t flag)
{
  switch (flag) {
    case INIT:
      period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
      setParameter("GUI label", some_parameter);
      setState("A State", some_state);
      break;

    case MODIFY:
      some_parameter = getParameter("GUI label").toDouble();
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
  QGridLayout* customlayout = DefaultGUIModel::getLayout();

  QGroupBox* button_group = new QGroupBox;

  QPushButton* abutton = new QPushButton("Button A");
  QPushButton* bbutton = new QPushButton("Button B");
  QHBoxLayout* button_layout = new QHBoxLayout;
  button_group->setLayout(button_layout);
  button_layout->addWidget(abutton);
  button_layout->addWidget(bbutton);
  QObject::connect(abutton, SIGNAL(clicked()), this, SLOT(aBttn_event()));
  QObject::connect(bbutton, SIGNAL(clicked()), this, SLOT(bBttn_event()));

  customlayout->addWidget(button_group, 0, 0);
  setLayout(customlayout);
}

// functions designated as Qt slots are implemented as regular C++ functions
void
WaveformAnalyzer::aBttn_event(void)
{
}

void
WaveformAnalyzer::bBttn_event(void)
{
}
