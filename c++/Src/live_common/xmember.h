#pragma once
#include <functional>

namespace std {

    template <typename C, typename S>
    class member_callback;

    template <typename T, typename R>
    class member_callback <T, R()>
    {
    public:
        member_callback(R (T::*fn)(), T* self)
            : m_self(*self)
			, m_entry(fn){
        }

    public:
        R operator()(){
            T&                      inst(m_self);
            std::mem_fun_ref_t<R,T> entry(m_entry);
            return entry(inst);  
        }

    private:
		T&							m_self;
		std::mem_fun_ref_t<R, T>    m_entry;
    };

    template <typename T, typename R, typename P1>
    class member_callback <T, R (P1)>
    {
    public:
        member_callback(R (T::*fn)(P1 p1), T* self)
            : m_self(*self)
            , m_entry(fn){
        }

    public:
        R operator()(P1 p1){
            return m_entry(m_self, p1);  
        }

    private:
        T&							    m_self;
        std::mem_fun1_ref_t<R, T, P1>   m_entry;
    };

    template <class T, class R>
    member_callback<T, R()>   mem_wrapper(T* c, R (T::*fn)()) {
        return member_callback<T, R()>(fn, c);
    }
    template <class T, class R, class P1>
    member_callback<T, R(P1)> mem_wrapper(T* c, R (T::*fn)(P1)) {
        return member_callback<T, R(P1)>(fn, c);
    }
};