#ifndef HASHMAP_H
#define HASHMAP_H

#include <Engine/Includes/Standard.h>
#include <Engine/Hashing/FNV1A.h>
#include <Engine/Hashing/Murmur.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>
#include <functional>

template <typename T> class HashMapElement {
public:
    Uint32 Key = 0x00000000U;
    bool   Used = false;
    T      Data;
};
template <typename T> class HashMap {
public:
    int Count = 0;
    int Capacity = 0;
    int CapacityMask = 0;
    int ChainLength = 16;
    HashMapElement<T>* Data = NULL;

    Uint32 (*HashFunction)(const void*, Uint32) = NULL;

    HashMap<T>(Uint32 (*hashFunc)(const void*, Uint32) = NULL, int capacity = 16) {
        HashFunction = hashFunc;
        if (HashFunction == NULL)
            HashFunction = Murmur::EncryptData;

        Count = 0;
        Capacity = capacity;
        CapacityMask = Capacity - 1;

        Data = (HashMapElement<T>*)Memory::TrackedCalloc("HashMap::Data", Capacity, sizeof(HashMapElement<T>));
    	if (!Data) {
            Log::Print(Log::LOG_ERROR, "Could not allocate memory for HashMap data!");
            exit(0);
        }
    }

    void Dispose() {
        if (Data)
            Memory::Free(Data);
        Data = NULL;
    }
    ~HashMap<T>() {
        Dispose();
    }

    Uint32 TranslateIndex(Uint32 index) {
        // Find index that works to put new data in.
        index += (index << 12);
        index ^= (index >> 22);
        index += (index << 4);
        index ^= (index >> 9);
        index += (index << 10);
        index ^= (index >> 2);
        index += (index << 7);
        index ^= (index >> 12);
        index = (index >> 3) * 0x9E3779B1U;
        index = index & CapacityMask; // index = index % Capacity;
        return index;
    }
    int    Resize() {
        Capacity <<= 1;
        CapacityMask = Capacity - 1;

        HashMapElement<T>* oldData = Data;
        const char* oldTrack = Memory::GetName(oldData);

        Data = (HashMapElement<T>*)Memory::TrackedCalloc(oldTrack, Capacity, sizeof(HashMapElement<T>));
        if (!Data) {
            Log::Print(Log::LOG_ERROR, "Could not allocate memory for HashMap data!");
            exit(0);
        }

        Count = 0;
        Uint32 index;
        for (int i = 0; i < Capacity / 2; i++) {
            if (oldData[i].Used) {
                index = TranslateIndex(oldData[i].Key);

                for (int c = 0; c < ChainLength; c++) {
                    if (!Data[index].Used) {
                        Count++;
            			break;
                    }
            		if (Data[index].Used && Data[index].Key == oldData[i].Key)
            			break;
            		index = (index + 1) & CapacityMask; // index = (index + 1) % Capacity;
            	}

                Data[index].Key = oldData[i].Key;
                Data[index].Used = true;
                Data[index].Data = oldData[i].Data;
            }
        }

        Memory::Free(oldData);

        return Capacity;
    }

    void   Put(Uint32 hash, T data) {
        Uint32 index;
        do {
            index = hash;
            index = TranslateIndex(index);

            for (int i = 0; i < ChainLength; i++) {
                if (!Data[index].Used) {
                    Count++;
                    if (Count >= Capacity / 2) {
                        index = 0xFFFFFFFFU;
                        Resize();
                        break;
                    }
        			break;
                }

        		if (Data[index].Used && Data[index].Key == hash)
        			break;

        		index = (index + 1) & CapacityMask; // index = (index + 1) % Capacity;
        	}
        }
        while (index == 0xFFFFFFFFU);

        Data[index].Key = hash;
        Data[index].Used = true;
        Data[index].Data = data;
    }
    void   Put(const char* key, T data) {
        Uint32 hash = HashFunction(key, strlen(key));
        Put(hash, data);
    }
    T      Get(Uint32 hash) {
        Uint32 index;

        index = hash;
        index = TranslateIndex(index);

        for (int i = 0; i < ChainLength; i++) {
            if (Data[index].Used && Data[index].Key == hash) {
                return Data[index].Data;
            }

            index = (index + 1) & CapacityMask; // index = (index + 1) % Capacity;
        }

        return T { 0 };
    }
    T      Get(const char* key) {
        Uint32 hash = HashFunction(key, strlen(key));
        return Get(hash);
    }
    bool   Exists(Uint32 hash) {
        Uint32 index;

        index = hash;
        index = TranslateIndex(index);

        for (int i = 0; i < ChainLength; i++) {
            // printf("Data[%d].Key (%X) == %X: %s     .Used == %s\n",
            //     index, Data[index].Key, hash,
            //     Data[index].Key == hash ? "TRUE" : "FALSE",
            //     Data[index].Used ? "TRUE" : "FALSE");
            if (Data[index].Used && Data[index].Key == hash) {
                return true;
            }

            index = (index + 1) & CapacityMask; // index = (index + 1) % Capacity;
        }

        return false;
    }
    bool   Exists(const char* key) {
        Uint32 hash = HashFunction(key, strlen(key));
        return Exists(hash);
    }

    bool   Remove(Uint32 hash) {
        Uint32 index;

        index = hash;
        index = TranslateIndex(index);

        for (int i = 0; i < ChainLength; i++) {
            if (Data[index].Used && Data[index].Key == hash) {
                Count--;
                Data[index].Used = false;
                return true;
            }

            index = (index + 1) & CapacityMask; // index = (index + 1) % Capacity;
        }
        return false;
    }
    bool   Remove(const char* key) {
        Uint32 hash = HashFunction(key, strlen(key));
        return Remove(hash);
    }

    void   Clear() {
        for (int i = 0; i < Capacity; i++) {
            if (Data[i].Used) {
                Data[i].Used = false;
            }
        }
    }

    void   ForAll(void (*forFunc)(Uint32, T)) {
        for (int i = 0; i < Capacity; i++) {
            if (Data[i].Used) {
                forFunc(Data[i].Key, Data[i].Data);
            }
        }
    }
    void   WithAll(std::function<void(Uint32, T)> forFunc) {
        for (int i = 0; i < Capacity; i++) {
            if (Data[i].Used) {
                forFunc(Data[i].Key, Data[i].Data);
            }
        }
    }

    void   PrintHashes() {
        printf("Printing...\n");
        for (int i = 0; i < Capacity; i++) {
            if (Data[i].Used) {
                printf("Data[%d].Key: %X\n", i, Data[i].Key);
            }
        }
    }

    Uint8* GetBytes(bool exportHashes) {
        Uint32 stride = ((exportHashes ? 4 : 0) + sizeof(T));
        Uint8* bytes = (Uint8*)Memory::TrackedMalloc("HashMap::GetBytes", Count * stride);
        if (exportHashes) {
            for (int i = 0, index = 0; i < Capacity; i++) {
                if (Data[i].Used) {
                    *(Uint32*)(bytes + index * stride) = Data[i].Key;
                    *(T*)(bytes + index * stride + 4) = Data[i].Data;
                    index++;
                }
            }
        }
        else {
            for (int i = 0, index = 0; i < Capacity; i++) {
                if (Data[i].Used) {
                    *(T*)(bytes + index * stride) = Data[i].Data;
                    index++;
                }
            }
        }
        return bytes;
    }
    void   FromBytes(Uint8* bytes, int count) {
        Uint32 stride = (4 + sizeof(T));
        for (int i = 0; i < count; i++) {
            Put(*(Uint32*)(bytes + i * stride),
                *(T*)(bytes + i * stride + 4));
        }
    }
};

#endif
