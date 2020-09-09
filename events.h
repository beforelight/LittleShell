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
	void SetEventReceiver(uint8_t _event_id, EventReceiver _receiver);
	void SendEvent(uint8_t  _event_id);
	void Process(void);
private:
	FIFO<uint8_t> fifo;
	std::map<uint8_t, std::vector<EventReceiver> > receiver;
};

