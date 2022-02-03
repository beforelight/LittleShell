#include"LittleShell.hpp"
#include"telnetservlib.hpp"
#include<iostream>
#include<ctime>
void Delay(int time)//time*1000为秒数
{
    clock_t now = clock();

    while (clock() - now < time);
}
void myConnected(SP_TelnetSession session) {
    std::cout << "myConnected got called\n";
    session->sendLine("Welcome to the Telnet Server.");
}

void myNewLine(SP_TelnetSession session, std::string line) {
    std::cout << "myNewLine got called with line: " << line << "\n";
    session->sendLine("Copy that.");
}

SP_TelnetSession session2send;
int sessionWrite(void *fp, const char *ptr, int len) {
    session2send->echoBack((char *) ptr, len);
    return len;
}
LittleShell::CLI cli(LittleShell::ostream(sessionWrite), LittleShell::istream(512));

void myNewChar(SP_TelnetSession session, char *buff, u_long buffSize) {
    session2send = session;
    cli.Put(buff, buffSize);
}

LittleShell::CLI::CMD task1("task","cli test",[](LittleShell::ios& ios,const std::vector<const char *> &argv)->int{
    ios.printf("task message");
    for (auto i:argv){
        std::cout<<i<<'\t';
    }
    std::cout<<"\r\n";
    if(argv.size()!=0){
        Delay(std::stoi(argv[0]));
    }

    ios.printf("task message");
    return 0;
});

int main() {
    auto ts = std::make_shared<TelnetServer>();
    LittleShell::CLI::InsertCMD(task1);
    ts->initialise(27015);
    ts->connectedCallback(myConnected);
    ts->newLineCallback(myNewLine);
    ts->newCharCallback(myNewChar);
    // Our loop
    do {
        ts->update();
        cli.Process();
        Sleep(16);
    } while (true);

    ts->shutdown();
    WSACleanup();

    return 0;
}