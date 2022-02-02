#ifndef LITTLESHELL_LITTLESHELL_HPP
#define LITTLESHELL_LITTLESHELL_HPP
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <map>
#include <list>
#include <stack>
namespace LittleShell {
    class FIFO;
    class ostream;
    using istream = FIFO;
    class CLI;

    class FIFO {
    public:
        //构造
        explicit FIFO(uint32_t _size = 1024);
        //析构
        ~FIFO();
        //拷贝构造
        FIFO(const FIFO &fifo);
        //写入buff，返回实际读写大小
        uint32_t Put(char *src, uint32_t len);
        //写入buff，返回实际读写大小
        inline uint32_t Put(unsigned char *src, uint32_t len) { return Put((char *) src, len); }
        //读出buff，返回实际读写大小
        uint32_t Get(char *dst, uint32_t len);
        //读出buff，返回实际读写大小
        inline uint32_t Get(unsigned char *dst, uint32_t len) { return Get((char *) dst, len); }
        //环形buff大小
        uint32_t Size(void) { return size; }
        //已使用大小
        uint32_t UsedSize(void) { return ((size + in - out) & size_mask); }
        //未使用大小
        uint32_t UnusedSize(void) { return ((size + out - in) & size_mask); }
        //清空buff
        void Clear(void) { out = in; }
    private:
        uint32_t size = 0;
        uint32_t size_mask = 0;
        volatile uint32_t in = 0;//addr of buffer to write in
        volatile uint32_t out = 0;//addr of buffer to read out
        char *buffer = nullptr;
        int *ref_counter = nullptr;
    };

    class ostream {
    public:
        typedef int (*Write)(void *fp, const char *ptr, int len);
        explicit ostream(Write _write, void *_fp = nullptr) : write(_write), fp(_fp) {}
        int wwrite(const char *ptr, int len) { return write(fp, ptr, len); }
        int printf(const char *formatString, ...);
        int putchar(int ch);
    private:
        Write write = nullptr;
        void *fp = nullptr;
        char printBuf[256] = {0};
    };

    class CLI : public ostream, public istream {
    public:
        struct CMD {
            CMD() = default;
            CMD(std::string _name,
                std::string _helpInfo,
                std::function<int(CLI &, const std::vector<const char *> &)> _func) :
                    name(std::move(_name)), helpInfo(std::move(_helpInfo)), func(std::move(_func)) {}
            std::function<int(CLI &, const std::vector<const char *> &)> func;
            std::string name;
            std::string helpInfo;
        };
        CLI(ostream _ostream, istream _istream, std::string _promptString = "shell>> ") :
                ostream(_ostream), istream(std::move(_istream)), shell_head(std::move(_promptString)) {}

        void Process(void);//命令行主函数
        void SetOstream(ostream &_ostream);//设置输出流
        void SetIstream(istream &_istream);//设置输入流
        void ShowHead(void);
        void SetHead(std::string _head) { shell_head = _head; }
        static int InsertCMD(CMD _cli_cmd);//注册命令，注册的命令会被所有实例访问
        static int Help(CLI &cli, const std::vector<const char *> &argv);//内置命令，打帮助信息
        static int Clr(CLI &cli, const std::vector<const char *> &argv);//内置命令，清空控制台
    private:
        std::string shell_head;
        static std::map<std::string, LittleShell::CLI::CMD> cmd_map;
    private:
        uint8_t ch = 0;//字符
        uint8_t state = 0;//字符状态机，用于处理多字节按键
        uint8_t key_size = 0;//单字符大小
        uint32_t key = 0;//字符缓存
        std::vector<char> str;//保存本行的字符
        uint32_t len = 0;//本行字符串长度
        uint32_t pos = 0;//本行字符串光标位置
        std::list<std::vector<char> > history;//命令历史，未被执行的命令不会进入到历史
        std::list<std::vector<char> >::iterator histIte;//命令历史iterator
        uint32_t history_size_limit = 100;//命令历史记录个数限制
        //uint32_t last_cmd_cursor_pos;//未执行命令的输入光标位置//此功能删除，因容易造成bug
        char is_history_mode = 0;//命令历史查询状态，当历史命令改变之后清0，查询开始置为1
        std::vector<const char *> cmd_argv;//构造参数列表
        std::string cmd_now;//构造Cmd
        char find_cmd_arg = 0;//构造参数列表时的flag
        std::map<std::string, CMD>::iterator map_ite;
        std::string tabstring;//tab键补全用
        int tablen = 0;//tab键补全用
        void ProcessKeySize1(void);//处理固定大小的字符
        void ProcessKeySize2(void);//处理固定大小的字符
        void ProcessKeySize3(void);//处理固定大小的字符
        void ProcessKeySize4(void);//处理固定大小的字符
    public://控制台指令
        enum class ConsoleColor : int {
            light_BLACK = 30,
            light_RED = 31,
            light_GREEN = 32,
            light_YELLOW = 33,
            light_BLUE = 34,
            light_PURPLE = 35,
            light_CYAN = 36,
            light_WHITE = 37,
            BLACK = 130,
            RED = 131,
            GREEN = 132,
            YELLOW = 133,
            BLUE = 134,
            PURPLE = 135,
            CYAN = 136,
            WHITE = 137,
        };
        enum class ConsoleColorSet : int {
            ConsoleFont = 0,//设置字体颜色
            ConsoleBackgrand = 1,//设置背景颜色
        };
        void SetConsoleColor(ConsoleColor _color, ConsoleColorSet fontORbackgrand);//设置控制台颜色
        void SetFontColor(ConsoleColor _color);//设置字体颜色
        void SetBackgrandColor(ConsoleColor _color);//设置背景颜色
        void SaveColor(ConsoleColorSet fontORbackgrand);//保存当前控制台颜色到堆
        void RestoreColor(ConsoleColorSet fontORbackgrand);//从堆恢复控制台颜色
        void SaveCursorPosition(void);//保存当前光标位置到堆
        void RestoreCursorPosition(void);//从堆恢复光标位置
        void ClearLine(void);//清除本行从光标开始的字符
        void MoveCursorForward(int n_characters);//光标右移
        void MoveCursorBackward(int n_characters);//光标左移
        void ResetCursor(void);
        void DisplayClear(void);//清屏
        void Blinking(void);//设置本行字符在控制台闪烁
    private://控制台指令
        std::stack<ConsoleColor> colorStack[2];
        ConsoleColor color[2];
    };

};


#endif //LITTLESHELL_LITTLESHELL_HPP
