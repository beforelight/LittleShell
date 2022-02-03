#include "LittleShell.hpp"
#include <cstdarg>
#ifdef CLI_DEBUG //调试输出
#include<iostream>
#endif // CLI_DEBUG
namespace LittleShell {
    FIFO::FIFO(uint32_t _size) {
        size = 1U;
        while (size < _size) {
            size <<= 1;
        }
        size_mask = size - 1;
        buffer = new char[size];
        ref_counter = new int(1);
    }
    FIFO::~FIFO() {
        *ref_counter--;
        if (*ref_counter == 0) {
            delete[] buffer;
            delete ref_counter;
        }
    }
    FIFO::FIFO(const FIFO &fifo) {
        memcpy(this, &fifo, sizeof(FIFO));
        *ref_counter++;
    }
    uint32_t FIFO::Put(char *src, uint32_t len) {
        len = std::min(len, size - in + out);//满了之后就不会再接收了
        uint32_t l = std::min(len, size - in);
        memcpy(buffer + in, src, l * sizeof(char));
        memcpy(buffer, src + l, (len - l) * sizeof(char));
        in = (in + len) & size_mask;
        return len;
    }
    uint32_t FIFO::Get(char *dst, uint32_t len) {
        len = std::min(len, in - out);
        uint32_t l = std::min(len, size - out);
        memcpy(dst, buffer + out, l * sizeof(char));
        memcpy(dst + l, buffer, (len - l) * sizeof(char));
        out = (out + len) & size_mask;
        return len;
    }
    int ostream::printf(const char *formatString, ...) {
        va_list arg;
        int logLength;
                va_start(arg, formatString);
        logLength = vsnprintf(printBuf, sizeof(printBuf), formatString, arg);
                va_end(arg);
        (*write)(fp, printBuf, logLength);
        return logLength;
    }
    int ostream::putchar(int ch) {
        (*write)(fp, (char *) &ch, 1);
        return ch;
    }




//字符 !=27
//字符 27		ESC//不处理
//字符 13		\n回车//不处理
//字符 8		退格//不处理
//字符 27 91 65 ↑
//字符 27 91 66 ↓
//字符 27 91 68 ←
//字符 27 91 67 →
//字符 27 91 51 126 delete
#define key1(a, b, c) (uint32_t)((a)|(b<<8)|(c<<16))//一类按键
#define key2(a, b, c, d) (uint32_t)(key1(a,b,c)|(d<<24))//二类按键
#define up key1(27,91,65)
#define down key1(27,91,66)
#define left key1(27,91,68)
#define right key1(27,91,67)
#define del key2(27,91,51,126)
#define enter (13)
#define backspace (8)
    void CLI::Process(void) {
        if (cmd_thread_flag.load() == 2) {
            //等待释放资源
            cmd_thread.join();
            cmd_thread_flag.store(0);
            //执行完命令之后开始准备
            pos = 0;
            len = 0;
            state = 0;
            wwrite("\r\n", 2);//发送换行
            ShowPrompt();
            Clear();//清空在执行命令期间接收的字符
        }
        while (UsedSize()) {
            Get((char *) &ch, 1);
            //先进行按键识别
            switch (state) {
                case 0:
                    key = ch;
                    if (ch == 27) { //多byte字符
                        state = 1;//下一个状态
                    } else {
                        key_size = 1;
                    }
                    break;
                case 1:
                    key = key | ch << 8;
                    if (ch == 91) {
                        state = 2;//多字节字符
                    } else {
                        state = 0;
                        key_size = 2;
                    }
                    break;
                case 2:
                    key = key | ch << 16;
                    if (ch == 51) {
                        state = 3;//多字节字符
                    } else {
                        state = 0;
                        key_size = 3;
                    }
                    break;
                case 3:
                    state = 0;
                    key = key | ch << 24;
                    key_size = 4;
                    break;
            }

            if (cmd_thread_flag.load() == 1) {
                switch (key) {
                    case 3://ctrl+c
#ifdef _M_X64
                        TerminateThread(cmd_thread_handle, 0);
                        //TODO 这里要使用os提供的api结束线程
#endif
                        cmd_thread.detach();
                        cmd_thread_flag.store(0);
                        //执行完命令之后开始准备
                        pos = 0;
                        len = 0;
                        state = 0;
                        wwrite("\r\n", 2);//发送换行
                        ShowPrompt();
                        Clear();//清空在执行命令期间接收的字符
                        break;
                    default://其他按键则传输到cmd中的ios
                        cmd_ios.Put((char *) &key, key_size);
                        break;
                }
                return;
            }
            //之后再处理按键
            if (state == 0) {
                switch (key_size) {
                    case 1:
                        ProcessKeySize1();
                        break;
                    case 2:
                        ProcessKeySize2();
                        break;
                    case 3:
                        ProcessKeySize3();
                        break;
                    case 4:
                        ProcessKeySize4();
                        break;
                }
            }
        }//while (UsedSize())
    }
    void CLI::ProcessKeySize1(void) {
        switch (key) {
            case 0:
                break;
            case 9://tab制表符自动补全
                str.push_back(0);
                tabstring = std::string(&str[0]);
                str.pop_back();
                tablen = tabstring.length();
                for (auto &i: cmd_map) {
                    //找到匹配的命令
                    if (0 == tabstring.compare(0, tablen, i.first, 0, tablen)) {
                        std::string a = (i.first.substr(tablen, i.first.length() - tablen));
                        printf(a.c_str());
                        for (const char *j = a.c_str(); *j != 0; j++) {
                            str.push_back(*j);
                            pos++;
                            len++;
                        }
                    }
                }
                break;
            case enter://换行
                wwrite("\r\n", 2);//发送换行
                if (str.empty()) {
                    ShowPrompt();
                    break;
                }//如果命令为空直接跳过
                str.push_back(0);//插入结束符号
#ifdef CLI_DEBUG //调试输出
                for (auto dbi: history) {
                    std::cout << "历史命令:" << &dbi[0] << std::endl;
                }
                std::cout << "当前行:" << &str[0] << std::endl;
#endif // CLI_DEBUG
                if (history.empty() || strcmp(&str[0], &history.back()[0]) != 0) {
                    //如果命令历史为空或者当前命令和历史最后一条不同的话插入
                    history.push_back(str);//将本命令插入到历史记录
                }
                is_history_mode = 0;
                if (history.size() > history_size_limit) { history.pop_front(); }//弹出多出来的历史记录
                cmd_argv.clear();//清空以开始构造参数列表
                find_cmd_arg = 0;
                for (char &i: str) {
                    if (i == ' ') {
                        i = 0;
                        find_cmd_arg = 1;
                    }//将空格替换为结束符号
                    else if (find_cmd_arg == 1) {
                        find_cmd_arg = 0;
                        cmd_argv.push_back(&i);
                    }
                }
                cmd_now = std::string(&str[0]);//构造命令
                map_ite = cmd_map.find(cmd_now);
                if (map_ite != cmd_map.end()) {
                    cmd_thread_flag.store(1);
                    cmd_thread = std::thread([this]() {
                        cmd_ios.Clear();
                        int retval = map_ite->second.func(cmd_ios, cmd_argv);//执行命令
                        if (retval != 0) {
                            SaveColor(ConsoleColorSet::ConsoleFont);
                            SetFontColor(ConsoleColor::light_RED);
                            printf("\r\nreturn code %d", retval);
                            RestoreColor(ConsoleColorSet::ConsoleFont);
                        }
                        cmd_thread_flag.store(2);
                    });
                    cmd_thread_handle = cmd_thread.native_handle();

                } else {
                    printf("%s: command not found", cmd_now.c_str());
                }
                str.clear();
                break;
            case backspace://退格
                if (len) {
                    putchar(ch);//光标左移一位
                    len--;
                    pos--;
                    SaveCursorPosition();
                    if (len == pos) {
                        putchar(' ');//刚刚的用空格填充
                        str.pop_back();
                    } else {
                        auto position = str.begin() + pos;
                        str.erase(position);
                        if (len) { wwrite(&str[pos], len - pos); }
                        putchar(' ');
                    }
                    RestoreCursorPosition();
                }
                break;
            default://普通字符回显处理
                if (len == pos)//正常回显状态
                {
                    str.push_back((char) ch);
                    putchar(ch);
                    len++;
                    pos++;
                } else//插入状态
                {
                    auto position = str.begin() + pos;
                    str.insert(position, (char) ch);
                    putchar(ch);
                    len++;
                    pos++;
                    SaveCursorPosition();
                    if (len) {
                        wwrite(&str[pos], len - pos);
                    }
                    RestoreCursorPosition();
                }
#ifdef CLI_DEBUG
                for (int i = 0; i < key_size; ++i) {
                    std::cout << (int) ((char *) &key)[i] << ' ';
                }
                std::cout << std::endl;
#endif
                break;
        }
    }
    void CLI::ProcessKeySize2(void) {
#ifdef CLI_DEBUG
        for (int i = 0; i < key_size; ++i) {
            std::cout << (int) ((char *) &key)[i] << ' ';
        }
        std::cout << std::endl;
#endif
    }
    void CLI::ProcessKeySize3(void) {
        switch (key) {
            case up://上
                if (is_history_mode == 0) {
                    if (history.size()) {
                        histIte = history.end();
                        is_history_mode = 1;
                    }
                }
                if (is_history_mode == 1) {
                    if (histIte != history.begin()) {
                        histIte--;
                        str = *histIte;
                        //str.pop_back();//剔除最后的那个0结束标识符，这里会引起bug
                        putchar('\r');
                        ClearLine();
                        str.pop_back();
                        len = str.size();
                        pos = len;
                        ShowPrompt();
                        if (len) {
                            wwrite(&str[0], len);
                        }
                    }
                }
                break;
            case down://下
                if (is_history_mode) {
                    auto history_ite_m1 = history.end();
                    history_ite_m1--;
                    if (histIte != history_ite_m1) {
                        histIte++;
                        str = *histIte;
                        putchar('\r');
                        ClearLine();
                        str.pop_back();
                        len = str.size();
                        pos = len;
                        ShowPrompt();
                        if (len) {
                            wwrite(&str[0], len);
                        }
                    } else {
                        is_history_mode = 0;
                        str.clear();
                        len = 0;
                        pos = 0;
                        putchar('\r');
                        ClearLine();
                        ShowPrompt();
                    }
                }
                break;
            case right://右
                if (len > pos) {
                    pos++;
                    wwrite((char *) &key, 3);
                }
                break;
            case left://左
                if (pos) {
                    pos--;
                    wwrite((char *) &key, 3);
                }
                break;
            default://不是处理的按键，退状态,2byte及以上的按键不回显
#ifdef CLI_DEBUG
                for (int i = 0; i < key_size; ++i) {
                    std::cout << (int) ((char *) &key)[i] << ' ';
                }
                std::cout << std::endl;
#endif
                break;
        }
    }
    void CLI::ProcessKeySize4(void) {
        switch (key) {
            case del://delete，delete不用回显
                if (pos < len) {
                    SaveCursorPosition();
                    auto position = str.begin() + pos;
                    str.erase(position);
                    len--;
                    if (len - pos > 0) {
                        wwrite(&str[pos], len - pos);
                    }
                    putchar(' ');
                    RestoreCursorPosition();
                }
                break;
            default:
#ifdef CLI_DEBUG
                for (int i = 0; i < key_size; ++i) {
                    std::cout << (int) ((char *) &key)[i] << ' ';
                }
                std::cout << std::endl;
#endif
                break;
        }
    }

    void CLI::SetOstream(ostream &_ostream) {
        ostream *a = static_cast<ostream *>(this);
        *a = _ostream;
    }

    void CLI::SetIstream(istream &_istream) {
        istream *a = static_cast<istream *>(this);
        *a = _istream;
    }
    void CLI::ShowPrompt(void) {
        SaveColor(ConsoleColorSet::ConsoleFont);
        SetFontColor(ConsoleColor::GREEN);
        wwrite((char *) promptString.c_str(), promptString.size());
        RestoreColor(ConsoleColorSet::ConsoleFont);
    }

    int CLI::InsertCMD(const CLI::CMD &_cli_cmd) {
        if (cmd_map.count(_cli_cmd.name)) {
            return -1;//已经存在有了
        } else {
            cmd_map[_cli_cmd.name] = _cli_cmd;
            return 0;
        }
    }
    int CLI::Help(ios &ios, const std::vector<const char *> &argv) {
        if (argv.size() == 1) {
            std::string arg(argv[0]);
            auto len = arg.length();
            for (auto &i: cmd_map) {
                if (0 == arg.compare(0, len, i.first, 0, len)) {
                    ios.printf("%s\t\t%s\r\n", i.first.c_str(), i.second.helpInfo.c_str());
                }
            }
        } else {
            for (auto &i: cmd_map) {
                ios.printf("%s\t\t%s\r\n", i.first.c_str(), i.second.helpInfo.c_str());
            }
        }
        return 0;
    }
    int CLI::Clr(ios &ios, const std::vector<const char *> &argv) {
        ios.printf("\033[H");
        ios.printf("\033[2J");
        return 0;
    }
    std::map<std::string, CLI::CMD> CLI::cmd_map = {
            {"help", CLI::CMD("help", "Print the help information", CLI::Help)},
            {"clr",  CLI::CMD("clr", "Clear the console", CLI::Clr)}
    };


    void CLI::SetConsoleColor(ConsoleColor _color, ConsoleColorSet fontORbackgrand) {
        color[(int) fontORbackgrand] = _color;
        printf("\033[%d;%dm", (int) color[(int) fontORbackgrand] % 100,
               (int) color[(int) fontORbackgrand] / 100 + (int) fontORbackgrand * 10);
    }

    void CLI::SetFontColor(ConsoleColor _color) {
        SetConsoleColor(_color, ConsoleColorSet::ConsoleFont);
    }

    void CLI::SetBackgrandColor(ConsoleColor _color) {
        SetConsoleColor(_color, ConsoleColorSet::ConsoleBackgrand);
    }

    void CLI::Blinking(void) {
        printf("\033[5m");
    }

    void CLI::SaveColor(ConsoleColorSet fontORbackgrand) {
        colorStack[(int) fontORbackgrand].push(color[(int) fontORbackgrand]);
    }

    void CLI::RestoreColor(ConsoleColorSet fontORbackgrand) {
        color[(int) fontORbackgrand] = colorStack[(int) fontORbackgrand].top();
        colorStack[(int) fontORbackgrand].pop();
        printf("\033[0;%dm", (int) color[(int) fontORbackgrand] + (int) fontORbackgrand * 10);
    }

    void CLI::SaveCursorPosition(void) {
        printf("\033[s");
    }

    void CLI::RestoreCursorPosition(void) {
        printf("\033[u");
    }

    void CLI::ClearLine(void) {
        printf("\033[K");
    }

    void CLI::MoveCursorForward(int n_characters) {
        printf("\033[%dC", n_characters);
    }

    void CLI::MoveCursorBackward(int n_characters) {
        printf("\033[%dD", n_characters);
    }

    void CLI::ResetCursor(void) {
        printf("\033[H");
    }

    void CLI::DisplayClear(void) {
        printf("\033[2J");
    }
}
