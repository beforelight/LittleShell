//
// Created by 17616 on 2020/9/2.
//

#include "CLI.h"
#include <list>
#include <stdarg.h>
#include <assert.h>

void shell::CLI::SetOstream(ostream& _ostream)
{
	ostream* a = static_cast<ostream*>(this);
	*a = _ostream;
}

void shell::CLI::SetIstream(istream& _istream)
{
	istream* a = static_cast<istream*>(this);
	*a = _istream;
}

void shell::CLI::ShowHead(void)
{
	SaveColor(ConsoleColorSet::ConsoleFont);
	SetFontColor(ConsoleColor::GREEN);
	wwrite((char*)shell_head.c_str(), shell_head.size());
	RestoreColor(ConsoleColorSet::ConsoleFont);
}

std::map<std::string, shell::CLI::CLI_CMD> shell::CLI::cmd_map;//static变量，意思注册的命令会被所有的实例访问


int shell::CLI::InsertCMD(CLI_CMD _cli_cmd)
{
	if (cmd_map.count(_cli_cmd.cmd)) {
		return -1;//已经存在有了
	}
	else
	{
		cmd_map[_cli_cmd.cmd] = _cli_cmd;
		return 0;
	}
}


//字符 !=27
//字符 27		ESC//不处理
//字符 13		\n回车//不处理
//字符 8		退格//不处理
//字符 27 91 65 ↑
//字符 27 91 66 ↓
//字符 27 91 68 ←
//字符 27 91 67 →
//字符 27 91 51 216 delete
#define key1(a,b,c) ((a)|(b<<8)|(c<<16))//一类按键
#define key2(a,b,c,d) (key1(a,b,c)|(d<<24))//二类按键
#define up key1(27,91,65)
#define down key1(27,91,66)
#define left key1(27,91,68)
#define right key1(27,91,67)
#define del key2(27,91,51,216)
#define enter (13)
#define backspace (8)


void shell::CLI::Process(void) {
	
	while (UsedSize()) {
		Get((char*)&ch, 1);
		std::printf("%d\r\n", (int)ch);//调试接口
		//先进行按键识别
		switch (state)
		{
		case 0:
			switch (ch)
			{
			case 27:
				state = 1;//下一个状态
				break;
			case enter://换行
				putchar('\r');//发送换行
				putchar('\n');
				str.push_back(0);//插入结束符号
				history.push_back(str);//将本命令插入到历史记录
				is_history_mode = 0;
				if (history.size() > 20) { history.pop_front(); }
				cmd_argv.clear();
				//构造参数列表
				for (auto i = str.begin(); i != str.end(); i++)
				{
					if (*i == ' ') {
						*i = 0;
						find_cmd_arg = 1;
					}//将空格替换为结束符号
					else if (find_cmd_arg == 1)
					{
						find_cmd_arg = 0;
						cmd_argv.push_back(&(*i));
					}
				}
				cmd_now = std::string(&str[0]);//构造命令
				map_ite = cmd_map.find(cmd_now);
				if (map_ite != cmd_map.end()) {
					int retval = (*map_ite->second.pFun)(*this, cmd_argv);//执行命令
					if (retval != 0) {
						SaveColor(ConsoleColorSet::ConsoleFont);
						SetFontColor(ConsoleColor::light_RED);
						printf("\r\nreturn code %d", retval);
						RestoreColor(ConsoleColorSet::ConsoleFont);
					}
				}
				else
				{
					printf("%s: command not found", cmd_now.c_str());
				}
				//执行完命令之后开始准备
				str.clear();
				pos = 0;
				len = 0;
				state = 0;
				putchar('\r');//发送换行
				putchar('\n');
				ShowHead();
				Clear();//清空在执行命令期间接收的字符
				break;
			case backspace://退格
				if (len) {
					putchar(ch);//光标左移一位
					len--;
					pos--;
					SaveCursorPosition();
					if (len == pos) {
						putchar(' ');//刚刚的用空格填充
					}
					else {
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
					str.push_back((char)ch);
					putchar(ch);
					len++;
					pos++;
				}
				else//插入状态
				{
					auto position = str.begin() + pos;
					str.insert(position, (char)ch);
					putchar(ch);
					len++;
					pos++;
					SaveCursorPosition();
					if (len) {
						wwrite(&str[pos], len - pos);
					}
					RestoreCursorPosition();
				}
				break;
			}
			break;
		case 1:
			state = 0;
			switch (ch)
			{
			case 91:
				state = 2;//下一个状态
				break;
			default://不是处理的按键，退状态，2byte及以上的按键不回显
				break;
			}
			break;
		case 2:
			state = 0;
			switch (ch)
			{
			case 51:
				state = 3;//下一个状态
				break;
			case 65://上
				if (is_history_mode == 0) {
					if (history.size()) {
						last_cmd_cursor_pos = pos;
						history.push_back(str);
						histIte = history.end();
						histIte--;//--是因为现在最后一个元素是缓存了没有执行的指令
						is_history_mode = 1;
					}
				}
				if (is_history_mode == 1) {
					if (histIte != history.begin()) {
						histIte--;
						str = *histIte;
						str.pop_back();//剔除最后的那个0结束标识符
						putchar('\r');
						ClearLine();
						len = str.size();
						pos = len;
						ShowHead();
						if (len) {
							wwrite(&str[0], len);
						}
					}
				}

				break;
			case 66://下
				if (is_history_mode) {
					auto history_ite_m1 = history.end();
					history_ite_m1--;
					if (histIte != history_ite_m1) {
						histIte++;
						if (histIte == history_ite_m1) {
							is_history_mode = 0;
							//恢复保存的未完成的指令
							str = *histIte;
							history.pop_back();
							putchar('\r');
							ClearLine();
							len = str.size();
							pos = last_cmd_cursor_pos;
							ShowHead();
							if (len) {
								wwrite(&str[0], len);
							}
							MoveCursorBackward(len - pos);//恢复光标的位置
						}
						else
						{
							str = *histIte;
							str.pop_back();//剔除最后的那个0结束标识符
							putchar('\r');
							ClearLine();
							len = str.size();
							pos = len;
							ShowHead();
							if (len) {
								wwrite(&str[0], len);
							}
						}
					}
				}
				break;
			case 67://右
				if (len > pos) {
					pos++;
					key = right;
					wwrite((char*)&key, 3);
				}
				break;
			case 68://左
				if (pos) {
					pos--;
					key = left;
					wwrite((char*)&key, 3);
				}
				break;
			default://不是处理的按键，退状态,2byte及以上的按键不回显
				break;
			}
			break;
		case 3:
			state = 0;
			switch (ch)
			{
			case 126://delete，delete不用回显
				if (pos < len) {
					SaveCursorPosition();
					auto position = str.begin() + pos;
					str.erase(position);
					len--;
					if (len) {
						wwrite(&str[pos], len - pos);
					}
					putchar(' ');
					RestoreCursorPosition();
				}
				break;
			default:
				break;
			}
			break;
		default:
			state = 0;
			break;
		}
	}
	//特殊按键响应功能
	//回车 执行一条指令
	//上下 历史命令翻滚
	//左右 光标左移右移
	//退格删除 使用保存光标位置然后清理行来实现
}

int shell::CLI::Help(CLI& cli, const std::vector<char*>& argv)
{
	if (argv.size() == 1) {
		std::string arg(argv[0]);
		int len = arg.length();
		for (auto i = cmd_map.begin(); i != cmd_map.end(); i++)
		{
			if (0 == arg.compare(0, len, i->first, 0, len)) {
				cli.printf("%s\t\t%s\r\n", i->first.c_str(), i->second.helpInfo.c_str());
			}
		}
	}
	else
	{
		for (auto i = cmd_map.begin(); i != cmd_map.end(); i++)
		{
			cli.printf("%s\t\t%s\r\n", i->first.c_str(), i->second.helpInfo.c_str());
		}
	}
	return 0;
}

int shell::CLI::Clr(CLI& cli, const std::vector<char*>& argv)
{
	cli.printf("\033[H");
	cli.printf("\033[2J");
	return 0;
}

void shell::CLI::SetConsoleColor(ConsoleColor _color, ConsoleColorSet fontORbackgrand)
{
	color[(int)fontORbackgrand] = _color;
	printf("\033[%d;%dm", (int)color[(int)fontORbackgrand] % 100,
		(int)color[(int)fontORbackgrand] / 100 + (int)fontORbackgrand * 10);
}

void shell::CLI::SetFontColor(ConsoleColor _color)
{
	SetConsoleColor(_color, ConsoleColorSet::ConsoleFont);
}

void shell::CLI::SetBackgrandColor(ConsoleColor _color)
{
	SetConsoleColor(_color, ConsoleColorSet::ConsoleBackgrand);
}

void shell::CLI::Blinking(void)
{
	printf("\033[5m");
}

void shell::CLI::SaveColor(ConsoleColorSet fontORbackgrand)
{
	colorStack[(int)fontORbackgrand].push(color[(int)fontORbackgrand]);
}

void shell::CLI::RestoreColor(ConsoleColorSet fontORbackgrand)
{
	color[(int)fontORbackgrand] = colorStack[(int)fontORbackgrand].top();
	colorStack[(int)fontORbackgrand].pop();
	printf("\033[0;%dm", (int)color[(int)fontORbackgrand] + (int)fontORbackgrand * 10);
}

void shell::CLI::SaveCursorPosition(void)
{
	printf("\033[s");
}

void shell::CLI::RestoreCursorPosition(void)
{
	printf("\033[u");
}

void shell::CLI::ClearLine(void)
{
	printf("\033[K");
}

void shell::CLI::MoveCursorForward(int n_characters)
{
	printf("\033[%dC", n_characters);
}

void shell::CLI::MoveCursorBackward(int n_characters)
{
	printf("\033[%dD", n_characters);
}

void shell::CLI::ResetCursor(void)
{
	printf("\033[H");
}

void shell::CLI::DisplayClear(void)
{
	printf("\033[2J");
}


int shell::ostream::printf(const char* formatString, ...)
{
	va_list arg;
	int logLength;
	va_start(arg, formatString);
	logLength = vsnprintf(printBuf, sizeof(printBuf), formatString, arg);
	va_end(arg);
	(*write)(fp, printBuf, logLength);
	return logLength;
}

int shell::ostream::putchar(int ch)
{
	(*write)(fp, (char*)&ch, 1);
	return ch;
}

int shell::ostream::wwrite(char* ptr, int len)
{
	return 	(*write)(fp, ptr, len);
}

int shell::istream::scanf(char* formatString, ...)
{
	unsigned int len = 0;//接收到的字符
	unsigned int size = Size();//能接收的最大字符量
	//先将数据读入到scanfBuf直到收到'\n';
	char* scanfBuf = new char[size];
	char buf;
	while (1) {
		while (UsedSize() == 0) {}
		Get(&buf, 1);
		if (buf == '\n') {//收到回车退
			scanfBuf[len] = 0;
			break;
		}
		else
		{
			scanfBuf[len] = buf;
		}
		len++;
		if (len == size) {//缓存满退
			scanfBuf[size - 1] = 0;
			break;
		}
	}
	va_list arg;
	int ret;
	va_start(arg, formatString);
	ret = vsscanf(scanfBuf, formatString, arg);
	va_end(arg);
	delete[] scanfBuf;
	return ret;
}

int shell::istream::getchar(void)
{
	char ch;
	while (UsedSize() == 0) {}
	Get(&ch, 1);
	return ch;
}





