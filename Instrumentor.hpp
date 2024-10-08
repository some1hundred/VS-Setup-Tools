//
// Basic instrumentation profiler by Cherno

// Usage: include this header file somewhere in your code (eg. precompiled header), and then use like:
//
// Instrumentor::Get().BeginSession("Session Name");        // Begin session 
// {
//     InstrumentationTimer timer("Profiled Scope Name");   // Place code like this in scopes you'd like to include in profiling
//     // Code
// }
// Instrumentor::Get().EndSession();                        // End Session
//
// You will probably want to macro-fy this, to switch on/off easily and use things like __FUNCSIG__ for the profile name.
//
#pragma once
/*in cmake remove comment indicator from debug line
#ifndef NDEBUG
#define PROFILE_SCOPE(name) InstrumentationTimer timer##__LINE__(name)
#define BEGIN_SESSION(name) Instrumentor::BeginSession(name)
#define END_SESSION() Instrumentor::EndSession()
#else
#define PROFILE_SCOPE(name) 
#define BEGIN_SESSION(name)
#define END_SESSION()
#endif
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

*/

#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>

#include <thread>
#if MY_DEBUG
#define PROFILE_SCOPE(name) InstrumentationTimer timer##__LINE__(name)
#define BEGIN_SESSION(name) Instrumentor::BeginSession(name)
#define END_SESSION() Instrumentor::EndSession()
#else
#define PROFILE_SCOPE(name) 
#define BEGIN_SESSION(name)
#define END_SESSION()
#endif
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
struct ProfileResult
{
    std::string Name;
    long long Start, End;
    uint32_t ThreadID;
};

struct InstrumentationSession
{
    std::string Name;
};

class Instrumentor
{
private:
    static std::ofstream m_OutputStream;
    static int m_ProfileCount;
public:
    Instrumentor() = delete;

    static void BeginSession(const std::string& name, const std::string& filepath = "results.json")
    {
        m_OutputStream.open(filepath);
        WriteHeader();
        
    }

    static void EndSession()
    {
        WriteFooter();
        m_OutputStream.close();
        m_ProfileCount = 0;
    }

    static void WriteProfile(const ProfileResult& result)
    {
        if (m_ProfileCount++ > 0)
            m_OutputStream << ",";

        std::string name = result.Name;
        std::replace(name.begin(), name.end(), '"', '\'');

        m_OutputStream << "{";
        m_OutputStream << "\"cat\":\"function\",";
        m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
        m_OutputStream << "\"name\":\"" << name << "\",";
        m_OutputStream << "\"ph\":\"X\",";
        m_OutputStream << "\"pid\":0,";
        m_OutputStream << "\"tid\":" << result.ThreadID << ",";
        m_OutputStream << "\"ts\":" << result.Start;
        m_OutputStream << "}";

        m_OutputStream.flush();
    }

    static void WriteHeader()
    {
        m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
        m_OutputStream.flush();
    }

    static void WriteFooter()
    {
        m_OutputStream << "]}";
        m_OutputStream.flush();
    }

};
inline int Instrumentor::m_ProfileCount = 0;
inline std::ofstream Instrumentor::m_OutputStream;

class InstrumentationTimer
{
public:
    InstrumentationTimer(const char* name)
        : m_Name(name), m_Stopped(false)
    {
        m_StartTimepoint = std::chrono::high_resolution_clock::now();
    }

    ~InstrumentationTimer()
    {
        if (!m_Stopped)
            Stop();
    }

    void Stop()
    {
        auto endTimepoint = std::chrono::high_resolution_clock::now();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
        Instrumentor::WriteProfile({ m_Name, start, end, threadID });

        m_Stopped = true;
    }
private:
    const char* m_Name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
    bool m_Stopped;
};
