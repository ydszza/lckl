#include "log.h"
#include <stdarg.h>
#include <iostream>
#include <map>
#include <functional>

namespace lckl {

const char* LogLevel::to_string(LogLevel::Level level) {
    switch (level) {
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

    XX(UNKNOWN);
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default: 
        return "UNKNOWN";
    }
    return "UNKNOWN"; 
}

LogLevel::Level LogLevel::from_string(const std::string& str) {
#define XX(level, v) \
    if (str == #v) { \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOWN;
#undef XX
}


LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
                  ,const char* file, int32_t line, uint32_t elapse
                  ,uint32_t threadid, uint32_t fiberid, uint64_t time
                  ,const std::string& threadname)
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadid(threadid)
    ,m_fiberid(fiberid)
    ,m_time(time)
    ,m_threadname(threadname)
    ,m_logger(logger)
    ,m_level(level) {
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if (len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e){
}

std::stringstream& LogEventWrap::get_ss() { 
    return m_event->get_ss();
}


LogFormatter::LogFormatter(const std::string& pattern) 
    : m_pattern(pattern) {
    init();
}

void LogFormatter::init() {
    //str   format  type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        //会执行以下步骤，那么此次循环开头是%

        //%%转义为%
        if (i+1 < m_pattern.size() && m_pattern[i+1] == '%') {
            nstr.append(1, '%');
            continue;
        }

        size_t n = i + 1;//跳过%号
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size()) {
            //格式状态非0 && 不是字母 && 不是{和}
            //即碰到了空格，%xxxx格式
            if (!fmt_status && !isalpha(m_pattern[n]) 
                    && m_pattern[n] != '}' && m_pattern[n] != '}') {
                str = m_pattern.substr(i+1, n-i-1);
                break;
            }
            //碰到了{，设置fmt状态为1，记录起始位置
            if (fmt_status == 0) {
                if (m_pattern[n] == '{') {
                    str = m_pattern.substr(i+1, n-i-1);
                    fmt_status = 1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }    
            }
            //碰到了}，保存解析到的{}内的格式，去除fmt状态退出
            else if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin+1, n-fmt_begin-1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            //跳过空格
            ++n;
            //即模板全文%xxx
            if (n == m_pattern.size()) {
                if (str.empty()) {
                    str = m_pattern.substr(i+1);
                }
            }
        }
        if (fmt_status == 0) {
            //非格式文本，直接保存原文本
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            //解析到的一个转义格式
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        //{}只出现了{
        else if (fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << m_pattern
                      << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str, C) \
    {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}
        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, NameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FilenameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:Tab
        XX(F, FiberIdFormatItem),           //F:协程id
        XX(N, ThreadNameFormatItem),        //N:线程名称
#undef XX
    };
    for (auto& i : vec) {
        if (std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))))
        }
        else {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i)+">>")));
                m_error = true;
            }
            else {
                m_items63.
                
            }
        }
    }
}

}