#pragma once
#include <fstream>
#include <mutex>
#include <string>
#include <filesystem>
#include <cstdarg>

class Logger 
{
public:
    ~Logger();
    bool Init(const std::string& filename);
    void Cleanup(void);
    
    bool LogSimple(const char* text, bool flushLine = true);
    bool LogFormatted(const char* fmt, ...);
    std::string ParseFormatting(const char* fmt, ...);
    std::string ParseFormatting(const char* fmt, va_list args);
    Logger(std::filesystem::path path=std::filesystem::current_path(), const char* filename="organik.log");
private:
    std::ofstream outFile;
    std::mutex logMutex;
    bool WriteToLog(const std::string& message, bool flushLine=true);
};
Logger* GetLogger();
