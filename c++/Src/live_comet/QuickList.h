#pragma once

template <class T>
struct QuickItem {
	QuickItem() : prev(NULL), next(NULL) { }
	T* prev;
	T* next;
};

template <class T>
class QuickList {

public:
	class Iterator {
	public:
		friend class QuickList;
		T next() {
			T ret = p;
			if (p) {
				p = p->next;
			}
			return ret;
		}
	private:
		T p;
	};
	friend class Iterator;

public:
	QuickList() {
		size = 0;
		head = NULL;
		tail = NULL;
	}
	Iterator iterator() const {
		Iterator it;
		it.p = this->head;
		return it;
	}
	bool empty() const {
		return size == 0;
	}
	void remove(T t) {
		this->size--;
		if (t->prev) {
			t->prev->next = t->next;
		}
		if (t->next) {
			t->next->prev = t->prev;
		}
		if (this->head == t) {
			this->head = t->next;
		}
		if (this->tail == t) {
			this->tail = t->prev;
		}
	}
	T pop_front() {
		T t = this->head;
		this->remove(t);
		return t;
	}
	void push_back(T t) {
		this->size++;
		t->prev = this->tail;
		t->next = NULL;
		if (this->tail) {
			this->tail->next = t;
		} else { 
			this->head = t;
		}
		this->tail = t;
	}
	int count() const {
		return size;
	}

private:
	int size;
	T head;
	T tail;
};

