#ifndef UTILS_LOGGER_H
#define UTILS_LOGGER_H

/**
* Logger Utility
*   4 levels of logging are defined: DEBUG, INFO, ERROR, NONE
*   2 different logging methods can be used (they are convenient macros to the corresponding Logger methods):
*     - functions, the parameter must be a string or a const char*:
          logDebug
          logInfo 
          logError
*     - Output streams, which can be used in the same way as std::cout: 
          debugOut
          infoOut
          errorOut

*   By default all the log levels are enabled and are written to the standard output stream (cout)

*   The logging levels can be enabled/disabled in 2 ways:
*     - During the build (recommended to optimize performance in the release build): use the macros  NO_DEBUG_LOG_BUILD,
*         NO_INFO_LOG_BUILD, NO_ERROR_LOG_BUILD.
*     - Dynamically: call Logger::setLevel(Logger::<MIN_LEVEL_TO_BE_ENABLED>). Possible values are: Logger::DEBUG, Logger::INFO or Logger::ERROR 
* 
*   Each log level can be configured to be written in a different log file. Use:
*     setLogFile(Level, logFile, policy, maxNumfiles, maxSize ) --> to set a logFile as output for a specific log level
*     setLogFile(logFile, policy, maxNumfiles, maxSize )        --> to set a logFile as output for all log levels
*   The policy indicates how the log files should be rotated. There are 3 possible policies:
*     - None => The log files does not rotate
*     - SizeRotation => The log file is rotated when a maximum size is reached (maxSize). The number of files is limited to MaxNumFiles
*     - DailyRotation => The log file is daily rotated at midnight.
* 
*   A profiling log method is also provided. A timer will be started when calling:
*     timerStart
*   And it will be stopped, showing the duration in the specified unit after calling:
*     timerStop(<unit_name>), <unit_name> can be any of the time unit names defined in the standard class chrono (e.g. seconds, milliseconds, etc)     
*   WATCH OUT: These timer methods will be disabled unless the macro ENABLE_PROFILING_LOG is added. It should not be used for the Release Build
*   By default the timer log writes to std:cout. It can be changed to a file by using th esetLogFile() method        
*   Several timers can be started in parallel, but they will be stopped in the reverse order, i.e., the last one to be started is stopped first.
*/



#include <iostream>
#include <fstream>
#include <sstream>

#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstdarg>
#include <array>
#include <vector>
#include <memory>
#include <filesystem>
#include <thread>
#include <mutex>
#include <time.h>

#include <dual_function.h>


#ifdef NO_DEBUG_LOG_BUILD
  #define logDebug(trace)
  #define debugOut *utils::Logger::getNullStream()
#else
  #define logDebug(trace) utils::Logger::debug(trace)
  #define debugOut *utils::Logger::getDebugStream()
#endif

#ifdef NO_INFO_LOG_BUILD
 #define logInfo(trace)
 #define infoOut
#else
  #define logInfo(trace) utils::Logger::info(trace)
  #define infoOut *utils::Logger::getInfoStream()
#endif

#ifdef NO_ERROR_LOG_BUILD
  #define logError(trace) utils::Logger::error(trace)
  #define errorOut *utils::Logger::getErrorStream()
#else
  #define logError(trace) utils::Logger::error(trace)
  #define errorOut *utils::Logger::getErrorStream()
#endif


#ifdef ENABLE_PROFILING_LOG
 #define timerStart       utils::Logger::startTimer(__FUNCTION__, __LINE__)
 #define timerStop(unit)  utils::Logger::stopTimer<std::chrono::##unit>(#unit, __FUNCTION__, __LINE__)
#else
 #define timerStart
 #define timerStop(unit)
#endif 


namespace utils
{
  enum LogLevel : uint8_t { DEBUG, INFO, ERROR, PROFILING, NONE };
  
    
  class Logger
  {
  public:
    enum class Policy : uint8_t { NONE, MAX_SIZE, DAILY };

  private:

    class NullStream : public std::ostream {
      inline static std::stringbuf _strbuf{};

    public:
      NullStream() : std::ostream{&_strbuf} {};
      template <typename T>
      NullStream& operator<<(T any) {
        return *this;
      }
    };

    struct LogSink {
      std::mutex lsm;

      std::ostream* stream{ nullptr };
      std::string fileName{ "" };
      std::string creationDate{ "" };
      uintmax_t maxSize{ 0 };
      uint8_t maxNumFiles{ 0 };
      bool rotateDaily{ false };

      LogSink(std::ostream& strPtr) : stream{ &strPtr } {}
      LogSink(const std::string& fileName, Policy policy, uint8_t maxNumFiles = 0, uintmax_t maxSize = 0)
        : fileName{ fileName }, maxSize{ maxSize }, 
        maxNumFiles{ policy == Policy::MAX_SIZE && maxSize ? std::max(maxNumFiles, static_cast<uint8_t>(2)) : static_cast<uint8_t>(0) } {

        if (policy == Policy::DAILY) {
          std::time_t timestamp;
          if (std::filesystem::exists(fileName)) {
            auto lastWrite = std::filesystem::last_write_time(fileName);
            auto timedif = decltype(lastWrite)::clock::now() - lastWrite;
            timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - timedif);
          }
          else {
            timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
          }
          std::stringstream strTM;
          strTM << std::put_time(std::localtime(&timestamp), "%Y%m%d");
          creationDate = strTM.str();

          stream = _openLogFile(fileName);

          // Run thread to rotate file
          std::thread rotThread(&Logger::_dailyRotationThread, std::ref(*this));
          rotThread.detach();
        }

        stream = _openLogFile(fileName);
      }

      LogSink(const LogSink& ls) = delete;
      LogSink(LogSink&& ls) = delete;

      ~LogSink() {
        if (fileName != "") delete stream;
      }
    };
    
    inline static const std::string _HEADERS[]{ "DEBUG: ", "INFO: ", "*** ERROR!\n                         " };
    inline static std::string _dateTimeFormat{ "%F %T" };

    inline static NullStream _nullStream{};

    inline static std::vector<std::unique_ptr<std::chrono::time_point<std::chrono::high_resolution_clock>>> _timers;
            
    inline static std::shared_ptr<LogSink> defLogSink{ new LogSink(std::cout) };
    inline static std::shared_ptr<LogSink> defErrLogSink{ new LogSink(std::cerr) };
    inline static std::array<std::shared_ptr<LogSink>, 4> _sinks{ defLogSink, defLogSink, defErrLogSink, defLogSink };
   
    static void _dailyRotationThread(LogSink& sink);

    static void _sizeRotation(LogSink& sink);
        
    static void _write(LogLevel level, std::string trace);

    static void _writeOut(LogLevel level);
       
    inline static std::function<std::ostream* (LogLevel)> _getStream{ [](LogLevel level) {
        Logger::_writeOut(level);
        return _sinks[level]->stream;
      }
    };

    static std::ofstream* _openLogFile(const std::string& fileName);

    Logger() = delete;
            
  public:
    inline static void startTimer(const std::string& function, int line);

    template <typename UNIT>
    static void stopTimer(const std::string& unit, const std::string& function, int line);

    static void setLevel(LogLevel level);

    static void setLogFile(LogLevel level, const std::string& fileName, Policy policy, uint8_t maxNumFiles = 0, uintmax_t maxSize = 0);
       
    static void setLogFile(const std::string& fileName, Policy policy, uint8_t maxNumFiles = 0, uintmax_t maxSize = 0);       

    inline static DualFunction<void(std::string)> debug{ [](std::string trace) {_write(LogLevel::DEBUG, trace); }, [](std::string) {} };
    inline static DualFunction<void(std::string)> info{ [](std::string trace) {_write(LogLevel::INFO, trace); }, [](std::string) {} };
    inline static DualFunction<void(std::string)> error{ [](std::string trace) {_write(LogLevel::ERROR, trace); }, [](std::string) {} };

    inline static DualFunction<std::ostream* ()> getDebugStream{ []() { return _getStream(utils::LogLevel::DEBUG); }, []() { return &_nullStream; } };
    inline static DualFunction<std::ostream* ()> getInfoStream{ []() { return _getStream(LogLevel::INFO); }, []() { return &_nullStream; } };
    inline static DualFunction<std::ostream* ()> getErrorStream{ []() { return _getStream(LogLevel::ERROR); }, []() { return &_nullStream; } };

  }; // END CLASS LOGGER


  //  LOGGER: Private Methods 
  //*************************************
  void Logger::_dailyRotationThread(LogSink& sink) {
    using namespace std::chrono_literals;
    while (true) {
      auto timestampNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::stringstream strTMNow;
      strTMNow << std::put_time(std::localtime(&timestampNow), "%Y%m%d");
      // Rotate file
      if (sink.creationDate != strTMNow.str() && std::filesystem::file_size(sink.fileName)) {
        sink.lsm.lock();
        sink.stream->flush();
        static_cast<std::ofstream*>(sink.stream)->close();
        std::filesystem::rename(sink.fileName, sink.fileName + "." + sink.creationDate);
        sink.stream = _openLogFile(sink.fileName);
        // Reset the creation date of the sink file
        sink.creationDate = strTMNow.str();
        sink.lsm.unlock();
      }
      std::tm creationTime00;
      std::stringstream strCreation00{ sink.creationDate };
      strCreation00 >> std::get_time(&creationTime00, "%Y%m%d");
      creationTime00.tm_hour = 0;
      creationTime00.tm_min = 0;
      creationTime00.tm_sec = 0;
      auto creationTP00{ std::chrono::system_clock::from_time_t(std::mktime(&creationTime00)) };
      auto sleep{ 24h - (std::chrono::system_clock::now() - creationTP00) };
      std::this_thread::sleep_for(sleep);
    }
  }

  void Logger::_write(LogLevel level, std::string trace) {
    _sinks[level]->lsm.lock();
    if (_sinks[level]->maxSize) _sizeRotation(*_sinks[level]);
    auto timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 1000;
    *_sinks[level]->stream << std::endl << std::put_time(std::localtime(&timestamp), _dateTimeFormat.c_str()) << "." << std::setw(3) << std::setfill('0') << milliseconds << " - " << _HEADERS[level] << trace;
    _sinks[level]->lsm.unlock();
  }

  void Logger::_writeOut(LogLevel level) {
    _sinks[level]->lsm.lock();
    if (_sinks[level]->maxSize) _sizeRotation(*_sinks[level]);
    auto timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 1000;
    *_sinks[level]->stream << std::endl << std::put_time(std::localtime(&timestamp), _dateTimeFormat.c_str()) << "." << std::setw(3) << std::setfill('0') << milliseconds << " - " << _HEADERS[level];
    _sinks[level]->lsm.unlock();
  }

  void Logger::_sizeRotation(LogSink& sink) {
    if (std::filesystem::file_size(sink.fileName) > sink.maxSize) {
      sink.stream->flush();
      static_cast<std::ofstream*>(sink.stream)->close();

      for (auto i = sink.maxNumFiles - 2; i > 0; --i) {
        // If file exists, rename
        std::string file = sink.fileName + "." + std::to_string(i);
        if (std::filesystem::exists(file))
          std::filesystem::rename(file, sink.fileName + "." + std::to_string(i + 1));
      }

      std::filesystem::rename(sink.fileName, sink.fileName + ".1");

      sink.stream = _openLogFile(sink.fileName);
      std::time_t timestamp{ std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
      std::stringstream strTM;
      strTM << std::put_time(std::localtime(&timestamp), "%Y%m%d");
      sink.creationDate = strTM.str();
    }
  }

  std::ofstream* Logger::_openLogFile(const std::string& fileName) {
    std::ofstream* logFile = new std::ofstream(fileName, std::ios::ate | std::ios::app);
    if (!logFile->is_open()) {
      delete logFile;
      throw std::runtime_error("Log File cannot be opened: " + fileName);
    }
    return logFile;
  }


  // LOGGER: Public methods definitions
  //*************************************
  void Logger::startTimer(const std::string& function, int line) {
    *_sinks[PROFILING]->stream << "\nTimer #" << _timers.size() + 1 << " STARTED at " << function << " (Line " << line << ")";
    _timers.push_back(std::make_unique<std::chrono::time_point<std::chrono::high_resolution_clock>>(std::chrono::high_resolution_clock::now()));
  }

  template <typename UNIT>
  void Logger::stopTimer(const std::string& unit, const std::string& function, int line) {
    auto stopTime{ std::chrono::high_resolution_clock::now() };
    if (_timers.size()) {
      auto duration{ std::chrono::duration_cast<UNIT>(stopTime - *_timers.back()).count() };
      *_sinks[PROFILING]->stream << "\nTimer #" << _timers.size() << " STOPPED at " << function << " (Line " << line << ") --- DURATION = " << duration << " " << unit;
      _timers.pop_back();
    }
    else
      *_sinks[PROFILING]->stream << "Timer not started!" << std::endl;
  }

  void Logger::setLevel(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
      debug.enableFirst();
      info.enableFirst();
      error.enableFirst();
      getDebugStream.enableFirst();
      getInfoStream.enableFirst();
      getErrorStream.enableFirst();
      break;
    case LogLevel::INFO:
      debug.enableSecond();
      info.enableFirst();
      error.enableFirst();
      getDebugStream.enableSecond();
      getInfoStream.enableFirst();
      getErrorStream.enableFirst();
      break;
    case LogLevel::ERROR:
      debug.enableSecond();
      info.enableSecond();
      error.enableFirst();
      getDebugStream.enableSecond();
      getInfoStream.enableSecond();
      getErrorStream.enableFirst();
      break;
    case LogLevel::NONE:
      debug.enableSecond();
      info.enableSecond();
      error.enableSecond();
      getDebugStream.enableSecond();
      getInfoStream.enableSecond();
      getErrorStream.enableSecond();
    }
  }

  void Logger::setLogFile(LogLevel level, const std::string& fileName, Policy policy, uint8_t maxNumFiles, uintmax_t maxSize) {
    if (_sinks[level]->fileName != "")
      throw std::runtime_error("Log file already assigned to this log level");

    for (auto& sink : _sinks) {
      if (sink->fileName == fileName) {
        // Reuse sink
        _sinks[level] = sink;
        return;
      }
    }

    // File not used in any other level
    _sinks[level] = std::shared_ptr<LogSink>(new LogSink(fileName, policy, maxNumFiles, maxSize));
  }

  void Logger::setLogFile(const std::string& fileName, Policy policy, uint8_t maxNumFiles, uintmax_t maxSize) {
    for (auto& sink : _sinks) {
      if (sink->fileName != "")
        throw std::runtime_error("Log file already assigned to a log sink: " + sink->fileName);
    }

    auto tmp{ std::shared_ptr<LogSink>(new LogSink(fileName, policy, maxNumFiles, maxSize)) };
    for (auto& sink : _sinks) sink = tmp;
  }
}


#endif UTILS_LOGGER_H