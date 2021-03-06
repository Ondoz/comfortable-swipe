#ifndef __comfortable_swipe_main__
#define __comfortable_swipe_main__
/**
 * The main driver program for `comfortable-swipe-buffer`
 *
 * This C++ program parses the output from libinput debug-events
 * into a buffer and dispatches C++ libxdo commands upon swipe.
 *
 * Possible paths of the executable:
 *
 *   /usr/bin/comfortable-swipe-buffer
 *   /usr/local/bin/comfortable-swipe-buffer
 */
/*
Comfortable Swipe
by Rico Tiongson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "comfortable-swipe-defines.cpp"
#include "comfortable-swipe-gesture-swipe-xdokey.cpp"
#include "comfortable-swipe-gesture-swipe-xdomouse.cpp"
#include "comfortable-swipe-gesture-swipe.cpp"
#include <iostream> // std::ios, std::cin, std::getline
#include <map>      // std::map
#include <set>      // std::set
#include <string>   // std::string

extern "C" {
#include <ini.h> // ini_parse
}

/**
 * Registers keys and values from the config file to a map.
 */
int parse_config(void *config, const char section[], const char name[],
                 const char value[]) {
  auto &configmap = *(std::map<std::string, std::string> *)config;
  configmap[name] = value;
  return 0;
}

/**
 * The main driver program.
 */
int main(int argc, char *argv[]) {
  using namespace std;
  using namespace comfortable_swipe;
  // unsync stdio for faster IO
  ios::sync_with_stdio(false);
  cin.tie(0);
  cout.tie(0);
  // parse configuration file
  map<string, string> config;
  if (ini_parse(COMFORTABLE_SWIPE_CONFIG, parse_config, &config) < 0) {
    cerr << "error: config " << COMFORTABLE_SWIPE_CONFIG << endl;
    return EXIT_FAILURE;
  }
  // clear config and just use "threshold" if --bare
  if (set<string>(argv + 1, argv + argc).count("--bare")) {
    if (config.count("threshold")) {
      auto threshold = config["threshold"];
      config.clear();
      config["threshold"] = threshold;
    } else {
      config.clear();
    }
  }
  // initialize keyboard swipe gesture handler
  // commands are: [left|up|right|down][3|4]
  // we will fetch our commands from the config in correct order
  // Examples:
  //   left3=ctrl+alt+Right   shift to right workspace
  //   right3=ctrl+alt+Left   shift to left workspace
  //   up3=super+Up           maximize
  //   down3=super+Down       minimize
  decltype(gesture_swipe_xdokey::commands) commands;
  for (size_t i = 0; i < commands.size(); ++i)
    commands[i] = config[gesture_swipe_xdokey::command_name[i]];
  // correctly parse threshold as float
  float threshold = 0.0f;
  try {
    threshold = stof(config["threshold"]);
  } catch (...) {
  }
  // create swipe handler
  gesture_swipe_xdokey keyswipe(commands, threshold);
  // initialize mouse hold gesture handler
  // for now, this supports 3-finger and 4-finger hold
  // Examples:
  //   mouse3=move     move mouse on 3 fingers
  //   mouse3=button1  hold button 1 on 3 fingers
  //   mouse4=button3  hold button 3 (right click) on 3 fingers
  // warn user that hold3 is deprecated
  if (config.count("hold3"))
    std::cerr << "WARNING: hold3 is deprecated. Use mouse3 instead."
              << std::endl;
  if (config.count("hold4"))
    std::cerr << "WARNING: hold4 is deprecated. Use mouse4 instead."
              << std::endl;
  // get input values
  string mouse3 = config.count("mouse3") ? config["mouse3"] : config["hold3"];
  string mouse4 = config.count("mouse4") ? config["mouse4"] : config["hold4"];
  bool nomouse = mouse3.empty() || mouse4.empty(); // TODO: check if mouse invalid
  // create our mouse gesture holder
  gesture_swipe_xdomouse mousehold(mouse3.data(), mouse4.data());
  // start reading lines from input one by one
  for (string line; getline(cin, line);) {
    if (nomouse) {
      keyswipe.run(line.data());
    } else {
      // optimization: if no mouse config is set, just run keyboard
      if (mousehold.is_swiping() && mousehold.button == MOUSE_NONE) {
        keyswipe.run(line.data());
      } else if (mousehold.run(line.data())) {
        // only allow keyswipe gestures on mouse move
        if (mousehold.button == MOUSE_NONE || mousehold.button == MOUSE_MOVE) {
          keyswipe.run(line.data());
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

#endif
