/*  This file is part of aither.
    Copyright (C) 2015-17  Michael Nucci (michael.nucci@gmail.com)

    Aither is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Aither is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <iostream>     // cout
#include <vector>
#include <string>
#include "fluid.hpp"
#include "inputStates.hpp"

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;

// construct initial condition from string
fluid::fluid(string &str, const string name) {
  const auto start = str.find("(") + 1;
  const auto end = str.find(")") - 1;
  const auto range = end - start + 1;  // +/-1 to ignore ()
  auto state = str.substr(start, range);
  const auto id = str.substr(0, start - 1);
  if (id != name) {
    cerr << "ERROR. State condition specifier " << id << " is not recognized!"
         << endl;
    exit(EXIT_FAILURE);
  }
  auto tokens = Tokenize(state, ";");

  // erase portion used so multiple states in same string can easily be found
  str.erase(0, end);

  // parameter counters
  auto nCount = 0;
  auto massCount = 0;
  auto vibCount = 0;
  auto nameCount = 0;

  for (auto &token : tokens) {
    auto param = Tokenize(token, "=");
    if (param.size() != 2) {
      cerr << "ERROR. Problem with " << name << " parameter " << token << endl;
      exit(EXIT_FAILURE);
    }

    if (param[0] == "n") {
      n_ = stod(RemoveTrailing(param[1], ","));
      nCount++;
    } else if (param[0] == "molarMass") {
      molarMass_ = stod(RemoveTrailing(param[1], ","));
      massCount++;
    } else if (param[0] == "vibrationalTemperature") {
      vibTemp_ = stod(RemoveTrailing(param[1], ","));
      vibCount++;
    } else if (param[0] == "name") {
      name_ = RemoveTrailing(param[1], ",");
      nameCount++;
    } else {
      cerr << "ERROR. " << name << " specifier " << param[0]
           << " is not recognized in fluid definition!" << endl;
      exit(EXIT_FAILURE);
    }
  }

  // sanity checks
  // optional variables
  if (nCount > 1 || nameCount > 1 || vibCount > 1 || massCount > 1) {
    cerr << "ERROR. For " << name << ", name, n, vibrationalTemperature, and "
         << "molarMass can only be specified once." << endl;
    exit(EXIT_FAILURE);
  }

  if (name != "fluid") {
    cerr << "ERROR. To specify fluid, properties must be enclosed in fluid()."
         << endl;
  }
}

void fluid::Nondimensionalize(const double &tRef) {
  if (!this->IsNondimensional()) {
    vibTemp_ /= tRef;
    this->SetNondimensional(true);
  }
}

// function to read initial condition state from string
vector<fluid> ReadFluidList(ifstream &inFile, string &str) {
  vector<fluid> fluidList;
  auto openList = false;
  do {
    const auto start = openList ? 0 : str.find("<");
    const auto listOpened = str.find("<") == string::npos ? false : true;
    const auto end = str.find(">");
    openList = (end == string::npos) ? true : false;

    // test for fluid on current line
    // if < or > is alone on a line, should not look for fluid
    auto fluidPos = str.find("fluid");
    if (fluidPos != string::npos) {  // there is a fluid in current line
      string list;
      if (listOpened && openList) {  // list opened on current line, remains open
        list = str.substr(start + 1, string::npos);
      } else if (listOpened && !openList) {  // list opened/closed on current line
        const auto range = end - start - 1;
        list = str.substr(start + 1, range);  // +/- 1 to ignore <>
      } else if (!listOpened && openList) {  // list was open and remains open
        list = str.substr(start, string::npos);
      } else {  // list was open and is now closed
        const auto range = end - start;
        list = str.substr(start, range);
      }

      fluid fluidProps(list);
      fluidList.push_back(fluidProps);

      auto nextFluid = list.find("fluid");
      while (nextFluid != string::npos) {  // there are more fluids to read
        list.erase(0, nextFluid);  // remove commas separating states
        fluidProps = fluid(list);
        fluidList.push_back(fluidProps);
        nextFluid = list.find("fluid");
      }
    }

    if (openList) {
      getline(inFile, str);
      str = Trim(str);
    }
  } while (openList);

  return fluidList;
}

ostream &operator<<(ostream &os, const fluid &fl) {
  os << "fluid(name=" << fl.Name() << "; n=" << fl.N()
     << "; molarMass=" << fl.MolarMass()
     << "; vibrationalTemperature=" << fl.VibrationalTemperature() << ")";
  return os;
}