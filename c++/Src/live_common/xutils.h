#pragma once
#include <vector>
#include <string>

namespace utils {
    template<bool>
    struct StaticAssert;

    template<> 
    struct StaticAssert<true>
    {};
};
#define STATIC_ASSERT(exp) {std::StaticAssert<((exp) != 0)> Failed;}

namespace utils {
    struct Nil;

    template <typename T>
    struct IsPointer {
        enum { Result = false };
        typedef Nil ValueType;
    };

    template <typename T>
    struct IsPointer<T*> {
        enum { Result = true };
        typedef T ValueType;
    };
};

namespace utils {

    template <class TMap, class TPred> 
    inline void map_remove_if(TMap& container, TPred pred)  { 
        for (typename TMap::iterator it = container.begin(); it != container.end();) {
            if (pred(*it)) {
                it = container.erase(it);
            } else {
                ++it;
            }
        }
    } 

    template <class TMap, class TKey> 
    inline void map_remove(TMap& container, TKey key)  { 
        for (typename TMap::iterator it = container.begin(); it != container.end();) {
            if (it->first == key) {
                it = container.erase(it);
            } else {
                ++it;
            }
        }
    }
}

namespace utils {
	bool isBytesEquals(void* pa, void* pb, int size);
	void randomGenerate(char* bytes, int size);			//All value in[0x0f, 0xf0]

	unsigned short uintFrom2BytesBE(const unsigned char* data);
	unsigned int uintFrom3BytesBE(const unsigned char* data);
	unsigned int uintFrom4BytesBE(const unsigned char* data);

	unsigned short uintFrom2BytesLE(const unsigned char* data);
	unsigned int uintFrom3BytesLE(const unsigned char* data);
	unsigned int uintFrom4BytesLE(const unsigned char* data);

	unsigned char* uintTo2BytesBE(unsigned short value, unsigned char* data);
	unsigned char* uintTo3BytesBE(unsigned int value, unsigned char* data);
	unsigned char* uintTo4BytesBE(unsigned int value, unsigned char* data);

	unsigned char* uintTo2BytesLE(unsigned short value, unsigned char* data);
	unsigned char* uintTo3BytesLE(unsigned int value, unsigned char* data);
	unsigned char* uintTo4BytesLE(unsigned int value, unsigned char* data);
	
	std::vector<std::string> split(const std::string& s, char seperator);
}