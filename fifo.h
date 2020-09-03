#pragma once
#include <stdint.h>
#include <string.h>

template<class T>
class FIFO {
public:
	FIFO(uint32_t _size = 1024);
	FIFO(const FIFO &fifo);
	~FIFO();
	uint32_t Put(T* src, uint32_t len);
	uint32_t Get(T* dst, uint32_t len);
	uint32_t Size(void);
	uint32_t UsedSize(void);
private:
	uint32_t Min(uint32_t left, uint32_t right) { return left > right ? right : left; }
	uint32_t size;
	uint32_t size_mask;
	volatile uint32_t in;//addr of buffer to write in
	volatile uint32_t out;//addr of buffer to read out
	T* buffer;
};


template<typename T>
FIFO<T>::FIFO(uint32_t _size) :in(0), out(0)
{
	size = 1U;
	while (size < _size) {
		size <<= 1;
	}
	size_mask = size - 1;
	buffer = new T[size];
}

template<class T>
inline FIFO<T>::FIFO(const FIFO& fifo)
{
	size = fifo.size;
	size_mask = fifo.size_mask;
	in = fifo.in;
	out = fifo.out;
	buffer = new T[size];
	memcpy(buffer, fifo.buffer, sizeof(T) * size);
}

template<typename T>
FIFO<T>::~FIFO()
{
	delete[] buffer;
}

template<typename T>
uint32_t FIFO<T>::Put(T* src, uint32_t len)
{
	len = Min(len, size - in + out);//满了之后就不会再接收了
	uint32_t l = Min(len, size - in);
	memcpy(buffer + in, src, l * sizeof(T));
	memcpy(buffer, src + l, (len - l) * sizeof(T));
	in = (in + len) & size_mask;
	return len;
}

template<typename T>
uint32_t FIFO<T>::Get(T* dst, uint32_t len)
{
	len = Min(len, in - out);
	uint32_t l = Min(len, size - out);
	memcpy(dst, buffer + out, l * sizeof(T));
	memcpy(dst + l, buffer, (len - l) * sizeof(T));
	out = (out + len) & size_mask;
	return len;
}

template<class T>
uint32_t FIFO<T>::Size(void)
{
	return size;
}

template<typename T>
uint32_t FIFO<T>::UsedSize(void)
{
	return ((size + in - out) & size_mask);
}