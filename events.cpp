#include "events.h"


void events::SetEventReceiver(uint8_t _event_id, EventReceiver _receiver)
{
	auto ite = receiver.find(_event_id);
	if (ite == receiver.end())//需要创建事件和id的映射，以及id和接收器的映射
	{
		std::vector<EventReceiver> a;
		a.push_back(_receiver);
		receiver[_event_id] = a;
	}
	else //只需要改id的接收器
	{
		std::vector<EventReceiver>& b = ite->second;
		auto i = b.begin();
		for (; i != b.end(); i++)
		{
			if (*i == _receiver) {
				return;
			}
		}
		b.push_back(_receiver);
		//ite->second = b;
	}
}

void events::SendEvent(uint8_t _event_id)
{
	//auto ite = receiver.find(_event_id);
	//if (ite != receiver.end()) {
	fifo.Put(&_event_id, 1);
	//}	
}

void events::Process(void)
{
	while (fifo.UsedSize()) {
		uint8_t id;
		fifo.Get(&id, 1);
		auto ite = receiver.find(id);
		if (ite != receiver.end()) {
			for (auto i : ite->second)
			{
				(*(*i))(id);
			}
		}
	}
}

