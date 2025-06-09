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

#include <default_gui_model.h>

class WaveformAnalyzer : public DefaultGUIModel
{

  Q_OBJECT

public:
  WaveformAnalyzer(void);
  virtual ~WaveformAnalyzer(void);

  void execute(void);
  void createGUI(DefaultGUIModel::variable_t*, int);
  void customizeGUI(void);

  double filter(std::vector<double> signal, int index, double v, int n_points);
  double calculate_slope(double x1, double x2, double dt);
  int get_mid_voltage_index_from_ref(int w_size, int ref);
  double get_slope(int point, int n_p_slope);
  int circular_distance(int from, int to, int buffer_size);

protected:
  virtual void update(DefaultGUIModel::update_flags_t);

private:
  
  double period;

  double th_spike;

  bool got_spike;
  int n_waveform;
  int peak_idx;

  //metrics
  double duration;
  double depol_slope;
  double repol_slope;  
  double amplitude;

  // aux metrics
  double duration_points;
  double v_max;
  double v_min;
  int time_slope;

  bool switch_th;
  double wsize_time;
  int n_points_filter;
  int pre_mid_idx;

  int vector_size;
  int cycle;

  std::vector<double> v_buffer;

  void initParameters();

private slots:
  // these are custom functions that can also be connected to events
  // through the Qt API. they must be implemented in plugin_template.cpp

};
