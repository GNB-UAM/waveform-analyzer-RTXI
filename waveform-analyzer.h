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

protected:
  virtual void update(DefaultGUIModel::update_flags_t);

private:
  double some_parameter;
  double some_state;
  double period;

  void initParameters();

private slots:
  // these are custom functions that can also be connected to events
  // through the Qt API. they must be implemented in plugin_template.cpp

  void aBttn_event(void);
  void bBttn_event(void);
};
