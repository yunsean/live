#pragma once
#include <mutex>
#include <condition_variable>
#include "os.h"
#include "SmartArr.h"

#ifndef INFINITE
#define INFINITE	(-1)
#endif
#ifndef IGNORE
#define IGNORE		0
#endif

template <typename _Ty>
class CSmartQue {
public:
    CSmartQue(const int initSize = 1024 * 200, const int pageSize = 1024 * 20, const bool autoIncre = false);
    virtual ~CSmartQue(){}

public:
    bool                Write(const _Ty* data, const int size, const int wait = INFINITE);
    bool                Read(_Ty* buf, const int want, int& read, const int wait = INFINITE);
    int                 Size();
    void                Finish();
    bool                IsFinish();

private:
    const bool              m_bIncre;
    const int               m_szGrow;
    bool                    m_bFinish;
    std::mutex              m_lkCache;
    CSmartArr<_Ty>          m_saCache;
    int                     m_szCache;
    int                     m_posRead;
    int                     m_posWrite;
    std::condition_variable m_cvWrite;
    std::condition_variable m_cvRead;
};

template <typename _Ty>
CSmartQue<_Ty>::CSmartQue(const int initSize /* = 1024 * 200 */, const int pageSize /* = 1024 * 20 */, const bool autoIncre /* = false */)
    : m_bIncre(autoIncre)
    , m_szGrow(pageSize)
    , m_bFinish(false)
    , m_lkCache()
    , m_saCache(initSize)
    , m_szCache(m_saCache ? initSize : 0)
    , m_posRead()
    , m_posWrite()
    , m_cvWrite()
    , m_cvRead(){
}

template <typename _Ty>
bool CSmartQue<_Ty>::Write(const _Ty* data, const int size, const int wait /* = INFINITE */) {
    if (size == 0)return true;
    if (data == NULL || size < 0)return false;
    if (m_bFinish)return false;

    std::unique_lock<std::mutex>lock(m_lkCache);
    if ((m_szCache < size * 2) || (m_bIncre && (m_szCache - (m_posWrite - m_posRead) < size))) {
        int                     szNew(size * 2);
        if (szNew < (m_posWrite - m_posRead) + size)
            szNew               = (m_posWrite - m_posRead) + size * 2;
        szNew                   += (m_szGrow - szNew % m_szGrow);
        if (szNew > m_szCache) {
            CSmartArr<_Ty>     saNew(new(std::nothrow) _Ty[szNew]);
            if (saNew) {
                if (m_posWrite > m_posRead && m_saCache) {
                    memcpy(saNew, m_saCache + m_posRead, (m_posWrite - m_posRead) * sizeof(_Ty));
                    m_posWrite  -= m_posRead;
                    m_posRead   = 0;
                } else {
                    m_posWrite  = 0;
                    m_posRead   = 0;
                }
                m_saCache       = saNew;
                m_szCache       = szNew;
            }
        }
    }
    if (m_szCache - (m_posWrite - m_posRead) < size) {
        if (wait == IGNORE) {
            return false;
        } else if (wait == INFINITE) {
            while ((m_posWrite - m_posRead) + size > m_szCache && !m_bFinish) {
                m_cvWrite.wait(lock);
            }
        } else {
            std::chrono::milliseconds waitms(wait);
            std::chrono::time_point<std::chrono::system_clock>	over(std::chrono::system_clock::now());
            over                += waitms;
            while ((m_posWrite - m_posRead) + size > m_szCache && !m_bFinish) {
                std::chrono::time_point<std::chrono::system_clock>	now(std::chrono::system_clock::now());
                if (now >= over)break;
                if (m_cvWrite.wait_until(lock, over) == std::cv_status::timeout)break;
            }
        }
    }
    if (m_posWrite + size < m_szCache) {
        memcpy(m_saCache + m_posWrite, data, size * sizeof(_Ty));
        m_posWrite              += size;
        m_cvRead.notify_all();
        return true;
    } else if ((m_posWrite - m_posRead) + size < m_szCache) {
        memmove(m_saCache, m_saCache + m_posRead, (m_posWrite - m_posRead) * sizeof(_Ty));
        m_posWrite              -= m_posRead;
        m_posRead               = 0;
        memcpy(m_saCache + m_posWrite, data, size * sizeof(_Ty));
        m_posWrite              += size;
        m_cvRead.notify_all();
        return true;
    } else {
        return false;
    }
}

template <typename _Ty>
int CSmartQue<_Ty>::Size() {
    return m_posWrite - m_posRead;
}

template <typename _Ty>
bool CSmartQue<_Ty>::Read(_Ty* buf, const int want, int& read, const int wait /* = INFINITE */) {
    if (want == 0){
        read                    = 0;
        return true;
    }
    if (buf == NULL || want < 0)return false;

    std::unique_lock<std::mutex> lock(m_lkCache);
    if (m_posWrite - m_posRead >= want) {
        read                    = want;
        memcpy(buf, m_saCache + m_posRead, read * sizeof(_Ty));
        m_posRead               += read;
        m_cvWrite.notify_all();
        return true;
    } else if (m_bFinish && m_posWrite > m_posRead) {
        read                    = m_posWrite - m_posRead;
        memcpy(buf, m_saCache + m_posRead, read * sizeof(_Ty));
        m_posRead               += read;
        m_cvWrite.notify_all();
        return true;
    } else if (m_bFinish) {
        return false;
    } else if (m_szCache < want * 2) {
        int                     szNew(want * 2);
        szNew                   += (m_szGrow - szNew % m_szGrow);
        if (szNew > m_szCache) {
            CSmartArr<_Ty>     saNew(new(std::nothrow) _Ty[szNew]);
            if (saNew) {
                if (m_posWrite > m_posRead && m_saCache) {
                    memcpy(saNew, m_saCache + m_posRead, (m_posWrite - m_posRead) * sizeof(_Ty));
                    m_posWrite  -= m_posRead;
                    m_posRead   = 0;
                } else {
                    m_posWrite  = 0;
                    m_posRead   = 0;
                }
                m_saCache       = saNew;
                m_szCache       = szNew;
                m_cvWrite.notify_all();
            }
        }
    } else if (m_szCache - m_posRead < want) {
        memmove(m_saCache, m_saCache + m_posRead, (m_posWrite - m_posRead) * sizeof(_Ty));
        m_posWrite              -= m_posRead;
        m_posRead               = 0;
    }
    
    if (wait == IGNORE) {
        return false;
    } else if (wait == INFINITE) {
        while (m_posWrite - m_posRead < want && !m_bFinish) {
            m_cvRead.wait(lock);
        }
    } else {
        std::chrono::milliseconds waitms(wait);
        std::chrono::time_point<std::chrono::system_clock>	over(std::chrono::system_clock::now());
        over                += waitms;
        while (m_posWrite - m_posRead < want && !m_bFinish) {
            std::chrono::time_point<std::chrono::system_clock>	now(std::chrono::system_clock::now());
            if (now >= over)break;
            if (m_cvWrite.wait_until(lock, over) == std::cv_status::timeout)break;
        }
    }
    if (m_posWrite > m_posRead) {
        read                    = m_posWrite - m_posRead;
        memcpy(buf, m_saCache + m_posRead, read * sizeof(_Ty));
        m_posRead               += read;
        m_cvWrite.notify_all();
        return true;
    } else {
        return false;
    } 
}