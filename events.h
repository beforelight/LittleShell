#pragma once
#include<stdint.h>
#include<string>
#include<map>
#include<vector>
#include"fifo.h"
class events
{
	typedef void(*EventReceiver)(const std::string _event);
public:
	events(uint32_t _size = 1024) :seed(1), fifo(_size)
	{
		receiver.clear();
		id2str.clear();
		str2id.clear();
	}
	void SetEventReceiver(std::string _event, EventReceiver _receiver);
	void PutEvent(std::string _event);
	void PutEvent(uint8_t  _event_id);
	void Process(void);

	uint8_t GetId(std::string _event);
	const std::string& GetEventString(uint8_t _id);
private:
	FIFO<uint8_t> fifo;
	uint8_t seed;
	std::map<uint8_t, std::vector<EventReceiver> > receiver;
	std::map<uint8_t, std::string> id2str;
	std::map<std::string, uint8_t> str2id;
};

