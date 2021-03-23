# logger
c++ logger utility

This is a header-only utility class to manage basic logging functionalities, including a timer for profiling. 
It can be used by just including the <logger.h> header file

Four levels of logging are defined: DEBUG, INFO, ERROR, NONE

Two different types of logging methods can be used:
  - Functions, the parameter must be a string or a const char*. Examples:
      - logDebug("trace");
      - logInfo("trace");
      - logError("trace");
  - Output streams, which can be used in the same way as std::cout. Examples:
    -   debugOut << "trace" << number << myObj;
    -   infoOut  << "trace" << number << myObj;
    -   errorOut << "trace" << number << myObj;
      
By default all the log levels are enabled and are written to the standard output stream (cout)

The logging levels can be enabled/disabled in 2 ways:
  - During the build (recommended to optimize performance in the release build): use the macros:
      - NO_DEBUG_LOG_BUILD
      - NO_INFO_LOG_BUILD
      - NO_ERROR_LOG_BUILD
  - Dynamically: call Logger::setLevel(Logger::<MIN_LEVEL_TO_BE_ENABLED>). Possible values are: Logger::DEBUG, Logger::INFO or Logger::ERROR 

Each log level can be configured to be written in a different log file. Use:
  - setLogFile(Level, logFile, policy, maxNumfiles, maxSize ) --> to set a logFile as output for a specific log level
  - setLogFile(logFile, policy, maxNumfiles, maxSize )        --> to set a logFile as output for all log levels

The policy indicates how the log files should be rotated. There are 3 possible policies:
  - None => The log files does not rotate
  - SizeRotation => The log file is rotated when a maximum size is reached (maxSize). The number of files is limited to MaxNumFiles
  - DailyRotation => The log file is daily rotated at midnight.

A profiling log method is also provided. A timer will be started when calling:
  * timerStart

And it will be stopped, showing the duration in the specified unit after calling:
  * timerStop(<unit_name>), <unit_name> can be any of the time unit names defined in the standard class chrono (e.g. seconds, milliseconds, etc)
WATCH OUT: These timer methods will be disabled unless the macro ENABLE_PROFILING_LOG is added. It should not be used for the Release Build
By default the timer log writes to std:cout. It can be changed to a file by using th esetLogFile() method        
Several timers can be started in parallel, but they will be stopped in the reverse order, i.e., the last one to be started is stopped first.

