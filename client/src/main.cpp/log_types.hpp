#pragma once
#include "client_config.hpp"
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <algorithm>

enum class LogLevel {
    INFO,
    WARN,
    ERR
};

enum class ProcessStatus {
    RUNNING,
    SLEEPING,
    STOPPED,
    ZOMBIE
};

inline std::string log_level_to_string(LogLevel level) {
    switch (level) {
    case LogLevel::INFO:  return "INFO";
    case LogLevel::WARN:  return "WARN";
    case LogLevel::ERR:   return "ERROR";
    default:              return "UNKNOWN";
    }
}

inline std::string status_to_string(ProcessStatus status) {
    switch (status) {
    case ProcessStatus::RUNNING:  return "RUNNING";
    case ProcessStatus::SLEEPING: return "SLEEPING";
    case ProcessStatus::STOPPED:  return "STOPPED";
    case ProcessStatus::ZOMBIE:   return "ZOMBIE";
    default:                      return "UNKNOWN";
    }
}

inline std::string truncate_field(const std::string& str, size_t max_length = MAX_FIELD_LENGTH) {
    if (str.length() <= max_length) {
        return str;
    }
    return str.substr(0, max_length - 3) + "...";
}


inline std::string escape_json(const std::string& str) {
    std::string truncated = truncate_field(str);

    std::string result;
    result.reserve(truncated.length() * 2); 

    for (char c : truncated) {
        switch (c) {
        case '"':  result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b";  break;
        case '\f': result += "\\f";  break;
        case '\n': result += "\\n";  break;
        case '\r': result += "\\r";  break;
        case '\t': result += "\\t";  break;
        default:
            if (c < 0x20) {
                char buf[7];
                snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                result += buf;
            }
            else {
                result += c;
            }
            break;
        }
    }

    return result;
}

struct ProcessInfo {
    int pid;
    std::string name;
    std::string user;
    double cpu_percent;
    long long memory_kb;
    ProcessStatus status;
    LogLevel log_level;
    std::time_t timestamp;

    ProcessInfo()
        : pid(0), name(""), user(""), cpu_percent(0.0),
        memory_kb(0), status(ProcessStatus::RUNNING),
        log_level(LogLevel::INFO), timestamp(0) {
    }

    // determina nivelul de log bazat pe metrici
    LogLevel get_log_level() const {
        if (cpu_percent > 80.0 || memory_kb > 1024 * 1024) {  // >80% CPU sau >1GB RAM
            return LogLevel::ERR;
        }
        else if (cpu_percent > 50.0 || memory_kb > 512 * 1024) {  // >50% CPU sau >512MB RAM
            return LogLevel::WARN;
        }
        return LogLevel::INFO;
    }

    
    bool validate_fields() const {
        return name.length() <= MAX_FIELD_LENGTH &&
            user.length() <= MAX_FIELD_LENGTH;
    }

    void normalize_fields() {
        name = truncate_field(name);
        user = truncate_field(user);
    }

    std::string to_json(const std::string& hostname) const {
        ProcessInfo normalized = *this;
        normalized.normalize_fields();

        std::string safe_hostname = truncate_field(hostname);

        std::stringstream ss;
        ss << "{"
            << "\"hostname\":\"" << escape_json(safe_hostname) << "\","
            << "\"timestamp\":" << normalized.timestamp << ","
            << "\"pid\":" << normalized.pid << ","
            << "\"name\":\"" << escape_json(normalized.name) << "\","
            << "\"user\":\"" << escape_json(normalized.user) << "\","
            << "\"cpu_percent\":" << normalized.cpu_percent << ","
            << "\"memory_kb\":" << normalized.memory_kb << ","
            << "\"status\":\"" << status_to_string(normalized.status) << "\","
            << "\"log_level\":\"" << log_level_to_string(normalized.log_level) << "\""
            << "}";

        std::string json = ss.str();

        if (json.length() > MAX_JSON_MESSAGE_SIZE) {
          
            normalized.name = truncate_field(normalized.name, 50);
            normalized.user = truncate_field(normalized.user, 50);

            ss.str("");
            ss.clear();
            ss << "{"
                << "\"hostname\":\"" << escape_json(safe_hostname) << "\","
                << "\"timestamp\":" << normalized.timestamp << ","
                << "\"pid\":" << normalized.pid << ","
                << "\"name\":\"" << escape_json(normalized.name) << "\","
                << "\"user\":\"" << escape_json(normalized.user) << "\","
                << "\"cpu_percent\":" << normalized.cpu_percent << ","
                << "\"memory_kb\":" << normalized.memory_kb << ","
                << "\"status\":\"" << status_to_string(normalized.status) << "\","
                << "\"log_level\":\"" << log_level_to_string(normalized.log_level) << "\""
                << "}";

            json = ss.str();
        }

        return json;
    }

    size_t estimate_json_size(const std::string& hostname) const {
        return hostname.length() + name.length() + user.length() + 200; // +200 pentru overhead
    }
};

struct ProcessSnapshot {
    std::string hostname;
    std::time_t timestamp;
    std::vector<ProcessInfo> processes;

    ProcessSnapshot() : hostname(""), timestamp(0) {}

    std::string to_json() const {
        std::string safe_hostname = truncate_field(hostname);

        std::stringstream ss;
        ss << "{"
            << "\"hostname\":\"" << escape_json(safe_hostname) << "\","
            << "\"timestamp\":" << timestamp << ","
            << "\"processes\":[";

        for (size_t i = 0; i < processes.size(); ++i) {
            if (i > 0) ss << ",";

            ProcessInfo normalized = processes[i];
            normalized.normalize_fields();

            ss << "{"
                << "\"pid\":" << normalized.pid << ","
                << "\"name\":\"" << escape_json(normalized.name) << "\","
                << "\"user\":\"" << escape_json(normalized.user) << "\","
                << "\"cpu_percent\":" << normalized.cpu_percent << ","
                << "\"memory_kb\":" << normalized.memory_kb << ","
                << "\"status\":\"" << status_to_string(normalized.status) << "\","
                << "\"log_level\":\"" << log_level_to_string(normalized.log_level) << "\""
                << "}";

            if (ss.str().length() > MAX_JSON_MESSAGE_SIZE - 100) {
                ss << ",{\"warning\":\"snapshot_truncated\"}";
                break;
            }
        }

        ss << "]}";
        return ss.str();
    }

    bool fits_in_single_message() const {
        size_t estimated_size = hostname.length() + 100; // overhead
        for (const auto& proc : processes) {
            estimated_size += proc.estimate_json_size(hostname);
        }
        return estimated_size <= MAX_JSON_MESSAGE_SIZE;
    }
};