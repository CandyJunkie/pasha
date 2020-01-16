#pragma once

template <class T>
class calc_queue {
public:
	calc_queue ();
	~calc_queue ();
	void put (T & value);
	T get ();
	int size ();
	calc_queue & operator<<(T value);
	calc_queue & operator>>(T & value);
private:
	void expand ();

	int m_size;
	int m_capacity;
	T * m_data;
};

#include "calc_queue.tpp"