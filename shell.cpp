#include <iostream>
#include <windows.h>
#include <conio.h>
#include "CLI.h"
#include<thread>
#include"SerialPort.h"
using namespace std;
CSerialPort mySerialPort;

int WriteSerialPort(void* fp, char* ptr, int len);
int initSerialPort(void);
shell::CLI cli(WriteSerialPort, &mySerialPort, 1024);

int mycmd(shell::CLI& cli, const std::vector<const char*>& argv) {
    for (auto i:argv)
    {
        cli.printf(i);
        cli.printf("\t");
    }
    cli.printf("\r\n");
    return 0;
}
shell::CLI::CMD xxxxx("cmd", mycmd, "printf argv");
int main() {
    //TODO
    //查询模式下不保留最后一次输入
    initSerialPort();
    cli.ShowTitle();
    cli.ShowHead();
    while (1) {
        cli.Process();
        Sleep(100);
    }
}


int WriteSerialPort(void* fp, char* ptr, int len)
{
    CSerialPort* mySerialPort = static_cast<CSerialPort*>(fp);
    mySerialPort->WriteData((uint8_t*)ptr, len);
    return 0;
}

int initSerialPort(void)
{
    mySerialPort.pcli = &cli;
    if (!mySerialPort.InitPort(8, 115200))
    {
        std::cout << "initPort fail !" << std::endl;
    }
    else
    {
        std::cout << "initPort success !" << std::endl;
    }

    if (!mySerialPort.OpenListenThread())
    {
        std::cout << "OpenListenThread fail !" << std::endl;
    }
    else
    {
        std::cout << "OpenListenThread success !" << std::endl;
    }
    return 0;
}