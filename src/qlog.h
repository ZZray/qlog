#ifndef __RAY_QLOG_HPP__
#define __RAY_QLOG_HPP__

/*
 * @brief 一个简单的单文件日志类，内置了console和file两个日志输出类。自定义输出参考ConsoleAppender以及AppenderRegistry::addAppenders
 * @version 1.0
 * @author RayZhang
 * @repo
 * @todo
 * 基本够用了，但是还有很多可以优化的地方。
 */
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <iostream>
#include <list>
#include <map>
#include <functional>
#include <filesystem>
#include <chrono>
#include <format>
#include <source_location>

#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif // _HAS_STD_BYTE
#define _HAS_STD_BYTE 0

#if defined(WIN32) || defined(_WIN32) || defined(Q_OS_WIN32)
#define localtime_r(_Time, _Tm) localtime_s(_Tm, _Time)
#endif

#ifdef USE_QT
#include <QDateTime>
#include <QThread>
#include <QString>
#include <QDebug>
#include <QApplication>
#endif

namespace ray::log
{
    // 在外部可以重定义这个参数，内置的两个日志输出器。
    //#ifndef console
    //#define console Logger(__FILE__, __LINE__, ray::log::Level::Debug, {"console"})
    //// #define console_debug Logger(__FILE__, __LINE__, Level::Debug, {"console"})
    //#endif
    //// file logger
    //#ifndef flogger
    //#define flogger Logger(__FILE__, __LINE__, ray::log::Level::Info, {"file"})
    //#endif
    //// global_logger
    //#ifndef glogger
    //#define glogger Logger(__FILE__, __LINE__, ray::log::Level::Info)
    //#endif

    // 不传入appender默认会调用这些appender进行日志记录
    // clang-format off
#ifndef DEFAULT_APPENDERS
#define DEFAULT_APPENDERS    {"file", "console"}
#endif

// 是否开启控制台输出
#ifndef DISABLE_CONSOLE
#define DISABLE_CONSOLE 0
#endif
// clang-format on
    class Event;

    // 日志级别
    enum class Level
    {
        Unknown = 0,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    // 日志内容流
    class Stream : public std::ostringstream
    {
    public:
        using Ptr = std::shared_ptr<Stream>;
#ifdef USE_QT
        // Stream& operator<<(const QString& str) {
        //  *this << str.toStdString();
        //  return *this;
        // }
#endif
    };

    // 工具类
    class Utils
    {
    public:
        template <typename... Args>
        static std::string string_format(const char* format, Args... args);
        // 将leve转为字符串
        inline static std::string levelToString(Level level);
        // 通过文件路径分割出文件名
        static std::string getFilename(const std::string& filepath);
    };

    // 日志格式化类, 根据特定格式，将数据格格式化成字符串。
    class Formatter
    {
    public:
        virtual ~Formatter() = default;
        using Ptr = std::shared_ptr<Formatter>;
        //
        virtual std::string format(std::shared_ptr<Event> logEvent);
    };

    // 日志事件, 每次写日志其实是一个事件，同步事件直接写，如果是异步事件则加入到日志记录的事件循环。
    class Event /*: std::enable_shared_from_this<Event>*/
    {
    public:
        using Ptr = std::shared_ptr<Event>;

        Event()
            : level(Level::Debug)
        { }

        // 当前时间
        // std::shared_ptr<struct tm> time;
        int64_t time = 0;
        // 等级
        Level level;
        // 错误码
        int code = 0;
        // 行号
        uint32_t line = 0;
        // 文件名
        std::string file;
        // 线程ID
        uint32_t threadId = 0;
        // 日志内容
        Stream content;

        // 本次的格式化方法，如果为空则使用，appender自己的格式化方法。
        Formatter::Ptr formatter;
        // key
        std::string key = "global";
        // std::string pattern;

        // 转为流
        // std::string toString(Formatter::Ptr formatter) {
        //  return formatter->format(shared_from_this());
        //}
    };

    // 日志交互类
    class Logger
    {
    public:
        Logger() = delete;
        virtual ~Logger();
        /**
         * @brief 构造函数
         * @param file
         * @param line
         * @param level
         * @param[in] appenders 本次的日志要记录在哪儿，默认全部的appender
         */
        explicit Logger(const std::string& file = std::source_location::current().file_name(),
            uint32_t line = std::source_location::current().line(),
            Level level = Level::Info,
            std::list<std::string> appenders = {});
        explicit Logger(const std::string& key, const std::string& file, uint32_t line, Level level = Level::Info, std::list<std::string> appenders = {});
        /**
         * @brief 写日志
         * @usage: Logger(Level::Debug).log("%d, %d, %.2f, %s", 1, 2, 4.1, "hello");
         */
        template <typename T = const char*, typename... Args>
        void log(T format, Args... args);
        // 一些便捷方法
        template <typename T = const char*, typename... Args>
        void debug(T format, Args... args);
        template <typename T = const char*, typename... Args>
        void info(T format, Args... args);
        template <typename T = const char*, typename... Args>
        void warning(T format, Args... args);
        template <typename T = const char*, typename... Args>
        void error(T format, Args... args);
        template <typename T = const char*, typename... Args>
        void fatal(T format, Args... args);

    private:
        void flush();

    public:
        // 一些便捷方法
        template <class T>
        Logger& operator<<(const T& s);
        // 设置等级
        Logger& set_level(Level level);
        // 错误码
        Logger& set_code(int code);
        // 设置appender
        Logger& set_appenders(std::list<std::string> appenders);
        // 设置格式
        Logger& set_formatter(Formatter::Ptr formatter);
        // 行号
        Logger& set_line(uint32_t line);
        // 文件
        Logger& set_file(std::string file);
        // Logger& pattern(std::string pattern) {
        //  return *this;
        // }
        Logger& set_key(const std::string& key);
        Logger& operator()(Level);

        template <typename T = const char*>
        std::string format(T message, Formatter::Ptr formatter = nullptr);

    public:
        // time() 与 console.timeEnd() 用来计算一段程序的运行时间。
        std::chrono::time_point<std::chrono::high_resolution_clock> time();
        // 传入上一步的结果, 默认返回毫秒
        template <typename Tm = std::chrono::milliseconds>
        uint32_t timeEnd(std::chrono::time_point<std::chrono::high_resolution_clock> start, std::string label);

    private:
        Event::Ptr _logEvent;
        std::list<std::string> _appenders;
    };

    // 负责写日志的组件, 每一个appender有自己的level等级
#ifdef USE_QT
    class Appender : public QObject
    {
#else
    class Appender
    {
#endif

    public:
        virtual ~Appender() = default;
        using Ptr = std::shared_ptr<Appender>;
        void setLevel(Level level) { _level = level; }
        Level level() { return _level; };
        // void setPattern();
        void setFormatter(Formatter::Ptr formatter) { _formatter = formatter; }
        // 写日志
        bool write(Event::Ptr);
        virtual bool flush(const Event::Ptr) = 0;

    protected:
        Formatter::Ptr getFormatter(Event::Ptr);

    protected:
        Level _level = Level::Info;
        Formatter::Ptr _formatter;

        std::mutex _mtxFlush;
    };

    class ConsoleAppender : public Appender
    {
    public:
        ConsoleAppender() { _level = Level::Debug; }
        bool flush(const Event::Ptr) override;
    };

    // class FileSplitPerDay {};
    // template <class FileSplitPolicy = FileSplitPerDay>
    // inline constexpr long double operator "" MB(long double v)
    //{
    //  return v;
    // }
#define MB

    class FileAppender : public Appender
    {
    public:
    public:
        FileAppender();
        virtual ~FileAppender();
        // 接收日志事件和FileAppender并返回新的文件名。如果文件名不变则不会进行分割。
        using FileSplitPolicy = std::function<std::string(Event::Ptr, FileAppender&)>;
        // 设置最后路径，最好不要加最后一个/
        void setBasePath(const std::string& basePath);
        void setPath(const std::string& path);
        // 刷新日志内容
        bool flush(const Event::Ptr) override;
        // 设置文件分割策略
        void setFileSplitPolicy(const FileSplitPolicy& cb);
        // 设置根据
        // 获取文件大小
        const size_t filesize();
        // 获取当前记录的文件名,
        std::string filename() const;
        // 当前路径
        std::string path() const;
        // 顶层路径
        std::string basePath() const;

    private:
        bool resetFile(Event::Ptr);

    private:
        std::ofstream _file;
        // 不要加最后一个/
        std::string _basePath;
        // 除去文件名的路径
        std::string _path;
        // 默认使用的是2022-06-06
        std::string _filename;
        // 文件分割策略
        FileSplitPolicy _fileSplitPolicy = nullptr;
    };

    // 日志appender工厂
    class AppenderFactory
    {
    public:
        using CreateMethod = std::function<Appender::Ptr()>;
        // 利用注册的方法创建appender实例
        Appender::Ptr create(const std::string& name);
        // 注册创建appender的方法
        void registerCreateMethod(const std::string& name, CreateMethod);

        void clear()
        {
            std::lock_guard<std::shared_mutex> lock(_mutex);
            _appenders.clear();
        };
        static AppenderFactory& instance();

    private:
        AppenderFactory();

    private:
        // 不用if else
        std::map<std::string, std::function<Appender::Ptr()>> _appenders;

        std::shared_mutex _mutex;
    };

    // 日志实例仓库
    class AppenderRegistry
    {
    public:
        // 所有定义的logger都应该是单例的，这里存储了这些单例,  使用name可兼容同一个种appender多个实例，以便输出到不同的位置。
        const Appender::Ptr get(const std::string& name);
        void addAppenders(std::list<std::string> appenders);
        void clear();
        // 获取所有的appender名
        // std::list<std::string> keys();
        static AppenderRegistry& instance();

    private:
        AppenderRegistry() = default;

    private:
        std::map<std::string, Appender::Ptr> _appenders;
        std::shared_mutex _mutex;
    };

    // std::map<std::string, Appender::Ptr> AppenderRegistry::_appenders;
    //////////////////////////   实现代码   ///////////////////
    // =============    Logger    ============
    inline Logger::Logger(const std::string& file, uint32_t line, Level level, std::list<std::string> appenders /*= {}*/)
        : Logger("global", file, line, level, appenders)

    { }

    inline Logger::Logger(const std::string& key, const std::string& file, uint32_t line, Level level, std::list<std::string> appenders)
    {
        using namespace std::chrono;
        _logEvent = std::make_shared<Event>();
        _appenders = appenders;
        _logEvent->level = level;
        _logEvent->line = line;
        _logEvent->file = file;
        _logEvent->key = key.empty() ? "global" : key;

        _logEvent->time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

        auto tid = std::this_thread::get_id();
        _logEvent->threadId = (*(uint32_t*)&tid);;
    }

    inline Logger::~Logger()
    {
        flush();
    }

    inline void Logger::flush()
    {
        if (_logEvent->content.str().empty()) {
            return;
        }
        if (_appenders.empty()) {
            _appenders = DEFAULT_APPENDERS;
        }

        for (auto key : _appenders) {
            if (key.empty()) {
                continue;
            }
            // 如果关闭控制台输出
            if constexpr (DISABLE_CONSOLE) {
                if (key == "console") {
                    continue;
                }
            }
            if (auto appender = AppenderRegistry::instance().get(key)) {
                if (_logEvent->level >= appender->level()) {
                    appender->write(_logEvent);
                }
            }
        }
    }

    inline ray::log::Logger& Logger::operator()(Level level)
    {
        _logEvent->level = level;
        return *this;
    }

    inline ray::log::AppenderRegistry& AppenderRegistry::instance()
    {
        static std::once_flag _flag;
        static std::unique_ptr<AppenderRegistry> _self;
        std::call_once(_flag,
            [&] {
            _self.reset(new AppenderRegistry);
        });
        return *_self;
    }

    // =========================  formatter
    inline std::string Formatter::format(Event::Ptr logEvent)
    {
#ifdef USE_QT
        std::string logHead = std::format("[{}][{}][{}][{}][{}:{}] ",
            Utils::levelToString(logEvent->level).c_str(),
            QDateTime::fromMSecsSinceEpoch(logEvent->time).toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString().c_str(),
            logEvent->threadId,
            logEvent->code,
            Utils::getFilename(logEvent->file).c_str(),
            logEvent->line);
#else
        const auto sec = logEvent->time / 1000;
        std::tm* time = std::gmtime(&sec);
        int year = time->tm_year + 1900;
        int month = time->tm_mon + 1;
        int day = time->tm_mday;
        const auto& logHead = std::format("[{0}][{1}-{2:02d}-{3:02d} {4:02d}:{5:02d}:{6:02d}][{7}][{8}:{9}] ",
            Utils::levelToString(logEvent->level),
            1900 + time->tm_year,
            time->tm_mon + 1,
            time->tm_mday,
            time->tm_hour,
            time->tm_min,
            time->tm_sec,
            logEvent->threadId,
            Utils::getFilename(logEvent->file),
            logEvent->line);
#endif
        return logHead + logEvent->content.str();
    }

    //=========================    Utils
    inline std::string Utils::levelToString(Level level)
    {
        switch (level) {
        case ray::log::Level::Debug: return "DEBUG";
        case ray::log::Level::Info: return "INFO ";
        case ray::log::Level::Warning: return "WARN ";
        case ray::log::Level::Error: return "ERROR";
        case ray::log::Level::Fatal: return "FATAL";
        default: break;
        }
        return "Unknown";
    }

    inline std::string Utils::getFilename(const std::string& filepath)
    {
        auto pos = filepath.find_last_of("/");
        if (pos == filepath.npos) {
            pos = filepath.find_last_of("\\");
            if (pos == filepath.npos) {
                return filepath;
            }
        }
        return filepath.substr(pos + 1, filepath.length());
    }

    //=========================== factory
    inline AppenderFactory::AppenderFactory()
    {
        _appenders = {
            {
                "console",
                [] {
                    return std::make_shared<ConsoleAppender>();
                }
            },
            {
                "file",
                [] {
                    return std::make_shared<FileAppender>();
                }
            }
            // add your appenders through registerCreateMethod
        };
    }

    inline Appender::Ptr AppenderFactory::create(const std::string& name)
    {
        // std::lock_guard<std::mutex> lock(_mutex);
        std::shared_lock lockShared(AppenderFactory::_mutex);
        if (_appenders.find(name) == _appenders.end()) {
            // 及时发现问题
            // throw std::runtime_error(__FUNCTION__);
            return nullptr;
        }
        // 延迟创建，按需加载。
        return _appenders[name]();
    }

    inline void AppenderFactory::registerCreateMethod(const std::string& name, AppenderFactory::CreateMethod method)
    {
        std::lock_guard<std::shared_mutex> lock(AppenderFactory::_mutex);
        _appenders[name] = method;
    }

    inline AppenderFactory& AppenderFactory::instance()
    {
        static std::once_flag _flag;
        static std::unique_ptr<AppenderFactory> _self;
        std::call_once(_flag,
            [&] {
            _self.reset(new AppenderFactory);
        });
        return *_self;
    }

    // appender Registry 仓库
    //
    // AppenderRegistry::~AppenderRegistry()
    //{
    //  // clear();
    //}

    inline void ray::log::AppenderRegistry::addAppenders(std::list<std::string> appenders)
    {
        std::unique_lock<std::shared_mutex> lock(AppenderRegistry::_mutex);
        for (const auto& name : appenders) {
            if (auto appender = AppenderFactory::instance().create(name)) {
                _appenders[name] = appender;
            }
        }
    }

    inline void AppenderRegistry::clear()
    {
        std::lock_guard<std::shared_mutex> lock(AppenderRegistry::_mutex);
        _appenders.clear();
    }

    inline const Appender::Ptr AppenderRegistry::get(const std::string& name)
    {
        // 这里提前创建好就不用加锁了
        // 延迟加载，多线程下不安全，加锁的话影响性能
        // if (_appenders.find(name) == _appenders.end()) {
        //  _appenders[name] = AppenderFactory::create(name);
        //}
        std::shared_lock<std::shared_mutex> lockShared(AppenderRegistry::_mutex);
        // call addAppenders first
        if (_appenders.find(name) == _appenders.end()) {
            return nullptr;
        }
        return _appenders[name];
    }

    // std::list<std::string> AppenderRegistry::keys()
    //{
    //  std::list<std::string> keys;
    //  for (const auto [key, _] : _appenders) {
    //    keys.push_back(key);
    //  }
    //  return keys;
    // }

    // =============================          appenders
    inline bool Appender::write(Event::Ptr event)
    {
        std::lock_guard lock(_mtxFlush);
        return flush(event);
    }

    inline Formatter::Ptr Appender::getFormatter(Event::Ptr event)
    {
        if (!event->formatter && !_formatter) {
            _formatter = std::make_shared<Formatter>();
        }
        return (event->formatter ? event->formatter : _formatter);
    }

    inline bool ConsoleAppender::flush(Event::Ptr event)
    {
        auto formatter = getFormatter(event);
#ifdef USE_QT
        // 判断日志级别，选择颜色
        static constexpr const char* RED = "\033[31m";
        static constexpr const char* GREEN = "\033[32m";
        static constexpr const char* YELLOW = "\033[33m";
        static constexpr const char* BLUE = "\033[34m";
        static constexpr const char* MAGENTA = "\033[35m";
        static constexpr const char* CYAN = "\033[36m";
        static constexpr const char* WHITE = "\033[37m";
        static constexpr const char* RESET = "\033[0m";
        const char* colorCode;
        switch (event->level) {
        case Level::Debug: colorCode = BLUE; break;
        case Level::Info: colorCode = GREEN; break;
        case Level::Warning: colorCode = YELLOW; break;
        case Level::Error: colorCode = RED; break;
        case Level::Fatal: colorCode = MAGENTA; break;
        default: // Level::Unknown 或其他未知级别
            colorCode = WHITE;
            break;
        }
        // 打印彩色日志
        qDebug() << colorCode << formatter->format(event).c_str() << RESET;
#else
        std::cout << formatter->format(event) << std::endl;
        std::cout.flush();
#endif
        return true;

        // file
    }

    inline FileAppender::FileAppender()
    {
#ifdef USE_QT
        _basePath = QApplication::applicationDirPath().toStdString() + "/";
#endif
        _basePath += "log";
        setLevel(Level::Info);
    }

    inline FileAppender::~FileAppender()
    {
        if (_file.is_open()) {
            _file.flush();
            _file.close();
        }
    }

    inline void FileAppender::setBasePath(const std::string& basePath)
    {
        _basePath = basePath;
        // std::filesystem::create_directories(_basePath);
    }

    inline void FileAppender::setPath(const std::string& path)
    {
        _path = path;
    }

    inline void FileAppender::setFileSplitPolicy(const FileSplitPolicy& cb)
    {
        _fileSplitPolicy = cb;
    }

    inline const size_t FileAppender::filesize()
    {
        // byte
        return _file.tellp();
    }

    inline std::string FileAppender::filename() const
    {
        return _filename;
    }

    inline std::string FileAppender::path() const
    {
        return _path;
    }

    inline std::string FileAppender::basePath() const
    {
        return _basePath;
    }

    inline bool FileAppender::resetFile(Event::Ptr event)
    {
        if (_fileSplitPolicy == nullptr) {
            _fileSplitPolicy = [](Event::Ptr evt, FileAppender& appender) {
                std::tm time;
                const auto sTime = evt->time / 1000;
                gmtime_s(&time, &sTime);
                int year = time.tm_year + 1900;
                int month = time.tm_mon + 1;
                int day = time.tm_mday;
                // 根据年月重新设置路径
                const std::string monthPath = std::format("{}/{}/{}/", appender.basePath().c_str(), year, month);
                if (monthPath != appender.path()) {
                    std::filesystem::create_directories(monthPath);
                    appender.setPath(monthPath);
                }
                // 年月日发生了变化就会产生一个新的文件名, 如果文件名发生了变化就重新创建文件
                std::string dayFilename = std::format("{}-{}-{}.log", year, month, day);
                if (dayFilename != appender.filename()
                    // 这种实现不太好，如果软件崩溃了，就会产生新的日志文件。
                    && dayFilename.compare(0, dayFilename.length() - 4, appender.filename().substr(0, dayFilename.length() - 4)) != 0) {
                    return dayFilename;
                }
                // 如果文件名没有发生变化，但是如果文件大小超出阈值
                double filesize = appender.filesize() / 1024.0 / 1024.0;
                if (filesize > 10.0 MB) {
                    //
                    return std::format("{}-{}-{}_{}_{}_{}.log",
                        year,
                        month,
                        day,
                        //
                        time.tm_hour,
                        time.tm_min,
                        time.tm_sec);
                }
                // 使用原始文件名，就不进行分割了
                return appender.filename();
            };
        }
        std::string newFilename = _fileSplitPolicy(event, *this);
        // 如果文件名发生了变化就重新创建文件
        if (_filename != newFilename || !_file.is_open()) {
            if (_file.is_open()) {
                _file.flush();
                _file.close();
            }
            //
            _file.open(_path + newFilename, std::fstream::out | std::fstream::app | std::fstream::ate);
            _file.seekp(0);
            if (_file.fail()) {
                return false;
            }
            _filename = newFilename;
        }
        return true;
    }

    inline bool FileAppender::flush(const Event::Ptr event)
    {
        if (!resetFile(event)) {
            return false;
        }
        auto formatter = getFormatter(event);
        _file << formatter->format(event) << "\n";
        _file.flush();
        return true;
    }

    template <class T>
    inline Logger& Logger::operator<<(const T& s)
    {
#ifdef USE_QT
        // 判断是不是QString,
        if constexpr (std::is_same_v<std::decay_t<T>, QString>) {
            _logEvent->content << s.toStdString();
        }
        else {
#endif
            _logEvent->content << s;
#ifdef USE_QT
        }
#endif
        // return _logEvent->content;
        return *this;
    }

    inline Logger& Logger::set_level(Level level)
    {
        _logEvent->level = level;
        return *this;
    }

    inline Logger& Logger::set_code(int code)
    {
        _logEvent->code = code;
        return *this;
    }

    inline Logger& Logger::set_appenders(std::list<std::string> appenders)
    {
        _appenders.clear();
        _appenders = std::move(appenders);
        return *this;
    }

    inline Logger& Logger::set_line(uint32_t line)
    {
        _logEvent->line = line;
        return *this;
    }

    inline Logger& Logger::set_file(std::string file)
    {
        _logEvent->file = Utils::getFilename(file);
        return *this;
    }

    inline Logger& Logger::set_key(const std::string& key)
    {
        _logEvent->key = key;
        return *this;
    }

    inline Logger& ray::log::Logger::set_formatter(Formatter::Ptr formatter)
    {
        _logEvent->formatter = std::move(formatter);
        return *this;
    }

    // 记录日志
    template <typename T, typename... Args>
    void ray::log::Logger::log(T format, Args... args)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            _logEvent->content << Utils::string_format(format.c_str(), std::forward<Args>(args)...);
        }
#ifdef USE_QT
        else if constexpr (std::is_same_v<std::decay_t<T>, QString>) {
            _logEvent->content << Utils::string_format(format.toStdString().c_str(), std::forward<Args>(args)...);
        }
#endif
        else {
            _logEvent->content << Utils::string_format(format, std::forward<Args>(args)...);
        }
    }

    template <typename T, typename... Args>
    void Logger::debug(T format, Args... args)
    {
        _logEvent->level = Level::Debug;
        log<T>(std::forward<T>(format), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    void Logger::info(T format, Args... args)
    {
        _logEvent->level = Level::Info;
        log<T>(std::forward<T>(format), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    void Logger::warning(T format, Args... args)
    {
        _logEvent->level = Level::Warning;
        log<T>(std::forward<T>(format), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    void Logger::error(T format, Args... args)
    {
        _logEvent->level = Level::Error;
        log<T>(std::forward<T>(format), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    void Logger::fatal(T format, Args... args)
    {
        _logEvent->level = Level::Fatal;
        log<T>(std::forward<T>(format), std::forward<Args>(args)...);
    }

    template <typename T>
    std::string Logger::format(T message, Formatter::Ptr formatter)
    {
        log<T>(std::forward<T>(message));
        if (formatter) {
            return formatter->format(_logEvent);
        }
        return Formatter().format(_logEvent);
    }

    // 时间统计
    std::chrono::time_point<std::chrono::high_resolution_clock> Logger::time()
    {
        return std::chrono::high_resolution_clock::now();
    }

    template <typename Tm>
    uint32_t Logger::timeEnd(std::chrono::time_point<std::chrono::high_resolution_clock> start, std::string label)
    {
        using namespace std::chrono;
        uint32_t duration = static_cast<uint32_t>(duration_cast<Tm>(time() - start).count());
        std::cout << label << duration << std::endl;
        // log("%s: %d", label.c_str(), duration);
        return duration;
    }

    template <typename... Args>
    std::string Utils::string_format(const char* format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format, args...) + 1; // Extra space for '\0'
        if (size_s <= 0) {
            return std::string("");
        }
        auto size = static_cast<size_t>(size_s);
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format, args...);
        // result
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    // =======================  便捷方法 ========================
    static Logger console(Level level = Level::Debug, const std::source_location& location = std::source_location::current())
    {
        return Logger(location.file_name(), location.line(), level, { "console" });
    }

    static Logger file(Level level = Level::Info, const std::source_location& location = std::source_location::current())
    {
        return Logger(location.file_name(), location.line(), level, { "file" });
    }

    static Logger log(Level level = Level::Info, const std::source_location& location = std::source_location::current())
    {
        return Logger(location.file_name(), location.line(), level);
    }
} // namespace ray::log
#endif // !__RAY_QLOG_HPP__
