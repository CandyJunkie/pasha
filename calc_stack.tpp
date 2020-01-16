#include <stdexcept>

template <class T>
calc_stack<T>::calc_stack ()
{
	m_size = 0;
	m_capacity = 8;
	m_data = new T[m_capacity];
}

template <class T>
calc_stack<T>::~calc_stack ()
{
	delete[] m_data;
}

template <class T>
void calc_stack<T>::push (T & value)
{
	if (m_size == m_capacity) {
		expand ();
	}
	m_data[m_size] = value;
	++m_size;
}

template <class T>
T calc_stack<T>::pop ()
{
	if (0 == m_size) {
		throw std::underflow_error("Stack is empty");
	}
	return m_data[--m_size]; // про m_size не забыл
}

template <class T>
T calc_stack<T>::top ()
{
	if (0 == m_size) {
		throw std::underflow_error ("Stack is empty");
	}
	return m_data[m_size - 1];
}

template <class T>
int calc_stack<T>::size ()
{
	return m_size;
}

template <class T>
void calc_stack<T>::expand ()
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
calc_stack<T> & calc_stack<T>::operator<<(T value)
{
	push (value);
	return *this;
}

template <class T>
calc_stack<T> & calc_stack<T>::operator>>(T & value)
{
	value = pop ();
	return *this;
}