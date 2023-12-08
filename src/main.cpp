#include <iostream>
#include "mylog.h"
#include <iostream>
#include <thread>
#include <future>
using namespace ray::log;

// 输出自定义类型
class A
{
private:
    double num      = 2022.0606;
    std::string str = "str";

public:
    friend void operator<<(LogStream& s, A a)
    {
        // s << "a.num: " << a.num << " a.str: " << a.str;
        s << ray::log::LogUtils::string_format("[a.num: %.4f a.str: %s]", a.num, a.str.c_str());
    }
};

int main(int argc, char* argv)
{
    // 初始化工作
    ray::log::AppenderRegistry::instance().addAppenders({"console", "file"});
    if (const auto& appender = AppenderRegistry::instance().get("file")) {
        const auto fileAppender = std::dynamic_pointer_cast<FileAppender>(appender);
        // 设置日志路径
        fileAppender->setBasePath("log");
        // 设置输出到文件的日志等级
        fileAppender->setLevel(LogLevel::Info);
        // 设置文件分割策略回调, 内置分割策略在resetFile函数里面调用。
        // fileAppender->setFileSplitPolicy([](LogEvent::Ptr, FileAppender&) {return "log_file_name";});
        //
    }
    if (const auto& appender = AppenderRegistry::instance().get("console")) {
        appender->setLevel(LogLevel::Debug);
    }

    glogger(LogLevel::Debug) << "invisible message";
    glogger(LogLevel::Error) << "visible message";

    Logger(__FILE__, __LINE__, LogLevel::Debug).log("%d, %d, %.2f, %s\n", 1, 2, 4.1, "Debug All");
    // 没必要的换行
    Logger(__FILE__, __LINE__) << "Info All";

    Logger(__FILE__, __LINE__, LogLevel::Info).set_appenders({"console"}).log("console info");

    console(LogLevel::Error) << "console error msg";
    console.log("console debug : hello %s %.2f", "ray::log", 6.66666);
    console << "console debug message";
    console.set_level(LogLevel::Info).log("%d", 6666);

    const A a;
    glogger(LogLevel::Info) << "log class A: " << a;

    // 多线程测试 console
    // console << "hello";
    const auto startTm = console.time();
    console << "multi thread test:";
    std::vector<std::future<void>> threadPool;
    // console log 1w
    for (int i = 0; i < 10000; ++i) {
        threadPool.emplace_back(std::async(std::launch::async | std::launch::deferred, [i] {
            // console << "index << " << i;
            // console.log("index log %d", i);
            // filelogger.log("index log %d", i);
            // logger.log("index log %d", i);
            flogger.error("index log %d", i);
        }));
    }
    for (auto& t : threadPool) {
        t.wait();
    }
    console.timeEnd(startTm, "duration(ms): ");
    system("pause");
    return 0;
}
