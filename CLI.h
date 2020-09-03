//
// Created by 17616 on 2020/9/2.
//

#ifndef SHELL_CLI_H
#define SHELL_CLI_H

#include <sstream>
#include <map>
#include <vector>
#include "fifo.h"
namespace shell {
	class ostream;
	class istream;
	class CLI;
	class CLI_CMD;
	typedef int (*CmdFuntion)(CLI& cli, const std::vector<char*>& argv);
	typedef int (*Write)(void* fp, char* ptr, int len);
	typedef int (*Console_Printf)(const char* formatString, ...);

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

	class CLI_CMD {
	public:
		CLI_CMD() {}
		CLI_CMD(std::string _cmd , CmdFuntion _pFun, std::string _helpInfo = "什么都没有写") :
			cmd(_cmd), pFun(_pFun), helpInfo(_helpInfo) {}
		std::string cmd;
		CmdFuntion pFun;
		std::string helpInfo;
	};

	static std::map<std::string, CLI_CMD> cmd_map;//static变量，意思注册的命令会被所有的实例访问

	class CLI :public ostream, public istream {
	public:
		CLI(ostream _ostream, istream _istream, std::string _shell_head = "firefly@firefly:~$ ") :
			ostream(_ostream), istream(_istream), shell_head(_shell_head)
		{
			/*InsertCMD(CLI_CMD("help", CLI::Help, "Print the help information"));
			InsertCMD(CLI_CMD("clr", CLI::Clr, "Clear the console"));*/
		}
		CLI(Write _write_ostream, void* _fp_ostream = NULL, uint32_t _size_istream = 1024) :
			CLI(ostream(_write_ostream, _fp_ostream), istream(_size_istream)) {
			InsertCMD(CLI_CMD("help", CLI::Help, "Print the help information"));
			InsertCMD(CLI_CMD("clr", CLI::Clr, "Clear the console"));
		}
		void SetOstream(ostream& _ostream);
		void SetIstream(istream& _istream);
		void ShowHead(void);
		void SetHead(std::string _head) { shell_head = _head; }
		void SetFontColor(void);
		void SetBackgrandColor(void);
		static int InsertCMD(CLI_CMD _cli_cmd);
		void Process(void);
		static int Help(CLI& cli, const std::vector<char*>& argv);//内置命令
		static int Clr(CLI& cli, const std::vector<char*>& argv);//内置命令，清空控制台
	protected:
		//控制台指令
		void SaveCursorPosition(void);
		void RestoreCursorPosition(void);
		void ClearLine(void);
		void MoveCursorForward(int n_characters);
		void MoveCursorBackward(int n_characters);
	private:
		std::string shell_head;
	};
}
#endif //SHELL_CLI_H

