//事件驱动类

#pragma once
#include<stdint.h>
#include<string>
#include<map>
#include<vector>
#include"fifo.h"
class events
{
	typedef void(*EventReceiver)(uint8_t _event_id);
public:
	events(uint32_t _size = 1024) :fifo(_size)
	{
		receiver.clear();
	}
	void SetEventReceiver(uint8_t _event_id, EventReceiver _receiver);//为特定事件设置接收者，一对多or多对一
	void SendEvent(uint8_t  _event_id);//发送事件
	void Process(void);//处理主程序
private:
	FIFO<uint8_t> fifo;
	std::map<uint8_t, std::vector<EventReceiver> > receiver;
};

