/*jslint indent: 2, nomen: true, maxlen: 100, sloppy: true, vars: true, white: true, plusplus: true, nonpropdel: true */
/*global require, module, Module, FS_MOVE, FS_REMOVE, FS_EXISTS, FS_IS_DIRECTORY, FS_LIST_TREE, 
  SYS_EXECUTE, SYS_LOAD, SYS_LOG, SYS_LOG_LEVEL, SYS_OUTPUT, SYS_PROCESS_STAT, SYS_READ,
  SYS_SPRINTF, SYS_TIME, SYS_START_PAGER, SYS_STOP_PAGER, SYS_SHA256, SYS_WAIT, SYS_GETLINE,
  SYS_PARSE, ARANGO_QUIET, MODULES_PATH, COLOR_OUTPUT, COLOR_OUTPUT_RESET, COLOR_BRIGHT,
  COLOR_BLACK, COLOR_BOLD_BLACK, COLOR_BLINK, COLOR_BLUE, COLOR_BOLD_BLUE, COLOR_BOLD_GREEN,
  COLOR_RED, COLOR_BOLD_RED, COLOR_GREEN, COLOR_WHITE, COLOR_BOLD_WHITE, COLOR_YELLOW,
  COLOR_BOLD_YELLOW, PRETTY_PRINT  */

////////////////////////////////////////////////////////////////////////////////
/// @brief module "internal"
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2013 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2010-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                 Module "internal"
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup V8ModuleInternal
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief internal module
////////////////////////////////////////////////////////////////////////////////

(function () {
  Module.prototype.ModuleCache["/internal"] = new Module("/internal");

  var internal = Module.prototype.ModuleCache["/internal"].exports;

  // system functions
  internal.execute = SYS_EXECUTE;
  delete SYS_EXECUTE;

  internal.getline = SYS_GETLINE;
  delete SYS_GETLINE;

  internal.load = SYS_LOAD;
  delete SYS_LOAD;

  internal.log = SYS_LOG;
  delete SYS_LOG;

  internal.logLevel = SYS_LOG_LEVEL;
  delete SYS_LOG_LEVEL;

  internal.output = SYS_OUTPUT;
  delete SYS_OUTPUT;

  internal.parse= SYS_PARSE;
  delete SYS_PARSE;

  internal.processStat = SYS_PROCESS_STAT;
  delete SYS_PROCESS_STAT;

  internal.read = SYS_READ;
  delete SYS_READ;

  internal.sha256 = SYS_SHA256;
  delete SYS_SHA256;

  internal.sprintf = SYS_SPRINTF;
  delete SYS_SPRINTF;

  internal.time = SYS_TIME;
  delete SYS_TIME;

  internal.wait = SYS_WAIT;
  delete SYS_WAIT;

  internal.exists = FS_EXISTS;
  delete FS_EXISTS;

  internal.isDirectory = FS_IS_DIRECTORY;
  delete FS_IS_DIRECTORY;

  internal.listTree = FS_LIST_TREE;
  delete FS_LIST_TREE;

  internal.move = FS_MOVE;
  delete FS_MOVE;

  internal.remove = FS_REMOVE;
  delete FS_REMOVE;

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                  public constants
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup V8ModuleInternal
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief modules path
////////////////////////////////////////////////////////////////////////////////

  internal.MODULES_PATH = "";

  if (typeof MODULES_PATH !== "undefined") {
    internal.MODULES_PATH = MODULES_PATH;
    delete MODULES_PATH;
  }

////////////////////////////////////////////////////////////////////////////////
/// @brief quiet flag
////////////////////////////////////////////////////////////////////////////////

  internal.ARANGO_QUIET = false;

////////////////////////////////////////////////////////////////////////////////
/// @brief pretty print flag
////////////////////////////////////////////////////////////////////////////////

  internal.PRETTY_PRINT = false;

  if (typeof PRETTY_PRINT !== "undefined") {
    internal.PRETTY_PRINT = PRETTY_PRINT;
  }

////////////////////////////////////////////////////////////////////////////////
/// @brief color constants
////////////////////////////////////////////////////////////////////////////////

  internal.COLOR_OUTPUT = false;
  internal.COLOR_OUTPUT_DEFAULT = "";
  internal.COLOR_OUTPUT_RESET = "";
  internal.COLOR_BRIGHT = "";

  if (typeof COLOR_OUTPUT !== "undefined") {
    internal.COLOR_OUTPUT = COLOR_OUTPUT;
    delete COLOR_OUTPUT;
  }

  if (typeof COLOR_OUTPUT_RESET !== "undefined") {
    internal.COLOR_OUTPUT_RESET = COLOR_OUTPUT_RESET;
    delete COLOR_OUTPUT_RESET;
  }

  if (typeof COLOR_BRIGHT !== "undefined") {
    internal.COLOR_BRIGHT = COLOR_BRIGHT;
    delete COLOR_BRIGHT;
  }

  if (internal.COLOR_OUTPUT) {
    internal.COLOR_OUTPUT_DEFAULT = internal.COLOR_BRIGHT;

    internal.COLOR_BLACK = COLOR_BLACK;
    delete COLOR_BLACK;

    internal.COLOR_BOLD_BLACK = COLOR_BOLD_BLACK;
    delete COLOR_BOLD_BLACK;

    internal.COLOR_BLINK = COLOR_BLINK;
    delete COLOR_BLINK;

    internal.COLOR_BLUE = COLOR_BLUE;
    delete COLOR_BLUE;

    internal.COLOR_BOLD_BLUE = COLOR_BOLD_BLUE;
    delete COLOR_BOLD_BLUE;

    internal.COLOR_GREEN = COLOR_GREEN;
    delete COLOR_GREEN;

    internal.COLOR_BOLD_GREEN = COLOR_BOLD_GREEN;
    delete COLOR_BOLD_GREEN;

    internal.COLOR_RED = COLOR_RED;
    delete COLOR_RED;

    internal.COLOR_BOLD_RED = COLOR_BOLD_RED;
    delete COLOR_BOLD_RED;

    internal.COLOR_WHITE = COLOR_WHITE;
    delete COLOR_WHITE;

    internal.COLOR_BOLD_WHITE = COLOR_BOLD_WHITE;
    delete COLOR_BOLD_WHITE;

    internal.COLOR_YELLOW = COLOR_YELLOW;
    delete COLOR_YELLOW;

    internal.COLOR_BOLD_YELLOW = COLOR_BOLD_YELLOW;
    delete COLOR_BOLD_YELLOW;
  }

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup V8ModuleInternal
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief start pager
////////////////////////////////////////////////////////////////////////////////

  internal.start_pager = function () {};

  if (typeof SYS_START_PAGER !== "undefined") {
    internal.start_pager = SYS_START_PAGER;
    delete SYS_START_PAGER;
  }

////////////////////////////////////////////////////////////////////////////////
/// @brief stop pager
////////////////////////////////////////////////////////////////////////////////

  internal.stop_pager = function () {};

  if (typeof SYS_STOP_PAGER !== "undefined") {
    internal.stop_pager = SYS_STOP_PAGER;
    delete SYS_STOP_PAGER;
  }

////////////////////////////////////////////////////////////////////////////////
/// @brief encode password using SHA256
////////////////////////////////////////////////////////////////////////////////

  internal.encodePassword = function (password) {
    var salt;
    var encoded;

    salt = internal.sha256("time:" + internal.time());
    salt = salt.substr(0,8);

    encoded = "$1$" + salt + "$" + internal.sha256(salt + password);
    
    return encoded;
  };

////////////////////////////////////////////////////////////////////////////////
/// @brief extends a prototype
////////////////////////////////////////////////////////////////////////////////

  internal.extend = function (target, source) {
    Object.getOwnPropertyNames(source)
      .forEach(function(propName) {
        Object.defineProperty(target, propName,
			      Object.getOwnPropertyDescriptor(source, propName));
      });

    return target;
  };

////////////////////////////////////////////////////////////////////////////////
/// @brief reads a file from the module path or the database
////////////////////////////////////////////////////////////////////////////////

  internal.readFile = function (path) {
    var i;
    var mc;
    var n;

    // try to load the file
    var paths = internal.MODULES_PATH;

    for (i = 0;  i < paths.length;  ++i) {
      var p = paths[i];

      if (p === "") {
        n = "." + path + ".js";
      }
      else {
        n = p + "/" + path + ".js";
      }

      if (internal.exists(n)) {
        Module.prototype.ModuleExistsCache[path] = true;
        return { path : n, content : internal.read(n) };
      }
    }

    // try to load the module from the database
    if (internal.db !== undefined) {
      mc = internal.db._collection("_modules");

      if (mc !== null && mc.hasOwnProperty("firstExample")) {
	n = mc.firstExample({ path: path });

	if (n !== null) {
          if (n.hasOwnProperty('content')) {
            Module.prototype.ModuleExistsCache[path] = true;
            return { path : "_collection/" + path, content : n.content };
          }

	  require("console").error("found empty content in '%s'", JSON.stringify(n));
	}
      }
    }

    Module.prototype.ModuleExistsCache[path] = false;

    throw "cannot find a file named '"
        + path
        + "' using the module path(s) '" 
        + internal.MODULES_PATH + "'";
  };

////////////////////////////////////////////////////////////////////////////////
/// @brief loads a file from the file-system
////////////////////////////////////////////////////////////////////////////////

  internal.loadFile = function (path) {
    var i;

    // try to load the file
    var paths = internal.MODULES_PATH;

    for (i = 0;  i < paths.length;  ++i) {
      var p = paths[i];
      var n;

      if (p === "") {
        n = "." + path + ".js";
      }
      else {
        n = p + "/" + path + ".js";
      }

      if (internal.exists(n)) {
        return internal.load(n);
      }
    }

    throw "cannot find a file named '"
        + path 
        + "' using the module path(s) '" 
        + internal.MODULES_PATH + "'";
  };

////////////////////////////////////////////////////////////////////////////////
/// @brief defines a module
////////////////////////////////////////////////////////////////////////////////

  internal.defineModule = function (path, file) {
    var content;
    var m;
    var mc;

    content = internal.read(file);

    mc = internal.db._collection("_modules");

    if (mc === null) {
      mc = internal.db._create("_modules", { isSystem: true });
    }

    path = module.normalise(path);
    m = mc.firstExample({ path: path });

    if (m === null) {
      mc.save({ path: path, module: content });
    }
    else {
      m.module = content;
      mc.replace(m, m);
    }
  };

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

}());

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// @addtogroup\\|// --SECTION--\\|/// @page\\|/// @}\\)"
// End:
