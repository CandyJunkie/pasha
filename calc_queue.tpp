#include <stdexcept>

template <class T>
calc_queue<T>::calc_queue ()
{
	m_size = 0;
	m_capacity = 8;
	m_data = new T[m_capacity];
}

template <class T>
calc_queue<T>::~calc_queue ()
{
	delete[] m_data;
}

template <class T>
void calc_queue<T>::put (T & value)
{
	if (m_size == m_capacity) {
		expand ();
	}
	m_data[m_size] = value;
	++m_size;
}

template <class T>
T calc_queue<T>::get ()
{
	if (0 == m_size) {
		throw std::underflow_error ("Queue is empty");
	}
	T tmp = m_data[0];
	for (int i = 0; i < m_size - 1; ++i) {
		m_data[i] = m_data[i + 1];
	}
	--m_size;
	return tmp;
}

template <class T>
int calc_queue<T>::size ()
{
	return m_size;
}

template <class T>
void calc_queue<T>::expand ()
{
	m_capacity *= 2;
	T * new_data = new T[m_capacity]; // новая
	for (int i = 0; i < m_size; ++i) {
		new_data[i] = m_data[i];
	}
	delete[] m_data;
	m_data = new_data;
}

template <class T>
calc_queue<T> & calc_queue<T>::operator<<(T value)
{
	put (value);
	return *this;
}

template <class T>
calc_queue<T> & calc_queue<T>::operator>>(T & value)
{
	value = get ();
	return *this;
}