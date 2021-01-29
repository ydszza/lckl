#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <memory>
#include <sstream>
#include <vector>

namespace lckl {

class Logger;
class LoggerManager;


/**
 * @brief 日志级别
 */
class LogLevel {
public: 
    enum Level {
        UNKNOWN = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    /**
     * @brief 将日志级别转为文本
     * 
     * @param level 日志级别
     * @return const char* 
     */
    static const char* to_string(LogLevel::Level level);

    /**
     * @brief 将文本转化为日志级别
     * 
     * @param str 日志级别文本
     * @return LogLevel::Level 
     */
    static LogLevel::Level from_string(const std::string& str);
};

/**
 * @brief 日志事件
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;

    /**
     * @brief Construct a new Log Event object
     * 
     * @param logger 日志器
     * @param level 日志级别
     * @param file 文件名
     * @param line 行号
     * @param elapse 运行时间
     * @param threadid 线程号
     * @param fiberid 协程号
     * @param time 时间戳
     * @param threadname 线程名
     */
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t threadid, uint32_t fiberid, uint64_t time
            ,const std::string& threadname);
    
    /**
     * @brief Get the file name
     */
    const char* get_file() const { return m_file; }
    /**
     * @brief Get the line 
     */
    int32_t get_line() const { return m_line; }
    /**
     * @brief Get the elapse
     */
    uint32_t get_elapse() const { return m_elapse; }
    /**
     * @brief Get the threadid
     */
    uint32_t get_threadid() const { return m_threadid; }
    /**
     * @brief Get the fiberid
     */
    uint32_t get_fiberid() const { return m_fiberid; }
    /**
     * @brief Get the time
     */
    uint64_t get_time() const { return m_time; }
    /**
     * @brief Get the threadname 
     */
    const std::string& get_threadname() const { return m_threadname; }
    /**
     * @brief Get the 日志内容字符串流 
     */
    std::stringstream& get_ss() { return m_ss; }
    /**
     * @brief 返回日志内容
    */
   std::string get_content() const { return m_ss.str(); }
    /**
     * @brief Get the logger
     */
    std::shared_ptr<Logger> get_logger() const { return m_logger; }
    /**
     * @brief Get the level 
     */
    LogLevel::Level get_level() const { return m_level; }
    /**
     * @brief 格式化写入日志内容
     */
    void format(const char* fmt, ...);
    /**
     * @brief 格式化写入日志内容
     */
    void format(const char* fmt, va_list al);

private:
    //文件名
    const char* m_file;
    //行号
    int32_t m_line = 0;
    //程序运行的时间
    uint32_t m_elapse = 0;
    //线程id
    uint32_t m_threadid = 0;
    //协程id
    uint32_t m_fiberid = 0;
    //时间戳
    uint64_t m_time = 0;
    //线程名
    std::string m_threadname;
    //日志内容流
    std::stringstream m_ss;
    //日志器
    std::shared_ptr<Logger> m_logger;
    //日志等级
    LogLevel::Level m_level;
};

/**
 * @brief 日志事件包装器
 */
class LogEventWrap {
public:
    /**
     * @brief Construct a new Log Event Wrap object
     * 
     * @param e 日志事件
     */
    LogEventWrap(LogEvent::ptr e);
    /**
     * @brief Destroy the Log Event Wrap object
     */
    ~LogEventWrap();
    /**
     * @brief Get the 日志事件
     */
    LogEvent::ptr get_event() const { return m_event; }
    /**
     * @brief Get the 日志内容流
     */
    std::stringstream& get_ss();

private:
    //日志事件
    LogEvent::ptr m_event; 
};

/**
 * @brief 日志格式化器
 */
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    /**
     * @brief Construct a new Log Formatter object
     * 
     * @param pattern 模板格式
     * %m消息
     * %p日志级别
     * %r运行时间
     * %c日志名称
     * %t线程id
     * %n换行
     * %d时间
     * %f文件名
     * %l行号
     * %T制表符
     * %F协程id
     * %N线程名称
     * 默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
    LogFormatter(const std::string& pattern);
    /**
     * @brief 返回格式化日志文本
     * 
     * @param logger 日志器
     * @param level 日志级别
     * @param event 日志事件
     */
    std::string format(std::shared_ptr<Logger> logger
                ,LogLevel::Level level, LogEvent::ptr event);
    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger
                ,LogLevel::Level level, LogEvent::ptr event);

public: 
    /**
     * @brief 日志内容项格式化
     */
    class FormatItem {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem() {}
            /**
             * @brief 格式化日志到流
             * 
             * @param os 
             * @param logger 
             * @param level 
             * @param event 
             */
            virtual void format(std::ostream& os, std::shared_ptr<Logger> logger
                                ,LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    /**
     * @brief 初始化，解析日志模板
     */
    void init();
    /**
     * @brief 是否有错误
     */
    bool is_error() const { return m_error; }
    /**
     * @brief 返回日志模板
     */
    const std::string get_pattern() const { return m_pattern; }
    
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};

/**
 * @brief 日志输出器
*/
class LogAppender {

};

class Logger {
public:
    typedef std::shared_ptr<Logger> ptr;
    std::string get_name() {}
};

}

#endif // !__LOG_H__