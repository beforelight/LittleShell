//通过串口实现的命令行
//仅支持 Xshell,SecureCRT等上位机
//

#ifndef SHELL_CLI_H
#define SHELL_CLI_H

#include <sstream>
#include <map>
#include <vector>
#include "fifo.h"
#include <stack>
#include <list>
namespace shell {
	class ostream;
	class istream;
	class CLI;
	typedef int (*CmdFuntion)(CLI& cli, const std::vector<const char*>& argv);
	typedef int (*Write)(void* fp, char* ptr, int len);
	class ostream {
	public:
		ostream(Write _write, void* _fp = NULL) :write(_write), fp(_fp) {
			printBuf[255] = 0;
		}
		int printf(const char* formatString, ...);
		int putchar(int ch);
		int wwrite(char* ptr, int len);
	private:
		char printBuf[256];
		Write write;
		void* fp;
	};

	class istream :public FIFO<char> {
	public:
		istream(uint32_t _size = 1024) :FIFO<char>(_size) {}
		int scanf(char* formatString, ...);
		int getchar(void);
		int kbhit(void) { return UsedSize(); }
	private:
	};

	class CLI :public ostream, public istream {
	public:
		struct CLI_CMD {
			CLI_CMD() {}
			CLI_CMD(std::string _cmd, CmdFuntion _pFun, std::string _helpInfo = "什么都没有写") :
				cmd(_cmd), pFun(_pFun), helpInfo(_helpInfo) {
				InsertCMD(*this);
			}
			std::string cmd;
			CmdFuntion pFun;
			std::string helpInfo;
		};
		CLI(ostream _ostream, istream _istream, std::string _shell_head = "shell>> ") :
			ostream(_ostream), istream(_istream), shell_head(_shell_head)
		{
			//注册默认命令
			InsertCMD(CLI_CMD("help", CLI::Help, "Print the help information"));
			InsertCMD(CLI_CMD("clr", CLI::Clr, "Clear the console"));

			//控制台指令变量
			color[(int)ConsoleColorSet::ConsoleFont] = ConsoleColor::light_WHITE;
			color[(int)ConsoleColorSet::ConsoleBackgrand] = ConsoleColor::BLACK;

			//命令行处理变量
			ch = 0;
			key = 0;
			state = 0;
			len = 0;
			pos = 0;
			history.clear();
			last_cmd_cursor_pos = 0;
			is_history_mode = 0;
			cmd_argv.clear();
			find_cmd_arg = 0;
		}
		CLI(Write _write_ostream, void* _fp_ostream = NULL, uint32_t _size_istream = 1024, std::string _shell_head = "shell>> ") :
			CLI(ostream(_write_ostream, _fp_ostream), istream(_size_istream), _shell_head) {
		}
		void Process(void);//命令行主函数
		void SetOstream(ostream& _ostream);//设置输出流
		void SetIstream(istream& _istream);//设置输入流
		void ShowHead(void);
		void ShowTitle(void);
		void SetHead(std::string _head) { shell_head = _head; }
		static int InsertCMD(CLI_CMD _cli_cmd);//注册命令，注册的命令会被所有实例访问
		static int Help(CLI& cli, const std::vector<const char*>& argv);//内置命令，打帮助信息
		static int Clr(CLI& cli, const std::vector<const char*>& argv);//内置命令，清空控制台
	private:
		std::string shell_head;
		static std::map<std::string, CLI_CMD> cmd_map;//注册的命令会被所有的实例访问
	private:
		uint8_t ch;//字符
		uint32_t key;//字符缓存
		uint8_t state;//字符状态机，用于处理多字节按键
		std::vector<char> str;//保存本行的字符
		uint32_t len;//本行字符串长度
		uint32_t pos;//本行字符串光标位置
		std::list<std::vector<char> > history;//命令历史
		std::list<std::vector<char> >::iterator histIte;//命令历史iterator
		uint32_t last_cmd_cursor_pos;//未执行命令的输入光标位置
		char is_history_mode;//命令历史查询状态，当历史命令改变之后清0，查询开始置为1
		std::vector<const char*>cmd_argv;//构造参数列表
		std::string cmd_now;//构造Cmd
		char find_cmd_arg;//构造参数列表时的flag
		std::map< std::string, CLI_CMD>::iterator map_ite;
		std::string tabstring;//tab键补全用
		int tablen;
	public://控制台指令
		enum class ConsoleColor :int
		{
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
		enum class ConsoleColorSet :int {
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
	private://控制台指令
		std::stack<ConsoleColor> colorStack[2];
		ConsoleColor color[2];
		void Blinking(void);//设置本行字符在控制台闪烁

	};
}
#endif //SHELL_CLI_H

