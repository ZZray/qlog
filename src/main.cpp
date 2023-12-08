#include <iostream>
#include "mylog.h"
#include <iostream>
#include <thread>
#include <future>
using namespace ray;

// 输出自定义类型
class A
{
private:
    double num      = 2022.0606;
    QString str = "object A";

public:
    friend void operator<<(log::LogStream& s, A a)
    {
        // s << "a.num: " << a.num << " a.str: " << a.str;
        s << QString("[a.num: %1 a.str: %2]").arg(a.num, 9, 'f', 4).arg(a.str);
    }
};

int main(int argc, char* argv)
{
    // 初始化工作
    log::AppenderRegistry::instance().addAppenders({"console", "file"});
    if (const auto& appender = log::AppenderRegistry::instance().get("file")) {
        const auto fileAppender = std::dynamic_pointer_cast<log::FileAppender>(appender);
        // 设置日志路径
        fileAppender->setBasePath("log");
        // 设置输出到文件的日志等级
        fileAppender->setLevel(log::Level::Info);
        // 设置文件分割策略回调, 内置分割策略在resetFile函数里面调用。
        // fileAppender->setFileSplitPolicy([](LogEvent::Ptr, FileAppender&) {return "log_file_name";});
        //
    }
    if (const auto& appender = log::AppenderRegistry::instance().get("console")) {
        appender->setLevel(log::Level::Debug);
    }
    log::log(log::Level::Debug) << "invisible message in file";
    log::log(log::Level::Error) << "visible message in file";

    log::Logger(__FILE__, __LINE__, log::Level::Debug).log(std::format("{}, {}, {:.2f}, {}\n", 1, 2, 4.1, "Debug All"));
    // 没必要的换行
    log::Logger(__FILE__, __LINE__) << "Info All";

    log::Logger(__FILE__, __LINE__,log:: Level::Info).set_appenders({"console"}).log("console info");

    log::console(log::Level::Error) << "console error msg";
    log::console().log("console debug : hello %s %.2f", "ray::log", 6.66666);
    log::console() << "console debug message";
    log::console().set_level(log::Level::Info).log("%d", 6666);

    const A a;
     log::log(log::Level::Info) << "log class A: " << a;

    // 多线程测试 console
    // console << "hello";
    const auto startTm = log::console().time();
     log::console() << "multi thread test:";
    std::vector<std::future<void>> threadPool;
    // console log 1w
    for (int i = 0; i < 10000; ++i) {
        threadPool.emplace_back(std::async(std::launch::async | std::launch::deferred, [i] {
            // console << "index << " << i;
            // console.log("index log %d", i);
            // filelogger.log("index log %d", i);
            // logger.log("index log %d", i);
            log::file().error("index log %d", i);
        }));
    }
    for (auto& t : threadPool) {
        t.wait();
    }
    log::console().timeEnd(startTm, "duration(ms): ");
    system("pause");
    return 0;
}
