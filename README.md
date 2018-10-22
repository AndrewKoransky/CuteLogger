# CuteLogger
Logger: simple, convinient and thread safe logger for Qt-based C++ apps

To use:
```C++
#include "Logger.h"
#include "OutputDebugAppender.h"
#include "RollingFileAppender.h"

// ...

CuteLogger::OutputDebugAppender* outputDebugAppendr = new CuteLogger::OutputDebugAppender();
cuteLogger->registerAppender(outputDebugAppendr);
CuteLogger::RollingFileAppender* rollingFileAppendr = new CuteLogger::RollingFileAppender("MyTest.log"); 
rollingFileAppendr->setComputeRollOverBasedOnLastModified(true);
rollingFileAppendr->setDatePattern("'.'yyyy-MM-dd-hh-mm'.log'");
rollingFileAppendr->setLogFilesLimit(30);
rollingFileAppendr->setDetailsLevel(CuteLogger::Logger::LogLevel::Debug);
cuteLogger->registerAppender(rollingFileAppendr);
LOG_INFO("Some information to log here");
```

Forked from https://github.com/dept2/CuteLogger. Added the following changes:
- Placed all classes in CuteLogger namespace to prevent conflict in C++ (especially with Visual C++ test projects).
- adjusted file rollover functionality to check the current log file's modified date and rollover immediately if the date is before the current log file date.
- added VC++ project files (not necessarily needed... you can create these from .pro files).
