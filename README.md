# C++日志库

如果使用Qt项目，为了避免QString与std::string转换的问题，推荐使用主分支。

## 特点

简单，方便，线程安全。

## 示例

```C++
    log::console(log::Level::Error) << "console error msg";
    log::console().log("console debug : hello %s %.2f", "ray::log", 6.66666);
    log::console() << "console debug message";
    log::console().set_level(log::Level::Info).log("%d", 6666);

    log::file().error("index log %d", 1);
```

## 输出自定义类

```C++
// 输出自定义类型
class A
{
private:
    double num      = 2022.0606;
    QString str = "object A";

public:
    friend void operator<<(log::Stream& s, A a)
    {
        // s << "a.num: " << a.num << " a.str: " << a.str;
        s << QString("[a.num: %1 a.str: %2]").arg(a.num, 9, 'f', 4).arg(a.str);
    }
};
```

## 自定义格式

```C++
    if (const auto& appender = log::AppenderRegistry::instance().get("console")) {
        class MyFormatter : public ray::log::Formatter
        { 
        public:
            QString format(const ray::log::Event::Ptr& logEvent) const override
            {
                return  logEvent->content.readAll();
            }
        };
        appender->setFormatter(std::make_shared<MyFormatter>());
        appender->setLevel(log::Level::Debug);
    }
```

