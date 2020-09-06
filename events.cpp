#include "events.h"
#include <algorithm>

void events::SetEventReceiver(std::string _event, EventReceiver _receiver)
{
	if (seed == 0) { return; }//最多255种事件
	auto ite = str2id.find(_event);
	if (ite == str2id.end())//需要创建事件和id的映射，以及id和接收器的映射
	{
		id2str[seed] = _event;
		str2id[_event] = seed;
		std::vector<EventReceiver> a;
		a.push_back(_receiver);
		receiver[seed] = a;
		seed++;
	}
	else //只需要改id的接收器
	{
		std::vector<EventReceiver>& b = receiver[str2id[_event]];
		if (std::find(b.begin(), b.end(), _receiver) != b.end()) {}//已经注册了相同的接收器
		else
		{
			b.push_back(_receiver);
		}
	}
}

void events::PutEvent(std::string _event)
{
	PutEvent(str2id[_event]);
}

void events::PutEvent(uint8_t _event_id)
{
	fifo.Put(&_event_id, 1);
}

void events::Process(void)
{
	uint8_t id;
	while (fifo.UsedSize()) {
		fifo.Get(&id, 1);
		const std::vector<EventReceiver>& receivers = receiver[id];
		for (auto i: receivers)
		{
			(*(*i))(id2str[id]);
		}
	}
}

uint8_t events::GetId(std::string _event)
{
	auto ite = str2id.find(_event);
	if (ite != str2id.end()) {
		return  ite->second;
	}
	else
	{
		return uint8_t(0);
	}
}

const std::string& events::GetEventString(uint8_t _id)
{
	auto ite = id2str.find(_id);
	if (ite != id2str.end()) {
		return  ite->second;
	}
	else
	{
		return std::string();
	}
}
