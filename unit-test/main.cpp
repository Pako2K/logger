// Uncomment this option to test 
//#define NO_DEBUG_LOG_BUILD

#define ENABLE_PROFILING_LOG

#include "logger.h"

int main() {
  using namespace utils;

  timerStart;

  Logger::setLevel(LogLevel::DEBUG);
  logError("Error info");
  infoOut << "test";
  logInfo("(INFO)");
  logDebug("Debug");
  debugOut << 2 << "dfsadf";

  Logger::setLevel(LogLevel::INFO);
  infoOut << "INFO set";
  logError("Error info");
  infoOut << "test";
  logInfo("(INFO)");
  logDebug("Debug");
  debugOut << 2 << "dfsadf";

  timerStart;

  Logger::setLevel(LogLevel::ERROR);
  errorOut << "ERROR set";
  logError("Error info");
  infoOut << "test";
  logInfo("(INFO)");
  logDebug("Debug");
  debugOut << 2 << "dfsadf";

  timerStop(microseconds);

  timerStop(milliseconds);

  Logger::setLevel(LogLevel::DEBUG);
  Logger::setLogFile(LogLevel::DEBUG, "logfileDEB.log", Logger::Policy::DAILY);
  Logger::setLogFile(LogLevel::INFO, "logfile.log", Logger::Policy::MAX_SIZE, 4, 500);
  Logger::setLogFile(LogLevel::ERROR, "logfile.log", Logger::Policy::MAX_SIZE);

  for (int i = 0; i < 10; ++i) {
    logError("Log file Error info");
    infoOut << "Log file test " << i;
    logInfo("Log file (INFO)");
    logDebug("Log file Debug");
    debugOut << "Log file" << i << "dfsadf";
  }
   
}